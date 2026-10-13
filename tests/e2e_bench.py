#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

"""chip-tool-link against a gadget running the Matter controller with the TLV path,
compared with chip-tool itself on the same device.

For each chip-tool command line: chip-tool-link prints the matter.* call, the
bench console makes it (as link.invoke would), and chip-tool-link prints the
answer. The same line then runs in chip-tool, on its own fabric (the device
is shared with matter.open_window), and the chip-tool lines ([TOO], and the
command status lines) of both are compared.

Env:
  PR_A_BENCH       the bench directory with test_pr_a.py (the serial console's
                   call()) and vdev.py (virtual Matter lights)
  PR_A_PORT        the board's serial port
  CHIP_TOOL        chip-tool to compare with, at the same CHIP commit (or
                   chip-tool-m, which is chip-tool when MATTER_GW is unset)
  CHIP_TOOL_LINK   default: out/chip-tool-link
Run: python3 tests/e2e_bench.py"""
from __future__ import annotations

import json
import os
import re
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
BENCH = Path(os.environ.get("PR_A_BENCH", HERE / "bench"))
sys.path.insert(0, str(BENCH))
import test_pr_a as bench  # noqa: E402  (console, call)
import vdev  # noqa: E402

LINK = os.environ.get("CHIP_TOOL_LINK", str(HERE.parent / "out/chip-tool-link"))
CHIP_TOOL = os.environ.get("CHIP_TOOL", "chip-tool")

ANSI = re.compile(r"\x1b\[[0-9;]*m")
PREFIX = re.compile(r"^\[[0-9.]+\] \[\d+:\d+\] \[(\w+)\] ")

# {n}: the node, the same ID on both fabrics.
LINES = [
    "onoff read on-off {n} 1",
    "onoff toggle {n} 1",
    "onoff read on-off {n} 1",
    "onoff on {n} 1 --timedInteractionTimeoutMs 2000",
    "onoff read start-up-on-off {n} 1",
    "onoff write on-time 5 {n} 1",
    "onoff read on-time {n} 1",
    "onoff read attribute-list {n} 1",
    "onoff read feature-map {n} 1",
    "basicinformation write node-label kitchen {n} 0",
    "basicinformation read node-label {n} 0",
    "basicinformation read vendor-name {n} 0",
    "basicinformation read capability-minima {n} 0",
    "basicinformation force-write vendor-name x {n} 0",
    "descriptor read parts-list {n} 0",
    "descriptor read server-list {n} 1",
    "descriptor read device-type-list {n} 1",
    "identify identify 5 {n} 1",
    "groups add-group 1 kitchen {n} 1",
    "groups get-group-membership [1] {n} 1",
    "levelcontrol move-to-level 128 0 0 0 {n} 1",
    "any command-by-id 6 2 {{}} {n} 1",
    "any read-by-id 6 0xFFFFFFFF {n} 1",
    "any write-by-id 6 0x4001 7 {n} 1",
    "generaldiagnostics read network-interfaces {n} 0",
    "accesscontrol read acl {n} 0",
]


# Values that differ by nature between the two runs: the device's data
# version and countdown move between them, and a fabric-filtered read shows
# each controller its own fabric.
LIVE = re.compile(r"(DataVersion|OnTime|FabricIndex): \d+")


def lines_of(output: str) -> list[str]:
    """chip-tool's own lines: [TOO], and the command status lines CommandSender
    logs (which chip-tool-link logs in its place); prefixes dropped."""
    kept = []
    for raw in output.splitlines():
        line = ANSI.sub("", raw)
        m = PREFIX.match(line)
        if not m:
            continue
        module, text = m.group(1), line[m.end():]
        if module == "TOO" or (module == "DMG" and text.startswith("Received Command Response")):
            if text.startswith(("Run command failure", "Sending command to node")):
                continue  # where the error came from differs; node lines carry the fabric's view
            kept.append(LIVE.sub(r"\1: N", text))
    return kept


def run(cmd: list[str], stdin: str | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, input=stdin, capture_output=True, text=True, timeout=120)


def via_link(args: list[str]) -> tuple[str, int, float]:
    request = run([LINK, *args])
    if request.returncode != 0:
        return request.stdout + request.stderr, request.returncode, 0.0
    call = json.loads(request.stdout.strip().splitlines()[-1])
    t = time.time()
    answer = bench.call(call["command"], call["params"], 90)
    took = time.time() - t
    result = run([LINK, *args, "--result", "-"], stdin=json.dumps(answer))
    return result.stdout + result.stderr, result.returncode, took


def via_chip_tool(args: list[str], storage: str) -> tuple[str, int]:
    r = run([CHIP_TOOL, *args, "--storage-directory", storage])
    return r.stdout + r.stderr, r.returncode


def main() -> int:
    if not bench.PORT:
        print("PR_A_PORT not set", file=sys.stderr)
        return 2
    dev = vdev.start(1)[0]
    storage = tempfile.mkdtemp(prefix="chip-tool-link-e2e-")
    node = None
    failures = 0
    try:
        p = bench.ok(bench.call("matter.commission", {"code": dev["code"], "label": "chip-tool-link"}, 170))
        node = p["node_id"]
        print(f"commissioned node {node} on the gadget's fabric")
        manual = bench.ok(bench.call("matter.open_window", {"node_id": node}, 90))["manual_code"]
        out, rc = via_chip_tool(["pairing", "code", str(node), manual], storage)
        if rc != 0:
            print(out[-3000:])
            raise RuntimeError("chip-tool could not join the device")
        print(f"shared with chip-tool's fabric as node {node}\n")

        for template in LINES:
            line = template.format(n=node)
            args = line.split()
            link_out, link_rc, took = via_link(args)
            tool_out, tool_rc = via_chip_tool(args, storage)
            a, b = lines_of(link_out), lines_of(tool_out)
            same = a == b and link_rc == tool_rc
            failures += not same
            print(f"{'SAME' if same else 'DIFF'}  rc={link_rc}/{tool_rc}  {took:4.1f}s  {line}")
            for text in a:
                print(f"      {text}")
            if not same:
                print("    chip-tool:")
                for text in b:
                    print(f"      {text}")
        print(f"\n{len(LINES) - failures}/{len(LINES)} the same")
        print(f"board heap: {bench.bench_heap()}")
    finally:
        if node:
            bench.call("matter.remove", {"node_id": node}, 90)
        vdev.stop()
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

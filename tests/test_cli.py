# SPDX-License-Identifier: Apache-2.0

"""chip-tool-link without a gadget: the matter.* call each chip-tool line
becomes, and what chip-tool prints for a recorded answer.

CHIP_TOOL_LINK: the binary (default: out/chip-tool-link).
Run: python3 -m unittest discover -s tests -p 'test_cli.py' -v"""
from __future__ import annotations

import json
import os
from pathlib import Path
import re
import subprocess
import unittest

HERE = Path(__file__).resolve().parent
LINK = Path(os.environ.get("CHIP_TOOL_LINK", HERE.parent / "out/chip-tool-link"))
# chip-tool's log prefix ([time] [pid:tid] [module]), and its colours
PREFIX = re.compile(r"^\[[0-9.]+\] \[\d+:\d+\] ")
ANSI = re.compile(r"\x1b\[[0-9;]*m")


def run(args: list[str], answer: dict | None = None) -> tuple[int, list[str]]:
    cmd = [str(LINK), *args]
    if answer is not None:
        cmd += ["--result", json.dumps(answer)]
    done = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
    lines = [PREFIX.sub("", ANSI.sub("", line)) for line in (done.stdout + done.stderr).splitlines()]
    return done.returncode, lines


def request(args: list[str]) -> dict:
    rc, lines = run(args)
    assert rc == 0, lines
    return json.loads(lines[-1])


@unittest.skipUnless(LINK.exists(), f"no chip-tool-link at {LINK}")
class RequestTest(unittest.TestCase):
    """The first step: the call to make, with the Interaction Model's TLV."""

    def test_invoke(self):
        self.assertEqual(request(["onoff", "toggle", "102", "1"]), {
            "command": "matter.invoke",
            "params": {"cluster": 6, "command": 2, "endpoint": 1, "format": "tlv", "node_id": 102, "tlv": "FRg="},
        })

    def test_invoke_with_a_list_argument(self):
        # GetGroupMembership {groupList: [1]}
        self.assertEqual(request(["groups", "get-group-membership", "[1]", "102", "1"])["params"]["tlv"], "FTYABAEYGA==")

    def test_read(self):
        self.assertEqual(request(["onoff", "read", "on-off", "102", "1"]), {
            "command": "matter.read",
            "params": {"attribute": 0, "cluster": 6, "endpoint": 1, "fabric_filtered": True, "format": "tlv",
                       "node_id": 102},
        })

    def test_write(self):
        # OnTime (0x4001) = 5
        self.assertEqual(request(["onoff", "write", "on-time", "5", "102", "1"]), {
            "command": "matter.write",
            "params": {"attribute": 0x4001, "cluster": 6, "endpoint": 1, "format": "tlv", "node_id": 102, "tlv": "BAU="},
        })

    def test_refused(self):
        rc, lines = run(["onoff", "subscribe", "on-off", "1", "10", "102", "1"])
        self.assertNotEqual(rc, 0)
        self.assertIn("[TOO] chip-tool-link: subscriptions (poll with read instead): not available through the "
                      "gadget's matter.* commands", lines)

    def test_usage_is_chip_tools(self):
        rc, lines = run([])
        self.assertNotEqual(rc, 0)
        self.assertTrue(any("onoff" in line for line in lines), lines)


@unittest.skipUnless(LINK.exists(), f"no chip-tool-link at {LINK}")
class AnswerTest(unittest.TestCase):
    """The second step: the gadget's answer, through chip-tool's callbacks."""

    def test_invoke_status(self):
        rc, lines = run(["onoff", "toggle", "102", "1"], {"ok": True, "payload": {}})
        self.assertEqual(rc, 0)
        self.assertEqual(lines, [
            "[TOO] Sending command to node 0x66",
            "[TOO] Sending cluster (0x00000006) command (0x00000002) on endpoint 1",
            "[DMG] Received Command Response Status for Endpoint=1 Cluster=0x0000_0006 Command=0x0000_0002 Status=0x0",
        ])

    def test_invoke_failure(self):
        rc, lines = run(["onoff", "toggle", "102", "1"], {"ok": True, "payload": {"status": 1}})
        self.assertEqual(rc, 1)
        self.assertIn("[TOO] Error: IM Error 0x00000501: General error: 0x01 (FAILURE)", lines)

    def test_invoke_response(self):
        # GetGroupMembershipResponse {capacity: 3, groupList: [1]}
        rc, lines = run(["groups", "get-group-membership", "[1]", "102", "1"],
                        {"ok": True, "payload": {"response_command": 2, "response_tlv": "FSQAAzYBBAEYGA=="}})
        self.assertEqual(rc, 0)
        self.assertEqual(lines[2:], [
            "[DMG] Received Command Response Data, Endpoint=1 Cluster=0x0000_0004 Command=0x0000_0002",
            "[TOO] Endpoint: 1 Cluster: 0x0000_0004 Command 0x0000_0002",
            "[TOO]   GetGroupMembershipResponse: {",
            "[TOO]     capacity: 3",
            "[TOO]     groupList: 1 entries",
            "[TOO]       [1]: 1",
            "[TOO]    }",
        ])

    def test_read_value(self):
        rc, lines = run(["onoff", "read", "on-off", "102", "1"], {"ok": True, "payload": {"values": [
            {"endpoint": 1, "cluster": 6, "attribute": 0, "data_version": 7, "tlv": "CQ=="}]}})
        self.assertEqual(rc, 0)
        self.assertEqual(lines, [
            "[TOO] Sending command to node 0x66",
            "[TOO] Sending ReadAttribute to:",
            "[TOO] \tcluster 0x0000_0006, attribute: 0x0000_0000, endpoint 1",
            "[TOO] Endpoint: 1 Cluster: 0x0000_0006 Attribute 0x0000_0000 DataVersion: 7",
            "[TOO]   OnOff: TRUE",
        ])

    def test_read_status(self):
        rc, lines = run(["onoff", "read", "on-off", "102", "1"], {"ok": True, "payload": {"values": [
            {"endpoint": 1, "cluster": 6, "attribute": 0, "status": 0x86}]}})
        self.assertEqual(rc, 1)
        self.assertIn("[TOO] Response Failure: IM Error 0x00000586: General error: 0x86 (UNSUPPORTED_ATTRIBUTE)", lines)

    def test_write(self):
        rc, lines = run(["onoff", "write", "on-time", "5", "102", "1"], {"ok": True, "payload": {}})
        self.assertEqual(rc, 0)
        self.assertEqual(lines[-1], "[TOO] \tcluster 0x0000_0006, attribute: 0x0000_4001, endpoint 1")

    def test_gadget_error(self):
        rc, lines = run(["onoff", "toggle", "102", "1"],
                        {"ok": False, "error": {"code": "timeout", "message": "no answer from the device"}})
        self.assertEqual(rc, 1)
        self.assertIn("[TOO] The gadget: no answer from the device", lines)
        self.assertTrue(any("CHIP Error 0x00000032: Timeout" in line for line in lines), lines)

    def test_answer_from_stdin(self):
        done = subprocess.run([str(LINK), "onoff", "toggle", "102", "1", "--result", "-"],
                              input=json.dumps({"ok": True, "payload": {}}), capture_output=True, text=True, timeout=60)
        self.assertEqual(done.returncode, 0)
        self.assertIn("Status=0x0", done.stdout + done.stderr)


if __name__ == "__main__":
    unittest.main()

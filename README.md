# chip-tool-link

chip-tool's cluster commands for a Muse gadget whose firmware is a Matter
controller: Muse's Gadget SDK with its Matter controller component, which
offers `matter.invoke`, `matter.read` and `matter.write` as Link commands. A
program in Muse's VM can't call Link commands, only the agent can, so
chip-tool-link takes two steps with the same chip-tool command line:

```sh
$ chip-tool-link onoff toggle 102 1
{"command":"matter.invoke","params":{"cluster":6,"command":2,"endpoint":1,"format":"tlv","node_id":102,"tlv":"FRg="}}
# the agent makes that call; then, with the gadget's answer:
$ chip-tool-link onoff toggle 102 1 --result '{"ok":true,"payload":{}}'
[…] [TOO] Sending command to node 0x66
[…] [TOO] Sending cluster (0x00000006) command (0x00000002) on endpoint 1
[…] [DMG] Received Command Response Status for Endpoint=1 Cluster=0x0000_0006 Command=0x0000_0002 Status=0x0
```

`--result -` reads the answer from stdin. The exit status is chip-tool's.

## What is chip-tool's and what is ours

Compiled unmodified from a connectedhomeip checkout:
- the generated `zzz_generated/chip-tool/.../Commands.h` (every cluster command,
  read and write, with its arguments and ranges);
- `ComplexArgumentParser`, `DataModelLogger` and `EntryToText`;
- `commands/common/Command.cpp` and `Commands.cpp` (argument parsing, usage,
  listings, errors);
- `ClusterCommand.h`, `ReportCommand.h`, `WriteAttributeCommand.h`,
  `ModelCommand.h` and `CHIPCommand.h`;
- the cluster objects' `Encode`/`Decode`, TLV and stdio logging.

Ours:
- `shim/.../InteractionModel.h`: found before CHIP's on the include path. It
  keeps the classes chip-tool's commands derive from, but a request becomes
  the gadget's call, and the answer goes back through chip-tool's own
  callbacks.
- `src/ModelCommand.cpp` and `src/CHIPCommand.cpp`, in place of chip-tool's:
  there is no device to connect to and no stack to start.
- `src/Link.{h,cpp}`: the JSON, base64 and the answer's replay.
- `src/main.cpp` and `src/Stubs.cpp`.

## The gadget's side

The firmware needs `format: "tlv"` on `matter.invoke`, `matter.read` and
`matter.write`, an addition to the Gadget SDK's Matter controller that isn't
upstream yet. The request and the answer carry the Interaction Model's TLV as
base64, and statuses as numbers. No JSON conversion happens, so esp-matter's
notation limits don't apply.

## Not available through the gadget

These stop with a message:
- subscriptions and events (poll with `read`);
- group commands and writes;
- more than one endpoint or path at a time (0xFFFF and 0xFFFFFFFF wildcards
  work);
- `--repeat-count`, `--busyWaitForMs`, `--suppressResponse` and
  `--data-version`.

chip-tool's other common options are accepted and ignored. Commissioning
stays on the `matter.*` commands.

## Get it

Releases have `chip-tool-link-<version>-linux-x86_64.tgz`, with its `.sha256`:
`bin/chip-tool-link` (stripped, about 11 MB), this README and the license. It
needs only libraries a stock Linux image has.

## Build and test

```sh
. CHIP_ROOT/scripts/activate.sh                # CHIP's environment, with gn
./build.sh CHIP_ROOT out                       # tested at 7d3dfb6c06b3cc696186f86d527b2f387a5603df
python3 -m unittest discover -s tests -p 'test_cli.py' -v   # no gadget needed
```

`tests/e2e_bench.py` compares chip-tool-link, through a gadget on a serial
console, with chip-tool on the same device (see its docstring for the bench).

## License

Apache-2.0 (`LICENSE`). `shim/.../InteractionModel.h` mirrors CHIP's classes
and keeps its header.

// SPDX-License-Identifier: Apache-2.0

// chip-tool-link's CHIPCommand, in place of chip-tool's CHIPCommand.cpp (its
// header is chip-tool's, unmodified): no stack, fabric or storage. A command
// prints its request, or replays the gadget's answer, before RunCommand
// returns, so there is nothing to wait for.

#include "commands/common/CHIPCommand.h"

CHIP_ERROR CHIPCommand::Run()
{
    // As chip-tool: an error from RunCommand is the result; otherwise the
    // status the command finished with.
    CHIP_ERROR err = RunCommand();
    if (err == CHIP_NO_ERROR)
    {
        err = mCommandExitStatus;
    }
    Shutdown();
    Cleanup();
    return err;
}

void CHIPCommand::StopWaiting() {}

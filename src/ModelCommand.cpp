// SPDX-License-Identifier: Apache-2.0

// chip-tool-link's ModelCommand, in place of chip-tool's ModelCommand.cpp
// (its header is chip-tool's, unmodified): the command runs at once, with no
// device to connect to; the gadget does that.

#include "commands/clusters/ModelCommand.h"

#include <Link.h>

#include <inttypes.h>

using namespace ::chip;

CHIP_ERROR ModelCommand::RunCommand()
{
    VerifyOrReturnError(!IsGroupId(mDestinationId), chip_tool_link::Unsupported("group commands"));
    VerifyOrReturnError(mEndPointId.size() <= 1,
                        chip_tool_link::Unsupported("more than one endpoint at a time (use 0xFFFF to read all)"));

    ChipLogProgress(chipTool, "Sending command to node 0x%" PRIx64, mDestinationId);
    chip_tool_link::SetNode(mDestinationId);
    return SendCommand(nullptr, mEndPointId);
}

void ModelCommand::OnDeviceConnectedFn(void * context, Messaging::ExchangeManager & exchangeMgr,
                                       const SessionHandle & sessionHandle)
{}

void ModelCommand::OnDeviceConnectionFailureFn(void * context, const ScopedNodeId & peerId, CHIP_ERROR err) {}

void ModelCommand::Shutdown()
{
    mOnDeviceConnectedCallback.Cancel();
    mOnDeviceConnectionFailureCallback.Cancel();

    CHIPCommand::Shutdown();
}

// No ICD client here: the gadget registers with devices and keeps their keys.
void ModelCommand::ClearICDEntry(const ScopedNodeId & nodeId) {}

void ModelCommand::StoreICDEntryWithKey(app::ICDClientInfo & clientInfo, ByteSpan key) {}

void ModelCommand::CheckPeerICDType() {}

bool ModelCommand::IsPeerLIT()
{
    return mIsPeerLIT.ValueOr(false);
}

bool ModelCommand::AllowLargePayload()
{
    return mAllowLargePayload.ValueOr(false);
}

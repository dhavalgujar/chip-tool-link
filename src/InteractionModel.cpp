// SPDX-License-Identifier: Apache-2.0

// The out-of-line half of the stand-in InteractionModel.h (shim/): reads go
// to the gadget's matter.read, one path at a time; subscriptions and events
// aren't available through it.

#include <app/tests/suites/commands/interaction_model/InteractionModel.h>

using namespace chip;

CHIP_ERROR InteractionModelReports::ReportAttribute(DeviceProxy * device, std::vector<EndpointId> endpointIds,
                                                    std::vector<ClusterId> clusterIds, std::vector<AttributeId> attributeIds,
                                                    app::ReadClient::InteractionType interactionType)
{
    VerifyOrReturnError(interactionType == app::ReadClient::InteractionType::Read,
                        chip_tool_link::Unsupported("subscriptions (poll with read instead)"));
    VerifyOrReturnError(!mDataVersions.HasValue(), chip_tool_link::Unsupported("--data-version"));
    VerifyOrReturnError(endpointIds.size() == 1 && clusterIds.size() == 1 && attributeIds.size() == 1,
                        chip_tool_link::Unsupported("reading more than one path at a time (use 0xFFFF / 0xFFFFFFFF wildcards)"));

    // As chip-tool logs the read (InteractionModel.cpp, GetAttributePaths).
    ChipLogProgress(chipTool, "Sending ReadAttribute to:");
    ChipLogProgress(chipTool, "\tcluster " ChipLogFormatMEI ", attribute: " ChipLogFormatMEI ", endpoint %u",
                    ChipLogValueMEI(clusterIds[0]), ChipLogValueMEI(attributeIds[0]), endpointIds[0]);

    if (chip_tool_link::Answering())
    {
        return chip_tool_link::AnswerRead(*mCallback);
    }
    return chip_tool_link::RequestRead(endpointIds[0], clusterIds[0], attributeIds[0], mFabricFiltered.ValueOr(true), *mCallback);
}

CHIP_ERROR InteractionModelReports::ReportEvent(DeviceProxy * device, std::vector<EndpointId> endpointIds,
                                                std::vector<ClusterId> clusterIds, std::vector<EventId> eventIds,
                                                app::ReadClient::InteractionType interactionType)
{
    return chip_tool_link::Unsupported("events");
}

CHIP_ERROR InteractionModelReports::ReportNone(DeviceProxy * device, app::ReadClient::InteractionType interactionType)
{
    return chip_tool_link::Unsupported("read-none and subscribe-none");
}

CHIP_ERROR InteractionModelReports::ReportAll(DeviceProxy * device, std::vector<EndpointId> endpointIds,
                                              std::vector<ClusterId> clusterIds, std::vector<AttributeId> attributeIds,
                                              std::vector<EventId> eventIds, app::ReadClient::InteractionType interactionType)
{
    return chip_tool_link::Unsupported("read-all and subscribe-all, which include events (use read-by-id)");
}

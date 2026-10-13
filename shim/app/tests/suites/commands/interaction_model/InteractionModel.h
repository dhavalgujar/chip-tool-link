/*
 *   Copyright (c) 2022 Project CHIP Authors
 *   All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

// chip-tool-link's stand-in for CHIP's
// src/app/tests/suites/commands/interaction_model/InteractionModel.h. It is
// found first on the include path, so chip-tool's ClusterCommand,
// ReportCommand and WriteAttribute compile unmodified on top of it. It keeps
// the classes, members and setters chip-tool uses; what used to build a
// CommandSender, ReadClient or WriteClient now goes to Link.h instead: the
// request becomes the gadget's matter.* call, and the gadget's answer comes
// back through chip-tool's own callbacks. The test suites' InteractionModel
// class isn't here: chip-tool's commands don't use it.

#pragma once

#include <app/BufferedReadCallback.h>
#include <app/ChunkedWriteCallback.h>
#include <app/CommandSender.h>
#include <app/DeviceProxy.h>
#include <app/InteractionModelEngine.h>
#include <app/ReadClient.h>
#include <app/WriteClient.h>
#include <app/data-model/Encode.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/UnitTestUtils.h>

#include <Link.h>

#include <type_traits>
#include <vector>

inline constexpr uint8_t kMaxAllowedPaths = 64;

class InteractionModelReports
{
public:
    InteractionModelReports(chip::app::ReadClient::Callback * callback) : mCallback(callback) { ResetOptions(); }

protected:
    CHIP_ERROR ReadAttribute(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds,
                             std::vector<chip::ClusterId> clusterIds, std::vector<chip::AttributeId> attributeIds)
    {
        return ReportAttribute(device, endpointIds, clusterIds, attributeIds, chip::app::ReadClient::InteractionType::Read);
    }

    CHIP_ERROR SubscribeAttribute(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds,
                                  std::vector<chip::ClusterId> clusterIds, std::vector<chip::AttributeId> attributeIds)
    {
        return ReportAttribute(device, endpointIds, clusterIds, attributeIds, chip::app::ReadClient::InteractionType::Subscribe);
    }

    CHIP_ERROR ReportAttribute(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds,
                               std::vector<chip::ClusterId> clusterIds, std::vector<chip::AttributeId> attributeIds,
                               chip::app::ReadClient::InteractionType interactionType);

    CHIP_ERROR ReadEvent(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds,
                         std::vector<chip::ClusterId> clusterIds, std::vector<chip::EventId> eventIds)
    {
        return ReportEvent(device, endpointIds, clusterIds, eventIds, chip::app::ReadClient::InteractionType::Read);
    }

    CHIP_ERROR SubscribeEvent(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds,
                              std::vector<chip::ClusterId> clusterIds, std::vector<chip::EventId> eventIds)
    {
        return ReportEvent(device, endpointIds, clusterIds, eventIds, chip::app::ReadClient::InteractionType::Subscribe);
    }

    CHIP_ERROR ReportEvent(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds,
                           std::vector<chip::ClusterId> clusterIds, std::vector<chip::EventId> eventIds,
                           chip::app::ReadClient::InteractionType interactionType);

    CHIP_ERROR ReadNone(chip::DeviceProxy * device) { return ReportNone(device, chip::app::ReadClient::InteractionType::Read); }

    CHIP_ERROR ReadAll(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds,
                       std::vector<chip::ClusterId> clusterIds, std::vector<chip::AttributeId> attributeIds,
                       std::vector<chip::EventId> eventIds)
    {
        return ReportAll(device, endpointIds, clusterIds, attributeIds, eventIds, chip::app::ReadClient::InteractionType::Read);
    }

    CHIP_ERROR SubscribeNone(chip::DeviceProxy * device)
    {
        return ReportNone(device, chip::app::ReadClient::InteractionType::Subscribe);
    }

    CHIP_ERROR SubscribeAll(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds,
                            std::vector<chip::ClusterId> clusterIds, std::vector<chip::AttributeId> attributeIds,
                            std::vector<chip::EventId> eventIds)
    {
        return ReportAll(device, endpointIds, clusterIds, attributeIds, eventIds,
                         chip::app::ReadClient::InteractionType::Subscribe);
    }

    CHIP_ERROR ReportNone(chip::DeviceProxy * device, chip::app::ReadClient::InteractionType interactionType);

    CHIP_ERROR ReportAll(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds,
                         std::vector<chip::ClusterId> clusterIds, std::vector<chip::AttributeId> attributeIds,
                         std::vector<chip::EventId> eventIds, chip::app::ReadClient::InteractionType interactionType);

    void OnDeallocatePaths(chip::app::ReadPrepareParams && aReadPrepareParams) {}

    void Shutdown() {}

    void CleanupReadClient(chip::app::ReadClient * aReadClient) {}

    chip::app::ReadClient::Callback * mCallback;

    InteractionModelReports & SetDataVersions(const std::vector<chip::DataVersion> & dataVersions)
    {
        mDataVersions.SetValue(dataVersions);
        return *this;
    }

    InteractionModelReports & SetDataVersions(const chip::Optional<std::vector<chip::DataVersion>> & dataVersions)
    {
        mDataVersions = dataVersions;
        return *this;
    }

    InteractionModelReports & SetIsUrgents(const std::vector<bool> isUrgents)
    {
        mIsUrgents.SetValue(isUrgents);
        return *this;
    }

    InteractionModelReports & SetIsUrgents(const chip::Optional<std::vector<bool>> & isUrgents)
    {
        mIsUrgents = isUrgents;
        return *this;
    }

    InteractionModelReports & SetEventNumber(const chip::Optional<chip::EventNumber> & eventNumber)
    {
        mEventNumber = eventNumber;
        return *this;
    }

    InteractionModelReports & SetEventNumber(chip::EventNumber eventNumber)
    {
        mEventNumber.SetValue(eventNumber);
        return *this;
    }

    InteractionModelReports & SetFabricFiltered(bool fabricFiltered)
    {
        mFabricFiltered.SetValue(fabricFiltered);
        return *this;
    }

    InteractionModelReports & SetKeepSubscriptions(bool keepSubscriptions)
    {
        mKeepSubscriptions.SetValue(keepSubscriptions);
        return *this;
    }

    InteractionModelReports & SetKeepSubscriptions(const chip::Optional<bool> & keepSubscriptions)
    {
        mKeepSubscriptions = keepSubscriptions;
        return *this;
    }

    InteractionModelReports & SetAutoResubscribe(bool autoResubscribe)
    {
        mAutoResubscribe.SetValue(autoResubscribe);
        return *this;
    }

    InteractionModelReports & SetAutoResubscribe(const chip::Optional<bool> & autoResubscribe)
    {
        mAutoResubscribe = autoResubscribe;
        return *this;
    }

    InteractionModelReports & SetMinInterval(uint16_t minInterval)
    {
        mMinInterval = minInterval;
        return *this;
    }

    InteractionModelReports & SetMaxInterval(uint16_t maxInterval)
    {
        mMaxInterval = maxInterval;
        return *this;
    }

    InteractionModelReports & SetPeerLIT(bool isPeerLIT)
    {
        mIsPeerLIT = isPeerLIT;
        return *this;
    }

    void ResetOptions()
    {
        mDataVersions      = chip::NullOptional;
        mIsUrgents         = chip::NullOptional;
        mEventNumber       = chip::NullOptional;
        mFabricFiltered    = chip::Optional<bool>(true);
        mKeepSubscriptions = chip::NullOptional;
        mAutoResubscribe   = chip::NullOptional;
        mIsPeerLIT         = false;
        mMinInterval       = 0;
        mMaxInterval       = 0;
    }

    chip::Optional<std::vector<chip::DataVersion>> mDataVersions;
    chip::Optional<std::vector<bool>> mIsUrgents;
    chip::Optional<chip::EventNumber> mEventNumber;
    chip::Optional<bool> mFabricFiltered;
    chip::Optional<bool> mKeepSubscriptions;
    chip::Optional<bool> mAutoResubscribe;
    bool mIsPeerLIT;
    uint16_t mMinInterval;
    uint16_t mMaxInterval;
};

class InteractionModelCommands
{
public:
    InteractionModelCommands(chip::app::CommandSender::Callback * callback) : mCallback(callback) { ResetOptions(); }

protected:
    template <class T>
    CHIP_ERROR SendCommand(chip::DeviceProxy * device, chip::EndpointId endpointId, chip::ClusterId clusterId,
                           chip::CommandId commandId, const T & value)
    {
        VerifyOrReturnError(!mRepeatCount.HasValue() && !mRepeatDelayInMs.HasValue(),
                            chip_tool_link::Unsupported("--repeat-count and --repeat-delay-ms"));
        VerifyOrReturnError(!mBusyWaitForMs.HasValue(), chip_tool_link::Unsupported("--busyWaitForMs"));
        VerifyOrReturnError(!mSuppressResponse.ValueOr(false), chip_tool_link::Unsupported("--suppressResponse"));

        chip::app::ConcreteCommandPath path(endpointId, clusterId, commandId);
        if (chip_tool_link::Answering())
        {
            return chip_tool_link::AnswerInvoke(path, *mCallback);
        }
        // The command's fields, as CommandSender would put them in the
        // request, but with an anonymous tag: the gadget adds its own.
        std::vector<uint8_t> fields;
        ReturnErrorOnFailure(chip_tool_link::Encode(fields, [&](chip::TLV::TLVWriter & writer) {
            return chip::app::DataModel::Encode(writer, chip::TLV::AnonymousTag(), value);
        }));
        return chip_tool_link::RequestInvoke(path, fields, mTimedInteractionTimeoutMs, *mCallback);
    }

    template <class T>
    CHIP_ERROR SendGroupCommand(chip::GroupId groupId, chip::FabricIndex fabricIndex, chip::ClusterId clusterId,
                                chip::CommandId commandId, const T & value)
    {
        return chip_tool_link::Unsupported("group commands");
    }

    void Shutdown()
    {
        for (auto & commandSender : mCommandSender)
        {
            commandSender.reset();
        }
        mCommandSender.clear();
    }

    // Always empty: nothing is sent from here.
    std::vector<std::unique_ptr<chip::app::CommandSender>> mCommandSender;
    chip::app::CommandSender::Callback * mCallback;

    InteractionModelCommands & SetTimedInteractionTimeoutMs(uint16_t timedInteractionTimeoutMs)
    {
        mTimedInteractionTimeoutMs.SetValue(timedInteractionTimeoutMs);
        return *this;
    }

    InteractionModelCommands & SetTimedInteractionTimeoutMs(const chip::Optional<uint16_t> & timedInteractionTimeoutMs)
    {
        mTimedInteractionTimeoutMs = timedInteractionTimeoutMs;
        return *this;
    }

    InteractionModelCommands & SetSuppressResponse(bool suppressResponse)
    {
        mSuppressResponse.SetValue(suppressResponse);
        return *this;
    }

    InteractionModelCommands & SetSuppressResponse(const chip::Optional<bool> & suppressResponse)
    {
        mSuppressResponse = suppressResponse;
        return *this;
    }

    InteractionModelCommands & SetRepeatCount(uint16_t repeatCount)
    {
        mRepeatCount.SetValue(repeatCount);
        return *this;
    }

    InteractionModelCommands & SetRepeatCount(const chip::Optional<uint16_t> & repeatCount)
    {
        mRepeatCount = repeatCount;
        return *this;
    }

    InteractionModelCommands & SetRepeatDelayInMs(uint16_t repeatDelayInMs)
    {
        mRepeatDelayInMs.SetValue(repeatDelayInMs);
        return *this;
    }

    InteractionModelCommands & SetRepeatDelayInMs(const chip::Optional<uint16_t> & repeatDelayInMs)
    {
        mRepeatDelayInMs = repeatDelayInMs;
        return *this;
    }

    InteractionModelCommands & SetBusyWaitForMs(uint16_t busyWaitForMs)
    {
        mBusyWaitForMs.SetValue(busyWaitForMs);
        return *this;
    }

    InteractionModelCommands & SetBusyWaitForMs(const chip::Optional<uint16_t> & busyWaitForMs)
    {
        mBusyWaitForMs = busyWaitForMs;
        return *this;
    }

    void ResetOptions()
    {
        mTimedInteractionTimeoutMs = chip::NullOptional;
        mSuppressResponse          = chip::NullOptional;
        mRepeatCount               = chip::NullOptional;
        mRepeatDelayInMs           = chip::NullOptional;
        mBusyWaitForMs             = chip::NullOptional;
    }

    chip::Optional<uint16_t> mTimedInteractionTimeoutMs;
    chip::Optional<bool> mSuppressResponse;
    chip::Optional<uint16_t> mRepeatCount;
    chip::Optional<uint16_t> mRepeatDelayInMs;
    chip::Optional<uint16_t> mBusyWaitForMs;
};

class InteractionModelWriter
{
public:
    InteractionModelWriter(chip::app::WriteClient::Callback * callback) : mCallback(callback) {}

protected:
    template <class T>
    CHIP_ERROR WriteAttribute(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds,
                              std::vector<chip::ClusterId> clusterIds, std::vector<chip::AttributeId> attributeIds,
                              const std::vector<T> & values)
    {
        VerifyOrReturnError(!mRepeatCount.HasValue() && !mRepeatDelayInMs.HasValue(),
                            chip_tool_link::Unsupported("--repeat-count and --repeat-delay-ms"));
        VerifyOrReturnError(!mBusyWaitForMs.HasValue(), chip_tool_link::Unsupported("--busyWaitForMs"));
        VerifyOrReturnError(!mSuppressResponse.ValueOr(false), chip_tool_link::Unsupported("--suppressResponse"));
        VerifyOrReturnError(!mDataVersions.HasValue(), chip_tool_link::Unsupported("--data-version"));
        VerifyOrReturnError(endpointIds.size() == 1 && clusterIds.size() == 1 && attributeIds.size() == 1 && values.size() == 1,
                            chip_tool_link::Unsupported("writing more than one attribute or endpoint at a time"));

        // As chip-tool logs the write's path (GetAttributePaths).
        ChipLogProgress(chipTool, "\tcluster " ChipLogFormatMEI ", attribute: " ChipLogFormatMEI ", endpoint %u",
                        ChipLogValueMEI(clusterIds[0]), ChipLogValueMEI(attributeIds[0]), endpointIds[0]);
        chip::app::ConcreteDataAttributePath path(endpointIds[0], clusterIds[0], attributeIds[0]);
        if (chip_tool_link::Answering())
        {
            return chip_tool_link::AnswerWrite(path, *mCallback);
        }
        // The value, as WriteClient would put it in the request, but with an
        // anonymous tag.
        std::vector<uint8_t> data;
        ReturnErrorOnFailure(chip_tool_link::Encode(data, [&](chip::TLV::TLVWriter & writer) {
            return EncodeValue(writer, values[0]);
        }));
        return chip_tool_link::RequestWrite(path, data, mTimedInteractionTimeoutMs, *mCallback);
    }

    template <class T>
    CHIP_ERROR WriteAttribute(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds,
                              std::vector<chip::ClusterId> clusterIds, std::vector<chip::AttributeId> attributeIds, const T & value)
    {
        std::vector<T> values = { value };
        return WriteAttribute(device, endpointIds, clusterIds, attributeIds, values);
    }

    template <class T>
    CHIP_ERROR WriteGroupAttribute(chip::GroupId groupId, chip::FabricIndex fabricIndex, chip::ClusterId clusterId,
                                   chip::AttributeId attributeId, const std::vector<T> & value,
                                   const chip::Optional<chip::DataVersion> & dataVersion = chip::NullOptional)
    {
        return chip_tool_link::Unsupported("group writes");
    }

    template <class T>
    CHIP_ERROR WriteGroupAttribute(chip::GroupId groupId, chip::FabricIndex fabricIndex, chip::ClusterId clusterId,
                                   chip::AttributeId attributeId, const T & value,
                                   const chip::Optional<chip::DataVersion> & dataVersion = chip::NullOptional)
    {
        return chip_tool_link::Unsupported("group writes");
    }

    void Shutdown() {}

    chip::app::WriteClient::Callback * mCallback;

    InteractionModelWriter & SetTimedInteractionTimeoutMs(uint16_t timedInteractionTimeoutMs)
    {
        mTimedInteractionTimeoutMs.SetValue(timedInteractionTimeoutMs);
        return *this;
    }

    InteractionModelWriter & SetTimedInteractionTimeoutMs(const chip::Optional<uint16_t> & timedInteractionTimeoutMs)
    {
        mTimedInteractionTimeoutMs = timedInteractionTimeoutMs;
        return *this;
    }

    InteractionModelWriter & SetSuppressResponse(bool suppressResponse)
    {
        mSuppressResponse.SetValue(suppressResponse);
        return *this;
    }

    InteractionModelWriter & SetSuppressResponse(const chip::Optional<bool> & suppressResponse)
    {
        mSuppressResponse = suppressResponse;
        return *this;
    }

    InteractionModelWriter & SetDataVersions(const std::vector<chip::DataVersion> & dataVersions)
    {
        mDataVersions.SetValue(dataVersions);
        return *this;
    }

    InteractionModelWriter & SetDataVersions(const chip::Optional<std::vector<chip::DataVersion>> & dataVersions)
    {
        mDataVersions = dataVersions;
        return *this;
    }

    InteractionModelWriter & SetRepeatCount(uint16_t repeatCount)
    {
        mRepeatCount.SetValue(repeatCount);
        return *this;
    }

    InteractionModelWriter & SetRepeatCount(const chip::Optional<uint16_t> & repeatCount)
    {
        mRepeatCount = repeatCount;
        return *this;
    }

    InteractionModelWriter & SetRepeatDelayInMs(uint16_t repeatDelayInMs)
    {
        mRepeatDelayInMs.SetValue(repeatDelayInMs);
        return *this;
    }

    InteractionModelWriter & SetRepeatDelayInMs(const chip::Optional<uint16_t> & repeatDelayInMs)
    {
        mRepeatDelayInMs = repeatDelayInMs;
        return *this;
    }

    InteractionModelWriter & SetBusyWaitForMs(uint16_t busyWaitForMs)
    {
        mBusyWaitForMs.SetValue(busyWaitForMs);
        return *this;
    }

    InteractionModelWriter & SetBusyWaitForMs(const chip::Optional<uint16_t> & busyWaitForMs)
    {
        mBusyWaitForMs = busyWaitForMs;
        return *this;
    }

    void ResetOptions()
    {
        mTimedInteractionTimeoutMs = chip::NullOptional;
        mSuppressResponse          = chip::NullOptional;
        mDataVersions              = chip::NullOptional;
        mRepeatCount               = chip::NullOptional;
        mRepeatDelayInMs           = chip::NullOptional;
        mBusyWaitForMs             = chip::NullOptional;
    }

    chip::Optional<uint16_t> mTimedInteractionTimeoutMs;
    chip::Optional<std::vector<chip::DataVersion>> mDataVersions;
    chip::Optional<bool> mSuppressResponse;
    chip::Optional<uint16_t> mRepeatCount;
    chip::Optional<uint16_t> mRepeatDelayInMs;
    chip::Optional<uint16_t> mBusyWaitForMs;

private:
    // As WriteClient encodes a value: EncodeForWrite where the type has one
    // (fabric-scoped structs, and lists or nullables of them), else Encode;
    // write-by-id's CustomArgument values come through a pointer.
    template <typename T, typename = void>
    struct HasEncodeForWrite : std::false_type
    {
    };

    template <typename T>
    struct HasEncodeForWrite<T,
                             std::void_t<decltype(chip::app::DataModel::EncodeForWrite(
                                 std::declval<chip::TLV::TLVWriter &>(), std::declval<chip::TLV::Tag>(), std::declval<const T &>()))>>
        : std::true_type
    {
    };

    template <typename T>
    static CHIP_ERROR EncodeValue(chip::TLV::TLVWriter & writer, const T & value)
    {
        if constexpr (std::is_pointer<T>::value)
        {
            return EncodeValue(writer, *value);
        }
        else if constexpr (HasEncodeForWrite<T>::value)
        {
            return chip::app::DataModel::EncodeForWrite(writer, chip::TLV::AnonymousTag(), value);
        }
        else
        {
            return chip::app::DataModel::Encode(writer, chip::TLV::AnonymousTag(), value);
        }
    }
};

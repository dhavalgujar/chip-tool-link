// SPDX-License-Identifier: Apache-2.0

#pragma once

// What chip-tool's commands do in place of sending: the request becomes the
// gadget's matter.* call, printed as one line of JSON for the agent to make;
// with --result, the gadget's answer is handed to chip-tool's own callbacks,
// as CommandSender, ReadClient and WriteClient would hand a device's.
//
// The gadget carries the Interaction Model's TLV as it is (format "tlv"):
//   matter.invoke  {node_id, endpoint, cluster, command, tlv, format, timed_ms?}
//                  -> {} | {response_command, response_tlv} | {status, cluster_status?}
//   matter.read    {node_id, endpoint, cluster, attribute, fabric_filtered, format}
//                  -> {values: [{endpoint, cluster, attribute, data_version?,
//                                tlv | status, cluster_status?}], truncated?}
//   matter.write   {node_id, endpoint, cluster, attribute, tlv, format, timed_ms?}
//                  -> {} | {status, cluster_status?}
// tlv values are base64, each one element with an anonymous tag.

#include <app/CommandSender.h>
#include <app/ConcreteAttributePath.h>
#include <app/ConcreteCommandPath.h>
#include <app/ReadClient.h>
#include <app/WriteClient.h>
#include <lib/core/CHIPError.h>
#include <lib/core/NodeId.h>
#include <lib/core/Optional.h>
#include <lib/core/TLVWriter.h>

#include <cstdint>
#include <vector>

namespace chip_tool_link {

// The largest request TLV the gadget takes.
inline constexpr size_t kMaxRequestTLV = 2048;

// Logs that `what` isn't available through the gadget, and returns the error
// to stop the command with.
CHIP_ERROR Unsupported(const char * what);

// The node the command is for (chip-tool's destination-id).
void SetNode(chip::NodeId node);

// Takes the gadget's answer: a link.result envelope ({ok, payload | error})
// or just its payload. False if it isn't JSON.
bool SetAnswer(const char * json);
// True when there is an answer to replay rather than a request to print.
bool Answering();

template <typename F>
CHIP_ERROR Encode(std::vector<uint8_t> & out, F && encode)
{
    out.resize(kMaxRequestTLV);
    chip::TLV::TLVWriter writer;
    writer.Init(out.data(), out.size());
    CHIP_ERROR err = encode(writer);
    if (err == CHIP_NO_ERROR)
    {
        err = writer.Finalize();
    }
    if (err == CHIP_ERROR_NO_MEMORY || err == CHIP_ERROR_BUFFER_TOO_SMALL)
    {
        return Unsupported("a request over 2048 bytes of TLV");
    }
    ReturnErrorOnFailure(err);
    out.resize(writer.GetLengthWritten());
    return CHIP_NO_ERROR;
}

CHIP_ERROR RequestInvoke(const chip::app::ConcreteCommandPath & path, const std::vector<uint8_t> & fields,
                         const chip::Optional<uint16_t> & timedMs, chip::app::CommandSender::Callback & callback);
CHIP_ERROR AnswerInvoke(const chip::app::ConcreteCommandPath & path, chip::app::CommandSender::Callback & callback);

CHIP_ERROR RequestRead(chip::EndpointId endpoint, chip::ClusterId cluster, chip::AttributeId attribute, bool fabricFiltered,
                       chip::app::ReadClient::Callback & callback);
CHIP_ERROR AnswerRead(chip::app::ReadClient::Callback & callback);

CHIP_ERROR RequestWrite(const chip::app::ConcreteDataAttributePath & path, const std::vector<uint8_t> & value,
                        const chip::Optional<uint16_t> & timedMs, chip::app::WriteClient::Callback & callback);
CHIP_ERROR AnswerWrite(const chip::app::ConcreteDataAttributePath & path, chip::app::WriteClient::Callback & callback);

} // namespace chip_tool_link

// SPDX-License-Identifier: Apache-2.0

#include "Link.h"

#include <app/MessageDef/StatusIB.h>
#include <json/json.h>
#include <lib/support/Base64.h>
#include <lib/support/logging/CHIPLogging.h>
#include <protocols/interaction_model/StatusCode.h>

#include <stdio.h>

#include <memory>
#include <sstream>
#include <string>

using namespace chip;

namespace chip_tool_link {
namespace {

NodeId sNode = kUndefinedNodeId;
bool sAnswering = false;
bool sOk = false;
std::string sError;  // the gadget's message, when !sOk
Json::Value sPayload; // the answer, when sOk

void Print(const char * command, Json::Value params)
{
    Json::Value call;
    call["command"] = command;
    params["node_id"] = Json::UInt64(sNode);
    params["format"] = "tlv";
    call["params"] = params;
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    printf("%s\n", Json::writeString(builder, call).c_str());
    fflush(stdout);
}

std::string ToBase64(const std::vector<uint8_t> & tlv)
{
    std::string out(BASE64_ENCODED_LEN(tlv.size()), '\0');
    uint32_t len = Base64Encode32(tlv.data(), static_cast<uint32_t>(tlv.size()), out.data());
    out.resize(len);
    return out;
}

// One TLV element in base64, positioned on for reading. Null if it isn't one.
struct Element
{
    std::vector<uint8_t> bytes;
    TLV::TLVReader reader;
};

std::unique_ptr<Element> FromBase64(const Json::Value & value)
{
    if (!value.isString())
    {
        return nullptr;
    }
    const std::string text = value.asString();
    auto element = std::make_unique<Element>();
    element->bytes.resize(BASE64_MAX_DECODED_LEN(text.size()) + 3);
    uint32_t len = Base64Decode32(text.data(), static_cast<uint32_t>(text.size()), element->bytes.data());
    if (len == UINT32_MAX)
    {
        return nullptr;
    }
    element->bytes.resize(len);
    element->reader.Init(element->bytes.data(), element->bytes.size());
    if (element->reader.Next() != CHIP_NO_ERROR)
    {
        return nullptr;
    }
    return element;
}

bool GetUInt(const Json::Value & object, const char * key, uint64_t max, uint64_t & out)
{
    const Json::Value & value = object[key];
    if (!value.isUInt64() || value.asUInt64() > max)
    {
        return false;
    }
    out = value.asUInt64();
    return true;
}

// {status, cluster_status?} as an IM status; success when there is no status.
bool GetStatus(const Json::Value & object, app::StatusIB & status)
{
    status = app::StatusIB();
    uint64_t code = 0;
    if (!object.isMember("status"))
    {
        return true;
    }
    if (!GetUInt(object, "status", UINT8_MAX, code))
    {
        return false;
    }
    status.mStatus = static_cast<Protocols::InteractionModel::Status>(code);
    if (object.isMember("cluster_status"))
    {
        uint64_t clusterStatus = 0;
        if (!GetUInt(object, "cluster_status", UINT8_MAX, clusterStatus))
        {
            return false;
        }
        status.mClusterStatus = static_cast<ClusterStatus>(clusterStatus);
    }
    return true;
}

// The gadget refused or couldn't do it. Its error code doesn't survive the
// Link envelope, only its message; CHIP's nearest error stands in for it.
CHIP_ERROR GadgetError()
{
    ChipLogError(chipTool, "The gadget: %s", sError.c_str());
    const bool noAnswer = sError.find("timeout") != std::string::npos || sError.find("answer") != std::string::npos ||
        sError.find("unreachable") != std::string::npos;
    return noAnswer ? CHIP_ERROR_TIMEOUT : CHIP_ERROR_INTERNAL;
}

CHIP_ERROR BadAnswer(const char * what)
{
    ChipLogError(chipTool, "chip-tool-link: the answer isn't a %s result in the \"tlv\" format", what);
    return CHIP_ERROR_INVALID_ARGUMENT;
}

} // namespace

CHIP_ERROR Unsupported(const char * what)
{
    ChipLogError(chipTool, "chip-tool-link: %s: not available through the gadget's matter.* commands", what);
    return CHIP_ERROR_NOT_IMPLEMENTED;
}

void SetNode(NodeId node)
{
    sNode = node;
}

bool SetAnswer(const char * json)
{
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;
    std::istringstream in(json);
    if (!Json::parseFromStream(builder, in, &root, &errors) || !root.isObject())
    {
        return false;
    }
    sAnswering = true;
    if (!root.isMember("ok"))
    {
        sOk      = true;
        sPayload = root;
        return true;
    }
    sOk = root["ok"].asBool();
    if (sOk)
    {
        sPayload = root.isMember("payload") ? root["payload"] : Json::Value(Json::objectValue);
        return true;
    }
    const Json::Value & error = root["error"];
    sError = error.isString() ? error.asString() : error.isObject() ? error["message"].asString() : "failed";
    return true;
}

bool Answering()
{
    return sAnswering;
}

CHIP_ERROR RequestInvoke(const app::ConcreteCommandPath & path, const std::vector<uint8_t> & fields,
                         const Optional<uint16_t> & timedMs, app::CommandSender::Callback & callback)
{
    Json::Value params;
    params["endpoint"] = path.mEndpointId;
    params["cluster"]  = path.mClusterId;
    params["command"]  = path.mCommandId;
    params["tlv"]      = ToBase64(fields);
    if (timedMs.HasValue())
    {
        params["timed_ms"] = timedMs.Value();
    }
    Print("matter.invoke", params);
    callback.OnDone(nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR AnswerInvoke(const app::ConcreteCommandPath & path, app::CommandSender::Callback & callback)
{
    if (!sOk)
    {
        callback.OnError(nullptr, GadgetError());
        callback.OnDone(nullptr);
        return CHIP_NO_ERROR;
    }

    app::StatusIB status;
    VerifyOrReturnError(sPayload.isObject() && GetStatus(sPayload, status), BadAnswer("matter.invoke"));
    if (sPayload.isMember("response_tlv"))
    {
        // As CommandSender logs and hands over a response with data.
        uint64_t command = 0;
        auto data        = FromBase64(sPayload["response_tlv"]);
        VerifyOrReturnError(data != nullptr && GetUInt(sPayload, "response_command", UINT32_MAX, command),
                            BadAnswer("matter.invoke"));
        app::ConcreteCommandPath responsePath(path.mEndpointId, path.mClusterId, static_cast<CommandId>(command));
        ChipLogProgress(DataManagement,
                        "Received Command Response Data, Endpoint=%u Cluster=" ChipLogFormatMEI " Command=" ChipLogFormatMEI,
                        responsePath.mEndpointId, ChipLogValueMEI(responsePath.mClusterId),
                        ChipLogValueMEI(responsePath.mCommandId));
        callback.OnResponse(nullptr, responsePath, status, &data->reader);
    }
    else
    {
        // As CommandSender logs and hands over a status.
        ChipLogProgress(DataManagement,
                        "Received Command Response Status for Endpoint=%u Cluster=" ChipLogFormatMEI " Command=" ChipLogFormatMEI
                        " Status=0x%x",
                        path.mEndpointId, ChipLogValueMEI(path.mClusterId), ChipLogValueMEI(path.mCommandId),
                        to_underlying(status.mStatus));
        if (status.IsSuccess())
        {
            callback.OnResponse(nullptr, path, status, nullptr);
        }
        else
        {
            callback.OnError(nullptr, status.ToChipError());
        }
    }
    callback.OnDone(nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR RequestRead(EndpointId endpoint, ClusterId cluster, AttributeId attribute, bool fabricFiltered,
                       app::ReadClient::Callback & callback)
{
    Json::Value params;
    params["endpoint"]        = endpoint;
    params["cluster"]         = cluster;
    params["attribute"]       = attribute;
    params["fabric_filtered"] = fabricFiltered;
    Print("matter.read", params);
    callback.OnDone(nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR AnswerRead(app::ReadClient::Callback & callback)
{
    if (!sOk)
    {
        callback.OnError(GadgetError());
        callback.OnDone(nullptr);
        return CHIP_NO_ERROR;
    }

    const Json::Value & values = sPayload["values"];
    VerifyOrReturnError(values.isArray(), BadAnswer("matter.read"));
    for (const Json::Value & value : values)
    {
        uint64_t endpoint = 0, cluster = 0, attribute = 0, dataVersion = 0;
        app::StatusIB status;
        VerifyOrReturnError(value.isObject() && GetUInt(value, "endpoint", UINT16_MAX, endpoint) &&
                                GetUInt(value, "cluster", UINT32_MAX, cluster) &&
                                GetUInt(value, "attribute", UINT32_MAX, attribute) && GetStatus(value, status),
                            BadAnswer("matter.read"));
        app::ConcreteDataAttributePath path(static_cast<EndpointId>(endpoint), static_cast<ClusterId>(cluster),
                                            static_cast<AttributeId>(attribute));
        if (value.isMember("data_version"))
        {
            VerifyOrReturnError(GetUInt(value, "data_version", UINT32_MAX, dataVersion), BadAnswer("matter.read"));
            path.mDataVersion.SetValue(static_cast<DataVersion>(dataVersion));
        }
        if (value.isMember("tlv"))
        {
            // As BufferedReadCallback hands over a whole attribute.
            auto data = FromBase64(value["tlv"]);
            VerifyOrReturnError(data != nullptr, BadAnswer("matter.read"));
            callback.OnAttributeData(path, &data->reader, status);
        }
        else
        {
            callback.OnAttributeData(path, nullptr, status);
        }
    }
    if (sPayload["truncated"].asBool())
    {
        ChipLogError(chipTool, "chip-tool-link: the gadget sent only the first %u values; read a narrower path",
                     static_cast<unsigned>(values.size()));
    }
    callback.OnDone(nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR RequestWrite(const app::ConcreteDataAttributePath & path, const std::vector<uint8_t> & value,
                        const Optional<uint16_t> & timedMs, app::WriteClient::Callback & callback)
{
    Json::Value params;
    params["endpoint"]  = path.mEndpointId;
    params["cluster"]   = path.mClusterId;
    params["attribute"] = path.mAttributeId;
    params["tlv"]       = ToBase64(value);
    if (timedMs.HasValue())
    {
        params["timed_ms"] = timedMs.Value();
    }
    Print("matter.write", params);
    callback.OnDone(nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR AnswerWrite(const app::ConcreteDataAttributePath & path, app::WriteClient::Callback & callback)
{
    if (!sOk)
    {
        callback.OnError(nullptr, GadgetError());
        callback.OnDone(nullptr);
        return CHIP_NO_ERROR;
    }

    app::StatusIB status;
    VerifyOrReturnError(sPayload.isObject() && GetStatus(sPayload, status), BadAnswer("matter.write"));
    callback.OnResponse(nullptr, path, status);
    callback.OnDone(nullptr);
    return CHIP_NO_ERROR;
}

} // namespace chip_tool_link

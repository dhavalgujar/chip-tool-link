// SPDX-License-Identifier: Apache-2.0

// What chip-tool's ClusterCommand.h reaches for but chip-tool-link has no use
// for: diagnostic logs over BDX need a session of our own with the device.

#include "commands/common/BDXDiagnosticLogsServerDelegate.h"

BDXDiagnosticLogsServerDelegate BDXDiagnosticLogsServerDelegate::sInstance;

void BDXDiagnosticLogsServerDelegate::AddFileDesignator(chip::app::CommandSender * sender, const chip::CharSpan & fileDesignator) {}

void BDXDiagnosticLogsServerDelegate::RemoveFileDesignator(chip::app::CommandSender * sender) {}

CHIP_ERROR BDXDiagnosticLogsServerDelegate::OnTransferBegin(chip::bdx::BDXTransferProxy * transfer)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}

CHIP_ERROR BDXDiagnosticLogsServerDelegate::OnTransferEnd(chip::bdx::BDXTransferProxy * transfer, CHIP_ERROR error)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}

CHIP_ERROR BDXDiagnosticLogsServerDelegate::OnTransferData(chip::bdx::BDXTransferProxy * transfer, const chip::ByteSpan & data)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}

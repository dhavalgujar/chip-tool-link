// SPDX-License-Identifier: Apache-2.0

// chip-tool-link: chip-tool's cluster commands, for a gadget whose firmware
// is the Matter controller (matter.* Link commands). A program in the agent's
// VM can't call Link commands, so it takes two steps with the same chip-tool
// command line:
//
//   chip-tool-link onoff toggle 100 1
//       prints the matter.invoke call for the agent to make;
//   chip-tool-link onoff toggle 100 1 --result '<the gadget's answer>'
//       (or --result - to read it from stdin) prints what chip-tool would
//       print for that answer, and exits as chip-tool would.

#include "commands/common/Commands.h"
#include "commands/common/CredentialIssuerCommands.h"

#include <zap-generated/cluster/Commands.h>

#include <Link.h>

#include <app/MessageDef/StatusIB.h>
#include <lib/core/CHIPError.h>
#include <lib/support/logging/CHIPLogging.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

// The commands take one, but nothing here commissions or signs: the gadget
// holds the fabric.
class NoCredentialIssuer : public CredentialIssuerCommands
{
public:
    CHIP_ERROR InitializeCredentialsIssuer(chip::PersistentStorageDelegate & storage) override
    {
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }
    CHIP_ERROR SetupDeviceAttestation(chip::Controller::SetupParams & setupParams,
                                      const chip::Credentials::AttestationTrustStore * trustStore,
                                      chip::Credentials::DeviceAttestationRevocationDelegate * revocationDelegate) override
    {
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }
    CHIP_ERROR AddAdditionalCDVerifyingCerts(const std::vector<std::vector<uint8_t>> & additionalCdCerts) override
    {
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }
    chip::Controller::OperationalCredentialsDelegate * GetCredentialIssuer() override { return nullptr; }
    void SetCredentialIssuerCATValues(chip::CATValues cats) override {}
    CHIP_ERROR GenerateControllerNOCChain(chip::NodeId nodeId, chip::FabricId fabricId, const chip::CATValues & cats,
                                          chip::Crypto::P256Keypair & keypair, chip::MutableByteSpan & rcac,
                                          chip::MutableByteSpan & icac, chip::MutableByteSpan & noc) override
    {
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }
};

} // namespace

int main(int argc, char * argv[])
{
    // --result is ours; everything else is chip-tool's command line.
    std::vector<char *> args;
    const char * result = nullptr;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--result") == 0 && i + 1 < argc)
        {
            result = argv[++i];
            continue;
        }
        args.push_back(argv[i]);
    }

    if (result != nullptr)
    {
        std::string text = result;
        if (text == "-")
        {
            text.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
        }
        if (!chip_tool_link::SetAnswer(text.c_str()))
        {
            fprintf(stderr, "chip-tool-link: --result: not the gadget's answer (JSON)\n");
            return EXIT_FAILURE;
        }
    }
    else
    {
        // The request alone on stdout, for the agent to pass on; errors
        // still show.
        chip::Logging::SetLogFilter(chip::Logging::kLogCategory_Error);
    }

    // Error names, as chip-tool's stack start-up registers them.
    chip::RegisterCHIPLayerErrorFormatter();
    chip::app::StatusIB::RegisterErrorFormatter();

    NoCredentialIssuer credIssuerCommands;
    Commands commands;
    registerClusters(commands, &credIssuerCommands);
    return commands.Run(static_cast<int>(args.size()), args.data());
}

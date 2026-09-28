#pragma once

#include <cstdint>
#include <string>

struct TransferRequestResult
{
    bool success = false;

    long httpStatus = 0;

    std::string transferId;
    std::string status;

    std::string error;
};


TransferRequestResult requestTransferFromPeer(
    const std::string& targetIp,
    int port,
    const std::string& fileName,
    std::uint64_t fileSize,
    const std::string& senderName
);
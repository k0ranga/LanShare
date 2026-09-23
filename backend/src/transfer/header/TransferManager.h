#pragma once

#include "Transfer.h"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class TransferManager
{
public:
    static TransferManager& instance();

    Transfer createTransfer(
        const std::string& fileName,
        std::uint64_t fileSize,
        const std::string& senderName,
        const std::string& senderIp
    );

    std::optional<Transfer> getTransfer(
        const std::string& transferId
    );

    std::optional<Transfer> updateStatus(
        const std::string& transferId,
        TransferStatus expectedStatus,
        TransferStatus newStatus
    );
private:
    TransferManager() = default;

    std::string generateTransferId();

    std::unordered_map<std::string, Transfer>
        transfers_;

    std::mutex mutex_;
};
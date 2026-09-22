#pragma once

#include <cstdint>
#include <string>

enum class TransferStatus
{
    Pending,
    Accepted,
    Rejected,
    Transferring,
    Completed,
    Failed
};

inline std::string transferStatusToString(TransferStatus status)
{
    switch (status)
    {
        case TransferStatus::Pending:
            return "pending";

        case TransferStatus::Accepted:
            return "accepted";

        case TransferStatus::Rejected:
            return "rejected";

        case TransferStatus::Transferring:
            return "transferring";

        case TransferStatus::Completed:
            return "completed";

        case TransferStatus::Failed:
            return "failed";
    }

    return "unknown";
}

struct Transfer
{
    std::string id;

    std::string fileName;
    std::uint64_t fileSize = 0;
    std::uint64_t bytesTransferred = 0;

    std::string senderId;
    std::string senderName;
    std::string senderIp;

    std::string receiverId;
    std::string receiverName;
    std::string receiverIp;

    TransferStatus status =
        TransferStatus::Pending;
};
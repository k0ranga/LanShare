#include "TransferManager.h"

#include <random>

TransferManager& TransferManager::instance()
{
    static TransferManager manager;

    return manager;
}


std::string TransferManager::generateTransferId()
{
    static const char characters[] =
        "0123456789abcdef";

    std::random_device randomDevice;
    std::mt19937 generator(
        randomDevice()
    );

    std::uniform_int_distribution<int>
        distribution(0, 15);

    std::string id;

    for (int i = 0; i < 16; ++i)
    {
        id += characters[
            distribution(generator)
        ];
    }

    return id;
}


Transfer TransferManager::createTransfer(
    const std::string& fileName,
    std::uint64_t fileSize,
    const std::string& senderName,
    const std::string& senderIp
)
{
    std::lock_guard<std::mutex>
        lock(mutex_);

    std::string id;

    do
    {
        id = generateTransferId();
    }
    while (transfers_.contains(id));


    Transfer transfer;

    transfer.id = id;

    transfer.fileName = fileName;
    transfer.fileSize = fileSize;

    transfer.bytesTransferred = 0;

    transfer.senderName = senderName;
    transfer.senderIp = senderIp;

    transfer.status =
        TransferStatus::Pending;


    transfers_[id] = transfer;

    return transfer;
}


std::optional<Transfer>
TransferManager::getTransfer(
    const std::string& transferId
)
{
    std::lock_guard<std::mutex>
        lock(mutex_);

    auto iterator =
        transfers_.find(transferId);

    if (iterator == transfers_.end())
    {
        return std::nullopt;
    }

    return iterator->second;
}
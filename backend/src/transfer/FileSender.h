#pragma once

#include <filesystem>
#include <string>

struct SendResult
{
    bool success = false;

    long httpStatus = 0;

    std::string responseBody;
    std::string error;
};


SendResult sendFileToPeer(
    const std::filesystem::path& filePath,
    const std::string& targetIp,
    int port
);
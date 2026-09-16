#include "FileSender.h"

#include <curl/curl.h>

#include <fstream>
#include <iostream>


namespace fs = std::filesystem;


static size_t readFileCallback(
    char* buffer,
    size_t size,
    size_t nitems,
    void* userdata
)
{
    auto* input =
        static_cast<std::ifstream*>(userdata);

    const size_t maxBytes =
        size * nitems;

    input->read(
        buffer,
        static_cast<std::streamsize>(maxBytes)
    );

    const auto bytesRead =
        input->gcount();

    if (bytesRead > 0)
    {
        return static_cast<size_t>(
            bytesRead
        );
    }

    if (input->bad())
    {
        return CURL_READFUNC_ABORT;
    }

    return 0;
}


static size_t responseCallback(
    char* data,
    size_t size,
    size_t nitems,
    void* userdata
)
{
    const size_t total =
        size * nitems;

    auto* response =
        static_cast<std::string*>(
            userdata
        );

    response->append(
        data,
        total
    );

    return total;
}


SendResult sendFileToPeer(
    const fs::path& filePath,
    const std::string& targetIp,
    int port
)
{
    SendResult result;

    std::error_code error;

    if (!fs::exists(filePath, error) ||
        !fs::is_regular_file(filePath, error))
    {
        result.error =
            "File does not exist";

        return result;
    }


    const auto fileSize =
        fs::file_size(
            filePath,
            error
        );

    if (error)
    {
        result.error =
            "Could not determine file size";

        return result;
    }


    std::ifstream input(
        filePath,
        std::ios::binary
    );

    if (!input)
    {
        result.error =
            "Could not open file";

        return result;
    }


    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        result.error =
            "Could not initialize curl";

        return result;
    }


    const std::string fileName =
        filePath.filename().string();


    const std::string url =
        "http://" +
        targetIp +
        ":" +
        std::to_string(port) +
        "/api/files";


    struct curl_slist* headers =
        nullptr;


    const std::string fileHeader =
        "X-File-Name: " +
        fileName;


    headers =
        curl_slist_append(
            headers,
            fileHeader.c_str()
        );


    headers =
        curl_slist_append(
            headers,
            "Content-Type: application/octet-stream"
        );


    headers =
        curl_slist_append(
            headers,
            "Expect:"
        );


    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str()
    );


    curl_easy_setopt(
        curl,
        CURLOPT_POST,
        1L
    );


    curl_easy_setopt(
        curl,
        CURLOPT_READFUNCTION,
        readFileCallback
    );


    curl_easy_setopt(
        curl,
        CURLOPT_READDATA,
        &input
    );


    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDSIZE_LARGE,
        static_cast<curl_off_t>(
            fileSize
        )
    );


    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );


    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        responseCallback
    );


    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &result.responseBody
    );


    curl_easy_setopt(
        curl,
        CURLOPT_CONNECTTIMEOUT,
        5L
    );


    const CURLcode curlResult =
        curl_easy_perform(curl);


    if (curlResult != CURLE_OK)
    {
        result.error =
            curl_easy_strerror(
                curlResult
            );

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return result;
    }


    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &result.httpStatus
    );


    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);


    if (result.httpStatus >= 200 &&
        result.httpStatus < 300)
    {
        result.success = true;
    }
    else
    {
        result.error =
            "Receiver returned HTTP " +
            std::to_string(
                result.httpStatus
            );
    }


    return result;
}
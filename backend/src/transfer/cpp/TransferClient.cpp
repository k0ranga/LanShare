#include "TransferClient.h"

#include <curl/curl.h>
#include <json/json.h>

#include <string>


static size_t responseCallback(
    char* data,
    size_t size,
    size_t nmemb,
    void* userdata
)
{
    const size_t total =size * nmemb;
    auto* response = static_cast<std::string*>(userdata);
    response->append(data,total);
    return total;
}


TransferRequestResult requestTransferFromPeer(
    const std::string& targetIp,
    int port,
    const std::string& fileName,
    std::uint64_t fileSize,
    const std::string& senderName
)
{
    TransferRequestResult result;

    CURL* curl =  curl_easy_init();
    if (!curl)
    {
        result.error ="Could not initialize curl";
        return result;
    }
    const std::string url =
        "http://" +
        targetIp +
        ":" +
        std::to_string(port) +
        "/api/transfers/request";


    Json::Value json;

    json["fileName"] = fileName;

    json["fileSize"] =static_cast<Json::UInt64>(fileSize);
    json["senderName"] =senderName;
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const std::string body =
        Json::writeString(
            builder,
            json
        );
    struct curl_slist* headers =
        nullptr;
    headers =
        curl_slist_append(
            headers,
            "Content-Type: application/json"
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
        CURLOPT_POSTFIELDS,
        body.c_str()
    );
    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDSIZE,
        static_cast<long>(
            body.size()
        )
    );
    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );
    std::string responseBody;
    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        responseCallback
    );
    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &responseBody
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
    Json::CharReaderBuilder readerBuilder;
    Json::Value responseJson;
    std::string errors;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    const bool parsed =
        reader->parse(
            responseBody.data(),
            responseBody.data() +
                responseBody.size(),
            &responseJson,
            &errors
        );
    if (!parsed)
    {
        result.error = "Invalid JSON returned by receiver";
        return result;
    }
    if (result.httpStatus < 200 ||
        result.httpStatus >= 300)
    {
        result.error =
            responseJson.get(
                "error",
                "Receiver rejected request"
            ).asString();

        return result;
    }
    result.transferId =
        responseJson["transferId"]
            .asString();
    result.status =
        responseJson["status"]
            .asString();
    result.success = true;
    return result;
}
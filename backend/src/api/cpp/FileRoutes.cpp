#include "FileRoutes.h"

#include "../../transfer/header/FileSender.h"

#include <drogon/drogon.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <cstdint>

namespace fs = std::filesystem;

void registerFileRoutes()
{
    drogon::app().registerHandler(
        "/api/files",

        [](const drogon::HttpRequestPtr& req,
           drogon::RequestStreamPtr&& stream,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        {
            std::string fileName =
                req->getHeader("X-File-Name");

            if (fileName.empty())
            {
                stream->setStreamReader(
                    drogon::RequestStreamReader::newNullReader()
                );

                Json::Value json;
                json["error"] =
                    "X-File-Name header is required";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(json);

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                callback(response);
                return;
            }

            std::string safeFileName =
                fs::path(fileName)
                    .filename()
                    .string();

            if (safeFileName.empty())
            {
                stream->setStreamReader(
                    drogon::RequestStreamReader::newNullReader()
                );

                Json::Value json;
                json["error"] = "Invalid filename";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(json);

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                callback(response);
                return;
            }

            fs::path receiveDirectory =
                fs::current_path() / "received";

            std::error_code directoryError;

            fs::create_directories(
                receiveDirectory,
                directoryError
            );

            if (directoryError)
            {
                stream->setStreamReader(
                    drogon::RequestStreamReader::newNullReader()
                );

                Json::Value json;
                json["error"] =
                    "Could not create receive directory";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(json);

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                callback(response);
                return;
            }


            fs::path destination =
                receiveDirectory / safeFileName;


            auto output =
                std::make_shared<std::ofstream>(
                    destination,
                    std::ios::binary |
                    std::ios::trunc
                );

            if (!output->is_open())
            {
                stream->setStreamReader(
                    drogon::RequestStreamReader::newNullReader()
                );

                Json::Value json;
                json["error"] =
                    "Could not create destination file";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(json);

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                callback(response);
                return;
            }


            auto bytesReceived =
                std::make_shared<std::uint64_t>(0);

            auto writeFailed =
                std::make_shared<bool>(false);


            auto responseCallback =
                std::make_shared<
                    std::function<
                        void(
                            const drogon::HttpResponsePtr&
                        )
                    >
                >(std::move(callback));


            std::cout
                << "Starting upload: "
                << safeFileName
                << std::endl;


            auto reader =
                drogon::RequestStreamReader::newReader(

                    [
                        output,
                        bytesReceived,
                        writeFailed
                    ]
                    (
                        const char* data,
                        size_t size
                    )
                    {
                        if (*writeFailed)
                        {
                            return;
                        }

                        output->write(
                            data,
                            static_cast<std::streamsize>(size)
                        );

                        if (!(*output))
                        {
                            *writeFailed = true;
                            return;
                        }

                        *bytesReceived +=
                            static_cast<std::uint64_t>(size);
                    },


                    [
                        output,
                        bytesReceived,
                        writeFailed,
                        destination,
                        safeFileName,
                        responseCallback
                    ]
                    (
                        std::exception_ptr exception
                    )
                    {
                        output->close();


                        if (exception)
                        {
                            std::error_code ec;
                            fs::remove(destination, ec);
                            Json::Value json;
                            json["error"] =
                                "Upload interrupted";
                            auto response =
                                drogon::HttpResponse::
                                    newHttpJsonResponse(json);

                            response->setStatusCode(
                                drogon::k400BadRequest
                            );

                            (*responseCallback)(response);

                            return;
                        }

                        if (*writeFailed || !(*output))
                        {
                            std::error_code ec;
                            fs::remove(destination, ec);

                            Json::Value json;
                            json["error"] =
                                "Failed while writing file";

                            auto response =
                                drogon::HttpResponse::
                                    newHttpJsonResponse(json);

                            response->setStatusCode(
                                drogon::
                                    k500InternalServerError
                            );

                            (*responseCallback)(response);

                            return;
                        }


                        std::cout
                            << "Upload complete: "
                            << safeFileName
                            << " ("
                            << *bytesReceived
                            << " bytes)"
                            << std::endl;

                        Json::Value json;

                        json["status"] = "received";
                        json["fileName"] =
                            safeFileName;

                        json["size"] =
                            static_cast<Json::UInt64>(
                                *bytesReceived
                            );


                        auto response =
                            drogon::HttpResponse::
                                newHttpJsonResponse(json);

                        response->setStatusCode(
                            drogon::k201Created
                        );

                        (*responseCallback)(response);
                    }
                );

            stream->setStreamReader(reader);
        },

        {drogon::Post}
    );

drogon::app().registerHandler(
    "/api/send",

    [](const drogon::HttpRequestPtr& req,
       std::function<void(
           const drogon::HttpResponsePtr&
       )>&& callback)
    {
        auto json =
            req->getJsonObject();

        if (!json)
        {
            Json::Value responseJson;

            responseJson["error"] =
                "Invalid JSON body";

            auto response =
                drogon::HttpResponse::
                    newHttpJsonResponse(
                        responseJson
                    );

            response->setStatusCode(
                drogon::k400BadRequest
            );

            callback(response);

            return;
        }


        if (!json->isMember("targetIp") ||
            !json->isMember("filePath"))
        {
            Json::Value responseJson;

            responseJson["error"] =
                "targetIp and filePath are required";

            auto response =
                drogon::HttpResponse::
                    newHttpJsonResponse(
                        responseJson
                    );

            response->setStatusCode(
                drogon::k400BadRequest
            );

            callback(response);

            return;
        }


        const std::string targetIp =
            (*json)["targetIp"].asString();

        const std::string filePath =
            (*json)["filePath"].asString();


        const int port =
            json->get("port", 8080).asInt();


        if (targetIp.empty() ||
            filePath.empty())
        {
            Json::Value responseJson;

            responseJson["error"] =
                "targetIp and filePath cannot be empty";

            auto response =
                drogon::HttpResponse::
                    newHttpJsonResponse(
                        responseJson
                    );

            response->setStatusCode(
                drogon::k400BadRequest
            );

            callback(response);

            return;
        }


        if (port <= 0 || port > 65535)
        {
            Json::Value responseJson;

            responseJson["error"] =
                "Invalid port";

            auto response =
                drogon::HttpResponse::
                    newHttpJsonResponse(
                        responseJson
                    );

            response->setStatusCode(
                drogon::k400BadRequest
            );

            callback(response);

            return;
        }
        SendResult sendResult =
            sendFileToPeer(
                filePath,
                targetIp,
                port
            );


        Json::Value responseJson;


        if (!sendResult.success)
        {
            responseJson["status"] = "failed";
            responseJson["error"] =
                sendResult.error;

            responseJson["receiverStatus"] =
                static_cast<int>(
                    sendResult.httpStatus
                );

            auto response =
                drogon::HttpResponse::
                    newHttpJsonResponse(
                        responseJson
                    );

            response->setStatusCode(
                drogon::
                    k500InternalServerError
            );

            callback(response);

            return;
        }


        responseJson["status"] = "sent";

        responseJson["targetIp"] =
            targetIp;

        responseJson["port"] =
            port;

        responseJson["filePath"] =
            filePath;

        responseJson["receiverStatus"] =
            static_cast<int>(
                sendResult.httpStatus
            );

        responseJson["receiverResponse"] =
            sendResult.responseBody;


        auto response =
            drogon::HttpResponse::
                newHttpJsonResponse(
                    responseJson
                );

        callback(response);
    },

    {drogon::Post}
);
}
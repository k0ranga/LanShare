#include <drogon/drogon.h>
#include <curl/curl.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

int main(){

    Json::Value config;
    config["app"]["enable_request_stream"] = true;

    drogon::app().loadConfigJson(config);

    drogon::app().setClientMaxBodySize(
        100ULL * 1024ULL * 1024ULL * 1024ULL
    );


    drogon::app().registerHandler(
        "/api/health",
        [](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback){
            Json::Value response;

            response["status"] = "ok";
            response["service"] = "LanShare";

            auto httpResponse = drogon::HttpResponse::newHttpJsonResponse(response);


            callback(httpResponse);
        },
        {drogon::Get}
    );


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
    std::cout << "LanShare running on port 8080\n";
    drogon::app().addListener("0.0.0.0", 8080).run();

    return 0;
}
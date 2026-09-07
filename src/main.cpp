#include <drogon/drogon.h>


#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

int main(){
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
           std::function<void(const drogon::HttpResponsePtr&)>&& callback){
                const std::string fileNameHeader = req->getHeader("X-File-Name");
                 Json::Value json;
                if(fileNameHeader.empty()){
                    json["error"] = "X-File-Name header is required";

                    auto response =
                    drogon::HttpResponse::newHttpJsonResponse(json);

                    response->setStatusCode(drogon::k400BadRequest);

                    callback(response);
                    return;
                    
                }

                const std::string safeFileName = fs::path(fileNameHeader).filename().string();

                if(safeFileName.empty()){
                   
                    json["error"] = "Invalid file name";

                    auto response =
                        drogon::HttpResponse::newHttpJsonResponse(json);

                    response->setStatusCode(drogon::k400BadRequest);

                    callback(response);
                    return;
                }

                fs::path receiveDirectory = fs::current_path() / "received";

                std::error_code error;

                fs::create_directories(receiveDirectory, error);

                if(error){
                     json["error"] = "Could not create receive directory";

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

                std::string_view body = req->body();
                
                std::ofstream output(
                    destination,
                    std::ios::binary
                );

                 if (!output)
                {
                    json["error"] = "Could not create destination file";

                    auto response =
                        drogon::HttpResponse::newHttpJsonResponse(json);

                    response->setStatusCode(
                        drogon::k500InternalServerError
                    );

                    callback(response);
                    return;
                }

                output.write(
                    body.data(),
                    static_cast<std::streamsize>(body.size())
                );

                output.close();

                if (!output)
                {
                    Json::Value json;
                    json["error"] = "Failed while writing file";

                    auto response =
                        drogon::HttpResponse::newHttpJsonResponse(json);

                    response->setStatusCode(
                        drogon::k500InternalServerError
                    );

                    callback(response);
                    return;
                }
                std::cout
                    << "Received file: "
                    << destination
                    << " ("
                    << body.size()
                    << " bytes)"
                    << std::endl;


              

                json["status"] = "received";
                json["fileName"] = safeFileName;
                json["size"] =
                    static_cast<Json::UInt64>(body.size());

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(json);

                response->setStatusCode(drogon::k201Created);

                callback(response);
            },
            {drogon::Post}
           
    );
    std::cout << "LanShare running on port 8080\n";
    drogon::app().addListener("0.0.0.0", 8080).run();

    return 0;
}
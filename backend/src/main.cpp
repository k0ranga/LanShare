#include <drogon/drogon.h>
#include <curl/curl.h>

#include <filesystem>
#include <fstream>
#include <iostream>

#include "api/header/HealthRoutes.h"
#include "api/header/FileRoutes.h"
#include "TransferRoutes.h"

namespace fs = std::filesystem;

int main(){

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    {
        std::cerr << "Failed to initialize libcurl\n";
        return 1;
    }

    registerHealthRoutes();
    registerFileRoutes();
    registerTransferRoutes();


    Json::Value config;
    config["app"]["enable_request_stream"] = true;

    drogon::app().loadConfigJson(config);

    drogon::app().setClientMaxBodySize(
        100ULL * 1024ULL * 1024ULL * 1024ULL
    );


    std::cout << "LanShare running on port 8080\n";
    drogon::app().addListener("0.0.0.0", 8080).run();

    return 0;
}
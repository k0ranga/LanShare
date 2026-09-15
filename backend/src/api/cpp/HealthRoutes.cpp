
#include "HealthRoutes.h"

#include <drogon/drogon.h>

void registerHealthRoutes()
{
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
}
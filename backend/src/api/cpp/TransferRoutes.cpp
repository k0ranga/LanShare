#include "TransferRoutes.h"

#include "TransferManager.h"

#include <drogon/drogon.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;


void registerTransferRoutes()
{
    drogon::app().registerHandler(
        "/api/transfers/request",

        [](
            const drogon::HttpRequestPtr& req,
            std::function<void(
                const drogon::HttpResponsePtr&
            )>&& callback
        )
        {
            auto json =
                req->getJsonObject();


        //    std::cout << json << std::endl;


            if (!json)
            {
                Json::Value result;

                result["error"] =
                    "Invalid JSON bodyyyy";

                auto response =
                    drogon::HttpResponse::
                        newHttpJsonResponse(result);

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                callback(response);

                return;
            }


            if (!json->isMember("fileName") ||
                !json->isMember("fileSize"))
            {
                Json::Value result;

                result["error"] =
                    "fileName and fileSize are required";

                auto response =
                    drogon::HttpResponse::
                        newHttpJsonResponse(result);

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                callback(response);

                return;
            }


            const std::string requestedFileName =
                (*json)["fileName"].asString();


            if (requestedFileName.empty())
            {
                Json::Value result;

                result["error"] =
                    "fileName cannot be empty";

                auto response =
                    drogon::HttpResponse::
                        newHttpJsonResponse(result);

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                callback(response);

                return;
            }


            const std::string fileName =
                fs::path(requestedFileName)
                    .filename()
                    .string();


            if (fileName.empty())
            {
                Json::Value result;

                result["error"] =
                    "Invalid filename";

                auto response =
                    drogon::HttpResponse::
                        newHttpJsonResponse(result);

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                callback(response);

                return;
            }


            const std::uint64_t fileSize =
                (*json)["fileSize"].asUInt64();


            const std::string senderName =
                json->get(
                    "senderName",
                    "Unknown device"
                ).asString();


            const std::string senderIp =
                req->peerAddr().toIp();


            Transfer transfer =
                TransferManager::instance()
                    .createTransfer(
                        fileName,
                        fileSize,
                        senderName,
                        senderIp
                    );


            std::cout
                << "Incoming transfer request\n"
                << "ID: "
                << transfer.id
                << "\nFile: "
                << transfer.fileName
                << "\nSize: "
                << transfer.fileSize
                << "\nSender: "
                << transfer.senderName
                << " ("
                << transfer.senderIp
                << ")\n";


            Json::Value result;

            result["transferId"] =
                transfer.id;

            result["status"] =
                transferStatusToString(
                    transfer.status
                );

            result["fileName"] =
                transfer.fileName;

            result["fileSize"] =
                static_cast<Json::UInt64>(
                    transfer.fileSize
                );

            result["senderName"] =
                transfer.senderName;

            result["senderIp"] =
                transfer.senderIp;


            auto response =
                drogon::HttpResponse::
                    newHttpJsonResponse(result);

            response->setStatusCode(
                drogon::k201Created
            );

            callback(response);
        },

        {drogon::Post}
    );


    drogon::app().registerHandler(
    "/api/transfers/respond",

    [](
        const drogon::HttpRequestPtr& req,
        std::function<void(
            const drogon::HttpResponsePtr&
        )>&& callback
    )
    {
        auto json = req->getJsonObject();

        if (!json ||
            !json->isMember("transferId") ||
            !json->isMember("decision"))
        {
            Json::Value result;
            result["error"] =
                "transferId and decision are required";

            auto response =
                drogon::HttpResponse::
                    newHttpJsonResponse(result);

            response->setStatusCode(
                drogon::k400BadRequest
            );

            callback(response);
            return;
        }


        const std::string transferId =
            (*json)["transferId"].asString();

        const std::string decision =
            (*json)["decision"].asString();


        TransferStatus newStatus;

        if (decision == "accept")
        {
            newStatus =
                TransferStatus::Accepted;
        }
        else if (decision == "reject")
        {
            newStatus =
                TransferStatus::Rejected;
        }
        else
        {
            Json::Value result;

            result["error"] =
                "decision must be accept or reject";

            auto response =
                drogon::HttpResponse::
                    newHttpJsonResponse(result);

            response->setStatusCode(
                drogon::k400BadRequest
            );

            callback(response);
            return;
        }


        auto transfer =
            TransferManager::instance()
                .updateStatus(
                    transferId,
                    TransferStatus::Pending,
                    newStatus
                );


        if (!transfer)
        {
            Json::Value result;

            result["error"] =
                "Transfer not found or not pending";

            auto response =
                drogon::HttpResponse::
                    newHttpJsonResponse(result);

            response->setStatusCode(
                drogon::k409Conflict
            );

            callback(response);
            return;
        }


        Json::Value result;

        result["transferId"] =
            transfer->id;

        result["fileName"] =
            transfer->fileName;

        result["status"] =
            transferStatusToString(
                transfer->status
            );


        auto response =
            drogon::HttpResponse::
                newHttpJsonResponse(result);

        callback(response);
    },

    {drogon::Post}
);
}
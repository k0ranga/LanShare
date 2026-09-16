cmake_minimum_required(VERSION 3.20)

project(LanShare)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)


add_executable(
    LanShare

    src/main.cpp

    src/api/cpp/HealthRoutes.cpp
    src/api/cpp/FileRoutes.cpp

    src/transfer/cpp/FileSender.cpp
    src/api/cpp/TransferRoutes.cpp

    src/transfer/cpp/TransferManager.cpp
)


target_include_directories(
    LanShare
    PRIVATE

        ${CMAKE_CURRENT_SOURCE_DIR}/src/api/header
        ${CMAKE_CURRENT_SOURCE_DIR}/src/transfer/header
)


target_link_libraries(
    LanShare
    PRIVATE

        Drogon::Drogon
        CURL::libcurl
)
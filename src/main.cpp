#include <cstdlib>
#if defined(USE_EXPERIMENTAL_FS)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#if defined(__APPLE__)
#include <unistd.h>
#endif
#endif

#include <string>
#include <cstdint>
#include <iomanip>
#include "SonyCamera.h"

#include <iostream>
#include <vector>
#include <thread>
#include <codecvt>
#include <algorithm>
#include <cstring>
#include <dev/devs.hpp>
#include <drogon/drogon.h>

using namespace std;

SonyCamera camera;

int t_main() {
    // Change global locale to native locale
    std::locale::global(std::locale(""));

    // Make the stream's locale the same as the current global locale
    cli::tin.imbue(std::locale());
    cli::tout.imbue(std::locale());

     drogon::app()
        .registerHandler("/", [](const drogon::HttpRequestPtr&,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setBody("Hello, Drogon");
            cb(resp);
        })
        .registerHandler("/version", [](const drogon::HttpRequestPtr&,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            std::string version = camera.version();
            resp->setBody(version.c_str());
            cb(resp);
        })
        .registerHandler("/device/scan", [](const drogon::HttpRequestPtr&,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            std::string version = camera.version();
            resp->setBody(version.c_str());
            cb(resp);
        })
        .addListener("0.0.0.0", 8080)
        .run();
}

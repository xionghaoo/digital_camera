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
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

SonyCamera camera;

int main() {
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
            std::string devList = camera.scan();
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeString("application/json; charset=utf-8");
            resp->setBody(devList);
            cb(resp);
        })
        .registerHandler("/device/connect", [](const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            int index = -1;
            if (req->getContentType() == drogon::CT_APPLICATION_JSON) {
                auto jsonPtr = req->getJsonObject();
                index = jsonPtr->get("index", 0).asInt();
            }
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeString("application/json; charset=utf-8");
            json data;
            if (index <= 0) {
                data["code"] = -1;
                data["data"] = false;
            } else {
                bool success = camera.connect(index);
                data["code"] = 0;
                data["data"] = success;
            }
            resp->setBody(data.dump());
            cb(resp);
        })
        .registerHandler("/af-shutter", [](const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeString("application/json; charset=utf-8");
            json data;
            bool success = camera.af_shutter();
            data["code"] = 0;
            data["data"] = success;
            resp->setBody(data.dump());
            cb(resp);
        })
        .addListener("0.0.0.0", 8080)
        .run();
}

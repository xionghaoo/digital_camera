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
#include "DocController.h"
#include "UserController.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

SonyCamera camera;

drogon::HttpResponsePtr response_json(int code, json data, std::string& msg) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeString("application/json; charset=utf-8");
    json respData;
    // int code = -1;
    // std::string msg;
    // if (rtmpUrl.size() == 0) {
    //     msg = "rtmp url not set";
    // } else {
    //     camera.enable_live_view(isEnable, isLocal, rtmpUrl);
    //     code = 0;
    //     msg = "success";
    // }
    respData["code"] = 0;
    respData["data"] = data;
    respData["message"] = msg;
    resp->setBody(respData.dump());
    return resp;
}

int main() {
    // Change global locale to native locale
    std::locale::global(std::locale(""));

    // Make the stream's locale the same as the current global locale
    cli::tin.imbue(std::locale());
    cli::tout.imbue(std::locale());
    
    drogon::app().registerController(std::make_shared<UserController>());
    // 注册文档控制器
    drogon::app().registerController(std::make_shared<DocController>());
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
            std::function<void (std::string)> callback = [&](std::string path) {
                auto resp = drogon::HttpResponse::newFileResponse(path.c_str());
                cb(resp);
            };
            bool success = camera.af_shutter(&callback);
        })
        .registerHandler("/capture", [](const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeString("application/json; charset=utf-8");
            json data;
            bool success = camera.capture();
            std::string path = camera.get_save_path();
            data["code"] = 0;
            data["data"] = path;
            resp->setBody(data.dump());
            cb(resp);
        })
        .registerHandler("/live/enable", [](const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            bool isEnable = false;
            bool isLocal = false;
            std::string rtmpUrl("");
            if (req->getContentType() == drogon::CT_APPLICATION_JSON) {
                auto jsonPtr = req->getJsonObject();
                isEnable = jsonPtr->get("is_enable", false).asBool();
                isLocal = jsonPtr->get("is_local", false).asBool();
                rtmpUrl = jsonPtr->get("rtmp_url", "").asString();
            }
            drogon::HttpResponsePtr respPtr;
            if (!isLocal) {
                // 推流
                if (rtmpUrl.size() == 0) {
                    std::string msg("rtmp url not set");
                    respPtr = response_json(-1, nullptr, msg);
                    cb(respPtr);
                    return;
                }
            }
            // 局域网
            bool success = camera.enable_live_view(isEnable, isLocal, rtmpUrl);    
            std::string msg(success ? "success" : "failure");
            int code = success ? 0 : -1;
            std::string url = isLocal ? ":8081" : rtmpUrl;
            json data = success ? url : nullptr;
            respPtr = response_json(code, data, msg);
            cb(respPtr);
        })
        .registerHandler("/live/start", [](const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeString("application/json; charset=utf-8");
            json data;
            camera.live_view();
            data["code"] = 0;
            data["data"] = nullptr;
            resp->setBody(data.dump());
            cb(resp);
        })
        .registerHandler("/power", [](const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            bool powerOn = false;
            if (req->getContentType() == drogon::CT_APPLICATION_JSON) {
                auto jsonPtr = req->getJsonObject();
                powerOn = jsonPtr->get("is_open", false).asBool();
            }
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeString("application/json; charset=utf-8");
            json data;
            if (powerOn) {
                camera.power_on();
            } else {
                camera.power_off();
            }
            data["code"] = 0;
            data["data"] = nullptr;
            resp->setBody(data.dump());
            cb(resp);
        })
        .registerHandler("/usb-connect", [](const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeString("application/json; charset=utf-8");
            bool success = camera.connect_with_usb();
            json data;
            data["code"] = 0;
            data["data"] = success;
            resp->setBody(data.dump());
            cb(resp);
        })
        .addListener("0.0.0.0", 9090)
        .run();
}

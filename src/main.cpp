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
#include "CameraController.h"
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
    
    // drogon::app().registerController(std::make_shared<UserController>());
    drogon::app().registerController(std::make_shared<CameraController>());
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
        .addListener("0.0.0.0", 9090)
        .run();
}

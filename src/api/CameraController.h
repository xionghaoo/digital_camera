#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpRequest.h>
#include <json/json.h>
#include "BaseController.h"
#include "DocController.h"
#include "AutoDocMacro.h"
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include "SonyCamera.h"
#include "NetworkUtils.h"

using namespace drogon;

class CameraController : public HttpController<CameraController>, public BaseController 
{
public:
    // 禁用自动创建，允许手动注册
    static constexpr bool isAutoCreation = false;
    METHOD_LIST_BEGIN
        // 使用宏自动注册到文档
        ADD_METHOD_WITH_BODY_PARAMS(CameraController, scan, "/api/camera/scan", Post,
                           "相机设备扫描", "扫描wifi/usb相机设备",
                           "device_type:string:相机类型：sony|obsbot");
        ADD_METHOD_WITH_AUTO_DOC(CameraController, connect, "/api/camera/connect/{index}", Post,
                           "相机连接", "通过序号连接相机设备，需要先扫描相机");
        ADD_METHOD_WITH_AUTO_DOC(CameraController, usbConnect, "/api/camera/connect/usb", Post,
                           "相机USB连接", "通过USB接口连接相机");
        ADD_METHOD_WITH_BODY_PARAMS(CameraController, power, "/api/camera/power", Post,
                           "相机电源控制", "控制相机开关机，目前只有关机可用",
                           "power_on:bool:是否开机：true|false");
        ADD_METHOD_WITH_FILE_RESPONSE(CameraController, afShutter, "/api/camera/shutter/af", Post,
                           "af拍摄", "af拍照，直接返回图片",
                           "image/jpeg");
        ADD_METHOD_WITH_BODY_PARAMS(CameraController, liveEnable, "/api/camera/live/enable", Post,
                           "预览开关", "是否开启预览",
                           "is_enable:bool:是否开启：true|false,is_local:bool:本地预览还是远程预览：true本地|false远程,rtmp_url:string:推流地址");    
        ADD_METHOD_WITH_AUTO_DOC(CameraController, liveStart, "/api/camera/live/start", Post,
                           "开启相机预览", "开启相机预览，需要先打开预览开关");
        ADD_METHOD_WITH_BODY_PARAMS(CameraController, zoom, "/api/camera/zoom", Post,
                           "焦距控制", "调整相机焦距",
                           "zoom_speed:int:缩放值：-8~8");
    METHOD_LIST_END

    CameraController() {};

    void scan(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback)
    {
        // 使用基类的验证方法
        const Json::Value* json = validateJsonRequest(req);
        if (!json) {
            sendErrorResponse(std::move(callback), 400, "请求体格式错误，需要JSON格式", k400BadRequest);
            return;
        }
        
        // 验证必填字段
        std::vector<std::string> missingFields = validateRequiredFields(json, {"device_type"});
        if (!missingFields.empty()) {
            std::string message = "缺少必填字段: " + missingFields[0];
            for (size_t i = 1; i < missingFields.size(); ++i) {
                message += ", " + missingFields[i];
            }
            sendErrorResponse(std::move(callback), 400, message, k400BadRequest);
            return;
        }

        std::lock_guard<std::mutex> lock(cameraMutex_);

        std::string deviceType = (*json)["device_type"].asString();
        if (deviceType == "sony") {
            Json::Value data = camera.scan();
            sendSuccessResponse(std::move(callback), "success", data, k200OK);
        } else if (deviceType == "obsbot") {
            sendErrorResponse(std::move(callback), 406, "暂不支持Obsbot相机", k406NotAcceptable);
        } else {
            sendErrorResponse(std::move(callback), 501, "未知相机类型", k501NotImplemented);
        }
    }

    void connect(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback,
                    const std::string& indexStr) 
    {
        int index = -1;
        try {
            index = std::stoi(indexStr);
        } catch (const std::exception& e) {
            sendErrorResponse(std::move(callback), 400, "相机序号格式错误", k400BadRequest);
            return;
        }
        
        std::lock_guard<std::mutex> lock(cameraMutex_);
        bool success = camera.connect(index);
        Json::Value data = success;
        sendSuccessResponse(std::move(callback), "success", data, k200OK);
    }

    void usbConnect(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback) 
    {
        std::lock_guard<std::mutex> lock(cameraMutex_);       
        bool success = camera.connect_with_usb();
        Json::Value data = success;
        if (success) {
            sendSuccessResponse(std::move(callback), "success", data, k200OK);
        } else {
            sendErrorResponse(std::move(callback), -2, "相机连接失败", k200OK);
        }
    }

    void power(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback) 
    {
        // 使用基类的验证方法
        const Json::Value* json = validateJsonRequest(req);
        if (!json) {
            sendErrorResponse(std::move(callback), 400, "请求体格式错误，需要JSON格式", k400BadRequest);
            return;
        }
        
        // 验证必填字段
        std::vector<std::string> missingFields = validateRequiredFields(json, {"power_on"});
        if (!missingFields.empty()) {
            std::string message = "缺少必填字段: " + missingFields[0];
            for (size_t i = 1; i < missingFields.size(); ++i) {
                message += ", " + missingFields[i];
            }
            sendErrorResponse(std::move(callback), 400, message, k400BadRequest);
            return;
        }
        
        std::lock_guard<std::mutex> lock(cameraMutex_);       
        bool powerOn = (*json)["power_on"].asBool();
        if (powerOn) {
            camera.power_on();
        } else {
            camera.power_off();
        }
        Json::Value data = true;
        sendSuccessResponse(std::move(callback), "success", data, k200OK);
    }

    void afShutter(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback) 
    {
        std::lock_guard<std::mutex> lock(cameraMutex_);       
        std::function<void (std::string)> cb = [&](std::string path) {
            auto resp = drogon::HttpResponse::newFileResponse(path.c_str());
            callback(resp);
        };
        bool success = camera.af_shutter(&cb);
    }

    void liveEnable(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback) 
    {
        // 使用基类的验证方法
        const Json::Value* json = validateJsonRequest(req);
        if (!json) {
            sendErrorResponse(std::move(callback), 400, "请求体格式错误，需要JSON格式", k400BadRequest);
            return;
        }
        
        // 验证必填字段
        std::vector<std::string> missingFields = validateRequiredFields(json, {"is_enable", "is_local", "rtmp_url"});
        if (!missingFields.empty()) {
            std::string message = "缺少必填字段: " + missingFields[0];
            for (size_t i = 1; i < missingFields.size(); ++i) {
                message += ", " + missingFields[i];
            }
            sendErrorResponse(std::move(callback), 400, message, k400BadRequest);
            return;
        }
        
        std::lock_guard<std::mutex> lock(cameraMutex_);  
        bool isEnable = (*json)["is_enable"].asBool();
        bool isLocal = (*json)["is_local"].asBool();
        std::string rtmpUrl = (*json)["rtmp_url"].asString();
        std::string localIP;
        if (!isLocal) {
            // 推流
            if (rtmpUrl.size() == 0) {
                sendErrorResponse(std::move(callback), -1, "rtmp url未设置", k200OK);
                return;
            }
        } else {
            localIP = getEth0IP();
            if (localIP.size() == 0) {
                sendErrorResponse(std::move(callback), -1, "wlan网口未连接", k200OK);
                return;
            }
        }
        // 局域网
        bool success = camera.enable_live_view(isEnable, isLocal, rtmpUrl);    
        std::string msg(success ? "success" : "failure");
        int code = success ? 0 : -1;
        std::string url = isLocal ? "http://"+ localIP + ":9091" : rtmpUrl;
        sendSuccessResponse(std::move(callback), "success", url, k200OK);
    }

    void liveStart(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback) 
    {
        std::lock_guard<std::mutex> lock(cameraMutex_);  
        bool ret = camera.live_view();  
        if (ret) {
            sendSuccessResponse(std::move(callback), "success", Json::nullValue, k200OK);
        } else {
            sendErrorResponse(std::move(callback), -1, "推流开启失败", k200OK);
        }
    }

    void zoom(const HttpRequestPtr& req,
                std::function<void(const HttpResponsePtr&)>&& callback) 
    {
        // 使用基类的验证方法
        const Json::Value* json = validateJsonRequest(req);
        if (!json) {
            sendErrorResponse(std::move(callback), 400, "请求体格式错误，需要JSON格式", k400BadRequest);
            return;
        }
        
        // 验证必填字段
        std::vector<std::string> missingFields = validateRequiredFields(json, {"zoom_speed"});
        if (!missingFields.empty()) {
            std::string message = "缺少必填字段: " + missingFields[0];
            for (size_t i = 1; i < missingFields.size(); ++i) {
                message += ", " + missingFields[i];
            }
            sendErrorResponse(std::move(callback), 400, message, k400BadRequest);
            return;
        }
        
        std::lock_guard<std::mutex> lock(cameraMutex_);
        int speed = (*json)["zoom_speed"].asInt();  
        bool ret = camera.zoom(speed);
        if (ret) {
            sendSuccessResponse(std::move(callback), "success", Json::nullValue, k200OK);
        } else {
            sendErrorResponse(std::move(callback), -1, "调焦失败", k200OK);
        }
    }

private:
    std::mutex cameraMutex_;
    SonyCamera camera;
};
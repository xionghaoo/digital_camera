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

using namespace drogon;

class CameraController : public HttpController<CameraController>, public BaseController 
{
public:
    // 禁用自动创建，允许手动注册
    static constexpr bool isAutoCreation = false;
    METHOD_LIST_BEGIN
        // 使用宏自动注册到文档
        ADD_METHOD_WITH_DOC(CameraController, scan, "/api/camera/scan", Post,
                           "相机设备扫描", "扫描wifi/usb相机设备");
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

private:
    std::mutex usersMutex_;
    SonyCamera camera;
};
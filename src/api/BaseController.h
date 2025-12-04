#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpRequest.h>
#include <json/json.h>
#include <string>
#include <functional>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

using namespace drogon;

/**
 * 基础控制器类
 * 提供所有子控制器共用的便利响应方法
 * 
 * 使用方式：
 * class YourController : public HttpController<YourController>, public BaseController
 * {
 *     // 在方法中可以直接使用 sendSuccessResponse, sendErrorResponse 等
 * };
 */
class BaseController
{
public:
    /**
     * 创建成功响应
     * @param callback 响应回调函数
     * @param message 响应消息（默认"成功"）
     * @param data 响应数据（可选）
     * @param statusCode HTTP状态码（默认200）
     */
    void sendSuccessResponse(std::function<void(const HttpResponsePtr&)>&& callback,
                             const std::string& message = "成功",
                             const Json::Value& data = Json::nullValue,
                             HttpStatusCode statusCode = k200OK) const
    {
        Json::Value ret;
        ret["code"] = 0;
        ret["message"] = message;
        if (!data.isNull()) {
            ret["data"] = data;
        }
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(statusCode);
        callback(resp);
    }
    
    /**
     * 创建错误响应
     * @param callback 响应回调函数
     * @param code 错误码
     * @param message 错误消息
     * @param statusCode HTTP状态码（默认400）
     */
    void sendErrorResponse(std::function<void(const HttpResponsePtr&)>&& callback,
                           int code,
                           const std::string& message,
                           HttpStatusCode statusCode = k400BadRequest) const
    {
        Json::Value ret;
        ret["code"] = code;
        ret["message"] = message;
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(statusCode);
        callback(resp);
    }
    
    /**
     * 创建分页响应
     * @param callback 响应回调函数
     * @param items 数据项数组
     * @param total 总记录数
     * @param page 当前页码
     * @param pageSize 每页大小
     * @param message 响应消息（默认"成功"）
     * @param itemsKey 数据项字段名（默认"items"）
     */
    void sendPaginatedResponse(std::function<void(const HttpResponsePtr&)>&& callback,
                               const Json::Value& items,
                               int total,
                               int page,
                               int pageSize,
                               const std::string& message = "成功",
                               const std::string& itemsKey = "items") const
    {
        Json::Value data;
        data[itemsKey] = items;
        data["total"] = total;
        data["page"] = page;
        data["pageSize"] = pageSize;
        data["totalPages"] = (total + pageSize - 1) / pageSize;
        
        sendSuccessResponse(std::move(callback), message, data);
    }
    
    /**
     * 获取查询参数，带默认值
     * @param req HTTP请求对象
     * @param key 参数名
     * @param defaultValue 默认值（如果参数不存在）
     * @return 参数值或默认值
     */
    std::string getQueryParam(const HttpRequestPtr& req, 
                              const std::string& key, 
                              const std::string& defaultValue = "") const
    {
        try {
            const auto& params = req->getParameters();
            auto it = params.find(key);
            if (it != params.end()) {
                return it->second;
            }
        } catch (...) {
            // 使用默认值
        }
        return defaultValue;
    }
    
    /**
     * 获取查询参数并转换为整数
     * @param req HTTP请求对象
     * @param key 参数名
     * @param defaultValue 默认值（如果参数不存在或转换失败）
     * @return 参数值或默认值
     */
    int getQueryParamAsInt(const HttpRequestPtr& req,
                            const std::string& key,
                            int defaultValue = 0) const
    {
        std::string value = getQueryParam(req, key, "");
        if (value.empty()) {
            return defaultValue;
        }
        try {
            return std::stoi(value);
        } catch (...) {
            return defaultValue;
        }
    }
    
    /**
     * 获取查询参数并转换为布尔值
     * @param req HTTP请求对象
     * @param key 参数名
     * @param defaultValue 默认值（如果参数不存在）
     * @return 参数值或默认值
     */
    bool getQueryParamAsBool(const HttpRequestPtr& req,
                             const std::string& key,
                             bool defaultValue = false) const
    {
        std::string value = getQueryParam(req, key, "");
        if (value.empty()) {
            return defaultValue;
        }
        // 支持 true/false, 1/0, yes/no
        std::string lowerValue = value;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
        return lowerValue == "true" || lowerValue == "1" || lowerValue == "yes";
    }
    
    /**
     * 验证JSON请求体是否存在
     * @param req HTTP请求对象
     * @return JSON对象指针，如果验证失败返回nullptr（需要调用者自行处理错误响应）
     */
    const Json::Value* validateJsonRequest(const HttpRequestPtr& req) const
    {
        return req->getJsonObject().get();
    }
    
    /**
     * 验证必填字段
     * @param json JSON对象
     * @param requiredFields 必填字段列表
     * @return 缺失的字段列表，如果为空则表示验证通过
     */
    std::vector<std::string> validateRequiredFields(const Json::Value* json,
                                                     const std::vector<std::string>& requiredFields) const
    {
        std::vector<std::string> missingFields;
        if (!json) {
            return {"请求体为空"};
        }
        
        for (const auto& field : requiredFields) {
            if (!json->isMember(field)) {
                missingFields.push_back(field);
            }
        }
        
        return missingFields;
    }
    
    /**
     * 获取当前时间戳字符串
     * @return 格式化的时间字符串 "YYYY-MM-DD HH:MM:SS"
     */
    std::string getCurrentTime() const
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm* tm = std::localtime(&time);
        std::stringstream ss;
        ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    /**
     * 从路径参数中获取整数ID
     * @param req HTTP请求对象
     * @param paramName 参数名（如 "id"）
     * @param callback 响应回调函数（如果转换失败会调用）
     * @return 转换后的整数，如果失败返回 -1 并发送错误响应
     */
    int getPathParamAsInt(const HttpRequestPtr& req,
                          const std::string& paramName,
                          std::function<void(const HttpResponsePtr&)>&& callback) const
    {
        try {
            const std::string& paramStr = req->getParameter(paramName);
            if (paramStr.empty()) {
                sendErrorResponse(std::move(callback), 400, "缺少路径参数: " + paramName, k400BadRequest);
                return -1;
            }
            return std::stoi(paramStr);
        } catch (const std::exception& e) {
            sendErrorResponse(std::move(callback), 400, "路径参数格式错误: " + paramName, k400BadRequest);
            return -1;
        }
    }

protected:
    // 基类构造函数和析构函数
    BaseController() = default;
    virtual ~BaseController() = default;
    
    // 禁止拷贝和赋值
    BaseController(const BaseController&) = delete;
    BaseController& operator=(const BaseController&) = delete;
};


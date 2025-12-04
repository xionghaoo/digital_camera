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

using namespace drogon;

/**
 * 用户控制器
 * 提供完整的用户 CRUD 操作
 */
class UserController : public HttpController<UserController>, public BaseController
{
public:
    // 禁用自动创建，允许手动注册
    static constexpr bool isAutoCreation = false;
    
    METHOD_LIST_BEGIN
        // 使用宏自动注册到文档
        ADD_METHOD_WITH_DOC(UserController, getUser, "/api/user/{id}", Get,
                           "获取用户信息", "根据用户ID获取用户详细信息");
        ADD_METHOD_WITH_DOC(UserController, getUserList, "/api/user", Get,
                           "获取用户列表", "获取所有用户列表，支持分页和搜索");
        ADD_METHOD_WITH_DOC(UserController, createUser, "/api/user", Post,
                           "创建用户", "创建新用户，需要提供姓名、邮箱等信息");
        ADD_METHOD_WITH_DOC(UserController, updateUser, "/api/user/{id}", Put,
                           "更新用户", "更新指定用户的信息");
        ADD_METHOD_WITH_DOC(UserController, deleteUser, "/api/user/{id}", Delete,
                           "删除用户", "删除指定用户");
        ADD_METHOD_WITH_DOC(UserController, searchUser, "/api/user/search", Get,
                           "搜索用户", "根据关键词搜索用户");
    METHOD_LIST_END

    // 模拟数据库（实际项目中应该使用真实的数据库）
    struct User {
        int id;
        std::string name;
        std::string email;
        std::string phone;
        int age;
        std::string createdAt;
    };
    
    UserController()
    {
        // 初始化一些示例数据
        initSampleData();
    }

    // GET /api/user/{id} - 获取用户信息
    void getUser(const HttpRequestPtr& req,
                 std::function<void(const HttpResponsePtr&)>&& callback,
                 const std::string& idStr)
    {
        // 将路径参数转换为整数
        int id = 0;
        try {
            id = std::stoi(idStr);
        } catch (const std::exception& e) {
            sendErrorResponse(std::move(callback), 400, "用户ID格式错误", k400BadRequest);
            return;
        }
        
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        auto it = users_.find(id);
        if (it == users_.end()) {
            sendErrorResponse(std::move(callback), 404, "用户不存在", k404NotFound);
            return;
        }
        
        const User& user = it->second;
        sendSuccessResponse(std::move(callback), "成功", userToJson(user));
    }

    // GET /api/user - 获取用户列表
    void getUserList(const HttpRequestPtr& req,
                     std::function<void(const HttpResponsePtr&)>&& callback)
    {
        // 获取查询参数（使用基类的便利方法）
        int pageNum = getQueryParamAsInt(req, "page", 1);
        int pageSizeNum = getQueryParamAsInt(req, "pageSize", 10);
        std::string keyword = getQueryParam(req, "keyword", "");
        
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        Json::Value data;
        Json::Value users(Json::arrayValue);
        
        int start = (pageNum - 1) * pageSizeNum;
        int count = 0;
        int total = 0;
        
        for (const auto& pair : users_) {
            const User& user = pair.second;
            
            // 关键词过滤
            if (!keyword.empty()) {
                if (user.name.find(keyword) == std::string::npos &&
                    user.email.find(keyword) == std::string::npos) {
                    continue;
                }
            }
            
            total++;
            
            if (count >= start && count < start + pageSizeNum) {
                users.append(userToJson(user));
            }
            
            count++;
        }
        
        // 使用基类的分页响应方法（保持原有的 users 字段名）
        sendPaginatedResponse(std::move(callback), users, total, pageNum, pageSizeNum, "成功", "users");
    }

    // POST /api/user - 创建用户
    void createUser(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback)
    {
        // 使用基类的验证方法
        const Json::Value* json = validateJsonRequest(req);
        if (!json) {
            sendErrorResponse(std::move(callback), 400, "请求体格式错误，需要JSON格式", k400BadRequest);
            return;
        }
        
        // 验证必填字段
        std::vector<std::string> missingFields = validateRequiredFields(json, {"name", "email"});
        if (!missingFields.empty()) {
            std::string message = "缺少必填字段: " + missingFields[0];
            for (size_t i = 1; i < missingFields.size(); ++i) {
                message += ", " + missingFields[i];
            }
            sendErrorResponse(std::move(callback), 400, message, k400BadRequest);
            return;
        }
        
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        // 检查邮箱是否已存在
        std::string email = (*json)["email"].asString();
        for (const auto& pair : users_) {
            if (pair.second.email == email) {
                sendErrorResponse(std::move(callback), 409, "邮箱已存在", k409Conflict);
                return;
            }
        }
        
        // 创建新用户
        User newUser;
        newUser.id = getNextUserId();
        newUser.name = (*json)["name"].asString();
        newUser.email = email;
        newUser.phone = json->isMember("phone") ? (*json)["phone"].asString() : "";
        newUser.age = json->isMember("age") ? (*json)["age"].asInt() : 0;
        newUser.createdAt = getCurrentTime();
        
        users_[newUser.id] = newUser;
        
        Json::Value data;
        data["id"] = newUser.id;
        data["name"] = newUser.name;
        data["email"] = newUser.email;
        
        sendSuccessResponse(std::move(callback), "创建成功", data, k201Created);
    }

    // PUT /api/user/{id} - 更新用户
    void updateUser(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback,
                    const std::string& idStr)
    {
        // 将路径参数转换为整数
        int id = 0;
        try {
            id = std::stoi(idStr);
        } catch (const std::exception& e) {
            sendErrorResponse(std::move(callback), 400, "用户ID格式错误", k400BadRequest);
            return;
        }
        
        // 使用基类的验证方法
        const Json::Value* json = validateJsonRequest(req);
        if (!json) {
            sendErrorResponse(std::move(callback), 400, "请求体格式错误，需要JSON格式", k400BadRequest);
            return;
        }
        
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        auto it = users_.find(id);
        if (it == users_.end()) {
            sendErrorResponse(std::move(callback), 404, "用户不存在", k404NotFound);
            return;
        }
        
        User& user = it->second;
        
        // 更新字段
        if (json->isMember("name")) {
            user.name = (*json)["name"].asString();
        }
        if (json->isMember("email")) {
            std::string newEmail = (*json)["email"].asString();
            // 检查邮箱是否被其他用户使用
            for (const auto& pair : users_) {
                if (pair.first != id && pair.second.email == newEmail) {
                    sendErrorResponse(std::move(callback), 409, "邮箱已被其他用户使用", k409Conflict);
                    return;
                }
            }
            user.email = newEmail;
        }
        if (json->isMember("phone")) {
            user.phone = (*json)["phone"].asString();
        }
        if (json->isMember("age")) {
            user.age = (*json)["age"].asInt();
        }
        
        sendSuccessResponse(std::move(callback), "更新成功", userToJson(user));
    }

    // DELETE /api/user/{id} - 删除用户
    void deleteUser(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback,
                    const std::string& idStr)
    {
        // 将路径参数转换为整数
        int id = 0;
        try {
            id = std::stoi(idStr);
        } catch (const std::exception& e) {
            sendErrorResponse(std::move(callback), 400, "用户ID格式错误", k400BadRequest);
            return;
        }
        
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        auto it = users_.find(id);
        if (it == users_.end()) {
            sendErrorResponse(std::move(callback), 404, "用户不存在", k404NotFound);
            return;
        }
        
        users_.erase(it);
        
        sendSuccessResponse(std::move(callback), "删除成功");
    }

    // GET /api/user/search - 搜索用户
    void searchUser(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback)
    {
        std::string keyword = getQueryParam(req, "q", "");
        if (keyword.empty()) {
            sendErrorResponse(std::move(callback), 400, "缺少搜索关键词参数 q", k400BadRequest);
            return;
        }
        
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        Json::Value users(Json::arrayValue);
        
        for (const auto& pair : users_) {
            const User& user = pair.second;
            
            // 在姓名、邮箱、电话中搜索
            if (user.name.find(keyword) != std::string::npos ||
                user.email.find(keyword) != std::string::npos ||
                user.phone.find(keyword) != std::string::npos) {
                users.append(userToJson(user));
            }
        }
        
        Json::Value data;
        data["users"] = users;
        data["count"] = static_cast<int>(users.size());
        data["keyword"] = keyword;
        
        sendSuccessResponse(std::move(callback), "成功", data);
    }

private:
    std::map<int, User> users_;
    std::mutex usersMutex_;
    int nextUserId_ = 1;
    
    /**
     * 将User对象转换为Json::Value
     */
    Json::Value userToJson(const User& user) const
    {
        Json::Value json;
        json["id"] = user.id;
        json["name"] = user.name;
        json["email"] = user.email;
        json["phone"] = user.phone;
        json["age"] = user.age;
        json["createdAt"] = user.createdAt;
        return json;
    }
    
    void initSampleData()
    {
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        User user1;
        user1.id = 1;
        user1.name = "张三";
        user1.email = "zhangsan@example.com";
        user1.phone = "13800138001";
        user1.age = 25;
        user1.createdAt = "2024-01-01 10:00:00";
        users_[1] = user1;
        
        User user2;
        user2.id = 2;
        user2.name = "李四";
        user2.email = "lisi@example.com";
        user2.phone = "13800138002";
        user2.age = 30;
        user2.createdAt = "2024-01-02 11:00:00";
        users_[2] = user2;
        
        User user3;
        user3.id = 3;
        user3.name = "王五";
        user3.email = "wangwu@example.com";
        user3.phone = "13800138003";
        user3.age = 28;
        user3.createdAt = "2024-01-03 12:00:00";
        users_[3] = user3;
        
        nextUserId_ = 4;
    }
    
    int getNextUserId()
    {
        return nextUserId_++;
    }
    
};


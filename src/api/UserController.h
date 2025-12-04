#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpRequest.h>
#include <json/json.h>
#include "DocController.h"
#include "AutoDocMacro.h"
#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

using namespace drogon;

/**
 * 用户控制器
 * 提供完整的用户 CRUD 操作
 */
class UserController : public HttpController<UserController>
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
                 int id)
    {
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        auto it = users_.find(id);
        if (it == users_.end()) {
            Json::Value ret;
            ret["code"] = 404;
            ret["message"] = "用户不存在";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        
        const User& user = it->second;
        Json::Value ret;
        ret["code"] = 0;
        ret["message"] = "成功";
        ret["data"]["id"] = user.id;
        ret["data"]["name"] = user.name;
        ret["data"]["email"] = user.email;
        ret["data"]["phone"] = user.phone;
        ret["data"]["age"] = user.age;
        ret["data"]["createdAt"] = user.createdAt;
        
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }

    // GET /api/user - 获取用户列表
    void getUserList(const HttpRequestPtr& req,
                     std::function<void(const HttpResponsePtr&)>&& callback)
    {
        // 获取查询参数
        std::string page = "1";
        std::string pageSize = "10";
        std::string keyword = "";
        
        try {
            const auto& params = req->getParameters();
            auto it = params.find("page");
            if (it != params.end()) {
                page = it->second;
            }
            it = params.find("pageSize");
            if (it != params.end()) {
                pageSize = it->second;
            }
            it = params.find("keyword");
            if (it != params.end()) {
                keyword = it->second;
            }
        } catch (...) {
            // 使用默认值
        }
        
        int pageNum = std::stoi(page);
        int pageSizeNum = std::stoi(pageSize);
        
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        Json::Value ret;
        ret["code"] = 0;
        ret["message"] = "成功";
        
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
                Json::Value userJson;
                userJson["id"] = user.id;
                userJson["name"] = user.name;
                userJson["email"] = user.email;
                userJson["phone"] = user.phone;
                userJson["age"] = user.age;
                userJson["createdAt"] = user.createdAt;
                users.append(userJson);
            }
            
            count++;
        }
        
        data["users"] = users;
        data["total"] = total;
        data["page"] = pageNum;
        data["pageSize"] = pageSizeNum;
        data["totalPages"] = (total + pageSizeNum - 1) / pageSizeNum;
        
        ret["data"] = data;
        
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }

    // POST /api/user - 创建用户
    void createUser(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback)
    {
        auto json = req->getJsonObject();
        if (!json) {
            Json::Value ret;
            ret["code"] = 400;
            ret["message"] = "请求体格式错误";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        
        // 验证必填字段
        if (!json->isMember("name") || !json->isMember("email")) {
            Json::Value ret;
            ret["code"] = 400;
            ret["message"] = "缺少必填字段：name 或 email";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        // 检查邮箱是否已存在
        std::string email = (*json)["email"].asString();
        for (const auto& pair : users_) {
            if (pair.second.email == email) {
                Json::Value ret;
                ret["code"] = 409;
                ret["message"] = "邮箱已存在";
                auto resp = HttpResponse::newHttpJsonResponse(ret);
                resp->setStatusCode(k409Conflict);
                callback(resp);
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
        
        Json::Value ret;
        ret["code"] = 0;
        ret["message"] = "创建成功";
        ret["data"]["id"] = newUser.id;
        ret["data"]["name"] = newUser.name;
        ret["data"]["email"] = newUser.email;
        
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k201Created);
        callback(resp);
    }

    // PUT /api/user/{id} - 更新用户
    void updateUser(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback,
                    int id)
    {
        auto json = req->getJsonObject();
        if (!json) {
            Json::Value ret;
            ret["code"] = 400;
            ret["message"] = "请求体格式错误";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        auto it = users_.find(id);
        if (it == users_.end()) {
            Json::Value ret;
            ret["code"] = 404;
            ret["message"] = "用户不存在";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k404NotFound);
            callback(resp);
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
                    Json::Value ret;
                    ret["code"] = 409;
                    ret["message"] = "邮箱已被其他用户使用";
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(k409Conflict);
                    callback(resp);
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
        
        Json::Value ret;
        ret["code"] = 0;
        ret["message"] = "更新成功";
        ret["data"]["id"] = user.id;
        ret["data"]["name"] = user.name;
        ret["data"]["email"] = user.email;
        ret["data"]["phone"] = user.phone;
        ret["data"]["age"] = user.age;
        
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }

    // DELETE /api/user/{id} - 删除用户
    void deleteUser(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback,
                    int id)
    {
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        auto it = users_.find(id);
        if (it == users_.end()) {
            Json::Value ret;
            ret["code"] = 404;
            ret["message"] = "用户不存在";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        
        users_.erase(it);
        
        Json::Value ret;
        ret["code"] = 0;
        ret["message"] = "删除成功";
        
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }

    // GET /api/user/search - 搜索用户
    void searchUser(const HttpRequestPtr& req,
                    std::function<void(const HttpResponsePtr&)>&& callback)
    {
        std::string keyword = "";
        try {
            const auto& params = req->getParameters();
            auto it = params.find("q");
            if (it != params.end()) {
                keyword = it->second;
            }
        } catch (...) {
            // 使用默认值
        }
        if (keyword.empty()) {
            Json::Value ret;
            ret["code"] = 400;
            ret["message"] = "缺少搜索关键词参数 q";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        Json::Value ret;
        ret["code"] = 0;
        ret["message"] = "成功";
        
        Json::Value users(Json::arrayValue);
        
        for (const auto& pair : users_) {
            const User& user = pair.second;
            
            // 在姓名、邮箱、电话中搜索
            if (user.name.find(keyword) != std::string::npos ||
                user.email.find(keyword) != std::string::npos ||
                user.phone.find(keyword) != std::string::npos) {
                
                Json::Value userJson;
                userJson["id"] = user.id;
                userJson["name"] = user.name;
                userJson["email"] = user.email;
                userJson["phone"] = user.phone;
                userJson["age"] = user.age;
                users.append(userJson);
            }
        }
        
        ret["data"]["users"] = users;
        ret["data"]["count"] = static_cast<int>(users.size());
        ret["data"]["keyword"] = keyword;
        
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }

private:
    std::map<int, User> users_;
    std::mutex usersMutex_;
    int nextUserId_ = 1;
    
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
    
    std::string getCurrentTime()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm* tm = std::localtime(&time);
        std::stringstream ss;
        ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};


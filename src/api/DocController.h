#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpTypes.h>
#include <json/json.h>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <regex>
#include <algorithm>
#include <cctype>

using namespace drogon;

/**
 * API文档控制器
 * 提供 Swagger UI 和 OpenAPI JSON
 */
class DocController : public HttpController<DocController>
{
public:
    // 禁用自动创建，允许手动注册
    static constexpr bool isAutoCreation = false;
    
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(DocController::swaggerUI, "/api-doc", Get);
        ADD_METHOD_TO(DocController::openAPIJson, "/api-doc/openapi.json", Get);
    METHOD_LIST_END

    // 返回 Swagger UI 页面
    void swaggerUI(const HttpRequestPtr& req,
                   std::function<void(const HttpResponsePtr&)>&& callback)
    {
        // 方法1: 从文件读取 Swagger UI HTML
        std::ifstream file("public/swagger-ui/index.html");
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            
            // 替换 OpenAPI JSON URL
            std::string openApiUrl = "/api-doc/openapi.json";
            size_t pos = content.find("https://petstore.swagger.io/v2/swagger.json");
            if (pos != std::string::npos) {
                content.replace(pos, 43, openApiUrl);
            }
            
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->setContentTypeCode(CT_TEXT_HTML);
            resp->setBody(content);
            callback(resp);
            return;
        }
        
        // 方法2: 内嵌简单 HTML（如果没有 Swagger UI 文件）
        std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>API Documentation</title>
    <link rel="stylesheet" type="text/css" href="https://unpkg.com/swagger-ui-dist@5.10.0/swagger-ui.css" />
</head>
<body>
    <div id="swagger-ui"></div>
    <script src="https://unpkg.com/swagger-ui-dist@5.10.0/swagger-ui-bundle.js"></script>
    <script>
        window.onload = function() {
            SwaggerUIBundle({
                url: "/api-doc/openapi.json",
                dom_id: '#swagger-ui',
                presets: [
                    SwaggerUIBundle.presets.apis,
                    SwaggerUIBundle.presets.standalone
                ]
            });
        };
    </script>
</body>
</html>
)";
        
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(html);
        callback(resp);
    }

    // 自动生成 OpenAPI JSON 规范（从 Drogon 路由）
    void openAPIJson(const HttpRequestPtr& req,
                     std::function<void(const HttpResponsePtr&)>&& callback)
    {
        Json::Value spec;
        
        // OpenAPI 基本信息
        spec["openapi"] = "3.0.0";
        spec["info"]["title"] = "API Documentation";
        spec["info"]["version"] = "1.0.0";
        spec["info"]["description"] = "自动生成的API文档";
        spec["info"]["contact"]["email"] = "support@example.com";
        
        // 服务器配置
        Json::Value servers(Json::arrayValue);
        Json::Value server;
        std::string host = req->getHeader("Host");
        if (host.empty()) {
            host = "localhost:8080";
        }
        server["url"] = "http://" + host;
        server["description"] = "API服务器";
        servers.append(server);
        spec["servers"] = servers;
        
        // 从注册的路由自动生成路径定义
        Json::Value paths = generatePathsFromRegisteredRoutes();
        
        // 如果没有任何注册的路由，使用示例路径
        if (paths.empty()) {
            paths = generateExamplePaths();
        }
        
        spec["paths"] = paths;
        
        // 组件定义（可选的 schema 定义）
        Json::Value components;
        Json::Value schemas;
        
        Json::Value userSchema;
        userSchema["type"] = "object";
        Json::Value userProperties;
        userProperties["id"]["type"] = "integer";
        userProperties["name"]["type"] = "string";
        userProperties["email"]["type"] = "string";
        userSchema["properties"] = userProperties;
        schemas["User"] = userSchema;
        
        components["schemas"] = schemas;
        spec["components"] = components;
        
        auto resp = HttpResponse::newHttpJsonResponse(spec);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
    }

private:
    // 从 Drogon 路由自动生成路径定义
    Json::Value generatePathsFromRoutes()
    {
        Json::Value paths;
        
        // 获取应用框架实例
        auto& app = drogon::app();
        
        // 方法1: 尝试通过反射获取路由信息
        // 注意：Drogon 可能没有直接提供获取所有路由的公开 API
        // 这里提供一个框架，需要根据实际 Drogon 版本调整
        
        // 方法2: 通过路由表获取（如果 Drogon 提供）
        // 某些版本的 Drogon 可能提供 getRoutingTable() 或类似方法
        
        // 方法3: 使用路由注册回调机制
        // 在注册路由时同时注册到文档生成器
        
        // 由于 Drogon 可能不直接提供路由列表 API，
        // 我们提供一个基于路径模式匹配的方法
        // 或者使用路由注册时的回调
        
        return paths;
    }
    
    // 解析路径参数（从路径中提取 {param} 格式的参数）
    Json::Value extractPathParameters(const std::string& path)
    {
        Json::Value parameters(Json::arrayValue);
        
        // 使用正则表达式匹配 {param} 格式
        std::regex paramRegex(R"(\{([^}]+)\})");
        std::sregex_iterator iter(path.begin(), path.end(), paramRegex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::smatch match = *iter;
            std::string paramName = match[1].str();
            
            Json::Value param;
            param["name"] = paramName;
            param["in"] = "path";
            param["required"] = true;
            param["description"] = paramName;
            
            // 尝试推断参数类型（简单推断）
            Json::Value schema;
            if (paramName.find("id") != std::string::npos || 
                paramName.find("Id") != std::string::npos ||
                paramName.find("ID") != std::string::npos) {
                schema["type"] = "integer";
                schema["example"] = 1;
            } else {
                schema["type"] = "string";
                schema["example"] = "example";
            }
            param["schema"] = schema;
            
            parameters.append(param);
        }
        
        return parameters;
    }
    
    // 根据 HTTP 方法创建操作定义
    Json::Value createOperation(const std::string& method,
                                const std::string& path,
                                const Json::Value& pathParams = Json::Value())
    {
        Json::Value operation;
        
        // 生成操作ID（从路径和方法生成）
        std::string operationId = generateOperationId(method, path);
        operation["operationId"] = operationId;
        
        // 生成摘要和描述
        operation["summary"] = method + " " + path;
        operation["description"] = "自动生成的API端点: " + method + " " + path;
        
        // 添加标签（从路径的第一部分提取）
        std::string tag = extractTagFromPath(path);
        if (!tag.empty()) {
            operation["tags"].append(tag);
        }
        
        // 添加路径参数
        if (!pathParams.empty() && pathParams.isArray()) {
            operation["parameters"] = pathParams;
        }
        
        // 添加默认响应
        Json::Value responses;
        Json::Value response200;
        response200["description"] = "成功响应";
        Json::Value content;
        Json::Value schema;
        schema["type"] = "object";
        content["application/json"]["schema"] = schema;
        response200["content"] = content;
        responses["200"] = response200;
        
        // 添加错误响应
        Json::Value response400;
        response400["description"] = "请求错误";
        responses["400"] = response400;
        
        Json::Value response500;
        response500["description"] = "服务器错误";
        responses["500"] = response500;
        
        operation["responses"] = responses;
        
        // 如果是 POST/PUT/PATCH，添加请求体
        if (method == "Post" || method == "Put" || method == "Patch") {
            Json::Value requestBody;
            requestBody["required"] = true;
            requestBody["description"] = "请求体";
            Json::Value reqContent;
            Json::Value reqSchema;
            reqSchema["type"] = "object";
            reqContent["application/json"]["schema"] = reqSchema;
            requestBody["content"] = reqContent;
            operation["requestBody"] = requestBody;
        }
        
        return operation;
    }
    
    // 从路径生成操作ID
    std::string generateOperationId(const std::string& method, const std::string& path)
    {
        std::string opId = method;
        std::string cleanPath = path;
        
        // 移除开头的斜杠
        if (!cleanPath.empty() && cleanPath[0] == '/') {
            cleanPath = cleanPath.substr(1);
        }
        
        // 替换斜杠和参数为驼峰命名
        std::regex slashRegex("/");
        cleanPath = std::regex_replace(cleanPath, slashRegex, "");
        
        std::regex paramRegex(R"(\{([^}]+)\})");
        cleanPath = std::regex_replace(cleanPath, paramRegex, "$1");
        
        // 转换为驼峰命名
        bool capitalize = false;
        for (size_t i = 0; i < cleanPath.length(); ++i) {
            if (cleanPath[i] == '-' || cleanPath[i] == '_') {
                capitalize = true;
            } else if (capitalize) {
                cleanPath[i] = std::toupper(cleanPath[i]);
                capitalize = false;
            }
        }
        
        opId += cleanPath;
        return opId;
    }
    
    // 从路径提取标签（路径的第一部分）
    std::string extractTagFromPath(const std::string& path)
    {
        if (path.empty() || path[0] != '/') {
            return "API";
        }
        
        size_t start = 1;
        size_t end = path.find('/', start);
        if (end == std::string::npos) {
            end = path.length();
        }
        
        std::string tag = path.substr(start, end - start);
        if (tag.empty()) {
            return "API";
        }
        
        // 移除 api 前缀
        if (tag == "api" && end < path.length()) {
            size_t nextStart = end + 1;
            size_t nextEnd = path.find('/', nextStart);
            if (nextEnd == std::string::npos) {
                nextEnd = path.length();
            }
            tag = path.substr(nextStart, nextEnd - nextStart);
        }
        
        // 首字母大写
        if (!tag.empty()) {
            tag[0] = std::toupper(tag[0]);
        }
        
        return tag.empty() ? "API" : tag;
    }
    
    // 生成示例路径（作为后备方案）
    Json::Value generateExamplePaths()
    {
        Json::Value paths;
        
        // 示例: GET /api/user/{id}
        Json::Value getUserPath;
        Json::Value getOp = createOperation("Get", "/api/user/{id}", 
                                              extractPathParameters("/api/user/{id}"));
        getOp["summary"] = "获取用户信息";
        getOp["description"] = "根据用户ID获取用户详细信息";
        getUserPath["get"] = getOp;
        paths["/api/user/{id}"] = getUserPath;
        
        // 示例: POST /api/user
        Json::Value createUserPath;
        Json::Value postOp = createOperation("Post", "/api/user");
        postOp["summary"] = "创建用户";
        postOp["description"] = "创建新用户";
        createUserPath["post"] = postOp;
        paths["/api/user"] = createUserPath;
        
        return paths;
    }
    
    // 路由注册表（用于存储路由信息）
    struct RouteInfo {
        std::string path;
        std::string method;
        std::string summary;
        std::string description;
    };
    
    static std::vector<RouteInfo> registeredRoutes;
    
public:
    // 注册路由到文档生成器（在控制器中调用）
    static void registerRoute(const std::string& path,
                             const std::string& method,
                             const std::string& summary = "",
                             const std::string& description = "")
    {
        RouteInfo info;
        info.path = path;
        info.method = method;
        info.summary = summary.empty() ? method + " " + path : summary;
        info.description = description;
        registeredRoutes.push_back(info);
    }
    
    // 从注册的路由生成路径定义
    Json::Value generatePathsFromRegisteredRoutes()
    {
        Json::Value paths;
        
        // 按路径分组
        std::map<std::string, Json::Value> pathMap;
        
        for (const auto& route : registeredRoutes) {
            std::string pathKey = route.path;
            std::string methodKey = route.method;
            
            // 转换为小写用于 OpenAPI
            std::string methodLower = methodKey;
            std::transform(methodLower.begin(), methodLower.end(), methodLower.begin(), ::tolower);
            
            // 如果路径不存在，创建路径对象
            if (pathMap.find(pathKey) == pathMap.end()) {
                pathMap[pathKey] = Json::Value(Json::objectValue);
            }
            
            // 提取路径参数
            Json::Value pathParams = extractPathParameters(pathKey);
            
            // 创建操作
            Json::Value operation = createOperation(methodKey, pathKey, pathParams);
            
            // 如果有自定义摘要和描述，使用它们
            if (!route.summary.empty()) {
                operation["summary"] = route.summary;
            }
            if (!route.description.empty()) {
                operation["description"] = route.description;
            }
            
            // 添加到路径
            pathMap[pathKey][methodLower] = operation;
        }
        
        // 转换为 Json::Value
        for (const auto& pair : pathMap) {
            paths[pair.first] = pair.second;
        }
        
        return paths;
    }
};

// 静态成员定义
std::vector<DocController::RouteInfo> DocController::registeredRoutes;


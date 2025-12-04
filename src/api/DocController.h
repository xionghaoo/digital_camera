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
    
    // 参数信息结构（需要在 public 中以便外部访问）
    struct ParameterInfo {
        std::string name;           // 参数名
        std::string in;             // 参数位置: "query", "path", "header", "cookie"
        std::string type;            // 参数类型: "string", "integer", "number", "boolean", "array", "object"
        std::string description;     // 参数描述
        bool required = false;       // 是否必填
        std::string example;         // 示例值
        std::string defaultValue;    // 默认值
        std::string format;          // 格式: "int32", "int64", "float", "double", "date", "date-time", "email" 等
        std::vector<std::string> enumValues;  // 枚举值（如果是枚举类型）
        std::string itemsType;       // 数组元素类型（当 type 为 "array" 时）
        Json::Value schema;          // 自定义 schema（用于复杂对象）
        
        ParameterInfo() = default;
        ParameterInfo(const std::string& n, const std::string& i, const std::string& t)
            : name(n), in(i), type(t) {}
    };
    
    // 请求体信息结构
    struct RequestBodyInfo {
        std::string description;     // 请求体描述
        bool required = false;       // 是否必填
        Json::Value schema;          // Schema 定义
        std::map<std::string, std::string> examples;  // 示例（key: content-type, value: example）
        
        RequestBodyInfo() = default;
    };
    
    // 响应信息结构
    struct ResponseInfo {
        int statusCode;              // 状态码: 200, 400, 404 等
        std::string description;     // 响应描述
        Json::Value schema;          // 响应 Schema
        std::string contentType;     // 内容类型: "application/json", "application/octet-stream", "image/jpeg" 等
        std::map<std::string, std::string> examples;  // 示例
        
        ResponseInfo() : statusCode(200), contentType("application/json") {}
        ResponseInfo(int code, const std::string& desc, const std::string& type = "application/json")
            : statusCode(code), description(desc), contentType(type) {}
    };
    
    // 路由注册表（用于存储路由信息）
    struct RouteInfo {
        std::string path;
        std::string method;
        std::string summary;
        std::string description;
        std::vector<ParameterInfo> parameters;      // 参数列表（query, path, header）
        RequestBodyInfo requestBody;                 // 请求体信息
        std::vector<ResponseInfo> responses;         // 响应列表
        std::vector<std::string> tags;               // 标签列表
        std::string operationId;                     // 操作ID
        bool deprecated = false;                      // 是否已废弃
        
        RouteInfo() = default;
    };
    
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(DocController::rapidocUI, "/api-doc", Get);  // 默认使用 RapiDoc
        ADD_METHOD_TO(DocController::swaggerUI, "/api-doc/swagger", Get);
        ADD_METHOD_TO(DocController::redocUI, "/api-doc/redoc", Get);
        ADD_METHOD_TO(DocController::rapidocUI, "/api-doc/rapidoc", Get);
        ADD_METHOD_TO(DocController::customUI, "/api-doc/custom", Get);
        ADD_METHOD_TO(DocController::openAPIJson, "/api-doc/openapi.json", Get);
    METHOD_LIST_END

    // 返回 Swagger UI 页面（默认风格）
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
        
        // 方法2: 内嵌 Swagger UI HTML
        std::string html = R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>API Documentation - Swagger UI</title>
    <link rel="stylesheet" type="text/css" href="https://unpkg.com/swagger-ui-dist@5.10.0/swagger-ui.css" />
    <style>
        html { box-sizing: border-box; overflow: -moz-scrollbars-vertical; overflow-y: scroll; }
        *, *:before, *:after { box-sizing: inherit; }
        body { margin:0; padding:0; }
    </style>
</head>
<body>
    <div id="swagger-ui"></div>
    <script src="https://unpkg.com/swagger-ui-dist@5.10.0/swagger-ui-bundle.js"></script>
    <script src="https://unpkg.com/swagger-ui-dist@5.10.0/swagger-ui-standalone-preset.js"></script>
    <script>
        window.onload = function() {
            SwaggerUIBundle({
                url: "/api-doc/openapi.json",
                dom_id: '#swagger-ui',
                deepLinking: true,
                presets: [
                    SwaggerUIBundle.presets.apis,
                    SwaggerUIStandalonePreset
                ],
                plugins: [
                    SwaggerUIBundle.plugins.DownloadUrl
                ],
                layout: "StandaloneLayout"
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
    
    // 返回 ReDoc UI 页面（三栏风格，更适合阅读）
    void redocUI(const HttpRequestPtr& req,
                 std::function<void(const HttpResponsePtr&)>&& callback)
    {
        std::string html = R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>API Documentation - ReDoc</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { margin: 0; padding: 0; }
    </style>
</head>
<body>
    <redoc spec-url="/api-doc/openapi.json"></redoc>
    <script src="https://cdn.redoc.ly/redoc/latest/bundles/redoc.standalone.js"></script>
</body>
</html>
)";
        
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(html);
        callback(resp);
    }
    
    // 返回 RapiDoc UI 页面（现代化风格，支持暗色主题）
    void rapidocUI(const HttpRequestPtr& req,
                   std::function<void(const HttpResponsePtr&)>&& callback)
    {
        std::string html = R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>API Documentation - RapiDoc</title>
    <meta name="viewport" content="width=device-width, initial-scale=1, minimum-scale=1">
</head>
<body>
    <rapi-doc
        spec-url="/api-doc/openapi.json"
        theme="light"
        render-style="read"
        show-header="true"
        show-info="true"
        allow-spec-url-load="false"
        allow-spec-file-load="false"
        allow-server-selection="true"
        regular-font="'Roboto', sans-serif"
        mono-font="'Roboto Mono', monospace"
        primary-color="#3f51b5"
        bg-color="#ffffff"
        text-color="#333333"
        header-color="#3f51b5"
        primary-text-color="#ffffff"
        nav-bg-color="#fafafa"
        nav-text-color="#666666"
        nav-hover-bg-color="#f0f0f0"
        nav-hover-text-color="#333333"
        nav-accent-color="#3f51b5"
        nav-item-spacing="relaxed"
        font-size="14px"
        nav-item-font-size="14px"
        use-path-in-nav-bar="true"
        show-components="true"
        show-meta="true"
        show-schema-description="true"
        default-schema-tab="model"
        schema-description-expanded="false"
        schema-style="table"
        schema-expand-level="2"
        show-tags="true"
        show-methods-in-nav-bar="as-colored-blocks"
        show-only-required-in-samples="false"
        show-common-attributes="false"
        show-extension="true"
        show-default="true"
        show-example="true"
        show-read-only="false"
        show-write-only="false"
        show-xml-sample="true"
        collapse-table="false"
        hide-single-request-sample-tab="false"
        request-snippets-enabled="true"
        request-snippets="curl, javascript, python, php, java, go, ruby, csharp"
        response-tab-controls="true"
        response-tab-default="body"
        response-area-height="600px"
        show-curl-before-try="true"
        try-it-out-enabled="true"
        allow-try-it-out="true"
        allow-authentication="true"
        allow-spec-file-download="true"
        show-method-in-nav-bar="as-colored-text"
        persist-auth="true"
        default-api-server=""
        api-key-name="X-API-Key"
        api-key-location="header"
        api-key-value=""
        sort-endpoints-by="method"
        sort-tags-by="name"
        sort-operations-by="method"
        match-paths=""
        match-type="exact"
        hide-schema-pattern="false"
        hide-default-server-url="false"
        hide-single-request-sample-tab="false"
        show-required-fields-first="false"
        fill-request-fields-with-example="false"
        layout="column"
        render-style="read"
        schema-style="table"
        schema-expand-level="2"
        schema-description-expanded="false"
        default-schema-tab="model"
        response-area-height="600px"
        show-components="true"
        show-meta="true"
        show-schema-description="true"
        show-tags="true"
        show-methods-in-nav-bar="as-colored-blocks"
        show-only-required-in-samples="false"
        show-common-attributes="false"
        show-extension="true"
        show-default="true"
        show-example="true"
        show-read-only="false"
        show-write-only="false"
        show-xml-sample="true"
        collapse-table="false"
        hide-single-request-sample-tab="false"
        request-snippets-enabled="true"
        request-snippets="curl, javascript, python, php, java, go, ruby, csharp"
        response-tab-controls="true"
        response-tab-default="body"
        show-curl-before-try="true"
        try-it-out-enabled="true"
        allow-try-it-out="true"
        allow-authentication="true"
        allow-spec-file-download="true"
        show-method-in-nav-bar="as-colored-text"
        persist-auth="true"
        default-api-server=""
        api-key-name="X-API-Key"
        api-key-location="header"
        api-key-value=""
        sort-endpoints-by="method"
        sort-tags-by="name"
        sort-operations-by="method"
        match-paths=""
        match-type="exact"
        hide-schema-pattern="false"
        hide-default-server-url="false"
        hide-single-request-sample-tab="false"
        show-required-fields-first="false"
        fill-request-fields-with-example="false"
    ></rapi-doc>
    <script type="module" src="https://unpkg.com/rapidoc@9.3.4/dist/rapidoc-min.js"></script>
</body>
</html>
)";
        
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(html);
        callback(resp);
    }
    
    // 返回自定义风格的文档页面（简洁风格，带导航）
    void customUI(const HttpRequestPtr& req,
                  std::function<void(const HttpResponsePtr&)>&& callback)
    {
        std::string html = R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>API Documentation</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            line-height: 1.6;
            color: #333;
            background: #f5f5f5;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }
        header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 40px 20px;
            text-align: center;
            margin-bottom: 30px;
            border-radius: 8px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        header p {
            font-size: 1.1em;
            opacity: 0.9;
        }
        .nav-tabs {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
            flex-wrap: wrap;
        }
        .nav-tab {
            padding: 12px 24px;
            background: white;
            border: 2px solid #e0e0e0;
            border-radius: 6px;
            text-decoration: none;
            color: #333;
            font-weight: 500;
            transition: all 0.3s;
            display: inline-block;
        }
        .nav-tab:hover {
            background: #667eea;
            color: white;
            border-color: #667eea;
            transform: translateY(-2px);
            box-shadow: 0 4px 8px rgba(102, 126, 234, 0.3);
        }
        .nav-tab.active {
            background: #667eea;
            color: white;
            border-color: #667eea;
        }
        .doc-frame {
            background: white;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
            overflow: hidden;
            min-height: 600px;
        }
        .doc-frame iframe {
            width: 100%;
            height: 800px;
            border: none;
        }
        .info-box {
            background: white;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .info-box h3 {
            color: #667eea;
            margin-bottom: 10px;
        }
        .info-box ul {
            list-style: none;
            padding-left: 0;
        }
        .info-box li {
            padding: 8px 0;
            border-bottom: 1px solid #f0f0f0;
        }
        .info-box li:last-child {
            border-bottom: none;
        }
        .info-box a {
            color: #667eea;
            text-decoration: none;
        }
        .info-box a:hover {
            text-decoration: underline;
        }
        footer {
            text-align: center;
            padding: 20px;
            color: #666;
            margin-top: 40px;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>📚 API 文档</h1>
            <p>选择您喜欢的文档风格</p>
        </header>
        
        <div class="nav-tabs">
            <a href="/api-doc/swagger" class="nav-tab">Swagger UI</a>
            <a href="/api-doc/redoc" class="nav-tab">ReDoc</a>
            <a href="/api-doc/rapidoc" class="nav-tab">RapiDoc</a>
            <a href="/api-doc/openapi.json" class="nav-tab" target="_blank">OpenAPI JSON</a>
        </div>
        
        <div class="info-box">
            <h3>📖 文档风格说明</h3>
            <ul>
                <li><strong>Swagger UI</strong> - 经典的交互式文档界面，支持在线测试API</li>
                <li><strong>ReDoc</strong> - 三栏布局，更适合阅读和浏览，界面简洁美观</li>
                <li><strong>RapiDoc</strong> - 现代化设计，支持暗色主题，功能丰富</li>
                <li><strong>OpenAPI JSON</strong> - 原始OpenAPI规范文件，可用于导入Postman等工具</li>
            </ul>
        </div>
        
        <div class="doc-frame">
            <iframe src="/api-doc/rapidoc" id="docFrame"></iframe>
        </div>
        
        <footer>
            <p>API 文档自动生成 | <a href="/api-doc/openapi.json" target="_blank">下载 OpenAPI 规范</a></p>
        </footer>
    </div>
    
    <script>
        // 根据当前URL高亮对应的导航标签
        const currentPath = window.location.pathname;
        document.querySelectorAll('.nav-tab').forEach(tab => {
            if (tab.getAttribute('href') === currentPath) {
                tab.classList.add('active');
            }
        });
        
        // 如果是在自定义页面，加载 RapiDoc 到 iframe
        if (currentPath === '/api-doc/custom' || currentPath === '/api-doc') {
            document.getElementById('docFrame').src = '/api-doc/rapidoc';
        }
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
    
    static std::vector<RouteInfo> registeredRoutes;
    
public:
    // 注册路由到文档生成器（基础版本，向后兼容）
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
    
    // 注册路由（完整版本，支持参数、请求体、响应等）
    static void registerRouteWithDetails(const RouteInfo& routeInfo)
    {
        registeredRoutes.push_back(routeInfo);
    }
    
    // 便捷方法：创建参数信息
    static ParameterInfo createParameter(const std::string& name,
                                        const std::string& in,  // "query", "path", "header"
                                        const std::string& type,  // "string", "integer", "number", "boolean"
                                        const std::string& description = "",
                                        bool required = false,
                                        const std::string& example = "",
                                        const std::string& defaultValue = "",
                                        const std::string& format = "")
    {
        ParameterInfo param;
        param.name = name;
        param.in = in;
        param.type = type;
        param.description = description;
        param.required = required;
        param.example = example;
        param.defaultValue = defaultValue;
        param.format = format;
        return param;
    }
    
    // 便捷方法：创建查询参数
    static ParameterInfo createQueryParam(const std::string& name,
                                         const std::string& type,
                                         const std::string& description = "",
                                         bool required = false,
                                         const std::string& example = "",
                                         const std::string& defaultValue = "")
    {
        return createParameter(name, "query", type, description, required, example, defaultValue);
    }
    
    // 便捷方法：创建路径参数
    static ParameterInfo createPathParam(const std::string& name,
                                        const std::string& type,
                                        const std::string& description = "",
                                        const std::string& example = "")
    {
        return createParameter(name, "path", type, description, true, example);  // 路径参数总是必填
    }
    
    // 便捷方法：创建请求体信息
    static RequestBodyInfo createRequestBody(const std::string& description,
                                            const Json::Value& schema,
                                            bool required = true)
    {
        RequestBodyInfo body;
        body.description = description;
        body.schema = schema;
        body.required = required;
        return body;
    }
    
    // 便捷方法：创建响应信息
    static ResponseInfo createResponse(int statusCode,
                                      const std::string& description,
                                      const Json::Value& schema = Json::Value(),
                                      const std::string& contentType = "application/json")
    {
        ResponseInfo resp;
        resp.statusCode = statusCode;
        resp.description = description;
        resp.schema = schema;
        resp.contentType = contentType;
        return resp;
    }
    
    // 便捷方法：创建文件响应信息
    static ResponseInfo createFileResponse(const std::string& description = "文件下载",
                                           const std::string& contentType = "application/octet-stream")
    {
        Json::Value schema;
        schema["type"] = "string";
        schema["format"] = "binary";
        return createResponse(200, description, schema, contentType);
    }
    
    // 将 ParameterInfo 转换为 OpenAPI 参数格式
    Json::Value parameterToJson(const ParameterInfo& param)
    {
        Json::Value paramJson;
        paramJson["name"] = param.name;
        paramJson["in"] = param.in;
        paramJson["required"] = param.required;
        if (!param.description.empty()) {
            paramJson["description"] = param.description;
        }
        
        // 构建 schema
        Json::Value schema;
        schema["type"] = param.type;
        
        if (!param.format.empty()) {
            schema["format"] = param.format;
        }
        
        if (!param.defaultValue.empty()) {
            if (param.type == "integer" || param.type == "number") {
                try {
                    if (param.type == "integer") {
                        schema["default"] = std::stoi(param.defaultValue);
                    } else {
                        schema["default"] = std::stod(param.defaultValue);
                    }
                } catch (...) {
                    schema["default"] = param.defaultValue;
                }
            } else if (param.type == "boolean") {
                schema["default"] = (param.defaultValue == "true" || param.defaultValue == "1");
            } else {
                schema["default"] = param.defaultValue;
            }
        }
        
        if (!param.example.empty()) {
            if (param.type == "integer") {
                try {
                    schema["example"] = std::stoi(param.example);
                } catch (...) {
                    schema["example"] = param.example;
                }
            } else if (param.type == "number") {
                try {
                    schema["example"] = std::stod(param.example);
                } catch (...) {
                    schema["example"] = param.example;
                }
            } else if (param.type == "boolean") {
                schema["example"] = (param.example == "true" || param.example == "1");
            } else {
                schema["example"] = param.example;
            }
        }
        
        // 处理枚举值
        if (!param.enumValues.empty()) {
            Json::Value enumArray(Json::arrayValue);
            for (const auto& enumVal : param.enumValues) {
                enumArray.append(enumVal);
            }
            schema["enum"] = enumArray;
        }
        
        // 处理数组类型
        if (param.type == "array" && !param.itemsType.empty()) {
            Json::Value items;
            items["type"] = param.itemsType;
            schema["items"] = items;
        }
        
        // 如果有自定义 schema，使用它
        if (!param.schema.empty()) {
            schema = param.schema;
        }
        
        paramJson["schema"] = schema;
        
        return paramJson;
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
            
            // 创建操作
            Json::Value operation;
            
            // 操作ID
            if (!route.operationId.empty()) {
                operation["operationId"] = route.operationId;
            } else {
                operation["operationId"] = generateOperationId(methodKey, pathKey);
            }
            
            // 摘要和描述
            if (!route.summary.empty()) {
                operation["summary"] = route.summary;
            } else {
                operation["summary"] = methodKey + " " + pathKey;
            }
            
            if (!route.description.empty()) {
                operation["description"] = route.description;
            }
            
            // 标签
            if (!route.tags.empty()) {
                Json::Value tagsArray(Json::arrayValue);
                for (const auto& tag : route.tags) {
                    tagsArray.append(tag);
                }
                operation["tags"] = tagsArray;
            } else {
                std::string tag = extractTagFromPath(pathKey);
                if (!tag.empty()) {
                    operation["tags"].append(tag);
                }
            }
            
            // 废弃标记
            if (route.deprecated) {
                operation["deprecated"] = true;
            }
            
            // 参数（query, path, header）
            if (!route.parameters.empty()) {
                Json::Value parameters(Json::arrayValue);
                for (const auto& param : route.parameters) {
                    parameters.append(parameterToJson(param));
                }
                operation["parameters"] = parameters;
            } else {
                // 如果没有显式定义参数，自动提取路径参数
                Json::Value pathParams = extractPathParameters(pathKey);
                if (!pathParams.empty() && pathParams.isArray()) {
                    operation["parameters"] = pathParams;
                }
            }
            
            // 请求体
            if (!route.requestBody.schema.empty() || route.requestBody.required) {
                Json::Value requestBody;
                requestBody["required"] = route.requestBody.required;
                if (!route.requestBody.description.empty()) {
                    requestBody["description"] = route.requestBody.description;
                }
                
                Json::Value content;
                Json::Value jsonContent;
                if (!route.requestBody.schema.empty()) {
                    jsonContent["schema"] = route.requestBody.schema;
                } else {
                    Json::Value defaultSchema;
                    defaultSchema["type"] = "object";
                    jsonContent["schema"] = defaultSchema;
                }
                
                // 添加示例
                if (!route.requestBody.examples.empty()) {
                    Json::Value examples;
                    for (const auto& example : route.requestBody.examples) {
                        examples[example.first]["value"] = example.second;
                    }
                    jsonContent["examples"] = examples;
                }
                
                content["application/json"] = jsonContent;
                requestBody["content"] = content;
                operation["requestBody"] = requestBody;
            } else if (methodLower == "post" || methodLower == "put" || methodLower == "patch") {
                // 如果没有定义请求体，为 POST/PUT/PATCH 添加默认请求体
                Json::Value requestBody;
                requestBody["required"] = true;
                requestBody["description"] = "请求体";
                Json::Value content;
                Json::Value jsonContent;
                Json::Value schema;
                schema["type"] = "object";
                jsonContent["schema"] = schema;
                content["application/json"] = jsonContent;
                requestBody["content"] = content;
                operation["requestBody"] = requestBody;
            }
            
            // 响应
            Json::Value responses;
            if (!route.responses.empty()) {
                for (const auto& resp : route.responses) {
                    Json::Value responseJson;
                    responseJson["description"] = resp.description;
                    
                    if (!resp.schema.empty()) {
                        Json::Value content;
                        Json::Value jsonContent;
                        jsonContent["schema"] = resp.schema;
                        
                        // 添加示例
                        if (!resp.examples.empty()) {
                            Json::Value examples;
                            for (const auto& example : resp.examples) {
                                examples[example.first]["value"] = example.second;
                            }
                            jsonContent["examples"] = examples;
                        }
                        
                        content[resp.contentType] = jsonContent;
                        responseJson["content"] = content;
                    } else if (resp.statusCode != 204) {
                        // 如果 schema 为空且不是 204，提供默认的 JSON schema
                        Json::Value content;
                        Json::Value jsonContent;
                        Json::Value schema;
                        schema["type"] = "object";
                        schema["properties"]["code"]["type"] = "integer";
                        schema["properties"]["message"]["type"] = "string";
                        schema["properties"]["data"]["nullable"] = true;
                        
                        // 根据状态码设置示例值
                        if (resp.statusCode >= 200 && resp.statusCode < 300) {
                            schema["properties"]["code"]["example"] = 0;
                            schema["properties"]["message"]["example"] = "success";
                            // data 可以是 null 或 object，这里不强制
                        } else {
                            schema["properties"]["code"]["example"] = resp.statusCode;
                            schema["properties"]["message"]["example"] = resp.description;
                        }
                        
                        jsonContent["schema"] = schema;
                        content["application/json"] = jsonContent;
                        responseJson["content"] = content;
                    }
                    
                    responses[std::to_string(resp.statusCode)] = responseJson;
                }
            } else {
                // 默认响应
                Json::Value response200;
                response200["description"] = "成功响应";
                Json::Value content;
                Json::Value jsonContent;
                Json::Value schema;
                schema["type"] = "object";
                jsonContent["schema"] = schema;
                content["application/json"] = jsonContent;
                response200["content"] = content;
                responses["200"] = response200;
                
                // 错误响应
                Json::Value response400;
                response400["description"] = "请求错误";
                responses["400"] = response400;
                
                Json::Value response500;
                response500["description"] = "服务器错误";
                responses["500"] = response500;
            }
            operation["responses"] = responses;
            
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


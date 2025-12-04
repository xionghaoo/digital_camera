#pragma once

#include "DocController.h"
#include "AutoDocHelper.h"
#include <string>
#include <sstream>

/**
 * 自动文档注册宏（基础版）
 * 在 METHOD_LIST_BEGIN 中使用，自动注册路由到文档生成器
 */
#define ADD_METHOD_WITH_DOC(controller, method, path, httpMethod, summary, description) \
    ADD_METHOD_TO(controller::method, path, httpMethod); \
    static int _doc_registered_##method = ([]() { \
        DocController::registerRoute(path, #httpMethod, summary, description); \
        return 0; \
    })()

/**
 * 自动文档注册宏（增强版 - 自动提取路径参数）
 * 自动从路径中提取路径参数并注册
 */
#define ADD_METHOD_WITH_AUTO_DOC(controller, methodArg, pathArg, httpMethodArg, summaryArg, descriptionArg) \
    ADD_METHOD_TO(controller::methodArg, pathArg, httpMethodArg); \
    static int _doc_registered_##methodArg = ([]() { \
        DocController::RouteInfo route; \
        route.path = pathArg; \
        route.method = #httpMethodArg; \
        route.summary = summaryArg; \
        route.description = descriptionArg; \
        route.operationId = #methodArg; \
        \
        /* 自动提取路径参数 */ \
        route.parameters = AutoDoc::Helper::extractPathParamsFromPath(route.path); \
        \
        /* 自动添加标准响应 */ \
        if (std::string(#httpMethodArg) == "Get") { \
            route.responses = AutoDoc::Helper::createStandardCRUDResponses(); \
        } else if (std::string(#httpMethodArg) == "Post") { \
            Json::Value requestSchema; \
            requestSchema["type"] = "object"; \
            route.requestBody = AutoDoc::Helper::createRequestBodyFromSchema(requestSchema); \
            route.responses = AutoDoc::Helper::createStandardCRUDResponses(Json::Value(), true); \
        } else if (std::string(#httpMethodArg) == "Put" || std::string(#httpMethodArg) == "Patch") { \
            Json::Value requestSchema; \
            requestSchema["type"] = "object"; \
            route.requestBody = AutoDoc::Helper::createRequestBodyFromSchema(requestSchema, "请求体", false); \
            route.responses = AutoDoc::Helper::createStandardCRUDResponses(); \
        } else if (std::string(#httpMethodArg) == "Delete") { \
            route.responses = AutoDoc::Helper::createStandardCRUDResponses(); \
        } \
        \
        DocController::registerRouteWithDetails(route); \
        return 0; \
    })()

/**
 * 带查询参数的自动文档宏
 * @param queryParamsArg 查询参数列表，格式: "param1:type:desc,param2:type:desc"
 */
#define ADD_METHOD_WITH_QUERY_PARAMS(controller, methodArg, pathArg, httpMethodArg, summaryArg, descriptionArg, queryParamsArg) \
    ADD_METHOD_TO(controller::methodArg, pathArg, httpMethodArg); \
    static int _doc_registered_##methodArg = ([]() { \
        DocController::RouteInfo route; \
        route.path = pathArg; \
        route.method = #httpMethodArg; \
        route.summary = summaryArg; \
        route.description = descriptionArg; \
        route.operationId = #methodArg; \
        \
        /* 自动提取路径参数 */ \
        route.parameters = AutoDoc::Helper::extractPathParamsFromPath(route.path); \
        \
        /* 解析查询参数 */ \
        std::string params = queryParamsArg; \
        std::istringstream iss(params); \
        std::string param; \
        while (std::getline(iss, param, ',')) { \
            std::istringstream paramStream(param); \
            std::string name, type, desc; \
            std::getline(paramStream, name, ':'); \
            std::getline(paramStream, type, ':'); \
            std::getline(paramStream, desc); \
            route.parameters.push_back(DocController::createQueryParam(name, type, desc, false)); \
        } \
        \
        /* 自动添加标准响应 */ \
        if (std::string(#httpMethodArg) == "Get") { \
            route.responses = AutoDoc::Helper::createStandardCRUDResponses(); \
        } else if (std::string(#httpMethodArg) == "Post") { \
            Json::Value requestSchema; \
            requestSchema["type"] = "object"; \
            route.requestBody = AutoDoc::Helper::createRequestBodyFromSchema(requestSchema); \
            route.responses = AutoDoc::Helper::createStandardCRUDResponses(Json::Value(), true); \
        } \
        \
        DocController::registerRouteWithDetails(route); \
        return 0; \
    })()

/**
 * 带请求体参数的自动文档宏
 * @param bodyParamsArg 请求体参数列表，格式: "param1:type:desc,param2:type:desc"
 */
#define ADD_METHOD_WITH_BODY_PARAMS(controller, methodArg, pathArg, httpMethodArg, summaryArg, descriptionArg, bodyParamsArg) \
    ADD_METHOD_TO(controller::methodArg, pathArg, httpMethodArg); \
    static int _doc_registered_##methodArg = ([]() { \
        DocController::RouteInfo route; \
        route.path = pathArg; \
        route.method = #httpMethodArg; \
        route.summary = summaryArg; \
        route.description = descriptionArg; \
        route.operationId = #methodArg; \
        \
        /* 自动提取路径参数 */ \
        route.parameters = AutoDoc::Helper::extractPathParamsFromPath(route.path); \
        \
        /* 解析请求体参数 */ \
        Json::Value requestSchema; \
        requestSchema["type"] = "object"; \
        Json::Value properties; \
        std::string params = bodyParamsArg; \
        std::istringstream iss(params); \
        std::string param; \
        while (std::getline(iss, param, ',')) { \
            std::istringstream paramStream(param); \
            std::string name, type, desc; \
            std::getline(paramStream, name, ':'); \
            std::getline(paramStream, type, ':'); \
            std::getline(paramStream, desc); \
            Json::Value prop; \
            prop["type"] = type; \
            prop["description"] = desc; \
            properties[name] = prop; \
        } \
        requestSchema["properties"] = properties; \
        \
        /* 创建请求体 */ \
        route.requestBody = AutoDoc::Helper::createRequestBodyFromSchema(requestSchema); \
        \
        /* 自动添加标准响应 */ \
        if (std::string(#httpMethodArg) == "Post") { \
            route.responses = AutoDoc::Helper::createStandardCRUDResponses(Json::Value(), true); \
        } else { \
            route.responses = AutoDoc::Helper::createStandardCRUDResponses(); \
        } \
        \
        DocController::registerRouteWithDetails(route); \
        return 0; \
    })()

/**
 * 带文件响应的自动文档宏（用于下载文件、图片等接口）
 * @param contentTypeArg 响应内容类型，如 "application/octet-stream", "image/jpeg"
 */
#define ADD_METHOD_WITH_FILE_RESPONSE(controller, methodArg, pathArg, httpMethodArg, summaryArg, descriptionArg, contentTypeArg) \
    ADD_METHOD_TO(controller::methodArg, pathArg, httpMethodArg); \
    static int _doc_registered_##methodArg = ([]() { \
        DocController::RouteInfo route; \
        route.path = pathArg; \
        route.method = #httpMethodArg; \
        route.summary = summaryArg; \
        route.description = descriptionArg; \
        route.operationId = #methodArg; \
        \
        /* 自动提取路径参数 */ \
        route.parameters = AutoDoc::Helper::extractPathParamsFromPath(route.path); \
        \
        /* 添加文件响应 */ \
        route.responses.push_back(DocController::createFileResponse("成功返回文件", contentTypeArg)); \
        route.responses.push_back(DocController::createResponse(404, "文件不存在")); \
        route.responses.push_back(DocController::createResponse(500, "服务器错误")); \
        \
        DocController::registerRouteWithDetails(route); \
        return 0; \
    })()

/**
 * 带分页的 GET 请求自动文档宏
 */
#define ADD_METHOD_WITH_PAGINATION(controller, methodArg, pathArg, httpMethodArg, summaryArg, descriptionArg) \
    ADD_METHOD_TO(controller::methodArg, pathArg, httpMethodArg); \
    static int _doc_registered_##methodArg = ([]() { \
        DocController::RouteInfo route; \
        route.path = pathArg; \
        route.method = #httpMethodArg; \
        route.summary = summaryArg; \
        route.description = descriptionArg; \
        route.operationId = #methodArg; \
        \
        /* 自动提取路径参数 */ \
        route.parameters = AutoDoc::Helper::extractPathParamsFromPath(route.path); \
        \
        /* 添加分页参数 */ \
        auto paginationParams = AutoDoc::Helper::createPaginationParams(); \
        route.parameters.insert(route.parameters.end(), paginationParams.begin(), paginationParams.end()); \
        \
        /* 添加标准分页响应 */ \
        Json::Value itemSchema; \
        itemSchema["type"] = "object"; \
        Json::Value paginatedSchema = AutoDoc::Helper::createPaginatedResponseSchema(itemSchema, "users"); \
        Json::Value responseSchema = AutoDoc::Helper::createStandardSuccessResponseSchema(paginatedSchema); \
        route.responses.push_back(DocController::createResponse(200, "成功", responseSchema)); \
        \
        DocController::registerRouteWithDetails(route); \
        return 0; \
    })()

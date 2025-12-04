#pragma once

#include "DocController.h"
#include <json/json.h>
#include <string>
#include <vector>
#include <regex>

// 使用独立的命名空间避免与 DocController 类名冲突
namespace AutoDoc {

/**
 * 自动文档生成辅助类
 * 提供自动推断和生成参数信息的功能
 */
class Helper
{
public:
    /**
     * 从路径自动提取路径参数
     * @param path 路径，如 "/api/user/{id}"
     * @return 路径参数列表
     */
    static std::vector<DocController::ParameterInfo> extractPathParamsFromPath(const std::string& path)
    {
        std::vector<DocController::ParameterInfo> params;
        std::regex paramRegex(R"(\{([^}]+)\})");
        std::sregex_iterator iter(path.begin(), path.end(), paramRegex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::smatch match = *iter;
            std::string paramName = match[1].str();
            
            // 根据参数名推断类型
            std::string type = "string";
            std::string format = "";
            std::string example = "example";
            
            // 如果参数名包含 id，推断为 integer
            if (paramName.find("id") != std::string::npos || 
                paramName.find("Id") != std::string::npos ||
                paramName.find("ID") != std::string::npos) {
                type = "integer";
                format = "int32";
                example = "1";
            }
            
            DocController::ParameterInfo param = DocController::createPathParam(paramName, type, paramName, example);
            if (!format.empty()) {
                param.format = format;
            }
            params.push_back(param);
        }
        
        return params;
    }
    
    /**
     * 创建标准的成功响应 Schema（BaseController 格式）
     */
    static Json::Value createStandardSuccessResponseSchema(const Json::Value& dataSchema = Json::Value())
    {
        Json::Value responseSchema;
        responseSchema["type"] = "object";
        responseSchema["properties"]["code"]["type"] = "integer";
        responseSchema["properties"]["code"]["example"] = 0;
        responseSchema["properties"]["message"]["type"] = "string";
        responseSchema["properties"]["message"]["example"] = "成功";
        
        if (!dataSchema.empty()) {
            responseSchema["properties"]["data"] = dataSchema;
        }
        
        return responseSchema;
    }
    
    /**
     * 创建标准的错误响应 Schema
     */
    static Json::Value createStandardErrorResponseSchema()
    {
        Json::Value responseSchema;
        responseSchema["type"] = "object";
        responseSchema["properties"]["code"]["type"] = "integer";
        responseSchema["properties"]["message"]["type"] = "string";
        return responseSchema;
    }
    
    /**
     * 从 JSON Schema 自动创建请求体
     */
    static DocController::RequestBodyInfo createRequestBodyFromSchema(const Json::Value& schema, 
                                                       const std::string& description = "请求体",
                                                       bool required = true)
    {
        return DocController::createRequestBody(description, schema, required);
    }
    
    /**
     * 创建分页查询参数（page, pageSize）
     */
    static std::vector<DocController::ParameterInfo> createPaginationParams()
    {
        std::vector<DocController::ParameterInfo> params;
        params.push_back(DocController::createQueryParam("page", "integer", "页码，从1开始", false, "1", "1"));
        params.push_back(DocController::createQueryParam("pageSize", "integer", "每页大小", false, "10", "10"));
        return params;
    }
    
    /**
     * 创建标准的分页响应 Schema
     */
    static Json::Value createPaginatedResponseSchema(const Json::Value& itemSchema, 
                                                     const std::string& itemsKey = "items")
    {
        Json::Value dataSchema;
        dataSchema["type"] = "object";
        dataSchema["properties"][itemsKey]["type"] = "array";
        dataSchema["properties"][itemsKey]["items"] = itemSchema;
        dataSchema["properties"]["total"]["type"] = "integer";
        dataSchema["properties"]["total"]["description"] = "总记录数";
        dataSchema["properties"]["page"]["type"] = "integer";
        dataSchema["properties"]["page"]["description"] = "当前页码";
        dataSchema["properties"]["pageSize"]["type"] = "integer";
        dataSchema["properties"]["pageSize"]["description"] = "每页大小";
        dataSchema["properties"]["totalPages"]["type"] = "integer";
        dataSchema["properties"]["totalPages"]["description"] = "总页数";
        return dataSchema;
    }
    
    /**
     * 从 C++ 类型名推断 OpenAPI 类型
     */
    static std::string inferTypeFromCppType(const std::string& cppType)
    {
        if (cppType == "int" || cppType == "int32_t" || cppType == "long" || cppType == "int64_t") {
            return "integer";
        } else if (cppType == "float" || cppType == "double") {
            return "number";
        } else if (cppType == "bool") {
            return "boolean";
        } else if (cppType == "std::string" || cppType == "string") {
            return "string";
        }
        return "string";  // 默认
    }
    
    /**
     * 创建标准的 CRUD 响应列表
     */
    static std::vector<DocController::ResponseInfo> createStandardCRUDResponses(const Json::Value& successSchema = Json::Value(),
                                                                  bool includeCreated = false)
    {
        std::vector<DocController::ResponseInfo> responses;
        
        if (includeCreated) {
            responses.push_back(DocController::createResponse(201, "创建成功", successSchema));
        } else {
            responses.push_back(DocController::createResponse(200, "成功", successSchema));
        }
        
        responses.push_back(DocController::createResponse(400, "请求参数错误"));
        responses.push_back(DocController::createResponse(404, "资源不存在"));
        responses.push_back(DocController::createResponse(500, "服务器错误"));
        
        return responses;
    }
};

} // namespace AutoDoc

// 注意：不能使用 namespace DocController，因为 DocController 是类名
// 使用 AutoDoc::Helper 或 DocController::AutoDocHelper（如果定义了别名）


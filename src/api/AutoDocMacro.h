#pragma once

#include "DocController.h"

/**
 * 自动文档注册宏
 * 在 METHOD_LIST_BEGIN 中使用，自动注册路由到文档生成器
 */
#define ADD_METHOD_WITH_DOC(controller, method, path, httpMethod, summary, description) \
    ADD_METHOD_TO(controller::method, path, httpMethod); \
    static int _doc_registered_##method = ([]() { \
        DocController::registerRoute(path, #httpMethod, summary, description); \
        return 0; \
    })()

/**
 * 简化版：只有路径和方法
 */
#define ADD_METHOD_WITH_AUTO_DOC(controller, method, path, httpMethod) \
    ADD_METHOD_TO(controller::method, path, httpMethod); \
    static int _doc_registered_##method = ([]() { \
        DocController::registerRoute(path, #httpMethod, "", ""); \
        return 0; \
    })()

/**
 * 使用示例：
 * 
 * class UserController : public HttpController<UserController> {
 * public:
 *     METHOD_LIST_BEGIN
 *         ADD_METHOD_WITH_DOC(UserController, getUser, "/api/user/{id}", Get, 
 *                             "获取用户信息", "根据用户ID获取用户详细信息");
 *         ADD_METHOD_WITH_AUTO_DOC(UserController, createUser, "/api/user", Post);
 *     METHOD_LIST_END
 *     ...
 * };
 */


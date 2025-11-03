cmake_minimum_required(VERSION 3.10)
project(digital_camera)

# 设置C++标准为C++17
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

### Append project cmake script dir ###
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

### Enumerate project files ###
include(enum_cli_hdr)
include(enum_cli_src)
include(enum_crsdk_hdr)


# 添加可执行文件
# add_executable(${PROJECT_NAME} src/main.cpp)
### Define output target ###
set(digitalcamera "${PROJECT_NAME}")
add_executable(${digitalcamera}
    ${cli_hdrs}
    ${cli_srcs}
    ${crsdk_hdrs}
    src/main.cpp
)

set_target_properties(${digitalcamera} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)

target_compile_options(${digitalcamera}
    PUBLIC
        -fstack-protector-all
    PRIVATE
        -fsigned-char
)

### Configure external library directories ###
set(ldir ${CMAKE_CURRENT_SOURCE_DIR}/sony/libs)
set(cr_ldir ${ldir}/crsdk)

target_include_directories(${digitalcamera}
    PRIVATE
        ${crsdk_hdr_dir} # defined in enum script
        ${__cli_hdr_dir}
        ${ldir}/opencv/include
)

### Link CRSDK library
find_library(camera_remote Cr_Core HINTS ${cr_ldir})
target_link_libraries(${digitalcamera}
    PRIVATE
        ${camera_remote}
)

### Linux specific configuration ###
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8)
        # Must use std::experimental namespace if older than GCC8
        message("[${PROJECT_NAME}] GCC version less than 8. Using std::experimental namespace.")
        target_compile_definitions(${digitalcamera} PRIVATE USE_EXPERIMENTAL_FS)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        # Must explicitly link separate std::filesystem if older than GCC9
        message("[${PROJECT_NAME}] GCC version less than 9. Explicitly linking separate std::filesystem library.")
        target_link_libraries(${digitalcamera} PRIVATE stdc++fs)
    endif()
endif()

target_link_libraries(${digitalcamera} PRIVATE 
    ${ldir}/opencv/Linux/libopencv_core.so.408
    ${ldir}/opencv/Linux/libopencv_highgui.so.408
    ${ldir}/opencv/Linux/libopencv_imgcodecs.so.408
    ${ldir}/opencv/Linux/libopencv_imgproc.so.408
)

## Copy required library binaries
add_custom_command(TARGET ${digitalcamera} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${cr_ldir} $<TARGET_FILE_DIR:${digitalcamera}>
)

add_custom_command(TARGET ${digitalcamera} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ldir}/opencv/Linux/libopencv_core.so.408" $<TARGET_FILE_DIR:${digitalcamera}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ldir}/opencv/Linux/libopencv_highgui.so.408" $<TARGET_FILE_DIR:${digitalcamera}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ldir}/opencv/Linux/libopencv_imgcodecs.so.408" $<TARGET_FILE_DIR:${digitalcamera}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ldir}/opencv/Linux/libopencv_imgproc.so.408" $<TARGET_FILE_DIR:${digitalcamera}>
)

install(TARGETS ${digitalcamera} DESTINATION .)
install(DIRECTORY ${cr_ldir}/ DESTINATION .)
#pragma once

#include <libusb-1.0/libusb.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

// Sony Remote Camera SDK 命令ID定义（示例，需要根据实际SDK调整）
enum CrCommandId {
    CrCommandId_PowerOn = 0x0001,
    CrCommandId_PowerOff = 0x0002,
    CrCommandId_Shutter = 0x0004,
    CrCommandId_PixelMapping = 0x0008
};

class SonyCameraShutterWake {
    libusb_context* ctx = nullptr;
    libusb_device_handle* handle = nullptr;
    static constexpr uint16_t SONY_VID = 0x054C;
    
public:
    SonyCameraShutterWake() {
        // Ubuntu 22.04 需要确保libusb正确安装和链接
        int result = libusb_init(&ctx);
        if (result < 0) {
            std::cerr << "Failed to initialize libusb: " << libusb_error_name(result) << std::endl;
            std::cerr << "Error code: " << result << std::endl;
            std::cerr << "\nTroubleshooting:" << std::endl;
            std::cerr << "1. Install libusb: sudo apt-get install libusb-1.0-0 libusb-1.0-0-dev" << std::endl;
            std::cerr << "2. Check library: ldconfig -p | grep libusb" << std::endl;
            std::cerr << "3. Verify linking: ldd your_program | grep usb" << std::endl;
            ctx = nullptr;
        } else {
            // Ubuntu 22.04 的 libusb 版本可能不支持 set_option
            // 使用条件编译避免兼容性问题
            #if defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000107)
            #ifdef DEBUG
            libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
            #endif
            #endif
        }
    }
    
    ~SonyCameraShutterWake() {
        disconnect();
        if (ctx) {
            libusb_exit(ctx);
        }
    }
    
    bool connect() {
        if (!ctx) return false;
        if (handle) return true;
        
        libusb_device** devices = nullptr;
        ssize_t count = libusb_get_device_list(ctx, &devices);
        std::cout << "count: " << count << std::endl;
        if (count < 0) return false;
        
        for (ssize_t i = 0; i < count; i++) {
            libusb_device* dev = devices[i];
            if (!dev) continue;
            
            libusb_device_descriptor desc{};
            if (libusb_get_device_descriptor(dev, &desc) != 0) continue;
            if (desc.idVendor != SONY_VID) continue;
            
            libusb_device_handle* tempHandle = nullptr;
            int ret = libusb_open(dev, &tempHandle);
            std::cout << "libusb_open: " << ret << " - " << tempHandle << std::endl;
            if (ret == 0 && tempHandle) {
                for (int iface = 0; iface < 4; iface++) {
                    if (libusb_claim_interface(tempHandle, iface) == 0) {
                        handle = tempHandle;
                        libusb_free_device_list(devices, 1);
                        return true;
                    }
                }
                std::cout << "libusb_close: " << std::endl;
                libusb_close(tempHandle);
            }
        }
        
        libusb_free_device_list(devices, 1);
        return false;
    }
    
    // 通过USB发送Sony SDK命令
    bool sendCrCommand(CrCommandId commandId, const uint8_t* data = nullptr, size_t dataSize = 0) {
        if (!handle && !connect()) {
            return false;
        }
        
        // Sony SDK命令格式（根据实际协议调整）
        // 通常格式: [命令ID(2字节)] [参数...]
        uint8_t cmdBuffer[64] = {0};
        cmdBuffer[0] = (commandId >> 8) & 0xFF;  // 高字节
        cmdBuffer[1] = commandId & 0xFF;          // 低字节
        
        if (data && dataSize > 0) {
            size_t copySize = (dataSize < 62) ? dataSize : 62;
            memcpy(cmdBuffer + 2, data, copySize);
        }
        
        size_t totalSize = 2 + dataSize;
        if (totalSize > 64) totalSize = 64;
        
        int result = libusb_control_transfer(
            handle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT,
            0x00,        // bRequest: 通用命令
            0x0000,      // wValue
            0x0000,      // wIndex
            cmdBuffer,
            totalSize,
            5000
        );
        
        return result >= 0;
    }
    
    // 发送Shutter Trigger信号（用于唤醒关机状态的相机）
    bool sendShutterTrigger() {
        std::cout << "Sending shutter trigger signal via USB..." << std::endl;
        
        // 方法1: 使用SDK命令ID
        if (sendCrCommand(CrCommandId_Shutter)) {
            std::cout << "Shutter trigger sent via SDK command" << std::endl;
            return true;
        }
        
        // 方法2: 直接USB控制传输（备用方案）
        if (!handle && !connect()) {
            return false;
        }
        
        // 尝试多种shutter命令格式
        struct ShutterCommand {
            uint8_t bRequest;
            uint16_t wValue;
            uint8_t data[1];
        } commands[] = {
            {0x04, 0x0000, {0x01}},  // 格式1: 按下
            {0x02, 0x0001, {0x00}},  // 格式2: 触发
            {0x06, 0x0000, {0x01}},  // 格式3: 快门
        };
        
        for (auto& cmd : commands) {  // 去掉const，允许修改
            int result = libusb_control_transfer(
                handle,
                LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT,
                cmd.bRequest,
                cmd.wValue,
                0x0000,
                reinterpret_cast<unsigned char*>(cmd.data),  // 显式转换为unsigned char*
                sizeof(cmd.data),
                5000
            );
            
            if (result >= 0) {
                std::cout << "Shutter trigger sent (bRequest=0x" << std::hex 
                          << (int)cmd.bRequest << ")" << std::dec << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return true;
            }
        }
        
        std::cerr << "All shutter trigger methods failed" << std::endl;
        return false;
    }
    
    // 唤醒关机状态的相机
    bool wakeUpFromPowerOff() {
        std::cout << "Waking up camera from power-off state..." << std::endl;
        
        // 根据文档，关机状态下使用shutter trigger
        if (sendShutterTrigger()) {
            std::cout << "Shutter trigger sent, waiting for camera to boot..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            return true;
        }
        
        return false;
    }
    
    void disconnect() {
        if (handle) {
            for (int iface = 0; iface < 4; iface++) {
                libusb_release_interface(handle, iface);
            }
            libusb_close(handle);
            handle = nullptr;
        }
    }
};


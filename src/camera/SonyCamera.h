#pragma once

#include <cstdlib>
#if defined(USE_EXPERIMENTAL_FS)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#if defined(__APPLE__)
#include <unistd.h>
#endif
#endif

#include <string>
#include <iostream>
#include <functional>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <thread>
#include "CRSDK/CameraRemote_SDK.h"
#include "CameraDevice.h"
#include "Text.h"
#include <json/json.h>
#include "json.hpp"
using json = nlohmann::json;

namespace SDK = SCRSDK;
typedef std::shared_ptr<cli::CameraDevice> CameraDevicePtr;
typedef std::vector<CameraDevicePtr> CameraDeviceList;

static std::atomic<bool> isLiveRunning{false};

enum LiveType {
    NONE, REMOTE, LOCAL
};

class SonyCamera {
    private:
        int initialize();
        int release();
    protected:
        CameraDeviceList cameraList;
        CameraDevicePtr camera;
        bool isInitialized = false;
    public:
        std::thread liveThread;
        SDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
        LiveType liveType = LiveType::NONE;

        SonyCamera();
        ~SonyCamera();
        std::string version();
        Json::Value scan();
        bool connect(int index);
        bool connect_with_usb();
        bool af_shutter(std::function<void (std::string)>* cb);
        bool capture();
        std::string get_save_path();
        void power_off();
        void power_on();
        bool live_view();
        bool enable_live_view(bool enable, bool isLocal, std::string& rtmpUrl);

        bool zoom(int speed);
        bool zoom_distance(int scale);
};
#include "SonyCamera.h"
#include "SonyCameraShutterWake.h"

int SonyCamera::initialize()
{
    cli::tout << "Initialize Remote SDK...\n";
    auto init_success = SDK::Init();
    if (!init_success) {
        cli::tout << "Failed to initialize Remote SDK. Terminating.\n";
        release();
        return -1;
    }
    cli::tout << "Remote SDK successfully initialized.\n\n";
    isInitialized = true;
    return 0;
}

int SonyCamera::release() {
    if (!isInitialized) return -1;
    // relese cameras
    CameraDeviceList::const_iterator it = cameraList.begin();
    for (std::int32_t j = 0; it != cameraList.end(); ++j, ++it) {
        if ((*it)->is_connected()) {
            cli::tout << "Initiate disconnect sequence.\n";
            auto disconnect_status = (*it)->disconnect();
            if (!disconnect_status) {
                // try again
                disconnect_status = (*it)->disconnect();
            }
            if (!disconnect_status)
                cli::tout << "Disconnect failed to initiate.\n";
            else
                cli::tout << "Disconnect successfully initiated!\n\n";
        }
        (*it)->release();
    }
    SDK::Release();
    isInitialized = false;
    std::cout << "release camera" << std::endl;
    return 0;
}

SonyCamera::SonyCamera()
{
    initialize();
}

SonyCamera::~SonyCamera() {
    // Release the camera
}

std::string SonyCamera::version() {
    CrInt32u version = SDK::GetSDKVersion();
    int major = (version & 0xFF000000) >> 24;
    int minor = (version & 0x00FF0000) >> 16;
    int patch = (version & 0x0000FF00) >> 8;
    cli::tout << "Remote SDK version: ";
    std::string versionStr = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    cli::tout << major << "." << minor << "." << std::setfill(TEXT('0')) << std::setw(2) << patch << "\n";
    json j;
    j["version"] = versionStr;
    return j.dump();
}

std::string SonyCamera::scan() {
    if (!isInitialized) {
        initialize();
    }
    
    json ret = json::array();
    cli::tout << "Enumerate connected camera devices...\n";
    auto enum_status = SDK::EnumCameraObjects(&camera_list);
    if (CR_FAILED(enum_status) || camera_list == nullptr) {
        cli::tout << "No cameras detected. Connect a camera and retry.\n";
        release();
        return ret.dump();
    }
    auto ncams = camera_list->GetCount();
    cli::tout << "Camera enumeration successful. " << ncams << " detected.\n\n";
    json dev;
    for (CrInt32u i = 0; i < ncams; ++i) {
        auto camera_info = camera_list->GetCameraObjectInfo(i);
        cli::text conn_type(camera_info->GetConnectionTypeName());
        cli::text model(camera_info->GetModel());
        cli::text id = TEXT("");
        bool isWifi = TEXT("IP") == conn_type;
        if (isWifi) {
            //id.append((TCHAR*)camera_info->GetMACAddressChar());
            // or
            id.append((TCHAR*)camera_info->GetMACAddressChar(), (size_t)camera_info->GetMACAddressCharSize());
        }
        else id = ((TCHAR*)camera_info->GetId());
        cli::tout << '[' << i + 1 << "] " << model.data() << " (" << id.data() << ")\n";
        dev["index"] = i + 1;
        dev["name"] = model.data();
        dev["id"] = id.data();
        dev["is_wifi"] = isWifi;
        ret.push_back(dev);
    }
    return ret.dump();
}

bool SonyCamera::connect(int index) {
    if (camera_list == nullptr) {
        return false;
    }
    
    std::int32_t cameraNumUniq = 1;
    std::int32_t selectCamera = 1;

    cli::tout << "Connect to selected camera...\n";
    auto* camera_info = camera_list->GetCameraObjectInfo(index - 1);

    cli::tout << "Create camera SDK camera callback object.\n";
    camera = CameraDevicePtr(new cli::CameraDevice(cameraNumUniq, camera_info));
    cameraList.push_back(camera); // add 1st
    camera_list->Release();

    if (camera->is_connected()) {
        cli::tout << "Please disconnect\n";
        return false;
    } else {
        camera->connect(SDK::CrSdkControlMode_Remote, SDK::CrReconnecting_ON);
    }
    return true;
}

bool SonyCamera::connect_with_usb() {
    SDK::CrError err = SDK::CrError_None;

    CrChar* serialNum = new CrChar[SDK::USB_SERIAL_LENGTH + 1];
    int serialSiz = sizeof(CrChar) * (SDK::USB_SERIAL_LENGTH + 1);
    memset(serialNum, 0, serialSiz);
    strncpy((char*)serialNum, "D18D5074C0AE", serialSiz);
    SDK::CrCameraDeviceModelList usbModel = SDK::CrCameraDeviceModelList::CrCameraDeviceModel_ZV_E10M2;
    SDK::ICrCameraObjectInfo* pCam = nullptr;
    typedef std::shared_ptr<cli::CameraDevice> CameraDevicePtr;
    typedef std::vector<CameraDevicePtr> CameraDeviceList;

    // CameraDeviceList cameraList; // all
    std::int32_t cameraNumUniq = 1;
    std::int32_t selectCamera = 1;
    // CameraDevicePtr camera;
     // USB
    pCam = nullptr;
    err = SDK::CreateCameraObjectInfoUSBConnection(&pCam, usbModel, (unsigned char*)serialNum);
    if (err == 0) {
        cli::tout << "[" << cameraNumUniq << "] " << pCam->GetModel() << "(" << (TCHAR*)pCam->GetId() << ")\n";
        camera = CameraDevicePtr(new cli::CameraDevice(cameraNumUniq, pCam));
        cameraList.push_back(camera);
        // camera_list->Release();

        if (camera->is_connected()) {
            cli::tout << "Please disconnect\n";
            return false;
        } else {
             cli::tout << "connect...\n";
            camera->connect(SDK::CrSdkControlMode_Remote, SDK::CrReconnecting_ON);
        }
        cameraNumUniq++;
        return true;
    }
    return false;
}

bool SonyCamera::af_shutter(std::function<void (std::string)>* cb) {
    if (camera == nullptr) {
        cli::tout << "camera not create\n";
        return false;
    }
    // Shutter Half and Full Release in AF mode
    cli::tout << "Shutter Half and Full Release in AF mode\n";
    camera->setCompeletedCallback(cb);
    camera->af_shutter();
    return true;
}

bool SonyCamera::capture() {
    if (camera == nullptr) {
        cli::tout << "camera not create\n";
        return false;
    }
    camera->s1_shooting();
    return true;
}

std::string SonyCamera::get_save_path() {
    if (camera == nullptr) {
        cli::tout << "camera not create\n";
        return "";
    }
    return camera->get_save_info();
}

void SonyCamera::power_off() {
    if (camera == nullptr) {
        cli::tout << "camera not create\n";
        return;
    }
    camera->power_off();
}

void SonyCamera::power_on() {
    // 对USB口重新插拔可以开机
    // 1. 系统层面控制USB插拔或者断电都没有效果
    // 2. 只能使用USB可控继电器的方式物理控制USB断电重连
    if (camera == nullptr) {
        cli::tout << "camera not create\n";
        return;
    }
    camera->power_on();

    // 关机后USB无法打开
    // SonyCameraShutterWake camera;
    // std::cout << "=== Sony Camera Shutter Wake Example ===" << std::endl;
    // // 1. 连接相机（即使关机状态也能连接USB）
    // std::cout << "\n1. Connecting to camera via USB..." << std::endl;
    // if (!camera.connect()) {
    //     std::cerr << "Failed to connect to camera. Please check USB connection." << std::endl;
    //     return;
    // }
    // std::cout << "Connected successfully" << std::endl;
}

void SonyCamera::live_view() {
    if (camera == nullptr) {
        cli::tout << "camera not create\n";
        return;
    }
    std::cout << "live view" << std::endl;
    liveThread = std::thread([&]() {
        while (isLiveRunning) {
            camera->change_live_view_enable();     
            camera->get_live_view_only();        
        }
        std::cout << "live view exited" << std::endl;
    });
}

bool SonyCamera::enable_live_view(bool enable, bool isLocal, std::string& rtmpUrl) {
     if (camera == nullptr) {
        cli::tout << "camera not create\n";
        return false;
    }
    isLiveRunning = enable;
    if (!enable) {
        if (liveThread.joinable()) { liveThread.join(); }
    }
    return camera->enable_live_view(enable, isLocal, rtmpUrl);
}

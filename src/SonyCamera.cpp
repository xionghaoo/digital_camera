#include "SonyCamera.h"

int SonyCamera::initialize()
{
    cli::tout << "Initialize Remote SDK...\n";
    auto init_success = SDK::Init();
    if (!init_success) {
        cli::tout << "Failed to initialize Remote SDK. Terminating.\n";
        SDK::Release();
        return -1;
    }
    cli::tout << "Remote SDK successfully initialized.\n\n";
    isInitialized = true;
    return 0;
}

int SonyCamera::release() {
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
    SDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
    auto enum_status = SDK::EnumCameraObjects(&camera_list);
    if (CR_FAILED(enum_status) || camera_list == nullptr) {
        cli::tout << "No cameras detected. Connect a camera and retry.\n";
        SDK::Release();
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

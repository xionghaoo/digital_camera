#include "SonyCamera.h"

SonyCamera::SonyCamera() {
    // Initialize the camera
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
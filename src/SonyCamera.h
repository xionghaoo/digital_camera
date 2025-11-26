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
#include <vector>
#include <cstdint>
#include <iomanip>
#include "CRSDK/CameraRemote_SDK.h"
#include "CameraDevice.h"
#include "Text.h"
#include "json.hpp"
using json = nlohmann::json;

namespace SDK = SCRSDK;
typedef std::shared_ptr<cli::CameraDevice> CameraDevicePtr;
typedef std::vector<CameraDevicePtr> CameraDeviceList;

class SonyCamera {
    private:
        int initialize();
        int release();
    protected:
        CameraDeviceList cameraList;
        CameraDevicePtr camera;
        bool isInitialized = false;
    public:
        SDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
        SonyCamera();
        ~SonyCamera();
        std::string version();
        std::string scan();
        bool connect(int index);
        bool af_shutter();
        bool capture();
        std::string get_save_path();
        void power_off();
        void power_on();
};
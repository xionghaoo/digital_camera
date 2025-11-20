#include <string>
#include <cstdint>
#include <iomanip>
#include "CRSDK/CameraRemote_SDK.h"
#include "CameraDevice.h"
#include "Text.h"
#include "json.hpp"
using json = nlohmann::json;

namespace SDK = SCRSDK;

class SonyCamera {
    public:
        SonyCamera();
        ~SonyCamera();
        std::string version();
};
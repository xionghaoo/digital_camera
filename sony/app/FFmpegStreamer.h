#include <cstdio>
#include <cstdint>
#include <string>
#include <iostream>
#include <sys/stat.h>

class FFmpegStreamer {
    FILE* ffmpegPipe = nullptr;

public:
    // HLS推流
    bool startHlsStream(const std::string& outputDir, int framerate = 25) {
        // 创建输出目录
        mkdir(outputDir.c_str(), 0755);

        std::string cmd = "ffmpeg -y -f image2pipe -framerate " + std::to_string(framerate) + " -i - "
                          "-c:v libx264 -preset ultrafast -tune zerolatency "
                          "-g " + std::to_string(framerate * 2) + " "  // GOP大小
                          "-f hls "
                          "-hls_time 2 "              // 每个切片2秒
                          "-hls_list_size 5 "         // 保留5个切片
                          "-hls_flags delete_segments "  // 自动删除旧切片
                          + outputDir + "/stream.m3u8 2>/dev/null";
        std::cout << "cmd: " << cmd << std::endl;
        ffmpegPipe = popen(cmd.c_str(), "w");
        return ffmpegPipe != nullptr;
    }

    // 推送帧数据
    void pushFrame(const char* data, size_t size) {
        if (ffmpegPipe) {
            std::cout << "push frame" << std::endl;
            fwrite(data, 1, size, ffmpegPipe);
            fflush(ffmpegPipe);
        }
    }

    bool isRunning() const {
        return ffmpegPipe != nullptr;
    }

    void stop() {
        if (ffmpegPipe) {
            pclose(ffmpegPipe);
            ffmpegPipe = nullptr;
        }
    }

    ~FFmpegStreamer() {
        stop();
    }
};
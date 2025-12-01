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
                          + outputDir + "/stream.m3u8 2>&1";
        std::cout << "cmd: " << cmd << std::endl;
        ffmpegPipe = popen(cmd.c_str(), "w");
        return ffmpegPipe != nullptr;
    }

    // RTSP推流
    bool startRtspStream(const std::string& rtspUrl, int framerate = 25, const std::string& transport = "tcp") {
        std::string cmd = "ffmpeg -y -f image2pipe -framerate " + std::to_string(framerate) + " -i - "
                          "-c:v libx264 -preset ultrafast -tune zerolatency "
                          "-g " + std::to_string(framerate) + " "  // GOP大小，RTSP用小GOP降低延迟
                          "-pix_fmt yuv420p "         // 像素格式
                          "-f rtsp "
                          "-rtsp_transport " + transport + " "  // TCP或UDP传输
                          "-rtsp_flags prefer_tcp "   // 优先使用TCP
                          + rtspUrl + " 2>&1";
        std::cout << "cmd: " << cmd << std::endl;
        ffmpegPipe = popen(cmd.c_str(), "w");
        return ffmpegPipe != nullptr;
    }

    // RTMP推流
    bool startRtmpStream(const std::string& rtmpUrl, int framerate = 25, int bitrate = 2000) {
        std::string cmd = "ffmpeg -y -f image2pipe -vcodec mjpeg -framerate " + std::to_string(framerate) + " -i - "
                          "-c:v libx264 -preset ultrafast -tune zerolatency "
                          "-b:v " + std::to_string(bitrate) + "k "  // 码率
                          "-maxrate " + std::to_string(bitrate) + "k "
                          "-bufsize " + std::to_string(bitrate * 2) + "k "
                          "-g " + std::to_string(framerate * 2) + " "  // GOP大小
                          "-pix_fmt yuv420p "         // 像素格式
                          "-f flv "                   // RTMP使用flv格式
                          "-flvflags no_duration_filesize "  // RTMP优化
                          + rtmpUrl + " 2>&1";
        std::cout << "cmd: " << cmd << std::endl;
        ffmpegPipe = popen(cmd.c_str(), "w");
        if (ffmpegPipe) {
            std::cout << "RTMP stream started, pushing to: " << rtmpUrl << std::endl;
        } else {
            std::cerr << "Failed to start RTMP stream" << std::endl;
        }
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
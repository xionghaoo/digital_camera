#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>

class MjpegHttpServer {
    int serverFd = -1;
    std::vector<int> clients;
    std::mutex clientsMutex;
    bool running = false;
    std::thread acceptThread;

public:
    bool start(int port) {
        serverFd = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) return false;
        if (listen(serverFd, 5) < 0) return false;
        
        running = true;
        acceptThread = std::thread(&MjpegHttpServer::acceptLoop, this);
        return true;
    }

    void pushFrame(const char* jpegData, size_t size) {
        char header[256];
        int headerLen = snprintf(header, sizeof(header),
            "\r\n--frame\r\n"
            "Content-Type: image/jpeg\r\n"
            "Content-Length: %zu\r\n\r\n", size);
        
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto it = clients.begin(); it != clients.end();) {
            // 合并发送头和数据
            struct iovec iov[2];
            iov[0].iov_base = header;
            iov[0].iov_len = headerLen;
            iov[1].iov_base = (void*)jpegData;
            iov[1].iov_len = size;
            
            if (writev(*it, iov, 2) <= 0) {
                close(*it);
                it = clients.erase(it);
            } else {
                ++it;
            }
        }
    }

    void stop() {
        running = false;
        if (serverFd >= 0) { shutdown(serverFd, SHUT_RDWR); close(serverFd); }
        if (acceptThread.joinable()) acceptThread.join();
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (int fd : clients) close(fd);
        clients.clear();
    }

    ~MjpegHttpServer() { stop(); }

private:
    void acceptLoop() {
        while (running) {
            int clientFd = accept(serverFd, nullptr, nullptr);
            if (clientFd < 0) continue;
            
            // 禁用 Nagle
            int flag = 1;
            setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
            
            // 读取并丢弃HTTP请求
            char buf[1024];
            recv(clientFd, buf, sizeof(buf), 0);
            
            // 发送HTTP响应头
            const char* response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                "Pragma: no-cache\r\n"
                "Connection: close\r\n\r\n";
            send(clientFd, response, strlen(response), 0);
            
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientFd);
        }
    }
};
#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#include <unistd.h>
#include <iostream>

class MjpegHttpServer {
    int serverFd = -1;
    std::vector<int> clients;
    std::mutex clientsMutex;
    std::atomic<bool> running{false};
    std::thread serverThread;

public:
    bool start(int port) {
        serverFd = socket(AF_INET, SOCK_STREAM, 0);
        if (serverFd < 0) return false;
        
        int opt = 1;
        setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) return false;
        if (listen(serverFd, 5) < 0) return false;
        
        running = true;
        serverThread = std::thread(&MjpegHttpServer::acceptLoop, this);
        std::cout << "mjpeag start" << std::endl;
        return true;  // 立即返回，不阻塞主线程
    }

    void pushFrame(const char* jpegData, size_t size) {
        std::string header = "--frame\r\n"
                             "Content-Type: image/jpeg\r\n"
                             "Content-Length: " + std::to_string(size) + "\r\n\r\n";
        
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto it = clients.begin(); it != clients.end();) {
            struct iovec iov[3];
            iov[0].iov_base = (void*)header.data();
            iov[0].iov_len = header.size();
            iov[1].iov_base = (void*)jpegData;
            iov[1].iov_len = size;
            iov[2].iov_base = (void*)"\r\n";
            iov[2].iov_len = 2;
            
            // 使用 writev 一次性发送头部、数据和尾部，减少分包风险
            if (writev(*it, iov, 3) <= 0) {
                close(*it);
                it = clients.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool isRunning() const { return running; }

    void stop() {
        running = false;
        // 先关闭所有客户端，避免pushFrame阻塞
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (int fd : clients) {
                shutdown(fd, SHUT_RDWR);
                close(fd);
            }
            clients.clear();
        }
        if (serverFd >= 0) { 
            shutdown(serverFd, SHUT_RDWR); 
            close(serverFd); 
            serverFd = -1;
        }
        if (serverThread.joinable()) serverThread.join();
        std::cout << "mjpeag stop" << std::endl;
    }

    ~MjpegHttpServer() { stop(); }

private:
    void acceptLoop() {
        while (running) {
            int fd = serverFd;
            if (fd < 0) break;
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);
            timeval timeout{0, 100000};  // 100ms超时，更快响应stop
            if (select(fd + 1, &readfds, nullptr, nullptr, &timeout) <= 0) continue;
            
            int clientFd = accept(serverFd, nullptr, nullptr);
            if (clientFd < 0) continue;
            
            int flag = 1;
            setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
            timeval recvTimeout{2, 0};  // 2秒接收超时
            setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
            timeval sendTimeout{2, 0};  // 2秒发送超时
            setsockopt(clientFd, SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, sizeof(sendTimeout));
            char buf[1024];
            if (recv(clientFd, buf, sizeof(buf), 0) <= 0) {
                close(clientFd);
                continue;
            }
            
            const char* response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"
                "Pragma: no-cache\r\n"
                "Connection: keep-alive\r\n"
                "Access-Control-Allow-Origin: *\r\n\r\n";
            send(clientFd, response, strlen(response), 0);
            
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientFd);
        }
        std::cout << "acceptLoop exit" << std::endl;
    }
};
#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <map>

/**
 * 获取本机所有IPv4地址
 * @return map<网卡名, IP地址>
 */
std::map<std::string, std::string> getLocalIPs() {
    std::map<std::string, std::string> ipMap;
    struct ifaddrs * ifAddrStruct = nullptr;
    struct ifaddrs * ifa = nullptr;
    void * tmpAddrPtr = nullptr;

    // 获取接口列表
    if (getifaddrs(&ifAddrStruct) != 0) {
        return ipMap; // 返回空或是抛出异常
    }

    // 遍历链表
    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        // 只检查 IPv4 (AF_INET)
        // 如果需要 IPv6，请添加或修改为 AF_INET6
        if (ifa->ifa_addr->sa_family == AF_INET) { 
            // 这是一个 IPv4 地址
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            
            std::string interfaceName = ifa->ifa_name;
            std::string ipAddress = addressBuffer;

            // 过滤掉回环地址 (127.0.0.1)
            if (interfaceName != "lo" && ipAddress != "127.0.0.1") {
                ipMap[interfaceName] = ipAddress;
            }
        }
    }
    
    // 释放内存
    if (ifAddrStruct != nullptr) {
        freeifaddrs(ifAddrStruct);
    }
    
    return ipMap;
}

/**
 * 获取本机所有IPv4地址
 * @return map<网卡名, IP地址>
 */
std::string getEth0IP() {
    struct ifaddrs * ifAddrStruct = nullptr;
    struct ifaddrs * ifa = nullptr;
    void * tmpAddrPtr = nullptr;

    // 获取接口列表
    if (getifaddrs(&ifAddrStruct) != 0) {
        return ""; // 返回空或是抛出异常
    }
    std::string retIP;
    // 遍历链表
    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        // 只检查 IPv4 (AF_INET)
        // 如果需要 IPv6，请添加或修改为 AF_INET6
        if (ifa->ifa_addr->sa_family == AF_INET) { 
            // 这是一个 IPv4 地址
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            
            std::string interfaceName = ifa->ifa_name;
            std::string ipAddress = addressBuffer;

            if (interfaceName == "enP8p1s0") {
                retIP = ipAddress;
            }
        }
    }
    
    // 释放内存
    if (ifAddrStruct != nullptr) {
        freeifaddrs(ifAddrStruct);
    }
    
    return retIP;
}
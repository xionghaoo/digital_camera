# 数码相机SDK

## 项目编译

第三方模块拉取
```shell
git submodule init
git submodule update
```

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

drogon依赖安装
```shell
sudo apt update
sudo apt install -y git cmake g++ build-essential pkg-config libssl-dev zlib1g-dev uuid-dev libjsoncpp-dev
```

```shell
sudo sh -c 'echo 150 > /sys/module/usbcore/parameters/usbfs_memory_mb'
```
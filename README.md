# luck_pico_rtsp_yolov5
    测试 luckfox-pico pro (128MB RAM) 使用 rknn 推理 retinaface 并进行 rtsp 推流。

# 开发环境
+ luckfox-pico sdk

# 编译
```
export LUCKFOX_SDK_PATH=<Your Luckfox-pico Sdk Path>
mkdir build
cd build
cmake ..
make && make install
```

# 运行
将编译生成的`luckfox_rtsp_yolov5_demo`上传到 luckfox-pico 上，进入文件夹运行
```
./luckfox_rtsp_yolov5
```
使用 vlc 打开网络串流 rtsp://192.168.10.4/live/0（按实际情况修改IP地址拉取图像）

**注意**：运行前请关闭系统默认 rkipc 程序，执行 `RkLunch-stop.sh` 关闭。
由于 yolov5 模型较大需要消耗更多的内存资源加载，在 luckfox-pico mini/plus （rv1103）上无法运行。




export LUCKFOX_SDK_PATH=/home/zehn/luckfox_sdk/luckfox-pico
mkdir build
cd build
cmake ..
make && make install 



cmake -DBUILD_ARAVIS=OFF -DBUILD_GST_1_0=ON -DBUILD_TOOLS=ON -DBUILD_V4L2=ON -DCMAKE_INSTALL_PREFIX=/usr ..

sudo apt-get install repo git ssh make gcc gcc-multilib g++-multilib module-assistant expect g++ gawk texinfo libssl-dev bison flex fakeroot cmake unzip gperf autoconf device-tree-compiler libncurses5-dev pkg-config

sudo apt-get install libc6-armhf-cross


export OPENSSL_ROOT_DIR=../../openssl-1.1.1v/install
export OPENSSL_LIBRARIES=../../openssl-1.1.1v/install/lib
 
 
cmake ../ -DPAHO_WITH_SSL=TRUE -DCMAKE_INSTALL_PREFIX=./mqttInstall -DCMAKE_C_COMPILER=/home/zehn/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc 


 
 
 
export RV1106_SDK_PATH=/home/zehn/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin
export PATH=$PATH:$ RV1106_SDK_PATH

 ./config no-asm shared no-async --prefix=$(pwd)/install --cross-compile-prefix=/home/zehn/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-
 
 
 nohup sh RkLunch-stop.sh &>/devnull &
 
 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
 
 fakeroot 问题：
 sudo update-alternatives --set fakeroot /usr/bin/fakeroot-tcp
 
问题：
E RKNN: failed to decode config data!
Segmentation fault (core dumped)：
解决：
lib(librknnmrt.so、librga.so) 放入程序当前目录
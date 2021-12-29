# 说明

本工程为使用[esp-idf](https://github.com/espressif/esp-idf) 进行编程的Demo。整理一些基础功能以快速开发新应用，主要目标soc为ESP32。

## 功能

- 常用网络功能的Kconfig配置(WIFI、以太网)。
- spiffs文件系统(文件读写及编译时生成镜像)。
- tftp服务器,可采用tftp工具在线修改spiffs文件系统的内容。

# 编译

本工程中没有包含SDK源代码文件，因此首先应该正确安装并使用[esp-idf](https://github.com/espressif/esp-idf) 。本工程中的编译方式同SDK中example中的工程相同（安装完成后可尝试编译SDK中的example中的工程，成功后再使用cd命令切换到本工程目录编译）。本工程暂时只支持在linux或wsl(wsl2)下编译。

环境安装正确后可采用以下方式编译（由SDK提供）：

- 使用CMAKE生成工程文件，再根据工程类型（CMake支持的工程类型，下面均为个人习惯的类型）进行编译：
  - 默认状态下生成可使用make进行编译的工程。
  - 生成codeblocks工程文件，再使用codeblocks编译（编译之前需确定环境变量设置正确）
- ESP IDF支持的方式（idf.py）。

根据平台不同，CMake还可支持nijia/Eclipse CDT等类型。

## ESP-IDF版本

- 已测试分支：master (20211229)

## 注意

为了精简Kconfig大小，分离了Kconfig文件。因此，除了在需要在esp-idf下执行以下指令，在本工程目录下也需要执行以下指令(若esp-idf安装在默认目录~/esp/esp-idf，只需要在本工程目录下执行):

```bash
source export.sh
```




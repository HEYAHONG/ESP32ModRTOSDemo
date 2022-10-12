# 说明

本工程为使用[esp-idf](https://github.com/espressif/esp-idf) 进行编程的Demo。整理一些基础功能以快速开发新应用，主要目标soc为ESP32。

## 功能

- 常用网络功能的Kconfig配置(WIFI、以太网)。
- spiffs文件系统(文件读写及编译时生成镜像)。
- tftp服务器,可采用tftp工具在线修改spiffs文件系统的内容。

## 源代码下载

由于本源代码包含第三方源代码,故直接下载可能有部分源代码缺失，需要通过以下方法解决:

- 在进行git clone 使用--recurse-submodules参数。

- 若已通过git clone下载,则在源代码目录中执行以下命令下载子模块:

  ```bash
   git submodule update --init --recursive
  ```

# 编译

## 编译环境安装

注意:仅支持github.com下载的代码，其他方式下载的代码需要完全按照官方的说明安装。

### Linux / WSL

执行工程目录下的 bootstrap.sh，当提示 初始化完成时 即表示编译环境安装完成。

### 其它

进入 [esp-idf](esp-idf/) 目录,安装官方的说明安装。

## 编译说明

环境安装正确后可采用以下方式编译（由SDK提供）：

- 使用CMAKE生成工程文件，再根据工程类型（CMake支持的工程类型，下面均为个人习惯的类型）进行编译：
  - 默认状态下生成可使用make进行编译的工程。
  - 生成codeblocks工程文件，再使用codeblocks编译（编译之前需确定环境变量设置正确）
- ESP IDF支持的方式（idf.py）。

根据平台不同，CMake还可支持nijia/Eclipse CDT等类型。

## ESP-IDF版本

若使用bootstrap.sh安装编译环境,无需注意此提示。

- 已测试分支：master(git version:fa5d0a351343b45ee06d221b7c29028672a4c3c2 )

## 注意

若使用bootstrap.sh安装编译环境,无需注意此提示。

为了精简Kconfig大小，分离了Kconfig文件。因此，除了在需要在esp-idf下执行以下指令，在本工程目录下也需要执行以下指令(若esp-idf安装在默认目录~/esp/esp-idf，只需要在本工程目录下执行):

```bash
source export.sh
```




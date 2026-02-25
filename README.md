# RK3568-RTSPMonitor

基于 RK3568 开发板的双码流 RTSP 视频监控系统，实现摄像头采集、MPP 硬件编码、live555 RTSP 推流，以及 PC 端 QT 客户端拉流显示、截图、录像。

## 系统架构

```
板子端 (RK3568 Buildroot Linux)                    PC端 (Windows QT 6)
┌──────────────────────────────────┐             ┌─────────────────────────────┐
│                                  │             │                             │
│  MIPI 摄像头 (IMX335 5MP)       │             │  FFmpeg RTSP Client         │
│         │                        │             │  + NVIDIA cuvid 硬件解码    │
│         ▼                        │             │         │                   │
│  ISP mainpath ─► V4L2 (1080p)   │   RTSP/RTP  │         ▼                   │
│         │                        │ ──────────► │  QT 单画面显示              │
│         ▼                        │             │  + 截图 (PNG)               │
│  MPP H.265 编码 ─► live555      │             │  + 录像 (MP4 remux)         │
│                   RTSP Server    │             │                             │
│  ISP selfpath ─► V4L2 (360p)    │   :8554     │  码流切换：                  │
│         │                        │ ──────────► │  主码流 H.265 1920x1080     │
│         ▼                        │             │  子码流 H.264 640x360       │
│  MPP H.264 编码 ─► live555      │             │                             │
│                                  │             │                             │
└──────────────────────────────────┘             └─────────────────────────────┘
```

## 功能特性

**板子端（推流服务）**
- V4L2 MPLANE + DMABUF 零拷贝采集
- RK MPP 硬件编码，双码流同时输出（H.265 1080p + H.264 360p）
- live555 RTSP Server，支持多客户端同时拉流

**PC 端（QT 客户端）**
- FFmpeg RTSP 拉流 + NVIDIA cuvid 硬件解码
- 主/子码流一键切换
- 截图保存 PNG（异步写入）
- 录像保存 MP4（Remux 直接封装，零编码开销）

## 技术栈

| 层级 | 板子端 (TLServer) | PC端 (LLClient) |
|------|-------------------|-----------------|
| 采集 | V4L2 MPLANE + DMABUF 零拷贝 | - |
| 编码/解码 | RK MPP 硬件编码 (H.265 / H.264) | FFmpeg + NVIDIA cuvid 硬件解码 |
| 传输 | live555 RTSP Server | FFmpeg avformat RTSP Client |
| 显示 | - | QT 6 QLabel + QPixmap |
| 录像 | - | FFmpeg remux 直接封装 MP4 |

## 双码流参数

| 参数 | 主码流 | 子码流 |
|------|--------|--------|
| 编码格式 | H.265 (HEVC) | H.264 (AVC) |
| 分辨率 | 1920x1080 | 640x360 |
| 帧率 | 30fps | 10fps |
| 码率 | 3Mbps CBR | 800Kbps CBR |
| GOP | 10 | 10 |
| RTSP URL | `rtsp://<ip>:8554/h265` | `rtsp://<ip>:8554/h264` |

## 项目结构

```
RK3568-RTSPMonitor/
├── server/TLServer/          # 板子端推流服务
│   ├── TLmain.cpp            # 主入口：RTSP Server + 编码线程启动
│   ├── v4l2.h / v4l2.cpp     # V4L2 采集模块（MPLANE + DMABUF）
│   ├── mpp.h / mpp.cpp       # MPP 硬件编码模块（H.265/H.264）
│   ├── EncodeThread.h / .cpp # 编码线程（V4L2 采集 → MPP 编码 → SharedQueue）
│   ├── SharedQueue.h / .cpp  # 线程安全帧队列 + EventTriggerId 传递
│   ├── LiveSource.h / .cpp   # live555 自定义 FramedSource（分片交付）
│   ├── H265LiveServerMediaSubsession.h / .cpp
│   ├── H264LiveServerMediaSubsession.h / .cpp
│   ├── const.h / const.cpp   # 全局参数（分辨率/码率/帧率/端口）
│   └── ...
│
└── client/LLClient/          # PC端 QT 客户端
    ├── main.cpp              # QT 应用入口
    ├── mainwindow.h / .cpp   # 主窗口（页面切换/截图/录像控制）
    ├── mainwindow.ui         # UI 布局（QStackedWidget 双页）
    ├── VideoDecoder.h / .cpp # 拉流解码线程（FFmpeg + cuvid + 录像）
    ├── const.h               # RTSP URL 和分辨率常量
    ├── style.qss             # 暗色监控主题样式
    ├── resources.qrc         # QT 资源文件
    └── LLClient.pro          # QT 项目文件
```

## 硬件环境

| 组件 | 型号 |
|------|------|
| 开发板 | RK3568 (正点原子) 2G+32G, Buildroot Linux |
| 摄像头 | ATK-MCIMX335, 500W 像素, MIPI 接口 |
| PC | Windows, NVIDIA GPU (cuvid 硬解) |

## 编译与运行

### 板子端

依赖：交叉编译工具链 (aarch64-buildroot-linux-gnu-)、MPP 库 (librockchip_mpp.so)、live555 静态库

```bash
# 交叉编译（需根据实际环境修改路径）
aarch64-buildroot-linux-gnu-g++ -o TLServer \
    TLmain.cpp EncodeThread.cpp v4l2.cpp mpp.cpp \
    SharedQueue.cpp LiveSource.cpp const.cpp \
    H265LiveServerMediaSubsession.cpp H264LiveServerMediaSubsession.cpp \
    -I<mpp_include_path> -I<live555_include_paths> \
    -L<live555_lib_path> -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment \
    -lrockchip_mpp -lpthread

# 板端运行
./TLServer
```

### PC 端

依赖：QT 6、FFmpeg (avformat/avcodec/swscale/avutil)、NVIDIA GPU 驱动 (cuvid)

使用 Qt Creator 打开 `client/LLClient/LLClient.pro`，配置 FFmpeg 库路径后编译运行。

#ifndef CONST_H
#define CONST_H

#include<iostream>
#include<stdio.h>
#include<cstring>
#include<fcntl.h>
#include<unistd.h>
#include<vector>
#include<deque>
#include<cstdint>
#include<thread>
#include<mutex>
#include<atomic>
#include<csignal>
#include<sys/ioctl.h>
#include<sys/mman.h>
#include<sys/time.h>
#include<linux/videodev2.h>
#include<rockchip/rk_mpi.h>
#include<rockchip/mpp_buffer.h>
#include<rockchip/mpp_frame.h>
#include<rockchip/mpp_packet.h>
#include<FramedSource.hh>
#include<UsageEnvironment.hh>
#include<OnDemandServerMediaSubsession.hh>
#include<H265VideoStreamFramer.hh>
#include<H265VideoRTPSink.hh>
#include<H264VideoStreamFramer.hh>
#include<H264VideoRTPSink.hh>
#include<BasicUsageEnvironment.hh>
#include<RTSPServer.hh>
#include<ServerMediaSession.hh>


//主码流参数配置 H265 1920*1080 ----------

//视频参数
constexpr int VIDEO_WIDTH = 1920;
constexpr int VIDEO_HEIGHT = 1080;
constexpr int VIDEO_HOR_STRIDE = 1920;
constexpr int VIDEO_VER_STRIDE = 1088;
constexpr int VIDEO_FRAME_SIZE = VIDEO_HOR_STRIDE * VIDEO_VER_STRIDE * 3 / 2;

//编码参数
constexpr int ENC_BITRATE = 3000000;
constexpr int ENC_BITRATE_MAX = 3400000;
constexpr int ENC_BITRATE_MIN = 2600000;
constexpr int ENC_FPS = 30;
constexpr int ENC_GOP = 10;

//Buffer参数
constexpr int BUFFER_COUNT = 6;

//RTSP参数
constexpr int RTSP_PORT = 8554;
constexpr int RTP_OUT_BUF_MAX = 200000;

//------------------------------

//子码流参数配置 H264 640*360 --------------

//视频参数
constexpr int SUB_VIDEO_WIDTH = 640;
constexpr int SUB_VIDEO_HEIGHT = 360;
constexpr int SUB_VIDEO_HOR_STRIDE = 640;
constexpr int SUB_VIDEO_VER_STRIDE = 368;
constexpr int SUB_VIDEO_FRAME_SIZE = SUB_VIDEO_HOR_STRIDE * SUB_VIDEO_VER_STRIDE * 3 / 2;

//编码参数
constexpr int SUB_ENC_BITRATE = 800000;
constexpr int SUB_ENC_BITRATE_MAX = 1000000;
constexpr int SUB_ENC_BITRATE_MIN = 600000;
constexpr int SUB_ENC_FPS = 10;
constexpr int SUB_ENC_GOP = 10;

// Buffer参数
constexpr int SUB_BUFFER_COUNT = 4;

// RTSP参数
constexpr int SUB_RTSP_PORT = 8554;
constexpr int SUB_RTP_OUT_BUF_MAX = 200000;

//--------------------------


struct H265EncodePacket
{
    std::vector<uint8_t>data;
    timeval timestamp{};
};

extern EventLoopWatchVariable watchVariable;
extern std::atomic<bool> exit_flag;

#endif


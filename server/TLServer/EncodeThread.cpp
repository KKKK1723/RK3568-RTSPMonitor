#include "EncodeThread.h"
#include "v4l2.h"
#include "mpp.h"


void start_encode_h265(SharedQueue *shared_queue, TaskScheduler *scheduler)
{
    V4l2class v4l2;
    Mppclass mpp;

    if (!v4l2.InitV4l2("/dev/video0", VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_FRAME_SIZE, BUFFER_COUNT))
    {
        std::cout << "主码流 V4L2 初始化失败" << std::endl;
        return;
    }

    if (!mpp.InitMpp(MPP_VIDEO_CodingHEVC,VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_HOR_STRIDE, VIDEO_VER_STRIDE, VIDEO_FRAME_SIZE,ENC_BITRATE, ENC_BITRATE_MAX, ENC_BITRATE_MIN,ENC_FPS, ENC_GOP, BUFFER_COUNT,shared_queue))
    {
        std::cout << "主码流 MPP 初始化失败" << std::endl;
        return;
    }

    for (int i = 0; i < BUFFER_COUNT; i++)
    {
        v4l2.Qbuf(mpp.GetMppBufferFd(i), i);
    }
    v4l2.StreamOn();

    std::cout << "主码流 H.265 编码线程启动" << std::endl;

    while (1)
    {
        if (exit_flag)
            return;

        int index = v4l2.Dqbuf();
        if (index == -1) return;

        MppFrame frame = NULL;
        mpp_frame_init(&frame);
        mpp_frame_set_width(frame, VIDEO_WIDTH);
        mpp_frame_set_height(frame, VIDEO_HEIGHT);
        mpp_frame_set_hor_stride(frame, VIDEO_HOR_STRIDE);
        mpp_frame_set_ver_stride(frame, VIDEO_VER_STRIDE);
        mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP);
        mpp_frame_set_buffer(frame, mpp.GetMppBuffer(index));
        mpp_frame_set_eos(frame, 0);

        if (mpp.mpi->encode_put_frame(mpp.ctx, frame) != MPP_SUCCESS)
        {
            std::cout << "主码流编码失败" << std::endl;
            mpp_frame_deinit(&frame);
            return;
        }

        MppPacket packet = NULL;
        if (mpp.mpi->encode_get_packet(mpp.ctx, &packet) != MPP_SUCCESS)
        {
            std::cout << "主码流获取packet失败" << std::endl;
            mpp_frame_deinit(&frame);
            return;
        }

        H265EncodePacket enc_pkt;
        void *p = mpp_packet_get_data(packet);
        size_t len = mpp_packet_get_length(packet);
        enc_pkt.data.assign(static_cast<uint8_t*>(p), static_cast<uint8_t*>(p) + len);
        gettimeofday(&enc_pkt.timestamp, NULL);

        shared_queue->push_packet(enc_pkt);
        if(shared_queue->GetMppEventID()!=0 && shared_queue->GetSource() != NULL)
        {
            scheduler->triggerEvent(shared_queue->GetMppEventID(), shared_queue->GetSource());
        }

        v4l2.Qbuf(mpp.GetMppBufferFd(index), index);
        mpp_frame_deinit(&frame);
        mpp_packet_deinit(&packet);
    }
}

void start_encode_h264(SharedQueue *shared_queue, TaskScheduler *scheduler)
{
    V4l2class v4l2;
    Mppclass mpp;

    if (!v4l2.InitV4l2("/dev/video1", SUB_VIDEO_WIDTH, SUB_VIDEO_HEIGHT, SUB_VIDEO_FRAME_SIZE, SUB_BUFFER_COUNT))
    {
        std::cout << "子码流 V4L2 初始化失败" << std::endl;
        return;
    }

    if (!mpp.InitMpp(MPP_VIDEO_CodingAVC, SUB_VIDEO_WIDTH, SUB_VIDEO_HEIGHT, SUB_VIDEO_HOR_STRIDE, SUB_VIDEO_VER_STRIDE, SUB_VIDEO_FRAME_SIZE,SUB_ENC_BITRATE, SUB_ENC_BITRATE_MAX, SUB_ENC_BITRATE_MIN,SUB_ENC_FPS, SUB_ENC_GOP, SUB_BUFFER_COUNT,shared_queue))
    {
        std::cout << "子码流 MPP 初始化失败" << std::endl;
        return;
    }

    for (int i = 0; i < SUB_BUFFER_COUNT; i++)
    {
        v4l2.Qbuf(mpp.GetMppBufferFd(i), i);
    }
    v4l2.StreamOn();

    std::cout << "子码流 H.264 编码线程启动" << std::endl;

    while (1)
    {
        if (exit_flag)
            return;

        int index = v4l2.Dqbuf();
        if (index == -1) return;

        MppFrame frame = NULL;
        mpp_frame_init(&frame);
        mpp_frame_set_width(frame, SUB_VIDEO_WIDTH);
        mpp_frame_set_height(frame, SUB_VIDEO_HEIGHT);
        mpp_frame_set_hor_stride(frame, SUB_VIDEO_HOR_STRIDE);
        mpp_frame_set_ver_stride(frame, SUB_VIDEO_VER_STRIDE);
        mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP);
        mpp_frame_set_buffer(frame, mpp.GetMppBuffer(index));
        mpp_frame_set_eos(frame, 0);

        if (mpp.mpi->encode_put_frame(mpp.ctx, frame) != MPP_SUCCESS)
        {
            std::cout << "子码流编码失败" << std::endl;
            mpp_frame_deinit(&frame);
            return;
        }

        MppPacket packet = NULL;
        if (mpp.mpi->encode_get_packet(mpp.ctx, &packet) != MPP_SUCCESS)
        {
            std::cout << "子码流获取packet失败" << std::endl;
            mpp_frame_deinit(&frame);
            return;
        }

        H265EncodePacket enc_pkt;
        void *p = mpp_packet_get_data(packet);
        size_t len = mpp_packet_get_length(packet);
        enc_pkt.data.assign(static_cast<uint8_t*>(p), static_cast<uint8_t*>(p) + len);
        gettimeofday(&enc_pkt.timestamp, NULL);

        shared_queue->push_packet(enc_pkt);
        if(shared_queue->GetMppEventID()!=0 && shared_queue->GetSource() != NULL)
        {
            scheduler->triggerEvent(shared_queue->GetMppEventID(), shared_queue->GetSource());
        }

        v4l2.Qbuf(mpp.GetMppBufferFd(index), index);
        mpp_frame_deinit(&frame);
        mpp_packet_deinit(&packet);
    }
}

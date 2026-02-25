#ifndef MPP_H
#define MPP_H

#include "const.h"
#include "SharedQueue.h"
class Mppclass
{
    public:
        Mppclass();
        ~Mppclass();
        int InitMpp(MppCodingType codec_type, int width, int height,int hor_stride, int ver_stride, int frame_size,int bitrate, int bitrate_max, int bitrate_min,int fps, int gop, int buffer_count,SharedQueue* shared_queue);
        int GetMppBufferFd(int index);
        MppBuffer GetMppBuffer(int index);
        void Close();

        MppApi *mpi;
        MppCtx ctx;
        MppPacket _mpp_hdr_packet;

    private:
        int MppCheck(MPP_RET type, std::string error);

        MPP_RET mpp_ret;
        MppEncCfg cfg;
        MppBufferGroup _mpp_buffer_group;
        MppBuffer _mpp_buffer[6];
        int _mpp_buffer_fd[6];
        int _buffer_count;
};

#endif

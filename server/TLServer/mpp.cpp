#include "mpp.h"

Mppclass::Mppclass()
{
    mpp_ret = MPP_SUCCESS;
    mpi = NULL;
    ctx = NULL;
    cfg = NULL;
    _mpp_buffer_group = NULL;
    _mpp_hdr_packet = NULL;
    _buffer_count = 0;
}

Mppclass::~Mppclass()
{
    Close();
}

int Mppclass::MppCheck(MPP_RET type, std::string error)
{
    if (type != MPP_SUCCESS)
    {
        std::cout << error << " 失败" << std::endl;
        return 0;
    }
    else
    {
        std::cout << error << " 成功" << std::endl;
        return 1;
    }
}

int Mppclass::InitMpp(MppCodingType codec_type, int width, int height,int hor_stride, int ver_stride, int frame_size,int bitrate, int bitrate_max, int bitrate_min,int fps, int gop, int buffer_count,SharedQueue *shared_queue)
{
    _buffer_count = buffer_count;
    mpp_ret = mpp_create(&ctx, &mpi);
    if (!MppCheck(mpp_ret, "mpp_create"))
        return 0;

    mpp_ret = mpp_init(ctx, MPP_CTX_ENC, codec_type);
    if (!MppCheck(mpp_ret, "mpp_init"))
        return 0;

    mpp_ret = mpp_enc_cfg_init(&cfg);
    if (!MppCheck(mpp_ret, "mpp_enc_cfg_init"))
        return 0;

    mpp_enc_cfg_set_s32(cfg, "prep:width", width);
    mpp_enc_cfg_set_s32(cfg, "prep:height", height);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", hor_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", ver_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT_YUV420SP);

    mpp_enc_cfg_set_s32(cfg, "rc:mode", MPP_ENC_RC_MODE_CBR);
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", bitrate);
    mpp_enc_cfg_set_s32(cfg, "rc:bps_max", bitrate_max);
    mpp_enc_cfg_set_s32(cfg, "rc:bps_min", bitrate_min);

    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", fps);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm", 1);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", fps);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", 1);

    mpp_enc_cfg_set_s32(cfg, "rc:gop", gop);

    mpp_enc_cfg_set_s32(cfg, "codec:type", codec_type);

    mpp_ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    if (!MppCheck(mpp_ret, "MPP_ENC_SET_CFG"))
        return 0;

    mpp_ret = mpp_buffer_group_get_internal(&_mpp_buffer_group, MPP_BUFFER_TYPE_DRM);
    if (!MppCheck(mpp_ret, "mpp_buffer_group_get_internal"))
        return 0;

    for (int i = 0; i < buffer_count; i++)
    {
        mpp_ret = mpp_buffer_get(_mpp_buffer_group, &_mpp_buffer[i], frame_size);
        if (!MppCheck(mpp_ret, "mpp_buffer_get"))
            return 0;

        //初始化mpp_buffer Y平面全黑
        void *ptr = mpp_buffer_get_ptr(_mpp_buffer[i]);
        size_t y_size = hor_stride * ver_stride;
        memset(ptr, 0x00, y_size);
        memset((uint8_t *)ptr + y_size, 0x80, y_size / 2);
    }

    for (int i = 0; i < buffer_count; i++)
    {
        _mpp_buffer_fd[i] = mpp_buffer_get_fd(_mpp_buffer[i]);
        if (_mpp_buffer_fd[i] >= 0)
        {
            std::cout << "mpp_buffer_get_fd 成功" << std::endl;
        }
        else
        {
            std::cout << "mpp_buffer_get_fd 失败" << std::endl;
            return 0;
        }
    }

    _mpp_hdr_packet = NULL;
    mpp_ret = mpi->control(ctx, MPP_ENC_GET_EXTRA_INFO, &_mpp_hdr_packet);
    if (mpp_ret == MPP_SUCCESS && _mpp_hdr_packet)
    {
        std::cout << "获取头信息成功" << std::endl;
    }
    else
    {
        std::cout << "获取头信息失败" << std::endl;
        return 0;
    }

    //解析头数据给SharedQueue再在创建RTPsink的时候传入
    void *p = mpp_packet_get_data(_mpp_hdr_packet);
    uint8_t *data = static_cast<uint8_t *>(p);
    size_t length = mpp_packet_get_length(_mpp_hdr_packet);
    std::vector<size_t> sc_pos;//存起始码索引下标
    for (size_t i = 0; i + 3 < length;i++)
    {
        if (data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x00 && data[i + 3] == 0x01)
        {
            sc_pos.push_back(i);
        }
    }

    //遍历每一个NAL
    for (size_t i = 0; i < sc_pos.size();i++)
    {
        uint8_t *nal_data = data + sc_pos[i] + 4;
        size_t nal_len;
        if( i+1 < sc_pos.size())//说明还有下一个nal
        {
            nal_len = sc_pos[i + 1] - (sc_pos[i] + 4);
        }
        else
        {
            nal_len = length - (sc_pos[i] + 4);
        }

        if (codec_type == MPP_VIDEO_CodingHEVC)
        {
            // H.265: NAL type = (byte>>1) & 0x3F, VPS=32 SPS=33 PPS=34
            uint8_t nal_type = (nal_data[0] >> 1) & 0x3F;
            if(nal_type==32)
                shared_queue->SetVPS(nal_data, nal_len);
            else if(nal_type==33)
                shared_queue->SetSPS(nal_data, nal_len);
            else if(nal_type==34)
                shared_queue->SetPPS(nal_data, nal_len);
        }
        else if (codec_type == MPP_VIDEO_CodingAVC)
        {
            // H.264: NAL type = byte & 0x1F, SPS=7 PPS=8, 无VPS
            uint8_t nal_type = nal_data[0] & 0x1F;
            if(nal_type==7)
                shared_queue->SetSPS(nal_data, nal_len);
            else if(nal_type==8)
                shared_queue->SetPPS(nal_data, nal_len);
        }
    }
    return 1;
}

int Mppclass::GetMppBufferFd(int index)
{
    return _mpp_buffer_fd[index];
}

MppBuffer Mppclass::GetMppBuffer(int index)
{
    return _mpp_buffer[index];
}

void Mppclass::Close()
{
    if (ctx == NULL)
        return;

    for (int i = 0; i < _buffer_count; i++)    {
        if (_mpp_buffer[i])
        {
            mpp_buffer_put(_mpp_buffer[i]);
            _mpp_buffer[i] = NULL;
        }
    }

    if (_mpp_buffer_group)
    {
        mpp_buffer_group_put(_mpp_buffer_group);
        _mpp_buffer_group = NULL;
    }

    if (cfg)
    {
        mpp_enc_cfg_deinit(cfg);
        cfg = NULL;
    }

    mpp_destroy(ctx);
    ctx = NULL;
    mpi = NULL;

    std::cout << "MPP 资源释放完成" << std::endl;
}

#include "v4l2_capture.h"

int open_dev(string dev_name)
{
    int fd = open(dev_name.c_str(), O_RDWR, 0);
    if(fd < 0)
    {
        perror("open file failed");
        return -1;
    }
    else
    {
        return fd;
    }
}

int inquery_v4l2_fmt(int fd, struct v4l2_fmtdesc* fmt)
{
    fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt->index = 0;
    int ret = ioctl(fd, VIDIOC_ENUM_FMT, fmt);
    if(ret < 0)
    {
        LOGE("VIDIOC_ENUM_FMT failed");
        close(fd);
        return -1;
    }
    else
    {
        return 0;
    }
}

int set_v4l2_fmt(int fd, int width, int height, struct v4l2_format* fmt)
{
    fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt->fmt.pix.width = width;
    fmt->fmt.pix.height = height;
    fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    int ret = ioctl(fd, VIDIOC_S_FMT, fmt);
    if(ret < 0)
    {
        LOGE("VIDIOC_S_FMT failed");
        return -1;
    }
    else
    {
        return 0;
    }
}

int v4l2_buf_mmap(int fd,unsigned char* mptr[4], unsigned int buf_size[4], struct v4l2_buffer* v4l_buffer)
{
    //ÉêÇë4¸öbuffer
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) 
    {
        LOGE("VIDIOC_REQBUFS failed");
        return -1;
    }

    v4l_buffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for(int i = 0; i < 4; i++)
    {
        v4l_buffer->index = i;
        int ret = ioctl(fd, VIDIOC_QUERYBUF, v4l_buffer);
        if(ret < 0)
        {
            LOGE("VIDIOC_QUERYBUF failed");
            return -1;
        }

        mptr[i] = (unsigned char*)mmap(NULL, v4l_buffer->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, v4l_buffer->m.offset);
        buf_size[i] = v4l_buffer->length;
        
        ret = ioctl(fd, VIDIOC_QBUF, v4l_buffer);
        if(ret < 0)
        {
            LOGE("VIDIOC_QBUF failed");
            return -1;
        }
    }
    
    return 0;
}

void capture_start(int fd)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if(ret < 0)
    {
        LOGE("VIDIOC_STREAMON failed");   
    }
    LOGI("start capture...\n");    
}

int capture_stop(int fd, unsigned char* mptr[4], unsigned int buf_size[4])
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    if(ret < 0)
    {
        LOGI("IDIOC_STREAMOFF failed");
        return -1;
    }
    LOGI("stop capture...");

    for(int i = 0; i < 4; i++)
    {
        ret = munmap(mptr[i], buf_size[i]);
        if(ret < 0)
        {
            LOGI("unmap failed");
            return -1;
        }
    }

    close(fd);
    return 0;
}
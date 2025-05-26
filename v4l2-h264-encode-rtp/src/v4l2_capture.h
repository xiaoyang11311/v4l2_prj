#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include "log.h"

int open_dev(string dev_name); //���豸�������ļ�������
int inquery_v4l2_fmt(int fd, struct v4l2_fmtdesc* fmt); //��ѯv4l2֧�ֵĸ�ʽ��0��YUYV
int set_v4l2_fmt(int fd, int width, int height, struct v4l2_format* fmt); //��������ͷ�ĸ�ʽ
int v4l2_buf_mmap(int fd, unsigned char* mptr[4], unsigned int buf_size[4], struct v4l2_buffer* v4l_buffer); //v4l2_buf�ڴ�ӳ�䣬ӳ�䵽�û��ռ�
void capture_start(int fd); //��ʼ�ɼ�
int capture_stop(int fd, unsigned char* mptr[4], unsigned int buf_size[4]); //ֹͣ�ɼ���ȡ��ӳ�䣬�ر��ļ�

#endif
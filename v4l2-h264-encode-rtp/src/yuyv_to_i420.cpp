#include "yuyv_to_i420.h"

void yuyv_to_i420_with_stride(unsigned char* yuyv,
                              unsigned char* y_plane, int y_stride,
                              unsigned char* u_plane, int u_stride,
                              unsigned char* v_plane, int v_stride,
                              int width, int height)
{
    for (int y = 0; y < height; y++) {
        unsigned char* y_row = y_plane + y * y_stride;
        unsigned char* u_row = u_plane + (y / 2) * u_stride;
        unsigned char* v_row = v_plane + (y / 2) * v_stride;

        for (int x = 0; x < width; x += 2) {
            int index = (y * width + x) * 2;

            unsigned char y0 = yuyv[index + 0];
            unsigned char u  = yuyv[index + 1];
            unsigned char y1 = yuyv[index + 2];
            unsigned char v  = yuyv[index + 3];

            y_row[x]     = y0;
            y_row[x + 1] = y1;

            if (y % 2 == 0) {  // 每 2 行采一行 U/V
                u_row[x / 2] = u;
                v_row[x / 2] = v;
            }
        }
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "stdint.h"

//opencv
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
extern "C"
{
#include "x264.h"
}

int main(int argc, char **argv)
{

     int ret;
     int y_size;
     int i, j;
     // using opencv get device camara
     cv::VideoCapture cap;
     if (!cap.open(0))
     {
          printf("Video capture device failure!\n");
          return 0;
     }
     int cv_width = 1280, cv_height = 720;
     cap.set(cv::CAP_PROP_FRAME_WIDTH, cv_width);
     cap.set(cv::CAP_PROP_FRAME_HEIGHT, cv_height);

     //FILE* fp_src  = fopen("../cuc_ieschool_640x360_yuv444p.yuv", "rb");
     FILE *fp_src = fopen("test1.yuv", "rb");

     FILE *fp_dst = fopen("test1_2M_80_0ms.h264", "wb");
     FILE *fp_output_dst = fopen("test1_2M_output_80_0ms.txt", "wt+");
     FILE *fp_delta_dst = fopen("test1_2M_delta_80_0ms.txt", "wt+");
     // FILE *fp_bps_dst = fopen("Johnny_bps_720_4M.txt", "wt+");
     // FILE *fp_bps_delta_dst = fopen("Johnny_bps_delta_720_4M.txt", "wt+");
     // FILE *fp_time = fopen("Jonny_donothing.txt", "wt+");
     //Encode 50 frame
     //if set 0, encode all frame
     ifstream infile;
     infile.open("bandwidthbps.txt");
     int bandwidth[654] = {0};
     for (int i = 0; i < 654; i++)
     {
          infile >> bandwidth[i];
     }
     int frame_num = 0;
     int csp = X264_CSP_I420;
     int width = 1280, height = 720;

     int iNal = 0;
     x264_nal_t *pNals = NULL;
     x264_t *pHandle = NULL;
     x264_picture_t *pPic_in = (x264_picture_t *)malloc(sizeof(x264_picture_t));
     x264_picture_t *pPic_out = (x264_picture_t *)malloc(sizeof(x264_picture_t));
     x264_param_t *pParam = (x264_param_t *)malloc(sizeof(x264_param_t));

     //Check
     if (fp_src == NULL || fp_dst == NULL)
     {
          printf("Error open files.\n");
          return -1;
     }

     x264_param_default(pParam);
     x264_param_default_preset(pParam, "ultrafast", "zerolatency");
     pParam->i_width = width;
     pParam->i_height = height;
     /*
         //Param
         pParam->i_log_level  = X264_LOG_DEBUG;
         pParam->i_threads  = X264_SYNC_LOOKAHEAD_AUTO;
         pParam->i_frame_total = 0;
         pParam->i_keyint_max = 10;
         pParam->i_bframe  = 5;
         pParam->b_open_gop  = 0;
         pParam->i_bframe_pyramid = 0;
         pParam->rc.i_qp_constant=0;
         pParam->rc.i_qp_max=0;
         pParam->rc.i_qp_min=0;
         pParam->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
         pParam->i_fps_den  = 1;
         pParam->i_fps_num  = 25;
         pParam->i_timebase_den = pParam->i_fps_num;
         pParam->i_timebase_num = pParam->i_fps_den;
         */
     pParam->i_csp = csp;
     pParam->rc.i_rc_method = X264_RC_ABR;
     pParam->rc.i_bitrate = 400; //  unit: kbit/s
     pParam->rc.i_vbv_max_bitrate = 400;
     pParam->rc.i_vbv_buffer_size = 1600;
     // pParam->rc.i_lookahead = 50;
     pParam->i_bframe = 0;
     // pParam->i_fps_num = 25;
     pParam->b_intra_refresh = 1;
     pParam->i_frame_reference = 1;

     x264_param_apply_profile(pParam, x264_profile_names[5]);

     pHandle = x264_encoder_open(pParam);

     x264_picture_init(pPic_out);
     x264_picture_alloc(pPic_in, csp, pParam->i_width, pParam->i_height);

     //ret = x264_encoder_headers(pHandle, &pNals, &iNal);
     unsigned int luma_size = width * height;;
     unsigned int chroma_size = luma_size / 4;
     int bufLen = width*height*3/2;
     uint8_t* pYuvBuf = new uint8_t[bufLen];
     y_size = pParam->i_width * pParam->i_height;
     //detect frame number
     if (frame_num == 0)
     {
          fseek(fp_src, 0, SEEK_END);
          switch (csp)
          {
          case X264_CSP_I444:
               frame_num = ftell(fp_src) / (y_size * 3);
               break;
          case X264_CSP_I420:
               frame_num = ftell(fp_src) / (y_size * 3 / 2);
               break;
          default:
               printf("Colorspace Not Support.\n");
               return -1;
          }
          fseek(fp_src, 0, SEEK_SET);
     }

     int bitratebps = 0;
     float consuming_time = 0.0;
     float delay_time = 0.0;
     float consuming_time_delay = 0.0;
     float delay_time_delay = 0.0;
     //Loop to Encode
     int frame_number = 0;

     float decrease_flag = 1.1925;

     for (i = 0; i < frame_num; i++)
     {
          // cv::Mat frame;
          // cap >> frame;
          // if (frame.empty())
          //      break; // end of video stream
          // cv::imshow("Sender", frame);
          // if (cv::waitKey(10) == 27)
          // {
          //      return 0;
          // }
          // cv::Mat yuv;
          // cv::cvtColor(frame, yuv, 128);
          // memcpy(pYuvBuf, yuv.data, bufLen * sizeof(unsigned char));
          // int frame_pointer = 0;
          // uint8_t *plane1 = pPic_in->img.plane[0];
          // for (int i = 0; i < luma_size; i++)
          // {
          //      *plane1 = pYuvBuf[frame_pointer];
          //      plane1++;
          //      frame_pointer++;
          // }
          // uint8_t *plane2 = pPic_in->img.plane[1];
          // for (int i = 0; i < chroma_size; i++)
          // {
          //      *plane2 = pYuvBuf[frame_pointer];
          //      plane2++;
          //      frame_pointer++;
          // }
          // uint8_t *plane3 = pPic_in->img.plane[2];
          // for (int i = 0; i < chroma_size; i++)
          // {
          //      *plane3 = pYuvBuf[frame_pointer];
          //      plane3++;
          //      frame_pointer++;
          // }
          // pPic_in->i_pts = frame_number;

          switch (csp)
          {
          case X264_CSP_I444:
          {
               fread(pPic_in->img.plane[0], y_size, 1, fp_src); //Y
               fread(pPic_in->img.plane[1], y_size, 1, fp_src); //U
               fread(pPic_in->img.plane[2], y_size, 1, fp_src); //V
               break;
          }
          case X264_CSP_I420:
          {
               fread(pPic_in->img.plane[0], y_size, 1, fp_src);     //Y
               fread(pPic_in->img.plane[1], y_size / 4, 1, fp_src); //U
               fread(pPic_in->img.plane[2], y_size / 4, 1, fp_src); //V
               break;
          }
          default:
          {
               printf("Colorspace Not Support.\n");
               return -1;
          }
          }
          pPic_in->i_pts = i;

          pParam->rc.i_bitrate = bandwidth[i / 25] / (2 * decrease_flag); //  unit: kbit/s
          pParam->rc.i_vbv_max_bitrate = bandwidth[i / 25] / (2 * decrease_flag);
          pParam->rc.i_vbv_buffer_size = bandwidth[i / 25];
          pParam->b_intra_refresh = 1;
          pParam->i_frame_reference = 1;

          int reconfig = x264_encoder_reconfig(pHandle, pParam);
          ret = x264_encoder_encode(pHandle, &pNals, &iNal, pPic_in, pPic_out);
          if (ret < 0)
          {
               printf("Error.\n");
               return -1;
          }

          // if (ret >= pParam->rc.i_bitrate * 5) {
          //      // cout << pParam->rc.i_bitrate;
          //      decrease_flag = 1.3;
          // }
          // else
          //      decrease_flag = 1.0;

          // do nothing if the frame arriving time has to be delayed
          float consuming_time_i = (float)(ret * 8) / (float)(bandwidth[i / 25] * 500);

          consuming_time += consuming_time_i;
          delay_time += 0.04;

          consuming_time_delay += consuming_time_i;
          delay_time_delay += 0.04;

          // has to wait until the camara get next frame
          if (consuming_time < delay_time - 0.0)
               consuming_time = delay_time - 0.0;

          if (consuming_time_delay < delay_time_delay - 0.0)
               consuming_time_delay = delay_time_delay - 0.0;

          int delay_frame_flag = 0;
          if (consuming_time > delay_time)
          {
               delay_frame_flag = 1;
          }
          // fprintf(fp_time, "frame: %d  consuming_time: %.4f   delay_time %.4f  delay_flag:%d \n",
          //                          i, consuming_time, delay_time, delay_frame_flag);

          //delay until next frame or drop the frame
          int drop_frame_flag = 0;
          if (consuming_time_delay > delay_time_delay)
          {
               drop_frame_flag = 1;
               consuming_time_delay = delay_time_delay;
          }

          // bitratebps += ret;
          // if (i % 25 == 0)
          // {
          //      fprintf(fp_bps_dst, "frame: %d  target bps: %d   actual bps: %d\n", i, pParam->rc.i_bitrate, bitratebps / 125);
          //      fprintf(fp_bps_delta_dst, "%d\n", pParam->rc.i_bitrate - bitratebps / 125);
          //      bitratebps = 0;
          // }

          printf("Succeed encode frame: %5d %d %.4f\n",i, ret, decrease_flag);
          fprintf(fp_output_dst, "frame: %d  bandwidth:%d  target size: %d   actual size: %d consuming_time: %.4f   delay_time %.4f  buffer_time: %.4f delay_flag: %d drop_flag: %d\n",
                  i, (bandwidth[i / 25] / 2) * 5, pParam->rc.i_bitrate * 5, ret, consuming_time, delay_time, consuming_time - delay_time, delay_frame_flag, drop_frame_flag);
          float delta = (float)((bandwidth[i / 25] / 2) * 5 - ret) * 100 / (float)((bandwidth[i / 25] / 2) * 5);
          fprintf(fp_delta_dst, "%.4f\n", delta);

          for (j = 0; j < iNal; ++j)
          {
               //if (drop_frame_flag != 1)
                    fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
          }
     }
     i = 0;
     //flush encoder
     while (1)
     {
          ret = x264_encoder_encode(pHandle, &pNals, &iNal, NULL, pPic_out);
          if (ret == 0)
          {
               break;
          }
          printf("Flush 1 frame.\n");
          for (j = 0; j < iNal; ++j)
          {
               fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
          }
          i++;
     }
     x264_picture_clean(pPic_in);
     x264_encoder_close(pHandle);
     pHandle = NULL;

     free(pPic_in);
     free(pPic_out);
     free(pParam);

     fclose(fp_src);
     fclose(fp_dst);

     return 0;
}

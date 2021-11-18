#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "stdint.h"

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

     //FILE* fp_src  = fopen("../cuc_ieschool_640x360_yuv444p.yuv", "rb");
     FILE *fp_src = fopen("Johnny.yuv", "rb");

     FILE *fp_dst = fopen("Johnny.h264", "wb");
     FILE *fp_output_dst = fopen("Johnny_output_720_4M.txt", "wt+");
     FILE *fp_delta_dst = fopen("Johnny_delta_720_4M.txt", "wt+");
     // FILE *fp_bps_dst = fopen("Johnny_bps_720_4M.txt", "wt+");
     // FILE *fp_bps_delta_dst = fopen("Johnny_bps_delta_720_4M.txt", "wt+");
     FILE *fp_time = fopen("Jonny_donothing.txt", "wt+");
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
     pParam->rc.i_bitrate = 400;
     pParam->rc.i_vbv_max_bitrate = 400;
     pParam->rc.i_vbv_buffer_size = 1600;
     // pParam->rc.i_lookahead = 50;
     pParam->i_bframe = 0;
     //pParam -> b_intra_refresh=1;
     //pParam -> i_frame_reference = 1;

     x264_param_apply_profile(pParam, x264_profile_names[5]);

     pHandle = x264_encoder_open(pParam);

     x264_picture_init(pPic_out);
     x264_picture_alloc(pPic_in, csp, pParam->i_width, pParam->i_height);

     //ret = x264_encoder_headers(pHandle, &pNals, &iNal);

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
     float delay_time = 0.5;
     //Loop to Encode
     for (i = 0; i < frame_num; i++)
     {
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

          if (i % 25 == 0)
          {
               pParam->rc.i_bitrate = bandwidth[i / 25]; //  unit: kbit/s
               pParam->rc.i_vbv_max_bitrate = bandwidth[i / 25];
               pParam->rc.i_vbv_buffer_size = bandwidth[i / 25];
               pParam->b_intra_refresh = 1;
               pParam->i_frame_reference = 1;
          }

          int reconfig = x264_encoder_reconfig(pHandle, pParam);
          ret = x264_encoder_encode(pHandle, &pNals, &iNal, pPic_in, pPic_out);
          if (ret < 0)
          {
               printf("Error.\n");
               return -1;
          }

          // do nothing if the frame arriving time has to be delayed
          float consuming_time_i = (float)(ret * 8) / (float)(bandwidth[i] * 1000);
          if (consuming_time_i > 0.04) {
               consuming_time += consuming_time_i;
          }
          else
               consuming_time += 0.04;

          delay_time += 0.04;
          int delay_frame_flag = 0;
          if (consuming_time > delay_time) {
               delay_frame_flag = 1;
          }
          fprintf(fp_time, "frame: %d  consuming_time: %.4f   delay_time %.4f  delay_flag:%d \n", i, consuming_time, delay_time, delay_frame_flag);


          // bitratebps += ret;
          // if (i % 25 == 0)
          // {
          //      fprintf(fp_bps_dst, "frame: %d  target bps: %d   actual bps: %d\n", i, pParam->rc.i_bitrate, bitratebps / 125);
          //      fprintf(fp_bps_delta_dst, "%d\n", pParam->rc.i_bitrate - bitratebps / 125);
          //      bitratebps = 0;
          // }

          printf("Succeed encode frame: %5d\n", i);
          fprintf(fp_output_dst, "frame: %d  target size: %d   actual size: %d\n", i, pParam->rc.i_bitrate * 5, ret);
          float delta = (float)(pParam->rc.i_bitrate * 5 - ret) * 1000 / (float)(pParam->rc.i_bitrate * 5);
          fprintf(fp_delta_dst, "%.4f\n", delta);

          for (j = 0; j < iNal; ++j)
          {
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

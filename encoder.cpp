#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include "stdint.h"

using namespace std;
extern "C"
{
#include "x264.h"
}

int main(int argc, char **argv)
{

     int ret;
     int ret_re_encode;
     int y_size;
     int i, j;
     
     FILE *fp_src = fopen("concert_1080.yuv", "rb");

     FILE *fp_dst = fopen("concert_1080_algo_5G.h264", "wb");
     FILE *fp_output_dst = fopen("concert_1080_output_algo_5G.txt", "wt+");
     FILE *fp_delta_dst = fopen("concert_1080_delta_algo_5G.txt", "wt+");
     FILE *fp_l_delta_dst = fopen("concert_1080_Ldelta_algo_5G.txt", "wt+");
     
     ifstream infile;
     infile.open("5G_5min.txt");
     int bandwidth[7823] = {0};
     for (int i = 0; i < 7823; i++)
     {
          infile >> bandwidth[i];
          bandwidth[i] /= 5;
     }
     int frame_num = 0;
     int csp = X264_CSP_I420;
     // 1920 1080; 1280 720
     int width = 1920, height = 1080;

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
     pParam->i_fps_num = 25;
     pParam->b_intra_refresh = 1;
     pParam->i_frame_reference = 1;

     x264_param_apply_profile(pParam, x264_profile_names[5]);

     pHandle = x264_encoder_open(pParam);

     x264_picture_init(pPic_out);
     x264_picture_alloc(pPic_in, csp, pParam->i_width, pParam->i_height);

     //ret = x264_encoder_headers(pHandle, &pNals, &iNal);
     unsigned int luma_size = width * height;
     ;
     unsigned int chroma_size = luma_size / 4;
     int bufLen = width * height * 3 / 2;
     uint8_t *pYuvBuf = new uint8_t[bufLen];
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

     float change_param = 1.0;
     float larger_param = 1.3;
     int larger_flag = 1;
     float biggest_delay = 0.0;
     float ave_delay = 0.0;
     float sum_delay = 0.0;
     int budget = 0;
     int delayed_frames = 0;
     float delay_ratio;
     int over2frames = 0;
     int over4frames = 0;
     float over2ratio;
     float over4ratio;

     int target = bandwidth[0];
     float time = 2.0;
     float buffer = 3.0;

     vector<float> ratioList;
     vector<float> deltaList;

     int biggerCount = 0;
     int constrainCount = 0;
     float previousN = 2;

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

          // i_bitrate = bandwidth[i]  -> Base
          pParam->rc.i_bitrate = target; //  unit: kbit/s
          pParam->rc.i_vbv_max_bitrate = target;
          pParam->rc.i_vbv_buffer_size = target;
          pParam->b_intra_refresh = 1;
          pParam->i_frame_reference = 1;

          int reconfig = x264_encoder_reconfig(pHandle, pParam);
          ret = x264_encoder_encode(pHandle, &pNals, &iNal, pPic_in, pPic_out);
          
          if (ret < 0)
          {
               printf("Error.\n");
               return -1;
          }

          if (ret - (bandwidth[i] * previousN) * 5 > 0)
               biggerCount++;
          // if (ret - target * 5 > 0)
          //      biggerCount++;

          float previous80ratio = 0;

          previousN = (delay_time + 0.12 - consuming_time ) / 0.04;

          /**
           * Statistical Encoder
           * 
           */
          target = bandwidth[i + 1] * ((delay_time + 0.12 - consuming_time ) / 0.04) / 1.712865;
          if (target > bandwidth[i + 1]) {
               target = bandwidth[i + 1];
               constrainCount++;
          }
          if (target < 0)
               target = 100;

          /**
           * Match Encoder
           * 
           */
          // budget = ret - bandwidth[i] * 5;
          // // (bandwidth[i / 25] / 2) bandwidth_size
          // // pParam->rc.i_bitrate * 5 target_size
          // // to make 90% sure next frame will compensate the budget / 1.19254
          // target = (bandwidth[(i + 1)] - budget / 5) / 1.19254;
          // if (target < 0)
          //      target = 100;

          /**
           * @brief delay calculation
           * 
           */
          // do nothing if the frame arriving time has to be delayed
          float consuming_time_i = (float)(ret * 8) / (float)(bandwidth[i] * 1000);

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

          delayed_frames += delay_frame_flag;
          if (consuming_time - delay_time > 0.08)
          {
               over2frames++;
          }

          if (consuming_time - delay_time > 0.16)
          {
               over4frames++;
          }

          //delay until next frame or drop the frame
          int drop_frame_flag = 0;
          if (consuming_time_delay > delay_time_delay)
          {
               drop_frame_flag = 1;
               consuming_time_delay = delay_time_delay;
          }


          if (consuming_time - delay_time > biggest_delay)
               biggest_delay = consuming_time - delay_time;
          sum_delay += consuming_time - delay_time;
          ave_delay = sum_delay / (i + 1);

          printf("Succeed encode frame: %5d %d %d %d %.4f\n", i, ret, bandwidth[i] * 5, target * 5, consuming_time - delay_time);
          fprintf(fp_output_dst, "frame: %d  bandwidth:%d  target size: %d   actual size: %d consuming_time: %.4f   delay_time %.4f  buffer_time: %.4f delay_flag: %d drop_flag: %d\n",
                  i, bandwidth[i] * 5, pParam->rc.i_bitrate * 5, ret, consuming_time, delay_time, consuming_time - delay_time, delay_frame_flag, drop_frame_flag);
          float delta = (float)(ret - (bandwidth[i]) * 5) / (float)((bandwidth[i]) * 5);
          float ldelta = (float)((((0.08 - consuming_time + delay_time) / 0.04) * bandwidth[i]) * 5 - ret) * 100 / (float)((((0.08 - consuming_time + delay_time) / 0.04) * bandwidth[i]) * 5);
          fprintf(fp_delta_dst, "%.4f\n", delta);
          fprintf(fp_l_delta_dst, "%.4f\n", ldelta);

          for (j = 0; j < iNal; ++j)
          {
               //if (drop_frame_flag != 1)
               fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
          }
          float ratio = (float) ret / (float) (target * 5);
          ratioList.push_back(ratio);
          deltaList.push_back(delta);
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

     delay_ratio = (float)delayed_frames / (float)frame_num;
     over2ratio = (float)over2frames / (float)frame_num;
     over4ratio = (float)over4frames / (float)frame_num;

     /**
      * @brief Calculating the CDF propability, the value will only be useful 
      * when the target size for every frame is bwe
      * 
      */
     sort(deltaList.begin(), deltaList.end());
     
     float ratio95 = deltaList[deltaList.size() * 0.95];
     float ratio90 = deltaList[deltaList.size() * 0.9];
     float ratio80 = deltaList[deltaList.size() * 0.8];
     cout << "biggest_delay: " << biggest_delay << "s, ave_delay: " << ave_delay << "s. " << endl;
     cout << "delay frames: " << delayed_frames << ", delay ratio: " << delay_ratio << endl;
     cout << "over 2-frame buffer frames: " << over2frames << ", over 2-frame buffer ratio: " << over2ratio << endl;
     cout << "over 4-frame buffer frames: " << over4frames << ", over 4-frame buffer ratio: " << over4ratio << endl;
     cout << "95% CDF: " << ratio95 << endl;
     cout << "90% CDF: " << ratio90 << endl;
     cout << "80% CDF: " << ratio80 << endl;
     cout << "bigger frames: " << biggerCount << endl;
     cout << "constraint frames: " << constrainCount << endl;

     pHandle = NULL;

     free(pPic_in);
     free(pPic_out);
     free(pParam);

     fclose(fp_src);
     fclose(fp_dst);

     return 0;
}

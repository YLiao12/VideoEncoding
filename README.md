# A Novel Statistical Video Frame Size Control for Real-time Video Streaming


## Introduction
Real-time video communication’s low delay characteristic
makes it widely used in video conferencing, live video
streaming. However, the performance of mainstream
video encoder is not that satisfying, and the variance of
**encoder’s output size** and **bandwidth** will easily cause high
latency and rebuffering. This paper analyses the widely
used video encoder X264’s frame-by-frame performance
on video encoding. Based on the statistical analysis of the
performance of X264, this paper proposes a new algorithm
Statistical Encoder. Compared to the raw output of the
X264 encoder and the improved Match Encoder, this new
statistical algorithm can firstly reduce the delay by more
than 90%, and also flexible responding user’s delay time
preference.

![image](https://user-images.githubusercontent.com/62742611/184880267-d2fc1267-49fd-450a-8040-56ab071b1332.png)

As shown in the figure above, the orange line is the output size, and the blue line is the given target size to the X264 Encoder, the gray area, which height is the delay of the video will accumulate.

## Analysis of X264

<img src="https://user-images.githubusercontent.com/62742611/184880662-aa5c6ebf-af4b-403a-8049-4bc69e2304ad.png" width="480" height="430">

As the figure shows, the output frame size of a 720p videoconferencing video normalized by target size (bandwidth estimation) is showing a normal distribution.

## Match X264 Algorithm

<img src="https://user-images.githubusercontent.com/62742611/184881035-66ea7a0c-eec9-49ea-934c-b39de886246b.png" width="480" height="350"><img src="https://user-images.githubusercontent.com/62742611/184882139-9a7fde0c-f981-4898-9a85-b41eeeedd4a0.png" width="510" height="300">

After apply Match X264 algorithm to the 720p videoconferencing video, we can sacrifice less bitrate to reach the goal of reducing delay.

Match X264 only sacrifice 9.91% of average bitrate to reduce biggest delay by 90% and average delay by 98%. With a strict decreasing strategy, the Match Encoder can decrease the biggest delay by 94% and the average delay by more than 99%


## Statistical X264 Algorithm

* Focus more on statistical results
* Target size should not exceed the bandwidth estimation

![image](https://user-images.githubusercontent.com/62742611/184883957-29815fbd-2629-4963-a092-1c3f708a609f.png)

Consider the CDF curve of the output delta probabation, which delta is `(output frame size – bandwidth estimation) / bandwidth estimation`. If we devide the target size by `(1 + delta)`, in fact we are moving the CDF curve to the left, to make sure 90% (this percentage is correspond to `delta`). As the trade off, the output bit rate will be `(1 + delta)` lower than before. 

If we can have a few prefetch frame on the player side, which could be treat *similar* as the player buffer, we can have a more aggresive Statistical Algorithm. for every frame, our target is the output size should be 90% percent sure smaller than `bandwidth estimation * (1 + N)`, which `N` is the remaining time buffer. The target size for the encoder should be `bandwidth estimation * (1 + N) / (1 + delta)`. Also, if the target size is bigger than the bandwidth estimation, we will constrain the target size to the bandwidth estimation.

## Evaluation

We evaluated the performance of the encoders on 4G, 5G and a poor condition wifi network trace data

The encoded video including a 720p concert video and football game video, and a 1080p concert video. The videos are 5 minutes and in 25 fps. (All videos have significant scene changes)


Here is the result:

![image](https://user-images.githubusercontent.com/62742611/184886268-6f61bf62-b9f5-43b6-ba0e-e2b34171be9a.png)

* Match encoder and Statistical Encoder can all achieve low delat frame ratio with prefetch frame buffer setting
* The average delay of Statistical Encoder is slightly shorter than Match Encoder, and for some extreme delay indicator, Statistical Encoder is slightly better than Match Encoder
* Statistical Encoder is more flexible, and user can customize their tolerable delay, and Statistical Encoder will provide encoded videos with different quality. 


## Build
put this encoder in x264 folder

input: YUV420 video
output: h264 video

```
$ g++ -o encoder encoder.cpp `pkg-config --libs --cflags x264`
```


# VideoEncoding

put this encoder in x264 folder

input: YUV420
output: h264

```
$ g++ -o encoder encoder.cpp `pkg-config --libs --cflags x264`
```


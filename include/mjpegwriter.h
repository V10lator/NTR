#pragma once


#include <stdint.h>



typedef struct _MPEG_CONTEXT {
	uintptr_t fd;
	int frameCount;
	int dataStreamOffset;
	int totalJpegSize;
	int filePos;
} MJPEG_CONTEXT;

typedef struct _MJPEG_JPEG_FILE {
	int offsetInStream;
	int alignedSize;
	int offsetInBuffer;
} MJPEG_JPEG_FILE;


int mjpegInitWithFile(MJPEG_CONTEXT *ctx, uintptr_t fd);
int mjpegAppendJPEGFile(MJPEG_CONTEXT *ctx, char* buffer, int size, MJPEG_JPEG_FILE* jpegFile);
int mjpegStartFileIndex(MJPEG_CONTEXT *ctx);
int mjpegAppendFileIndex(MJPEG_CONTEXT *ctx, MJPEG_JPEG_FILE* jpegFile);
int mjpegFinish(MJPEG_CONTEXT *ctx, int width, int height, int usecPerFrame);
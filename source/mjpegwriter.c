

#include "global.h"
#include "mjpegwriter.h"
#include "avifmt.h"

void rpMjpegWriteCached(int fd, int offset, void *buf, int size);

#define WRITE_BUFFER(buf, len) rpMjpegWriteCached(ctx->fd, ctx->filePos, buf, (len)); ctx->filePos += (len);
#define WRITE_U32(dat) tmp = (dat); WRITE_BUFFER(&tmp, 4);
#define LILEND4(x) (x)



int mjpegInitWithFile(MJPEG_CONTEXT *ctx, uintptr_t fd) {
	ctx->fd = fd;
	ctx->filePos = 0;
	ctx->frameCount = 0;
	ctx->dataStreamOffset = 0;
	ctx->totalJpegSize = 0;
	char tmpBuffer[12 + sizeof(struct AVI_list_hdrl) + 12] = { 0 };
	WRITE_BUFFER(tmpBuffer, sizeof(tmpBuffer));
	return 0;
}

int mjpegAppendJPEGFile(MJPEG_CONTEXT *ctx, char* buffer, int size, MJPEG_JPEG_FILE* jpegFile) {
	uint32_t tmp;
	int padding = (4 - (size % 4)) % 4;
	jpegFile->alignedSize = size + padding;
	WRITE_BUFFER("00db", 4);
	WRITE_U32(size + padding);
	ctx->dataStreamOffset += 4;
	jpegFile->offsetInStream = ctx->dataStreamOffset;
	buffer[6] = 'A';
	buffer[7] = 'V';
	buffer[8] = 'I';
	buffer[9] = '1';
	WRITE_BUFFER(buffer, size);
	WRITE_BUFFER("     ", padding);
	ctx->dataStreamOffset += size + padding + 4;
	ctx->totalJpegSize += size + padding;
	ctx->frameCount++;
	return 0;
}

int mjpegStartFileIndex(MJPEG_CONTEXT *ctx) {
	uint32_t tmp;

	WRITE_BUFFER("idx1", 4);
	WRITE_U32(16 * ctx->frameCount);
	return 0;
}

int mjpegAppendFileIndex(MJPEG_CONTEXT *ctx, MJPEG_JPEG_FILE* jpegFile) {
	uint32_t tmp;

	WRITE_BUFFER("00db", 4);
	WRITE_U32(18);
	WRITE_U32(jpegFile->offsetInStream);
	WRITE_U32(jpegFile->alignedSize);
	return 0;
}

int mjpegFinish(MJPEG_CONTEXT *ctx, int width, int height, int usecPerFrame) {
	uint32_t tmp;
	struct AVI_list_hdrl hdrl = {
		/* header */
		{
			{ 'L', 'I', 'S', 'T' },
			LILEND4(sizeof(struct AVI_list_hdrl) - 8),
			{ 'h', 'd', 'r', 'l' }
		},

		/* chunk avih */
		{ 'a', 'v', 'i', 'h' },
		LILEND4(sizeof(struct AVI_avih)),
		{
			LILEND4(usecPerFrame),
			LILEND4(1000000 * (ctx->totalJpegSize / ctx->frameCount) / usecPerFrame),
			LILEND4(0),
			LILEND4(AVIF_HASINDEX),
			LILEND4(ctx->frameCount),
			LILEND4(0),
			LILEND4(1),
			LILEND4(0),
			LILEND4(width),
			LILEND4(height),
			{ LILEND4(0), LILEND4(0), LILEND4(0), LILEND4(0) }
		},

		/* list strl */
		{
			{
				{ 'L', 'I', 'S', 'T' },
				LILEND4(sizeof(struct AVI_list_strl) - 8),
				{ 's', 't', 'r', 'l' }
			},

			/* chunk strh */
			{ 's', 't', 'r', 'h' },
			LILEND4(sizeof(struct AVI_strh)),
			{
				{ 'v', 'i', 'd', 's' },
				{ 'M', 'J', 'P', 'G' },
				LILEND4(0),
				LILEND4(0),
				LILEND4(0),
				LILEND4(usecPerFrame),
				LILEND4(1000000),
				LILEND4(0),
				LILEND4(ctx->frameCount),
				LILEND4(0),
				LILEND4(0),
				LILEND4(0)
			},

			/* chunk strf */
			{ 's', 't', 'r', 'f' },
			sizeof(struct AVI_strf),
			{
				LILEND4(sizeof(struct AVI_strf)),
				LILEND4(width),
				LILEND4(height),
				LILEND4(1 + 24 * 256 * 256),
				{ 'M', 'J', 'P', 'G' },
				LILEND4(width * height * 3),
				LILEND4(0),
				LILEND4(0),
				LILEND4(0),
				LILEND4(0)
			},

			/* list odml */
			{
				{
					{ 'L', 'I', 'S', 'T' },
					LILEND4(16),
					{ 'o', 'd', 'm', 'l' }
				},
				{ 'd', 'm', 'l', 'h' },
				LILEND4(4),
				LILEND4(ctx->frameCount)
			}
		}
	};
	uint32_t riffSize = sizeof(struct AVI_list_hdrl) + 4 + 4 +
		ctx->totalJpegSize + 8 * ctx->frameCount +
		8 + 8 + 16 * ctx->frameCount;
	ctx->filePos = 0;
	WRITE_BUFFER("RIFF", 4);
	WRITE_U32(riffSize);
	WRITE_BUFFER("AVI ", 4);
	/* list hdrl */
	hdrl.avih.us_per_frame = LILEND4(usecPerFrame);
	hdrl.avih.max_bytes_per_sec = LILEND4(1000000 * (ctx->totalJpegSize / ctx->frameCount)
		/ usecPerFrame);
	hdrl.avih.tot_frames = LILEND4(ctx->frameCount);
	hdrl.avih.width = LILEND4(width);
	hdrl.avih.height = LILEND4(height);
	hdrl.strl.strh.scale = LILEND4(usecPerFrame);
	hdrl.strl.strh.rate = LILEND4(1000000);
	hdrl.strl.strh.length = LILEND4(ctx->frameCount);
	hdrl.strl.strf.width = LILEND4(width);
	hdrl.strl.strf.height = LILEND4(height);
	hdrl.strl.strf.image_sz = LILEND4(width * height * 3);
	hdrl.strl.list_odml.frames = LILEND4(ctx->frameCount);	/*  */
	WRITE_BUFFER(&hdrl, sizeof(hdrl));
	WRITE_BUFFER("LIST", 4);
	WRITE_U32(ctx->totalJpegSize + 8 * ctx->frameCount + 4);
	WRITE_BUFFER("movi", 4);
	return 0;
}

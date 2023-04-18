#include "../capture/capture.h"
#include "decode.h"
#include "h264bsd_decoder.h"
#include "h264bsd_util.h"
#include <stdio.h>
#include <stdlib.h>

/*
https://github.com/oneam/h264bsd
Currently, the process of decoding modifies the input data. This has tripped me
a few times in the past so others should be aware of it.

The decoder only works nicely if it has a single consistent stream to deal with.
If you want to change the width/height or restart the stream with a new access
unit delimiter, it's better to shutdown and init a new decoder.
*/

struct SoftwareDecoder {
  struct Decoder decoder;
  storage_t *hantro_decoder;
};

struct Decoder *setup_decoder(DecompressedFrameHandler handler) {
  int err;
  struct SoftwareDecoder *decoder = calloc(1, sizeof(*decoder));
  decoder->decoder.decompressed_frame_handler = handler;
  decoder->hantro_decoder = h264bsdAlloc();
  err = h264bsdInit(decoder->hantro_decoder, HANTRO_FALSE);
  if (err)
    return NULL;
  return &decoder->decoder;
}

uint8_t *split_u32(uint32_t *buf, uint64_t len) {
  uint8_t *split = calloc(len * 4, sizeof(*split));
  for (uint64_t i = 0; i < len; i++)
    split[i] = buf[i] & 0xF000, split[i + 1] = buf[i] & 0x0F00,
    split[i + 2] = buf[i] & 0x00F0, split[i + 3] = buf[i] & 0x000F;
  return split;
}

volatile int frames = 0;

// TODO: make async
void decode_frame(struct Decoder *subdec, struct CFrame *frame) {
  int err;
  uint32_t nread;
  uint32_t result;
  uint32_t ta; /* THROWAWAY */
  uint32_t *decoded;
  struct SoftwareDecoder *decoder = (struct SoftwareDecoder *)subdec;

  // https://github.com/oneam/h264bsd/issues/22
  // Feed it nalu one by one

  if (frame->parameter_sets_count) {
    h264bsdShutdown(decoder->hantro_decoder);
    h264bsdFree(decoder->hantro_decoder);
    decoder->hantro_decoder = h264bsdAlloc();
    h264bsdInit(decoder->hantro_decoder, HANTRO_FALSE);

    for (uint64_t i = 0; i < frame->parameter_sets_count; i++) {
      result = h264bsdDecode(decoder->hantro_decoder, frame->parameter_sets[i],
                             frame->parameter_sets_lengths[i], 0, &nread);
      if (result)
        printf("ps decode finished with: %d\n", result);
    }
  }

  printf("finished ps decode\n");
  uint32_t croppingFlag, left, width, top, height;

  for (uint64_t i = 0; i < frame->nalus_count; i++) {
    printf("nalu: ");
    for (int j = 0; j < 10; j++)
      printf("%x", frame->nalus[i][j]);
    printf("\n");
    result = h264bsdDecode(decoder->hantro_decoder, frame->nalus[i],
                           frame->nalus_lengths[i], 0, &nread);

    switch (result) {
    case H264BSD_PIC_RDY:
      printf("getting decoded!!\n");
      decoded =
          h264bsdNextOutputPictureBGRA(decoder->hantro_decoder, &ta, &ta, &ta);
      break;
    case H264BSD_HDRS_RDY:
      h264bsdCroppingParams(decoder->hantro_decoder, &croppingFlag, &left,
                            &width, &top, &height);
      if (!croppingFlag) {
        width = h264bsdPicWidth(decoder->hantro_decoder) * 16;
        height = h264bsdPicHeight(decoder->hantro_decoder) * 16;
      }

      char *cropped = croppingFlag ? "(cropped) " : "";
      printf("Decoded headers. Image size %s%dx%d.\n", cropped, width, height);

      printf("headers ready\n");
      break;
    case H264BSD_RDY:
      printf("normal ready\n");
      break;
    case H264BSD_ERROR:
      printf("h264 error\n");
      break;
    case H264BSD_PARAM_SET_ERROR:
      printf("h264 param set error\n");
      break;
    default:
      printf("result is %d\n", result);
      break;
    }
  }

  printf("finished one frame\n");

  frames++;
  if (frames == 2)
    exit(0);
  else
    return;

  struct DFrame *dframe = calloc(1, sizeof(*dframe));
  dframe->width = h264bsdPicWidth(decoder->hantro_decoder);
  dframe->height = h264bsdPicHeight(decoder->hantro_decoder);
  dframe->data = split_u32(decoded, dframe->width * dframe->height * 4);
  dframe->bytes_per_row = dframe->width * 4;
  subdec->decompressed_frame_handler(dframe);
  decoded = NULL;
}

int stop_decoder(struct Decoder *subdec) {
  struct SoftwareDecoder *decoder = (struct SoftwareDecoder *)subdec;
  h264bsdShutdown(decoder->hantro_decoder);
  h264bsdFree(decoder->hantro_decoder);
  free(decoder);
  return 0;
}

void release_dframe(struct DFrame *frame) {
  free(frame->data);
  free(frame);
}

#include "decode.h"
#include "vk_video/vulkan_video_codec_h264std.h"
#include "vk_video/vulkan_video_codec_h264std_decode.h"
#include <stdlib.h>

struct VulkanDecoder {
  struct Decoder decoder;
};

struct Decoder *setup_decoder(DecompressedFrameHandler handler) {
  struct VulkanDecoder *decoder = calloc(1, sizeof(*decoder));
  return &decoder->decoder;
}

int start_decoder(struct Decoder *decoder, struct CFrame *frame) { return 0; }

void decode_frame(struct Decoder *decoder, struct CFrame *frame) {}

int stop_decoder(struct Decoder *decoder) { return 0; }

void release_dframe(struct DFrame **frame_ptr) {
  struct DFrame *frame = *frame_ptr;
  free(frame->data);
  free(frame);
  *frame_ptr = NULL;
}

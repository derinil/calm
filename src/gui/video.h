#ifndef GUI_VIDEO_H_
#define GUI_VIDEO_H_

#include "../data/stack.h"

void setup_video_renderer();
void draw_video_frame(struct DStack *stack);
void destroy_video_renderer();

#endif
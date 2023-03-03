#include "calm.h"
#include "capture/capture.h"

void frame_callback(char *data, size_t length)
{
    printf("received frame %d %d %d %d %d\n", data[0], data[1], data[2], data[3], data[4]);
}

int main()
{
    int err;
    struct Capturer *capturer;
    if ((err = setup(&capturer, frame_callback)))
        printf("failed to setup capturer: %d\n", err);
    if ((err = start_capture(capturer)))
        printf("failed to start capture: %d\n", err);
    getchar();
    if ((err = stop_capture(capturer)))
        printf("failed to stop capture: %d\n", err);
    return 0;
}

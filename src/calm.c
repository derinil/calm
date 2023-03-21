#include "calm.h"
#include "capture/capture.h"
#include "gui/gui.h"

void setup_debug_handlers(void);

void frame_callback(char *data, size_t length)
{
    printf("received frame %d %d %d %d %d with length %lu\n", data[0], data[1], data[2], data[3], data[4], length);
}

int capture()
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

int main(int argl, char **argv)
{
    setup_debug_handlers();
    if (argl > 1)
    {
        if (memcmp(argv[1], "capture", 7))
        {
            return capture();
        }
    }

    return start_gui();
}

#include <stdio.h>
#include "calm.h"
#include "client.h"
#include "server.h"

// This is defined in stack_tracer.zig to provide better stack traces when crash
void setup_debug_handlers(void);

int main(int argl, char **argv)
{
    int ret;
    setup_debug_handlers();

    if (argl != 2 || argv[1][0] == 's')
    {
        ret = start_server();
    }
    else
    {
        ret = start_client();
    }

    printf("exiting with code: %d\n", ret);
    return ret;
}

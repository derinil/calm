#include <stdio.h>
#include "calm.h"
#include "client.h"
#include "server.h"

// This is defined in stack_tracer.zig to provide better stack traces when crash
void setup_debug_handlers(void);

int main(int argl, char **argv)
{
    setup_debug_handlers();

#if 1
    return start_server();
#else
    return start_client();
#endif
}

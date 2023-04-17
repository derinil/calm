#include <stdio.h>
#include "calm.h"
#include "client.h"
#include "server.h"

int main(int argl, char **argv)
{
    int ret;

    if (argl != 2 || argv[1][0] == 's')
        ret = start_server();
    else
        ret = start_client();

    printf("exiting with code: %d\n", ret);
    return ret;
}

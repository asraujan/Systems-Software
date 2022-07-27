#include    "connmgr.h"
#include    "config.h"
#include    "./lib/tcpsock.h"
#include    "./lib/dplist.h"
#include    <poll.h>
#include    <stdio.h>
#include    <stdlib.h>

int main()
{
    printf("Starting main\n");
    connmgr_listen(1234);
    printf("End\n");
    return 0;
}
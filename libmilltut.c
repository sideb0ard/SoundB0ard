#include <libmill.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONN_ESTABLISHED 1
#define CONN_SUCCEEDED 2
#define CONN_FAILED 3

coroutine void statistics(chan ch) {
    int connections = 0;
    int active = 0;
    int failed = 0;
    
    while(1) {
        int op = chr(ch, int);

        if(op == CONN_ESTABLISHED)
            ++connections, ++active;
        else
            --active;
        if(op == CONN_FAILED)
            ++failed;

        printf("Total number of connections: %d\n", connections);
        printf("Active connections: %d\n", active);
        printf("Failed connections: %d\n\n", failed);
    }
}

coroutine void dialogue(tcpsock as, chan ch) 
{
    printf("New connnection!\n");
    chs(ch, int, CONN_ESTABLISHED);

    int64_t deadline = now() + 10000;

    tcpsend(as, "What's your nom nom?\r\n", 22, deadline);
    if( errno != 0 )
        goto cleanup;
    tcpflush(as, deadline);
    if( errno != 0 )
        goto cleanup;

    char inbuf[256];
    size_t sz = tcprecvuntil(as, inbuf, sizeof(inbuf), "\r", 1, deadline);

    inbuf[sz - 1] = 0;
    char outbuf[256];
    int rc = snprintf(outbuf, sizeof(outbuf), "Hello %s!\r\n", inbuf);
    
    tcpsend(as, outbuf, rc, deadline);
    if( errno != 0 )
        goto cleanup;
    tcpflush(as, deadline);
    if( errno != 0 )
        goto cleanup;

cleanup:
    if ( errno != 0 )
        chs(ch, int, CONN_SUCCEEDED);
    else
        chs(ch, int, CONN_FAILED);
    tcpclose(as);
}

int main(int argc, char **argv) {

    // setup
    int port = 5555;
    if ( argc > 1 ) 
        port = atoi(argv[1]);
    ipaddr addr = iplocal(NULL, port, 0);
    tcpsock ls = tcplisten(addr, 10);
    if ( !ls ) {
        perror("Can't open listening socket");
        return 1;
    }

    chan ch = chmake(int, 0);

    go(statistics(ch));

    // loopy
    while (1) {
        tcpsock as = tcpaccept(ls, -1);
        if ( !as )
            continue;
        go(dialogue(as, ch));
    }

    return 0;
}

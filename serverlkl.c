/* Modified from LKL issue 355
*  https://github.com/lkl/linux/issues/355
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include <lkl.h>
#include <lkl_host.h>

#ifdef __FreeBSD__
#include <sys/types.h>
#endif

int main(int argc, char** argv)
{
    int listenfd = 0;
    int connfd = 0;
    struct sockaddr_in serv_addr;
    char sendBuff[1025];
    int totalbyteswritten = 0;
    int byteswritten = 0;
    struct lkl_netdev *nd = NULL;
    int ret, nd_id = -1, nd_ifindex = -1;

    nd = lkl_netdev_tap_create("tap0", 0);
    if (!nd) {
        fprintf(stderr, "init netdev failed\n");
        return -1;
    }

    ret = lkl_netdev_add(nd, NULL);
    if (ret < 0) {
        fprintf(stderr, "failed to add netdev: %s\n",
                lkl_strerror(ret));
    }
    nd_id = ret;

    ret = lkl_start_kernel(&lkl_host_ops, "mem=32M loglevel=8");
    if (ret) {
        fprintf(stderr, "can't start kernel: %s\n", lkl_strerror(ret));
        return -1;
    }

    lkl_if_up(1);

    if (nd_id >= 0) {
        nd_ifindex = lkl_netdev_get_ifindex(nd_id);
        if (nd_ifindex > 0)
            lkl_if_up(nd_ifindex);
        else
            fprintf(stderr, "failed to get ifindex for netdev id %d: %s\n",
                    nd_id, lkl_strerror(nd_ifindex));
    }

    /*if (nd_ifindex >=0) {
        lkl_qdisc_parse_add(nd_ifindex, "root|fq");
        lkl_sysctl_parse_write("net.ipv4.tcp_congestion_control=bbr;net.ipv4.tcp_wmem=4096 16384 100000000");
    }*/

    if (nd_ifindex >= 0) {
        unsigned int addr = inet_addr("10.0.0.2"); // old 192.168.70.99
        int nmlen = atoi("22");

        if (addr != INADDR_NONE && nmlen > 0 && nmlen < 32) {
            ret = lkl_if_set_ipv4(nd_ifindex, addr, nmlen);
            if (ret < 0)
                fprintf(stderr, "failed to set IPv4 address: %s\n",
                        lkl_strerror(ret));
        }
    }

    if (nd_ifindex >= 0) {
        unsigned int addr = inet_addr("10.0.0.1"); //192.168.68.1

        if (addr != INADDR_NONE) {
            ret = lkl_set_ipv4_gateway(addr);
            if (ret < 0)
                fprintf(stderr, "failed to set IPv4 gateway: %s\n",
                        lkl_strerror(ret));
        }
    }

    listenfd = lkl_sys_socket(AF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(9600);

    while (1) {
        if (lkl_sys_bind(listenfd, (struct lkl_sockaddr*)&serv_addr, sizeof(serv_addr)) != 0 ) {
            printf("BIND: Failed to bind to port. Waiting....\n");
            sleep(10);
        } else {
            printf("BIND: Successful --- Serving Port:%d.\n", serv_addr.sin_port);
            break;
        }
    }

    if(lkl_sys_listen(listenfd, 10) == -1)
    {
        printf("Failed to listen\n");
        return -1;
    }

    while(1)
    {
        connfd = lkl_sys_accept(listenfd, (struct lkl_sockaddr*)NULL ,NULL);

        FILE *fp = fopen(argv[1],"rb");
        if(fp==NULL)
        {
            printf("File open error");
            return 1;
        }

        while(1)
        {
            char buff[256]={0};
            const char *wbuff = buff;
            int nread = fread(buff,1,256,fp);

            if(nread > 0)
            {
                byteswritten = lkl_sys_write(connfd, wbuff, nread);
                totalbyteswritten += byteswritten;
                printf("totalbyteswritten so far = {%d}\n", totalbyteswritten);
            }

            if (nread < 256)
            {
                if (feof(fp))
                    printf("End of file\n");
                if (ferror(fp))
                    printf("Error reading\n");
                break;
            }
        }
        printf("Total bytes sent = {%d}\n", totalbyteswritten);
        totalbyteswritten = 0;
        lkl_sys_close(connfd);
        sleep(1);
    }
    return 0;
}

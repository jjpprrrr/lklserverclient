/* Modified from LKL issue 355
*  https://github.com/lkl/linux/issues/355
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

void getlogtime(char *currentTime);

int main(int argc, char** argv)
{
    int sockfd = 0;
    int bytesReceived = 0;
    char recvBuff[256];
    memset(recvBuff, '0', sizeof(recvBuff));
    struct sockaddr_in serv_addr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9600);
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    }

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        perror("Error : Connect Failed \n");
        return 1;
    }

    FILE *fp;
    char fname[256];
    char logtime[100];
    int totalBytesRcvd = 0;

    getlogtime(logtime);
    sprintf(fname, "rcvdfle.%s", logtime);
    fp = fopen(fname, "ab");
    if(NULL == fp)
    {
        printf("Error opening file");
        return 1;
    }

    getlogtime(logtime);
    printf("Start time = {%s}\n", logtime);
    while((bytesReceived = read(sockfd, recvBuff, 256)) > 0)
    {
        totalBytesRcvd += bytesReceived;
        fwrite(recvBuff, 1,bytesReceived,fp);
    }
    printf("Total Bytes received %d\n",totalBytesRcvd);

    getlogtime(logtime);
    printf("End time = {%s}\n", logtime);

    if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
    }
    close(sockfd);

    return 0;
}

void getlogtime(char *currentTime)
{
    struct timeval curTime;
    int milli;
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80];

    gettimeofday(&curTime, NULL);
    milli = curTime.tv_usec / 1000;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 80, "%H:%M:%S", timeinfo);
    if(currentTime)
        sprintf(currentTime, "%s:%d", buffer, milli);
    printf("current time: %s \n", currentTime);
}

//
//  main.cpp
//  TSFtpServer
//
//  Created by zhaoxy on 14-5-7.
//  Copyright (c) 2014年 tsinghua. All rights reserved.
//

#include <iostream>
#include "define.h"
#include "CFtpHandler.h"
#include <sys/types.h>
#include <sys/socket.h>

int main(int argc, const char * argv[])
{
    int port = 2100;
    int listenFd = startup(port);
    //ignore SIGCHLD signal, which created by child process when exit, to avoid zombie process
    signal(SIGCHLD,SIG_IGN);
    while (1) {
        int newFd = accept(listenFd, (struct sockaddr *)NULL, NULL);
        if (newFd == -1) {
            //when child process exit, it'll generate a signal which will cause the parent process accept failed.
            //If happens, continue.
            if (errno == EINTR) continue;
            printf("accept error: %s(errno: %d)\n",strerror(errno),errno);
        }
        //timeout of recv
        struct timeval timeout = {3,0};
        setsockopt(newFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));
        int pid = fork();
        //fork error
        if (pid < 0) {
            printf("fork error: %s(errno: %d)\n",strerror(errno),errno);
        }
        //child process
        else if (pid == 0) {
            //close useless socket
            close(listenFd);
            send(newFd, TS_FTP_STATUS_READY, strlen(TS_FTP_STATUS_READY), 0);
            CFtpHandler handler(newFd);
            int freeTime = 0;
            while (1) {
                char buff[256];
                int len = (int)recv(newFd, buff, sizeof(buff), 0);
                //connection interruption
                if (len == 0) break;
                //recv timeout return -1
                if (len < 0) {
                    freeTime += 3;
                    //max waiting time exceed
                    if (freeTime >= 30) {
                        break;
                    }else {
                        continue;
                    }
                }
                buff[len] = '\0';
                //reset free time
                freeTime = 0;
                if (handler.handleRequest(buff)) {
                    break;
                }
            }
            close(newFd);
            std::cout<<"exit"<<std::endl;
            exit(0);
        }
        //parent process
        else {
            //close useless socket
            close(newFd);
        }
    }
    close(listenFd);
    return 0;
}


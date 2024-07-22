/*
 * Copyright (c) 2022-2024 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Simulation of device gateway sub device
 * Running the "make gateway_demo" command in a Linux environment to compile a gateway demo
 * gateway_server_test needs to be started first so that the sub devices can connect
 * 1. Input: Login:{deviceId};{passWord}, log in to the device. For example, input: Login:test;12345678
 * 2. Entering an int type data will trigger property reporting, such as entering: 12
 * 3. Input: Logout, device goes offline
 * 4. Input: newSecret:{newSecret}, change the device password
 */

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "log_util.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 15615
#define MAX_MESSAGE_BUF_LEN 30

int main(void)
{
    struct sockaddr_in serverAddr;
    int clientSocket;
    char sendbuf[MAX_MESSAGE_BUF_LEN];
    char recvbuf[MAX_MESSAGE_BUF_LEN];
    int iDataNum;

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect");
        return -1;
    }

    while (1) {
        printf("Input the messages you want to send, input \"quit\" to exit:");
        if (scanf_s("%s", sendbuf, MAX_MESSAGE_BUF_LEN) == -1) {
            printf("get messages error");
            break;
        }

        printf("\n");
        send(clientSocket, sendbuf, strlen(sendbuf), 0);

        if (strcmp(sendbuf, "quit") == 0) {
            break;
        }

        printf("Receive messages:");

        recvbuf[0] = '\0';
        iDataNum = recv(clientSocket, recvbuf, MAX_MESSAGE_BUF_LEN, 0);
        if (iDataNum > 0) {
            recvbuf[iDataNum] = '\0';
            printf("%s\n", recvbuf);
        }
    }

    close(clientSocket);
    return 0;
}
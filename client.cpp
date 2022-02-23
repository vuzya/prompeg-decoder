#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "socketConnection.h"
#include "packetQueue.h"
#include "packetBuffer.h"
#include "monitor.h"
#define RECVBUFLEN 1500

using namespace std;

void *threadproc(void *arg);
packetBuffer *myPacketBuffer = new packetBuffer(2048);
monitor *myMonitor = new monitor();
char* outputPortGlobal;

int main(int argc, char *argv[]) {
    string mediaIP;
    string mediaPort;
    string fecTimes;
    string maxDelay;
    string outputIP;
    string outputPort;
    unsigned char *sockRecvBuf = (unsigned char*)malloc(RECVBUFLEN * sizeof(unsigned char));

    mediaIP = "239.255.0.1";
    mediaPort = "20000";
    outputIP = "127.0.0.1";
    outputPort = "8000";
    maxDelay = "500";

    if(argc == 2) {
        maxDelay = argv[1];
    }
    else if(argc == 3) {
        mediaIP = argv[1];
        mediaPort = argv[2];
    }
    else if(argc == 4) {
        mediaIP = argv[1];
        mediaPort = argv[2];
        maxDelay = argv[4];
    }
    else if(argc == 5) {
        mediaIP = argv[1];
        mediaPort = argv[2];
        outputIP = argv[3];
        outputPort = argv[4];
    }
    else if(argc == 6) {
        mediaIP = argv[1];
        mediaPort = argv[2];
        outputIP = argv[3];
        outputPort = argv[4];
        maxDelay = argv[5];
    }
    cout << mediaIP << ":" << mediaPort << " " << outputIP << ":" << outputPort << " " << maxDelay;

    outputPortGlobal = (char*)calloc(outputPort.length() + 1, sizeof(char*));
    strncpy(outputPortGlobal, outputPort.c_str(), outputPort.length());
    
    socketUtility *mySocketUtility = new socketUtility(mediaIP.c_str() , mediaPort.c_str(), outputIP.c_str() , outputPort.c_str());

    fd_set master;
    fd_set read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(mySocketUtility -> media_Sockfd , &master);
    FD_SET(mySocketUtility -> fecRow_Sockfd , &master);
    FD_SET(mySocketUtility -> fecCol_Sockfd , &master);

    read_fds = master;

    pthread_t tid;
    pthread_create(&tid , NULL , &threadproc , NULL);

    for(;;) {
        myPacketBuffer -> updateFecQueue();
        myPacketBuffer -> fecRecovery();
        myPacketBuffer -> updateMediaQueue( mySocketUtility -> output_Sockfd , stoi(maxDelay) * 100 );
        myPacketBuffer -> updateMinSN();

        myMonitor -> updateRecovered(myPacketBuffer -> recovered);

        read_fds = master;
        struct timeval tv = {0 , 50};
        select( (mySocketUtility -> fdmax)+1 , &read_fds , NULL , NULL , &tv);

        if(FD_ISSET(mySocketUtility -> media_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recv(mySocketUtility -> media_Sockfd , sockRecvBuf , RECVBUFLEN , 0)) < 0)
                mySocketUtility -> ~socketUtility();

            myMonitor -> media -> updateStatus(sockRecvBuf);
            myPacketBuffer -> newMediaPacket(sockRecvBuf , bytes);
        }

        if(FD_ISSET(mySocketUtility -> fecRow_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recv(mySocketUtility -> fecRow_Sockfd , sockRecvBuf , RECVBUFLEN , 0)) < 0)
                mySocketUtility -> ~socketUtility();

            myMonitor -> fecRow -> updateStatus(sockRecvBuf);
            myPacketBuffer -> newFecPacket(sockRecvBuf , bytes);
        }

        if(FD_ISSET(mySocketUtility -> fecCol_Sockfd , &read_fds)) {
            int bytes = 0;
            if((bytes = recv(mySocketUtility -> fecCol_Sockfd , sockRecvBuf , RECVBUFLEN , 0)) < 0)
                mySocketUtility -> ~socketUtility();

            myMonitor -> fecCol -> updateStatus(sockRecvBuf);
            myPacketBuffer -> newFecPacket(sockRecvBuf , bytes);
        }
    }

    return 0; //never reached
}

void *threadproc(void *arg) {
    while(1) {
      printf("--stats for :%s\n", outputPortGlobal);
      sleep(5);
        myPacketBuffer -> bufferMonitor();
        myMonitor -> printMonitor();
    }
    return 0;
}

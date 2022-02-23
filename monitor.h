#ifndef INCLUDED_MONITOR_H
#define INCLUDED_MONITOR_H

#include <iostream>
#include <ctime>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "packetParser.h"

using namespace std;

class streamCounter {
    public:
        int recvd;
        int lost;
        uint16_t latestSN;
        streamCounter() {
            recvd = 0;
            lost = 0;
            latestSN = 0;
        }
        ~streamCounter(){};
        void printStatus() {
            printf("    recvd: %d , lost: %d , loss rate: %lf\n", recvd , lost , ( 100*lost / (double)(lost + recvd)));
        }
        void updateStatus(const void *buffer) {
            rtpHeader_ *rtpHeader = (rtpHeader_ *)buffer;
            uint16_t thisSN = ntohs(rtpHeader -> sequenceNum);

            if( latestSN > 0 && (thisSN - 1) > latestSN ) {
                int lostCounter = (thisSN - latestSN) - 1;
                lost+= lostCounter;
            }

            recvd++;
            latestSN = thisSN;
            return;
        }
};

class monitor {
    public:
        streamCounter *media;
        streamCounter *fecRow;
        streamCounter *fecCol;
        int recovered;
        monitor() {
            media = new streamCounter();
            fecRow = new streamCounter();
            fecCol = new streamCounter();
            recovered = 0;
        }
        ~monitor(){};
        void updateRecovered(int newRecovered) {
            recovered = newRecovered;
        }
        void printMonitor() {
            std::time_t t = std::time(0);
            printf("media stream at %d :\n", t);
            media -> printStatus();
            printf("    recovered/lost: %d/%d = %lf\n" , recovered , media -> lost , (100 * recovered / (double)media -> lost));
            printf("    loss rate after recovery: %d/%d = %lf\n" , (media -> lost - recovered) , (media -> recvd + media -> lost) , ( 100 * (media -> lost - recovered) / (double)(media -> recvd + media -> lost) ) );
            printf("fecRow stream:\n");
            fecRow -> printStatus();
            printf("fecCol stream:\n");
            fecCol -> printStatus();
            printf("\n");
        }
};

#endif
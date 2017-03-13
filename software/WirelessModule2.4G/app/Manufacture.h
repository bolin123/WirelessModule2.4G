#ifndef MANUFACTURE_H
#define MANUFACTURE_H

#include "Sys.h"

typedef void (*MFSendDataFunc_t)(const uint8_t *data, uint8_t len);

void MFStop(void);
void MFStart(MFSendDataFunc_t sendFunc);
void MFRecvByte(uint8_t byte);
void MFPoll(void);
void MFInitialize(void);

#endif


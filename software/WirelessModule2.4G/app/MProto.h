#ifndef MPROTO_H
#define MPROTO_H

#include "Sys.h"

bool MProtoGotDeviceInfo(void);
void MProtoRecvByte(uint8_t byte);
void MProtoInitialize(void);
void MProtoPoll(void);

#endif

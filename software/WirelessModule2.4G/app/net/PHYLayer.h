#ifndef PHY_LAYER_H
#define PHY_LAYER_H

#include "HalCtype.h"
#include "NRF24L01.h"

#define PHY_MAC_LEN 4

typedef enum
{
    PHY_FRAME_TYPE_OPERATION = 0,
    PHY_FRAME_TYPE_EST_NETWORK,
    PHY_FRAME_TYPE_ACK,
    PHY_FRAME_TYPE_HEARTBEAT,
}PHYFrameType_t;

typedef void (* PHYOptHandle_cb)(uint8_t srcAddr, bool isSleep, bool isBroadcast, uint8_t *data, uint8_t len);
typedef void (* PHYEstnetHandle_cb)(uint8_t srcMac[PHY_MAC_LEN], bool isBroadcast, uint8_t *data, uint8_t len);
typedef void (* PHYHeartbeatHandle_cb)(uint8_t srcAddr, bool isSleep);

bool PHYInitError(void);
void PHYInitialize(void);
void PHYPoll(void);
void PHYEstnetPacketSend(const uint8_t dstMac[PHY_MAC_LEN], const uint8_t *data, uint8_t len, bool directly);
void PHYOperatePacketSend(uint8_t dstAddr, bool isSleep, const uint8_t *data, uint8_t len, bool directly);
void PHYHeartbeatSend(uint8_t dstAddr, bool isSleep);
void PHYPacketRecvHandle(uint8_t *data, uint8_t len);
void PHYSetMac(const uint8_t mac[PHY_MAC_LEN]);
uint8_t *PHYGetMac(uint8_t *mac);
void PHYPowerSet(bool sleep);
bool PHYSendListEmpty(void);
void PHYSetNetAddr(uint8_t addr);
void PHYSetSegAddr(const uint8_t segAddr[PHY_MAC_LEN]);
void PHYPacketHandleCallbackRegiste(PHYOptHandle_cb optCb, PHYEstnetHandle_cb estnetCb, PHYHeartbeatHandle_cb hbCb);
void PHYSwitchChannel(uint8_t ch);

#endif


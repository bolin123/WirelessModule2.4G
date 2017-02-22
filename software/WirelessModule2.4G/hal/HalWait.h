#ifndef HAL_WAIT_H
#define HAL_WAIT_H

#include "HalCtype.h"
#include "HalWdt.h"
#include "stm32f0xx_tim.h"

#define HAL_WAIT_US 0
#define HAL_WAIT_MS 1

void hal_wait(uint16_t utime, uint8_t type);
void hal_wait_init(uint8_t type);
void hal_wait_stop(void);
void hal_wait_us(uint16_t usec);
void hal_wait_ms(uint16_t msec);
void hal_wait_check(uint8_t type);

#define hal_wait_us_cond(us, expression) \
  do{\
    uint16_t tmp = 0;\
      hal_wait_init(HAL_WAIT_US);\
        while(tmp < us && !(expression))\
          {\
            hal_wait_check(HAL_WAIT_US);\
              HalIwdtFeed();\
                if((TIM1->SR)&TIM_IT_Update)\
                  {\
                      TIM_ClearITPendingBit(TIM1, TIM_IT_Update);\
                        tmp++;\
      }\
    }\
  }while(0)

#define hal_wait_ms_cond(ms, expression) \
  do{\
    uint16_t tmp = 0;\
      hal_wait_init(HAL_WAIT_MS);\
        while(tmp < ms && !(expression))\
          {\
            hal_wait_check(HAL_WAIT_MS);\
              HalIwdtFeed();\
                if((TIM1->SR)&TIM_IT_Update)\
                  {\
                    TIM_ClearITPendingBit(TIM1, TIM_IT_Update);\
                        tmp++;\
      }\
    }\
  }while(0)

#define hal_wait_cond(type, time, expression, flag) \
  do{\
    flag = 0;\
      uint16_t count = 0;\
        hal_wait_init(type);\
          while(count < time && !(expression))\
            {\
              hal_wait_check(type);\
                HalIwdtFeed();\
                  if((TIM1->SR)&TIM_IT_Update)\
                    {\
                      TIM_ClearITPendingBit(TIM1, TIM_IT_Update);\
                          count++;\
      }\
    }\
      if(count == time)flag = 1;\
  }while(0)

#endif // HAL_WAIT_H

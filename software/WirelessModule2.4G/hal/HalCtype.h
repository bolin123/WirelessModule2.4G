#ifndef HAL_CTYPE_H
#define HAL_CTYPE_H

#include "HalConfig.h"

#undef bool
#define bool unsigned char
#undef uint8_t 
#define uint8_t unsigned char
#undef uint16_t 
#define uint16_t unsigned short
#undef uint32_t
#define uint32_t unsigned int
#undef int8_t 
#define int8_t signed char
#undef int16_t 
#define int16_t signed short
#undef int32_t
#define int32_t signed int

#undef true
#define true (1)
#undef false
#define false (0)

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef ROM_FUNC
#define ROM_FUNC
#endif

#define HalPrintf(...)  printf(__VA_ARGS__)
#define SysPrintf  HalPrintf
#define SysLog(...) {SysPrintf("%s:", __FUNCTION__); SysPrintf(__VA_ARGS__); SysPrintf("\n");}
#define SysErrLog(...) {SysPrintf("%s:[!!!ERROR]",__FUNCTION__); SysPrintf(__VA_ARGS__); SysPrintf("\n");}

#endif

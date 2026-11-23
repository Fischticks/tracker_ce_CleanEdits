#pragma once
#include <stdint.h>
static inline int32_t lxr_sat16(int32_t x) { return x > 32767 ? 32767 : (x < -32768 ? -32768 : x); }
static inline int32_t lxr_qadd16(int32_t a,int32_t b) { uint32_t x=(uint32_t)a,y=(uint32_t)b; int16_t l=(int16_t)lxr_sat16((int16_t)x+(int16_t)y),h=(int16_t)lxr_sat16((int16_t)(x>>16)+(int16_t)(y>>16)); return (int32_t)(((uint32_t)(uint16_t)h<<16)|(uint16_t)l); }
static inline int32_t lxr_qsub16(int32_t a,int32_t b) { uint32_t x=(uint32_t)a,y=(uint32_t)b; int16_t l=(int16_t)lxr_sat16((int16_t)x-(int16_t)y),h=(int16_t)lxr_sat16((int16_t)(x>>16)-(int16_t)(y>>16)); return (int32_t)(((uint32_t)(uint16_t)h<<16)|(uint16_t)l); }
static inline int32_t lxr_ssat(int32_t x,uint32_t n) { int32_t hi=(int32_t)((1u<<(n-1))-1u),lo=-(int32_t)(1u<<(n-1)); return x>hi?hi:(x<lo?lo:x); }
#ifndef __QADD16
#define __QADD16(a,b) lxr_qadd16((a),(b))
#endif
#ifndef __QSUB16
#define __QSUB16(a,b) lxr_qsub16((a),(b))
#endif
#ifndef __SSAT
#define __SSAT(x,n) lxr_ssat((x),(n))
#endif
#ifndef __CLZ
#define __CLZ(x) ((x)==0 ? 32 : __builtin_clz((uint32_t)(x)))
#endif
#define LXR_DEFAULT_NOTE 60
static inline float lxr_calc_detune(uint8_t value) { float f=((float)value/127.0f)-0.5f; return 1.0f+f*(1.05946309435929526456f-1.0f); }

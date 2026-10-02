#pragma once
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <algorithm>
#define OUTPUT 1
#define ESP_ARDUINO_VERSION_VAL(a,b,c) (((a)<<16)|((b)<<8)|(c))
#define INPUT_PULLUP 2
#define HIGH 1
#define LOW 0
inline void pinMode(uint8_t,uint8_t){}
inline void digitalWrite(uint8_t,int){}
extern unsigned long g_ledc_freq; extern unsigned long g_ledc_duty;
inline void ledcSetup(uint8_t,uint32_t f,uint8_t){ g_ledc_freq=f; }
inline void ledcAttachPin(uint8_t,uint8_t){}
inline void ledcWrite(uint8_t,uint32_t d){ g_ledc_duty=d; }
template<typename T> inline T constrain(T a,T b,T c){ return a<b?b:(a>c?c:a); }

#include "abVec4.h"

abVec4u32 abVec4_SignMask = abVec4u32_ConstInit(0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u);
abVec4u32 abVec4_AbsMask = abVec4u32_ConstInit(0x7fffffffu, 0x7fffffffu, 0x7fffffffu, 0x7fffffffu);

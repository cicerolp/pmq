#pragma once

#include <stdint.h>
#include <limits.h>

/* ********** 64 bit versions ********************/
inline
uint32_t m_undilate_2(uint64_t t) {
   t = (t | (t >> 1)) & 0x3333333333333333;
   t = (t | (t >> 2)) & 0x0F0F0F0F0F0F0F0F;
   t = (t | (t >> 4)) & 0x00FF00FF00FF00FF;
   t = (t | (t >> 8)) & 0x0000FFFF0000FFFF;
   t = (t | (t >> 16)) & 0x00000000FFFFFFFF;
   return ((uint32_t) t);
}

/**
 * @brief mortonDecode_RAM  decode a morton code to its (x,y) index;
 * @param i [IN] A 64-bit morton code
 * @param x [OUT] x coordinates
 * @param y [OUT] y coordinates
 */
inline
void mortonDecode_RAM(uint64_t i, uint32_t& x, uint32_t& y) {
   x = 0;
   y = 0;
   x = m_undilate_2(i & 0x5555555555555555);
   y = m_undilate_2((i >> 1) & 0x5555555555555555);
   return;
}

inline
uint64_t m_dilate_2(uint32_t t) {
   uint64_t r = t;
   r = (r | (r << 16)) & 0x0000FFFF0000FFFF;
   r = (r | (r << 8)) & 0x00FF00FF00FF00FF;
   r = (r | (r << 4)) & 0x0F0F0F0F0F0F0F0F;
   r = (r | (r << 2)) & 0x3333333333333333;
   r = (r | (r << 1)) & 0x5555555555555555;
   return (r);
}

inline
uint64_t mortonEncode_RAM(uint32_t x, uint32_t y) {
   return m_dilate_2(x) | m_dilate_2(y) << 1;
}

inline
void morton_min_max(uint64_t code, uint32_t diff, uint64_t& min, uint64_t& max) {
   min = code << 2 * (diff);
   max = min | ((uint64_t) ~0 >> (64 - 2 * diff));
}

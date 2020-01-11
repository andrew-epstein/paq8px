#include "Ilog.hpp"

int Ilog::log(uint16_t x) const { return (int) t[x]; }

Ilog *Ilog::mPInstance = nullptr;

Ilog *Ilog::getInstance() {
  if( !mPInstance )   // Only allow one getInstance of class to be generated.
    mPInstance = new Ilog;

  return mPInstance;
}

Ilog *ilog = Ilog::getInstance();

int llog(uint32_t x) {
  if( x >= 0x1000000 )
    return 256 + ilog->log(uint16_t(x >> 16U));
  else if( x >= 0x10000 )
    return 128 + ilog->log(uint16_t(x >> 8U));
  else
    return ilog->log(uint16_t(x));
}

uint32_t bitCount(uint32_t v) {
  v -= ((v >> 1U) & 0x55555555U);
  v = ((v >> 2U) & 0x33333333U) + (v & 0x33333333U);
  v = ((v >> 4U) + v) & 0x0f0f0f0fU;
  v = ((v >> 8U) + v) & 0x00ff00ffU;
  v = ((v >> 16U) + v) & 0x0000ffffU;
  return v;
}

int VLICost(uint64_t n) {
  int cost = 1;
  while( n > 0x7F ) {
    n >>= 7U;
    cost++;
  }
  return cost;
}

float rsqrt(const float x) {
  float r = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x))); //SSE
  return (0.5f * (r + 1.0f / (x * r)));
}
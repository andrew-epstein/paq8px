#ifndef PAQ8PX_IM32_HPP
#define PAQ8PX_IM32_HPP

#include "../Encoder.hpp"
#include "../file/File.hpp"
#include "Filter.hpp"
#include <cstdint>

/**
 * Filter for 32-bit images.
 */
class Im32Filter : public Filter {
private:
    Shared *shared = Shared::getInstance();
    int width = 0;
public:
    void setWidth(int w) {
      width = w;
    }

    int encode(File *in, File *out, uint64_t len, int width, int &headerSize) override {
      int r = 0;
      int g = 0;
      int b = 0;
      int a = 0;
      for( int i = 0; i < static_cast<int>(len / width); i++ ) {
        for( int j = 0; j < width / 4; j++ ) {
          b = in->getchar();
          g = in->getchar();
          r = in->getchar();
          a = in->getchar();
          out->putChar(g);
          out->putChar((shared->options & OPTION_SKIPRGB) != 0u ? r : g - r);
          out->putChar((shared->options & OPTION_SKIPRGB) != 0u ? b : g - b);
          out->putChar(a);
        }
        for( int j = 0; j < width % 4; j++ ) {
          out->putChar(in->getchar());
        }
      }
      for( int i = len % width; i > 0; i-- ) {
        out->putChar(in->getchar());
      }
      return 1;
    }

    uint64_t decode(File *in, File *out, FMode mode, uint64_t size, uint64_t &diffFound) override {
      int r = 0;
      int g = 0;
      int b = 0;
      int a = 0;
      int p = 0;
      bool rgb = (width & (1U << 31U)) > 0;
      if( rgb ) {
        width ^= (1U << 31U);
      }
      for( int i = 0; i < static_cast<int>(size / width); i++ ) {
        p = i * width;
        for( int j = 0; j < width / 4; j++ ) {
          b = encoder->decompress(), g = encoder->decompress(), r = encoder->decompress(), a = encoder->decompress();
          if( mode == FDECOMPRESS ) {
            out->putChar((shared->options & OPTION_SKIPRGB) != 0u ? r : b - r);
            out->putChar(b);
            out->putChar((shared->options & OPTION_SKIPRGB) != 0u ? g : b - g);
            out->putChar(a);
            if((j == 0) && ((i & 0xf) == 0)) {
              encoder->printStatus();
            }
          } else if( mode == FCOMPARE ) {
            if((((shared->options & OPTION_SKIPRGB) != 0u ? r : b - r) & 255U) != out->getchar() && (diffFound == 0u)) {
              diffFound = p + 1;
            }
            if( b != out->getchar() && (diffFound == 0u)) {
              diffFound = p + 2;
            }
            if((((shared->options & OPTION_SKIPRGB) != 0u ? g : b - g) & 255U) != out->getchar() && (diffFound == 0u)) {
              diffFound = p + 3;
            }
            if(((a) & 255) != out->getchar() && (diffFound == 0u)) {
              diffFound = p + 4;
            }
            p += 4;
          }
        }
        for( int j = 0; j < width % 4; j++ ) {
          if( mode == FDECOMPRESS ) {
            out->putChar(encoder->decompress());
          } else if( mode == FCOMPARE ) {
            if( encoder->decompress() != out->getchar() && (diffFound == 0u)) {
              diffFound = p + j + 1;
            }
          }
        }
      }
      for( int i = size % width; i > 0; i-- ) {
        if( mode == FDECOMPRESS ) {
          out->putChar(encoder->decompress());
        } else if( mode == FCOMPARE ) {
          if( encoder->decompress() != out->getchar() && (diffFound == 0u)) {
            diffFound = size - i;
            break;
          }
        }
      }
      return size;
    }

};

#endif //PAQ8PX_IM32_HPP

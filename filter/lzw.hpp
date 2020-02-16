#ifndef PAQ8PX_LZW_HPP
#define PAQ8PX_LZW_HPP

#include "Filter.hpp"

#include "LzwDictionary.hpp"

class LZWFilter : Filter {
private:
    static constexpr int lzwResetCode = 256;
    static constexpr int lzwEofCode = 257;
    static inline void
    writeCode(File *f, const FMode fMode, int *buffer, uint64_t *pos, int *bitsUsed, const int bitsPerCode, const int code,
              uint64_t *diffFound) {
      *buffer <<= bitsPerCode;
      *buffer |= code;
      (*bitsUsed) += bitsPerCode;
      while((*bitsUsed) > 7 ) {
        const uint8_t b = *buffer >> (*bitsUsed -= 8);
        (*pos)++;
        if( fMode == FDECOMPRESS ) {
          f->putChar(b);
        } else if( fMode == FCOMPARE && b != f->getchar()) {
          *diffFound = *pos;
        }
      }
    }

public:
    int encode(File *in, File *out, uint64_t size, int info, int &headerSize) override {
      LzwDictionary dic;
      int parent = -1;
      int code = 0;
      int buffer = 0;
      int bitsPerCode = 9;
      int bitsUsed = 0;
      bool done = false;
      while( !done ) {
        buffer = in->getchar();
        if( buffer < 0 ) {
          return 0;
        }
        for( int j = 0; j < 8; j++ ) {
          code += code + ((buffer >> (7 - j)) & 1U), bitsUsed++;
          if( bitsUsed >= bitsPerCode ) {
            if( code == lzwEofCode ) {
              done = true;
              break;
            }
            if( code == lzwResetCode ) {
              dic.reset();
              parent = -1;
              bitsPerCode = 9;
            } else {
              if( code < dic.index ) {
                if( parent != -1 ) {
                  dic.addEntry(parent, dic.dumpEntry(out, code));
                } else {
                  out->putChar(code);
                }
              } else if( code == dic.index ) {
                int a = dic.dumpEntry(out, parent);
                out->putChar(a);
                dic.addEntry(parent, a);
              } else {
                return 0;
              }
              parent = code;
            }
            bitsUsed = 0;
            code = 0;
            if((1 << bitsPerCode) == dic.index + 1 && dic.index < 4096 ) {
              bitsPerCode++;
            }
          }
        }
      }
      return 1;
    }

    auto decode(File *in, File *out, FMode fMode, uint64_t  /*size*/, uint64_t &diffFound) -> uint64_t override {
      LzwDictionary dic;
      uint64_t pos = 0;
      int parent = -1;
      int code = 0;
      int buffer = 0;
      int bitsPerCode = 9;
      int bitsUsed = 0;
      writeCode(out, fMode, &buffer, &pos, &bitsUsed, bitsPerCode, lzwResetCode, &diffFound);
      while((code = in->getchar()) >= 0 && diffFound == 0 ) {
        int index = dic.findEntry(parent, code);
        if( index < 0 ) { // entry not found
          writeCode(out, fMode, &buffer, &pos, &bitsUsed, bitsPerCode, parent, &diffFound);
          if( dic.index > 4092 ) {
            writeCode(out, fMode, &buffer, &pos, &bitsUsed, bitsPerCode, lzwResetCode, &diffFound);
            dic.reset();
            bitsPerCode = 9;
          } else {
            dic.addEntry(parent, code, index);
            if( dic.index >= (1U << bitsPerCode)) {
              bitsPerCode++;
            }
          }
          parent = code;
        } else {
          parent = index;
        }
      }
      if( parent >= 0 ) {
        writeCode(out, fMode, &buffer, &pos, &bitsUsed, bitsPerCode, parent, &diffFound);
      }
      writeCode(out, fMode, &buffer, &pos, &bitsUsed, bitsPerCode, lzwEofCode, &diffFound);
      if( bitsUsed > 0 ) { // flush buffer
        pos++;
        if( fMode == FDECOMPRESS ) {
          out->putChar(uint8_t(buffer));
        } else if( fMode == FCOMPARE && uint8_t(buffer) != out->getchar()) {
          diffFound = pos;
        }
      }
      return pos;
    }
};

#endif //PAQ8PX_LZW_HPP
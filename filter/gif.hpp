#ifndef PAQ8PX_GIF_HPP
#define PAQ8PX_GIF_HPP

#define LZW_TABLE_SIZE 9221

#define LZW_FIND(k) \
  { \
    offset     = ( int ) finalize64(k * PHI64, 13); \
    int stride = (offset > 0) ? LZW_TABLE_SIZE - offset : 1; \
    while (true) { \
      if ((index = table[offset]) < 0) { \
        index = -offset - 1; \
        break; \
      } else if (dict[index] == int(k)) { \
        break; \
      } \
      offset -= stride; \
      if (offset < 0) \
        offset += LZW_TABLE_SIZE; \
    } \
  }

#define LZW_RESET \
  { \
    for (int i = 0; i < LZW_TABLE_SIZE; table[i] = -1, i++) \
      ; \
  }

int encodeGif(File *in, File *out, uint64_t len, int &headerSize) {
  int codeSize = in->getchar(), diffPos = 0, clearPos = 0, bsize = 0, code, offset = 0;
  uint64_t beginIn = in->curPos(), beginOut = out->curPos();
  Array <uint8_t> output(4096);
  headerSize = 6;
  out->putChar(headerSize >> 8U);
  out->putChar(headerSize & 255U);
  out->putChar(bsize);
  out->putChar(clearPos >> 8U);
  out->putChar(clearPos & 255U);
  out->putChar(codeSize);
  Array<int> table(LZW_TABLE_SIZE);
  for( int phase = 0; phase < 2; phase++ ) {
    in->setpos(beginIn);
    int bits = codeSize + 1, shift = 0, buffer = 0;
    int blockSize = 0, maxcode = (1 << codeSize) + 1, last = -1;
    Array<int> dict(4096);
    LZW_RESET
    bool end = false;
    while((blockSize = in->getchar()) > 0 && in->curPos() - beginIn < len && !end ) {
      for( int i = 0; i < blockSize; i++ ) {
        buffer |= in->getchar() << shift;
        shift += 8;
        while( shift >= bits && !end ) {
          code = buffer & ((1U << bits) - 1);
          buffer >>= bits;
          shift -= bits;
          if( !bsize && code != (1 << codeSize)) {
            headerSize += 4;
            out->put32(0);
          }
          if( !bsize )
            bsize = blockSize;
          if( code == (1U << codeSize)) {
            if( maxcode > (1U << codeSize) + 1 ) {
              if( clearPos && clearPos != 69631 - maxcode )
                return 0;
              clearPos = 69631 - maxcode;
            }
            bits = codeSize + 1, maxcode = (1 << codeSize) + 1, last = -1;
            LZW_RESET
          } else if( code == (1 << codeSize) + 1 )
            end = true;
          else if( code > maxcode + 1 )
            return 0;
          else {
            int j = (code <= maxcode ? code : last), size = 1;
            while( j >= (1 << codeSize)) {
              output[4096 - (size++)] = dict[j] & 255;
              j = dict[j] >> 8;
            }
            output[4096 - size] = j;
            if( phase == 1 )
              out->blockWrite(&output[4096 - size], size);
            else
              diffPos += size;
            if( code == maxcode + 1 ) {
              if( phase == 1 )
                out->putChar(j);
              else
                diffPos++;
            }
            if( last != -1 ) {
              if( ++maxcode >= 8191 )
                return 0;
              if( maxcode <= 4095 ) {
                int key = (last << 8U) + j, index = -1;
                LZW_FIND(key)
                dict[maxcode] = key;
                table[(index < 0) ? -index - 1 : offset] = maxcode;
                if( phase == 0 && index > 0 ) {
                  headerSize += 4;
                  j = diffPos - size - (code == maxcode);
                  out->put32(j);
                  diffPos = size + (code == maxcode);
                }
              }
              if( maxcode >= ((1U << bits) - 1) && bits < 12 )
                bits++;
            }
            last = code;
          }
        }
      }
    }
  }
  diffPos = (int) out->curPos();
  out->setpos(beginOut);
  out->putChar(headerSize >> 8);
  out->putChar(headerSize & 255);
  out->putChar(255 - bsize);
  out->putChar((clearPos >> 8) & 255);
  out->putChar(clearPos & 255);
  out->setpos(diffPos);
  return in->curPos() - beginIn == len - 1;
}

#define GIF_WRITE_BLOCK(count) \
  { \
    output[0] = (count); \
    if (mode == FDECOMPRESS) \
      out->blockWrite(&output[0], (count) + 1); \
    else if (mode == FCOMPARE) \
      for (int j = 0; j < (count) + 1; j++) \
        if (output[j] != out->getchar() && ! diffFound) { \
          diffFound = outsize + j + 1; \
          return 1; \
        } \
    outsize += (count) + 1; \
    blockSize = 0; \
  }

#define GIF_WRITE_CODE(c) \
  { \
    buffer += (c) << shift; \
    shift += bits; \
    while (shift >= 8) { \
      output[++blockSize] = buffer & 255; \
      buffer >>= 8; \
      shift -= 8; \
      if (blockSize == bsize) GIF_WRITE_BLOCK(bsize); \
    } \
  }

int decodeGif(File *in, uint64_t size, File *out, FMode mode, uint64_t &diffFound) {
  int diffCount = in->getchar(), curDiff = 0;
  Array<int> diffPos(4096);
  diffCount = ((diffCount << 8) + in->getchar() - 6) / 4;
  int bsize = 255 - in->getchar();
  int clearPos = in->getchar();
  clearPos = (clearPos << 8) + in->getchar();
  clearPos = (69631 - clearPos) & 0xffff;
  int codesize = in->getchar(), bits = codesize + 1, shift = 0, buffer = 0, blockSize = 0;
  if( diffCount > 4096 || clearPos <= (1U << codesize) + 2 )
    return 1;
  int maxcode = (1U << codesize) + 1, input, code, offset = 0;
  Array<int> dict(4096);
  Array<int> table(LZW_TABLE_SIZE);
  LZW_RESET
  for( int i = 0; i < diffCount; i++ ) {
    diffPos[i] = in->getchar();
    diffPos[i] = (diffPos[i] << 8) + in->getchar();
    diffPos[i] = (diffPos[i] << 8) + in->getchar();
    diffPos[i] = (diffPos[i] << 8) + in->getchar();
    if( i > 0 )
      diffPos[i] += diffPos[i - 1];
  }
  Array <uint8_t> output(256);
  size -= 6 + diffCount * 4;
  int last = in->getchar(), total = (int) size + 1, outsize = 1;
  if( mode == FDECOMPRESS )
    out->putChar(codesize);
  else if( mode == FCOMPARE )
    if( codesize != out->getchar() && !diffFound )
      diffFound = 1;
  if( diffCount == 0 || diffPos[0] != 0 ) GIF_WRITE_CODE(1 << codesize) else
    curDiff++;
  while( size != 0 && (input = in->getchar()) != EOF ) {
    size--;
    int key = (last << 8) + input, index = (code = -1);
    if( last < 0 )
      index = input;
    else LZW_FIND(key)
    code = index;
    if( curDiff < diffCount && total - (int) size > diffPos[curDiff] )
      curDiff++, code = -1;
    if( code < 0 ) {
      GIF_WRITE_CODE(last)
      if( maxcode == clearPos ) {
        GIF_WRITE_CODE(1 << codesize)
        bits = codesize + 1, maxcode = (1 << codesize) + 1;
        LZW_RESET
      } else {
        ++maxcode;
        if( maxcode <= 4095 ) {
          dict[maxcode] = key;
          table[(index < 0) ? -index - 1 : offset] = maxcode;
        }
        if( maxcode >= (1 << bits) && bits < 12 )
          bits++;
      }
      code = input;
    }
    last = code;
  }
  GIF_WRITE_CODE(last)
  GIF_WRITE_CODE((1U << codesize) + 1)
  if( shift > 0 ) {
    output[++blockSize] = buffer & 255U;
    if( blockSize == bsize ) GIF_WRITE_BLOCK(bsize)
  }
  if( blockSize > 0 ) GIF_WRITE_BLOCK(blockSize)
  if( mode == FDECOMPRESS )
    out->putChar(0);
  else if( mode == FCOMPARE )
    if( 0 != out->getchar() && !diffFound )
      diffFound = outsize + 1;
  return outsize + 1;
}


#endif //PAQ8PX_GIF_HPP
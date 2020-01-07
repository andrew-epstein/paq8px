#ifndef PAQ8PX_LZWDICTIONARY_HPP
#define PAQ8PX_LZWDICTIONARY_HPP

class LZWDictionary {
private:
    static constexpr int hashSize = 9221;
    LZWentry dictionary[4096] {};
    short table[hashSize] {};
    uint8_t buffer[4096] {};

public:
    int index;

    LZWDictionary() : index(0) { reset(); }

    void reset() {
      memset(&dictionary, 0xFF, sizeof(dictionary));
      memset(&table, 0xFF, sizeof(table));
      for( int i = 0; i < 256; i++ ) {
        table[-findEntry(-1, i) - 1] = (short) i;
        dictionary[i].suffix = i;
      }
      index = 258; //2 extra codes, one for resetting the dictionary and one for signaling EOF
    }

    int findEntry(const int prefix, const int suffix) {
      int i = finalize64(hash(prefix, suffix), 13);
      int offset = (i > 0) ? hashSize - i : 1;
      while( true ) {
        if( table[i] < 0 ) //free slot?
          return -i - 1;
        else if( dictionary[table[i]].prefix == prefix && dictionary[table[i]].suffix == suffix ) //is it the entry we want?
          return table[i];
        i -= offset;
        if( i < 0 )
          i += hashSize;
      }
    }

    void addEntry(const int prefix, const int suffix, const int offset = -1) {
      if( prefix == -1 || prefix >= index || index > 4095 || offset >= 0 )
        return;
      dictionary[index].prefix = prefix;
      dictionary[index].suffix = suffix;
      table[-offset - 1] = index;
      index += (index < 4096);
    }

    int dumpEntry(File *f, int code) {
      int n = 4095;
      while( code > 256 && n >= 0 ) {
        buffer[n] = uint8_t(dictionary[code].suffix);
        n--;
        code = dictionary[code].prefix;
      }
      buffer[n] = uint8_t(code);
      f->blockWrite(&buffer[n], 4096 - n);
      return code;
    }
};

#endif //PAQ8PX_LZWDICTIONARY_HPP
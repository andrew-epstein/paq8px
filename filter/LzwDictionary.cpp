#include "LzwDictionary.hpp"
#include "../Hash.hpp"

LzwDictionary::LzwDictionary() : index(0) { reset(); }

void LzwDictionary::reset() {
  memset(&dictionary, 0xFF, sizeof(dictionary));
  memset(&table, 0xFF, sizeof(table));
  for( int i = 0; i < 256; i++ ) {
    table[-findEntry(-1, i) - 1] = static_cast<short>(i);
    dictionary[i].suffix = static_cast<short>(i);
  }
  index = 258; //2 extra codes, one for resetting the dictionary and one for signaling EOF
}

auto LzwDictionary::findEntry(const int prefix, const int suffix) -> int {
  int i = finalize64(hash(prefix, suffix), 13);
  int offset = (i > 0) ? hashSize - i : 1;
  while( true ) {
    // Is this a free slot?
    if( table[i] < 0 ) {
      return -i - 1;
    }
    // If this is the entry we want, return it
    if( dictionary[table[i]].prefix == prefix && dictionary[table[i]].suffix == suffix ) {
      return table[i];
    }
    i -= offset;
    if( i < 0 ) {
      i += hashSize;
    }
  }
}

void LzwDictionary::addEntry(const int prefix, const int suffix, const int offset) {
  if( prefix == -1 || prefix >= index || index > 4095 || offset >= 0 ) {
    return;
  }
  dictionary[index].prefix = static_cast<short>(prefix);
  dictionary[index].suffix = static_cast<short>(suffix);
  table[-offset - 1] = static_cast<short>(index);
  index += static_cast<int>(index < 4096);
}

auto LzwDictionary::dumpEntry(File *f, int code) -> int {
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

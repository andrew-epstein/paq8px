#ifndef PAQ8PX_LZWDICTIONARY_HPP
#define PAQ8PX_LZWDICTIONARY_HPP

#include "../file/File.hpp"
#include "LzwEntry.hpp"
#include <cstdint>

class LzwDictionary {
private:
    static constexpr int hashSize = 9221;
    LzWentry dictionary[4096] {};
    short table[hashSize] {};
    uint8_t buffer[4096] {};

public:
    int index;
    LzwDictionary();
    void reset();
    auto findEntry(int prefix, int suffix) -> int;
    void addEntry(int prefix, int suffix, int offset = -1);
    auto dumpEntry(File *f, int code) -> int;
};

#endif //PAQ8PX_LZWDICTIONARY_HPP

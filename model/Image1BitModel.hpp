#ifndef PAQ8PX_IMAGE1BITMODEL_HPP
#define PAQ8PX_IMAGE1BITMODEL_HPP

#include "../Shared.hpp"
#include "../Mixer.hpp"
#include "../Random.hpp"
#include "../RingBuffer.hpp"
#include "../StateMap.hpp"
#include "ImageModel.hpp"

/**
 * Model for 1-bit image data
 */
class Image1BitModel : ImageModel {
private:
    static constexpr int s = 11;
    Shared *shared = Shared::getInstance();
    Random rnd;
    int w = 0;
    uint32_t r0 = 0, r1 = 0, r2 = 0, r3 = 0; /**< last 4 rows, bit 8 is over current pixel */
    Array<uint8_t> t {0x23000}; /**< model: cxt -> state */
    int cxt[s] {}; /**< contexts */
    StateMap sm;

public:
    static constexpr int MIXERINPUTS = s;
    Image1BitModel();
    void setParam(int info0);
    void mix(Mixer &m);
};

#endif //PAQ8PX_IMAGE1BITMODEL_HPP

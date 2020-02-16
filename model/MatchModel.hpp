#ifndef PAQ8PX_MATCHMODEL_HPP
#define PAQ8PX_MATCHMODEL_HPP

#include "../Shared.hpp"
#include "../ContextMap2.hpp"
#include "../IndirectContext.hpp"
#include "../ModelStats.hpp"
#include "../SmallStationaryContextMap.hpp"
#include "../StationaryMap.hpp"

/**
 * Predict the next bit based on a preceding long matching byte sequence.
 *
 * This model monitors byte sequences and keeps their most recent positions in a hashtable.
 * When the current byte sequence matches an older sequence (having the same hash) the model predicts the forthcoming bits.
 */
class MatchModel {
private:
    static constexpr int numHashes = 3;
    static constexpr int numContextMaps = 2; /**< @todo: better name for this variable */
    static constexpr int numStateMaps = 3;
    static constexpr int numSmallStationaryContextMaps = 1;
    static constexpr int numStationaryMaps = 2;
    Shared *shared = Shared::getInstance();
    ModelStats *stats;
    enum Parameters : uint32_t {
        MaxExtend = 0, /**< longest allowed match expansion. Warning: larger value -> slowdown */
        MinLen = 5, /**< minimum required match length */
        StepSize = 2, /**< additional minimum length increase per higher order hash */
    };
    Array<uint32_t> table;
    StateMap stateMaps[numStateMaps];
    ContextMap2 contextMap2;
    SmallStationaryContextMap smallStationaryContextMap;
    StationaryMap stationaryMaps[numStationaryMaps];
    IndirectContext<uint8_t> indirectContext;
    uint32_t hashes[numHashes] {0};
    uint32_t ctx[numStateMaps] {0};
    uint32_t length = 0; /**< rebased length of match (length=1 represents the smallest accepted match length), or 0 if no match */
    uint32_t index = 0; /**< points to next byte of match in buf, 0 when there is no match */
    uint32_t lengthBak = 0; /**< allows match recovery after a 1-byte mismatch */
    uint32_t indexBak = 0;
    uint8_t expectedByte = 0; /**< prediction is based on this byte (buf[index]), valid only when length>0 */
    bool delta = false; /**< indicates that a match has just failed (delta mode) */
    const uint32_t mask;
    const int hashBits;
    Ilog *ilog = Ilog::getInstance();

public:
    static constexpr int MIXERINPUTS =
            2 + (numContextMaps * (ContextMap2::MIXERINPUTS + ContextMap2::MIXERINPUTS_RUN_STATS)) + numStateMaps +
            (numSmallStationaryContextMaps * SmallStationaryContextMap::MIXERINPUTS) +
            (numStationaryMaps * StationaryMap::MIXERINPUTS); // 23
    static constexpr int MIXERCONTEXTS = 8;
    static constexpr int MIXERCONTEXTSETS = 1;
    MatchModel(ModelStats *st, uint64_t size);
    void update();
    void mix(Mixer &m);
};

#endif //PAQ8PX_MATCHMODEL_HPP

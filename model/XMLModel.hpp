#ifndef PAQ8PX_XMLMODEL_HPP
#define PAQ8PX_XMLMODEL_HPP

#include "../ContextMap.hpp"
#include "../Shared.hpp"
#include <cstdint>

static constexpr uint32_t cacheSize = 32;

struct XMLAttribute {
    uint32_t name, value, length;
};

struct XMLContent {
    uint32_t data, length, type;
};

struct XMLTag {
    uint32_t name, length;
    int level;
    bool endTag, empty;
    XMLContent content;
    struct XMLAttributes {
        XMLAttribute items[4];
        uint32_t Index;
    } attributes;
};

struct XMLTagCache {
    XMLTag tags[cacheSize];
    uint32_t Index;
};

enum ContentFlags {
    Text = 0x001,
    Number = 0x002,
    Date = 0x004,
    Time = 0x008,
    URL = 0x010,
    Link = 0x020,
    Coordinates = 0x040,
    Temperature = 0x080,
    ISBN = 0x100,
};

enum XMLState {
    None = 0, ReadTagName = 1, ReadTag = 2, ReadAttributeName = 3, ReadAttributeValue = 4, ReadContent = 5, ReadCDATA = 6, ReadComment = 7,
};

class XMLModel {
private:
    static constexpr int nCM = 4;
    static_assert((cacheSize & (cacheSize - 1)) == 0);
    static_assert(cacheSize > 8);
    Shared *shared = Shared::getInstance();
    ContextMap cm;
    XMLTagCache cache {};
    uint32_t stateBh[8] {};
    XMLState state = None;
    XMLState pState = None;
    uint32_t whiteSpaceRun = 0;
    uint32_t pWsRun = 0;
    uint32_t indentTab = 0;
    uint32_t indentStep = 2;
    uint32_t lineEnding = 2;
    void detectContent(XMLContent *content);

public:
    static constexpr int MIXERINPUTS = nCM * (ContextMap::MIXERINPUTS); /**< 20 */
    static constexpr int MIXERCONTEXTS = 0;
    static constexpr int MIXERCONTEXTSETS = 0;
    explicit XMLModel(uint64_t size);
    void update();
    void mix(Mixer &m);
};

#endif //PAQ8PX_XMLMODEL_HPP

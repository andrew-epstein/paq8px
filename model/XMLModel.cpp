#include "XMLModel.hpp"

void XMLModel::detectContent(XMLContent *content) {
  INJECT_SHARED_buf
  if((shared->c4 & 0xF0F0F0F0) == 0x30303030 ) { //may be 4 digits (dddd)
    int i = 0;
    int j = 0;
    while((i < 4) && ((j = (shared->c4 >> (8 * i)) & 0xFFU) >= 0x30 && j <= 0x39)) {
      i++;
    }
    if( i == 4 /*????dddd*/ && (((shared->c8 & 0xFDF0F0FD) == 0x2D30302D && buf(9) >= 0x30 && buf(9) <= 0x39 /*d-dd-dddd or d.dd.dddd*/) ||
                                ((shared->c8 & 0xF0FDF0FD) == 0x302D302D) /*d-d-dddd or d.d.dddd*/)) {
      (*content).type |= ContentFlags::Date;
    }
  } else if(((shared->c8 & 0xF0F0FDF0) == 0x30302D30 /*dd.d???? or dd-d????*/ || (shared->c8 & 0xF0F0F0FD) == 0x3030302D) &&
            buf(9) >= 0x30 && buf(9) <= 0x39 /*dddd-???? or dddd.????*/) {
    int i = 2;
    int j = 0;
    while((i < 4) && ((j = (shared->c8 >> (8 * i)) & 0xFFU) >= 0x30 && j <= 0x39)) {
      i++;
    }

    if( i == 4 && (shared->c4 & 0xF0FDF0F0) == 0x302D3030 ) { //dd??d.dd or dd??d-dd
      (*content).type |= ContentFlags::Date;
    }
  }

  if((shared->c4 & 0xF0FFF0F0) == 0x303A3030 && buf(5) >= 0x30 && buf(5) <= 0x39 && ((buf(6) < 0x30 || buf(6) > 0x39) /*?dd:dd*/ ||
                                                                                     ((shared->c8 & 0xF0F0FF00) == 0x30303A00 &&
                                                                                      (buf(9) < 0x30 || buf(9) > 0x39) /*?dd:dd:dd*/))) {
    (*content).type |= ContentFlags::Time;
  }

  if((*content).length >= 8 && (shared->c8 & 0x80808080) == 0 && (shared->c4 & 0x80808080) == 0 ) { //8 ~ascii
    (*content).type |= ContentFlags::Text;
  }

  if((shared->c8 & 0xF0F0FFU) == 0x3030C2 && (shared->c4 & 0xFFF0F0FF) == 0xB0303027 ) { //dd {utf8 C2B0: degree sign} dd {apostrophe}
    int i = 2;
    while((i < 7) && buf(i) >= 0x30 && buf(i) <= 0x39 ) {
      i += (i & 1U) * 2 + 1;
    }

    if( i == 10 ) {
      (*content).type |= ContentFlags::Coordinates;
    }
  }

  if((shared->c4 & 0xFFFFFAU) == 0xC2B042 && (shared->c4 & 0xffU) != 0x47 &&
     (((shared->c4 >> 24U) >= 0x30 && (shared->c4 >> 24U) <= 0x39) ||
      ((shared->c4 >> 24U) == 0x20 && (buf(5) >= 0x30 && buf(5) <= 0x39)))) {
    (*content).type |= ContentFlags::Temperature;
  }

  if( shared->c1 >= 0x30 && shared->c1 <= 0x39 ) {
    (*content).type |= ContentFlags::Number;
  }

  if( shared->c4 == 0x4953424E && (shared->c8 & 0xffu) == 0x20 ) { // " ISBN"
    (*content).type |= ContentFlags::ISBN;
  }
}

XMLModel::XMLModel(const uint64_t size) : cm(size, nCM) {}

void XMLModel::update() {
  XMLTag *pTag = &cache.tags[(cache.Index - 1) & (cacheSize - 1)];
  XMLTag *tag = &cache.tags[cache.Index & (cacheSize - 1)];
  XMLAttribute *attribute = &((*tag).attributes.items[(*tag).attributes.Index & 3U]);
  XMLContent *content = &(*tag).content;
  pState = state;
  if((shared->c1 == TAB || shared->c1 == SPACE) && (shared->c1 == static_cast<uint8_t>(shared->c4 >> 8U) || (whiteSpaceRun == 0u))) {
    whiteSpaceRun++;
    indentTab = static_cast<uint32_t>(shared->c1 == TAB);
  } else {
    if((state == None || (state == ReadContent && (*content).length <= lineEnding + whiteSpaceRun)) && whiteSpaceRun > 1 + indentTab &&
       whiteSpaceRun != pWsRun ) {
      indentStep = abs(static_cast<int>(whiteSpaceRun - pWsRun));
      pWsRun = whiteSpaceRun;
    }
    whiteSpaceRun = 0;
  }
  if( shared->c1 == NEW_LINE ) {
    lineEnding = 1 + static_cast<int>(static_cast<uint8_t>(shared->c4 >> 8U) == CARRIAGE_RETURN);
  }

  switch( state ) {
    case None: {
      if( shared->c1 == 0x3C ) {
        state = ReadTagName;
        memset(tag, 0, sizeof(XMLTag));
        (*tag).level = ((*pTag).endTag || (*pTag).empty) ? (*pTag).level : (*pTag).level + 1;
      }
      if((*tag).level > 1 ) {
        detectContent(content);
      }

      cm.set(hash(pState, state, ((*pTag).level + 1) * indentStep - whiteSpaceRun));
      break;
    }
    case ReadTagName: {
      if((*tag).length > 0 && (shared->c1 == TAB || shared->c1 == NEW_LINE || shared->c1 == CARRIAGE_RETURN || shared->c1 == SPACE)) {
        state = ReadTag;
      } else if((shared->c1 == 0x3A || (shared->c1 >= 'A' && shared->c1 <= 'Z') || shared->c1 == 0x5F ||
                 (shared->c1 >= 'a' && shared->c1 <= 'z')) ||
                ((*tag).length > 0 && (shared->c1 == 0x2D || shared->c1 == 0x2E || (shared->c1 >= '0' && shared->c1 <= '9')))) {
        (*tag).length++;
        (*tag).name = (*tag).name * 263 * 32 + (shared->c1 & 0xDFU);
      } else if( shared->c1 == 0x3E ) {
        if((*tag).endTag ) {
          state = None;
          cache.Index++;
        } else {
          state = ReadContent;
        }
      } else if( shared->c1 != 0x21 && shared->c1 != 0x2D && shared->c1 != 0x2F && shared->c1 != 0x5B ) {
        state = None;
        cache.Index++;
      } else if((*tag).length == 0 ) {
        if( shared->c1 == 0x2F ) {
          (*tag).endTag = true;
          (*tag).level = max(0, (*tag).level - 1);
        } else if( shared->c4 == 0x3C212D2D ) {
          state = ReadComment;
          (*tag).level = max(0, (*tag).level - 1);
        }
      }

      if((*tag).length == 1 && (shared->c4 & 0xFFFF00U) == 0x3C2100 ) {
        memset(tag, 0, sizeof(XMLTag));
        state = None;
      } else if((*tag).length == 5 && shared->c8 == 0x215B4344 && shared->c4 == 0x4154415B ) {
        state = ReadCDATA;
        (*tag).level = max(0, (*tag).level - 1);
      }

      int i = 1;
      do {
        pTag = &cache.tags[(cache.Index - i) & (cacheSize - 1)];
        i += 1 + static_cast<int>((*pTag).endTag && cache.tags[(cache.Index - i - 1) & (cacheSize - 1)].name == (*pTag).name);
      } while( i < cacheSize && ((*pTag).endTag || (*pTag).empty));

      cm.set(hash(pState, state, (*tag).name, (*tag).level, (*pTag).name, static_cast<uint64_t>((*pTag).level != (*tag).level)));
      break;
    }
    case ReadTag: {
      if( shared->c1 == 0x2F ) {
        (*tag).empty = true;
      } else if( shared->c1 == 0x3E ) {
        if((*tag).empty ) {
          state = None;
          cache.Index++;
        } else {
          state = ReadContent;
        }
      } else if( shared->c1 != TAB && shared->c1 != NEW_LINE && shared->c1 != CARRIAGE_RETURN && shared->c1 != SPACE ) {
        state = ReadAttributeName;
        (*attribute).name = shared->c1 & 0xDFU;
      }
      cm.set(hash(pState, state, (*tag).name, shared->c1, (*tag).attributes.Index));
      break;
    }
    case ReadAttributeName: {
      if((shared->c4 & 0xFFF0U) == 0x3D20 && (shared->c1 == 0x22 || shared->c1 == 0x27)) {
        state = ReadAttributeValue;
        if((shared->c8 & 0xDFDFU) == 0x4852 && (shared->c4 & 0xDFDF0000) == 0x45460000 ) {
          (*content).type |= Link;
        }
      } else if( shared->c1 != 0x22 && shared->c1 != 0x27 && shared->c1 != 0x3D ) {
        (*attribute).name = (*attribute).name * 263 * 32 + (shared->c1 & 0xDFU);
      }

      cm.set(hash(pState, state, (*attribute).name, (*tag).attributes.Index, (*tag).name, (*content).type));
      break;
    }
    case ReadAttributeValue: {
      if( shared->c1 == 0x22 || shared->c1 == 0x27 ) {
        (*tag).attributes.Index++;
        state = ReadTag;
      } else {
        (*attribute).value = (*attribute).value * 263 * 32 + (shared->c1 & 0xDFU);
        (*attribute).length++;
        if((shared->c8 & 0xDFDFDFDF) == 0x48545450 && ((shared->c4 >> 8U) == 0x3A2F2F || shared->c4 == 0x733A2F2F)) {
          (*content).type |= URL;
        }
      }
      cm.set(hash(pState, state, (*attribute).name, (*content).type));
      break;
    }
    case ReadContent: {
      if( shared->c1 == 0x3C ) {
        state = ReadTagName;
        cache.Index++;
        memset(&cache.tags[cache.Index & (cacheSize - 1)], 0, sizeof(XMLTag));
        cache.tags[cache.Index & (cacheSize - 1)].level = (*tag).level + 1;
      } else {
        (*content).length++;
        (*content).data = (*content).data * 997 * 16 + (shared->c1 & 0xDFU);
        detectContent(content);
      }
      cm.set(hash(pState, state, (*tag).name, shared->c4 & 0xC0FFU));
      break;
    }
    case ReadCDATA: {
      if((shared->c4 & 0xFFFFFFU) == 0x5D5D3E ) {
        state = None;
        cache.Index++;
      }
      cm.set(hash(pState, state));
      break;
    }
    case ReadComment: {
      if((shared->c4 & 0xFFFFFFU) == 0x2D2D3E ) {
        state = None;
        cache.Index++;
      }
      cm.set(hash(pState, state));
      break;
    }
  }

  stateBh[pState] = (stateBh[pState] << 8U) | shared->c1;
  pTag = &cache.tags[(cache.Index - 1) & (cacheSize - 1)];
  uint64_t i = 64;
  cm.set(hash(++i, state, (*tag).level, pState * 2 + static_cast<int>((*tag).endTag), (*tag).name));
  cm.set(hash(++i, (*pTag).name, state * 2 + static_cast<int>((*pTag).endTag), (*pTag).content.type, (*tag).content.type));
  cm.set(hash(++i, state * 2 + static_cast<int>((*tag).endTag), (*tag).name, (*tag).content.type, shared->c4 & 0xE0FFU));
}

void XMLModel::mix(Mixer &m) {
  if( shared->bitPosition == 0 ) {
    update();
  }
  cm.mix(m);
}

#include "DmcForest.hpp"

DmcForest::DmcForest(const uint64_t size) : dmcModels(MODELS) {
  for( int i = MODELS - 1; i >= 0; i-- ) {
    dmcModels[i] = new DmcModel(size / dmcMem[i], dmcParams[i]);
  }
}

DmcForest::~DmcForest() {
  for( int i = MODELS - 1; i >= 0; i-- ) {
    delete dmcModels[i];
  }
}

void DmcForest::mix(Mixer &m) {
  int i = MODELS;
  // the slow models predict individually
  m.add(dmcModels[--i]->st() >> 3U);
  m.add(dmcModels[--i]->st() >> 3U);
  // the fast models are combined for better stability
  while( i > 0 ) {
    const int pr1 = dmcModels[--i]->st();
    const int pr2 = dmcModels[--i]->st();
    m.add((pr1 + pr2) >> 4U);
  }

  // reset models when their structure can't adapt anymore
  // the two slow models are never reset
  if( shared->bitPosition == 0 ) {
    for( int j = MODELS - 3; j >= 0; j-- ) {
      if( dmcModels[j]->isFull()) {
        dmcModels[j]->resetStateGraph(dmcParams[j]);
      }
    }
  }
}

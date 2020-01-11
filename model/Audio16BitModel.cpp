#include "Audio16BitModel.hpp"

Audio16BitModel::Audio16BitModel(ModelStats *st) : AudioModel(st), sMap1B {
        /*nOLS: 0-3*/ {{17, 1, 7, 128}, {17, 1, 10, 128}, {17, 1, 6, 86}, {17, 1, 6, 128}},
                      {{17, 1, 7, 128}, {17, 1, 10, 128}, {17, 1, 6, 86}, {17, 1, 6, 128}},
                      {{17, 1, 7, 128}, {17, 1, 10, 128}, {17, 1, 6, 86}, {17, 1, 6, 128}},
                      {{17, 1, 7, 128}, {17, 1, 10, 128}, {17, 1, 6, 86}, {17, 1, 6, 128}},
        /*nOLS: 4-7*/
                      {{17, 1, 7, 128}, {17, 1, 10, 128}, {17, 1, 6, 86}, {17, 1, 6, 128}},
                      {{17, 1, 7, 128}, {17, 1, 10, 128}, {17, 1, 6, 86}, {17, 1, 6, 128}},
                      {{17, 1, 7, 128}, {17, 1, 10, 128}, {17, 1, 6, 86}, {17, 1, 6, 128}},
                      {{17, 1, 7, 128}, {17, 1, 10, 128}, {17, 1, 6, 86}, {17, 1, 6, 128}},
        /*nLMS: 0-2*/
                      {{17, 1, 7, 86},  {17, 1, 10, 86},  {17, 1, 6, 64}, {17, 1, 6, 86}},
                      {{17, 1, 7, 86},  {17, 1, 10, 86},  {17, 1, 6, 64}, {17, 1, 6, 86}},
                      {{17, 1, 7, 86},  {17, 1, 10, 86},  {17, 1, 6, 64}, {17, 1, 6, 86}},
        /*nSSM: 0-2*/
                      {{17, 1, 7, 86},  {17, 1, 10, 86},  {17, 1, 6, 64}, {17, 1, 6, 86}},
                      {{17, 1, 7, 86},  {17, 1, 10, 86},  {17, 1, 6, 64}, {17, 1, 6, 86}},
                      {{17, 1, 7, 86},  {17, 1, 10, 86},  {17, 1, 6, 64}, {17, 1, 6, 86}}} {}

void Audio16BitModel::setParam(int info) {
  INJECT_STATS_blpos
  INJECT_SHARED_bpos
  if( blPos == 0 && bpos == 0 ) {
    info |= 4U; // comment this line if skipping the endianness transform
    assert((info & 2) != 0);
    stereo = (info & 1U);
    lsb = (info < 4);
    mask = 0;
    stats->wav = (stereo + 1) * 2;
    wMode = info;
    for( int i = 0; i < nLMS; i++ )
      lms[i][0].reset(), lms[i][1].reset();
  }
}

void Audio16BitModel::mix(Mixer &m) {
  INJECT_SHARED_bpos
  INJECT_STATS_blpos
  INJECT_SHARED_c0
  INJECT_SHARED_c1
  if( bpos == 0 && blPos != 0 ) {
    ch = (stereo) ? (blPos & 2U) >> 1U : 0;
    lsb = (blPos & 1U) ^ (wMode < 4);
    if((blPos & 1U) == 0 ) {
      sample = (wMode < 4) ? s2(2) : t2(2);
      const int pCh = ch ^stereo;
      int i = 0;
      for( errLog = 0; i < nOLS; i++ ) {
        ols[i][pCh].update(sample);
        residuals[i][pCh] = sample - prd[i][pCh][0];
        const auto absResidual = (uint32_t) abs(residuals[i][pCh]);
        mask += mask + (absResidual > 128);
        errLog += square(absResidual >> 6U);
      }
      for( int j = 0; j < nLMS; j++ )
        lms[j][pCh].update(sample);
      for( ; i < nSSM; i++ )
        residuals[i][pCh] = sample - prd[i][pCh][0];
      errLog = min(0xF, ilog2(errLog));

      if( stereo ) {
        for( int i = 1; i <= 24; i++ )
          ols[0][ch].add(x2(i));
        for( int i = 1; i <= 104; i++ )
          ols[0][ch].add(x1(i));
      } else
        for( int i = 1; i <= 128; i++ )
          ols[0][ch].add(x1(i));

      int k1 = 90, k2 = k1 - 12 * stereo;
      for( int j = (i = 1); j <= k1; j++, i += 1U << ((j > 16) + (j > 32) + (j > 64)))
        ols[1][ch].add(x1(i));
      for( int j = (i = 1); j <= k2; j++, i += 1U << ((j > 5) + (j > 10) + (j > 17) + (j > 26) + (j > 37)))
        ols[2][ch].add(x1(i));
      for( int j = (i = 1); j <= k2; j++, i += 1U << ((j > 3) + (j > 7) + (j > 14) + (j > 20) + (j > 33) + (j > 49)))
        ols[3][ch].add(x1(i));
      for( int j = (i = 1); j <= k2; j++, i += 1 + (j > 4) + (j > 8))
        ols[4][ch].add(x1(i));
      for( int j = (i = 1); j <= k1; j++, i += 2 + ((j > 3) + (j > 9) + (j > 19) + (j > 36) + (j > 61)))
        ols[5][ch].add(x1(i));

      if( stereo ) {
        for( i = 1; i <= k1 - k2; i++ ) {
          const auto s = (double) x2(i);
          ols[2][ch].addFloat(s);
          ols[3][ch].addFloat(s);
          ols[4][ch].addFloat(s);
        }
      }

      k1 = 28, k2 = k1 - 6 * stereo;
      for( i = 1; i <= k2; i++ )
        ols[6][ch].add(x1(i));
      for( i = 1; i <= k1 - k2; i++ )
        ols[6][ch].add(x2(i));

      k1 = 32, k2 = k1 - 8 * stereo;
      for( i = 1; i <= k2; i++ )
        ols[7][ch].add(x1(i));
      for( i = 1; i <= k1 - k2; i++ )
        ols[7][ch].add(x2(i));

      for( i = 0; i < nOLS; i++ )
        prd[i][ch][0] = signedClip16((int) floor(ols[i][ch].predict()));
      for( ; i < nOLS + nLMS; i++ )
        prd[i][ch][0] = signedClip16((int) floor(lms[i - nOLS][ch].predict(sample)));
      prd[i++][ch][0] = signedClip16(x1(1) * 2 - x1(2));
      prd[i++][ch][0] = signedClip16(x1(1) * 3 - x1(2) * 3 + x1(3));
      prd[i][ch][0] = signedClip16(x1(1) * 4 - x1(2) * 6 + x1(3) * 4 - x1(4));
      for( i = 0; i < nSSM; i++ )
        prd[i][ch][1] = signedClip16(prd[i][ch][0] + residuals[i][pCh]);
    }
    stats->Audio = 0x80 | (mxCtx = ilog2(min(0x1F, bitCount(mask))) * 4 + ch * 2 + lsb);
  }

  const auto b = short(
          (wMode < 4) ? (lsb) ? uint8_t(c0 << (8 - bpos)) : (c0 << (16 - bpos)) | c1 : (lsb) ? (c1 << 8U) | uint8_t(c0 << (8 - bpos)) :
                                                                                       c0 << (16 - bpos));

  for( int i = 0; i < nSSM; i++ ) {
    const uint32_t ctx0 = uint16_t(prd[i][ch][0] - b);
    const uint32_t ctx1 = uint16_t(prd[i][ch][1] - b);

    const int shift = (!lsb);
    sMap1B[i][0].set((lsb << 16) | (bpos << 13) | (ctx0 >> (3U << shift)));
    sMap1B[i][1].set((lsb << 16) | (bpos << 13) | (ctx0 >> ((!lsb) + (3U << shift))));
    sMap1B[i][2].set((lsb << 16) | (bpos << 13) | (ctx0 >> ((!lsb) * 2 + (3U << shift))));
    sMap1B[i][3].set((lsb << 16) | (bpos << 13) | (ctx1 >> ((!lsb) + (3U << shift))));
    sMap1B[i][0].mix(m);
    sMap1B[i][1].mix(m);
    sMap1B[i][2].mix(m);
    sMap1B[i][3].mix(m);
  }

  m.set((errLog << 9U) | (lsb << 8U) | c0, 8192);
  m.set((uint8_t(mask) << 4U) | (ch << 3U) | (lsb << 2U) | (bpos >> 1U), 4096);
  m.set((mxCtx << 7U) | (c1 >> 1U), 2560);
  m.set((errLog << 4U) | (ch << 3U) | (lsb << 2U) | (bpos >> 1U), 256);
  m.set(mxCtx, 20);
}
#ifndef PAQ8PX_IMAGEMODEL_HPP
#define PAQ8PX_IMAGEMODEL_HPP

class ImageModel {
public:
    static inline auto paeth(uint8_t const W, uint8_t const N, uint8_t const NW) -> uint8_t {
      int p = W + N - NW;
      int pW = abs(p - static_cast<int>(W));
      int pN = abs(p - static_cast<int>(N));
      int pNW = abs(p - static_cast<int>(NW));
      if( pW <= pN && pW <= pNW ) {
        return W;
      }
      if( pN <= pNW ) {
        return N;
      }
      return NW;
    }
};

#endif //PAQ8PX_IMAGEMODEL_HPP

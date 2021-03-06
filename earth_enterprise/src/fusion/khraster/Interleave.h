/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_INTERLEAVE_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_INTERLEAVE_H_

#include "fusion/khraster/khRasterTile.h"
#include "common/khCalcHelper.h"


// ****************************************************************************
// ***  PixelInterleaver - HelperClass
// ***
// ***    This class is a specialized template used during the extracting
// ***  of interleaved subtiles. The purpose of this template is to allow
// ***  efficient code inside the tight extract loop.
// ***    These calls will be completely inlined.
// ****************************************************************************
template <class T, uint numcomp>
class PixelInterleaver { };

// specialized for numcomp == 1
template <class T>
class PixelInterleaver<T, 1> {
 public:
  inline static void Interleave(T *destbuf, const T *const src[],
                                uint32 pos) {
    destbuf[0] = src[0][pos];
  }
};

// specialized for numcomp == 3
template <class T>
class PixelInterleaver<T, 3> {
 public:
  inline static void Interleave(T *destbuf, const T *const src[],
                                uint32 pos) {
    destbuf[0] = src[0][pos];
    destbuf[1] = src[1][pos];
    destbuf[2] = src[2][pos];
  }
};

// specialized for numcomp == 4
template <class T>
class PixelInterleaver<T, 4> {
 public:
  inline static void Interleave(T *destbuf, const T *const src[],
                                uint32 pos) {
    destbuf[0] = src[0][pos];
    destbuf[1] = src[1][pos];
    destbuf[2] = src[2][pos];
    destbuf[3] = src[3][pos];
  }
};

template <class T, class U, uint numcomp>
class PixelSeparator { };

// specialized for numcomp == 1
template <class T, class U>
class PixelSeparator<T, U, 1> {
 public:
  inline static void Separate(T *const dest[], const U *srcbuf,
                              uint32 pos) {
    dest[0][pos] = ClampRange<T>(srcbuf[0]);
  }
};

// specialized for numcomp == 3
template <class T, class U>
class PixelSeparator<T, U, 3> {
 public:
  inline static void Separate(T *const dest[], const U *srcbuf,
                              uint32 pos) {
    dest[0][pos] = ClampRange<T>(srcbuf[0]);
    dest[1][pos] = ClampRange<T>(srcbuf[1]);
    dest[2][pos] = ClampRange<T>(srcbuf[2]);
  }
};

// specialized for numcomp == 3
template <class T, class U>
class PixelSeparator<T, U, 4> {
 public:
  inline static void Separate(T *const dest[], const U *srcbuf,
                              uint32 pos) {
    dest[0][pos] = ClampRange<T>(srcbuf[0]);
    dest[1][pos] = ClampRange<T>(srcbuf[1]);
    dest[2][pos] = ClampRange<T>(srcbuf[2]);
    dest[4][pos] = ClampRange<T>(srcbuf[4]);
  }
};


// ****************************************************************************
// ***  ExtractAndInterleave
// ***
// ***  Extract a subpiece of a tile into an interleaved buffer. Possibly
// ***  change the orientation StartLowerLeft -> StartUpperLeft at the same
// ***  time.
// ****************************************************************************
template <class SrcTile>
inline void
ExtractAndInterleave(const SrcTile &src,
                     uint beginRow, uint beginCol,
                     const khSize<uint> &outTileSize,
                     TileOrientation outOri,
                     typename SrcTile::PixelType* outbuf,
                     uint32 outbuf_step = SrcTile::NumComp) {
  typedef PixelInterleaver<typename SrcTile::PixelType, SrcTile::NumComp>
    Interleaver;

  switch (outOri) {
    case StartUpperLeft:
      for (uint32 sy = beginRow + outTileSize.height - 1; sy >= beginRow;
           --sy) {
        uint32 rowoff = sy * SrcTile::TileWidth;
        for (uint32 sx = beginCol; sx < beginCol + outTileSize.width;
             ++sx) {
          uint32 coloff = rowoff + sx;
          Interleaver::Interleave(outbuf, src.bufs, coloff);
          outbuf += outbuf_step;
        }
        // don't wrap our unsigned int
        if (sy == 0) break;
      }
      break;
    case StartLowerLeft:
      for (uint32 sy = beginRow ; sy < beginRow + outTileSize.height;
           ++sy) {
        uint32 rowoff = sy * SrcTile::TileWidth;
        for (uint32 sx = beginCol; sx < beginCol + outTileSize.width;
             ++sx) {
          uint32 coloff = rowoff + sx;
          Interleaver::Interleave(outbuf, src.bufs, coloff);
          outbuf += outbuf_step;
        }
      }
      break;
  }
}



// ****************************************************************************
// ***  SeparateComponents
// ***
// ***  Separate an interleaved buffer. Possibly change the orientation
// ***  StartUpperLeft -> StartLowerLeft at the same time.
// ****************************************************************************
template <class DestTile, class SrcPixelType>
inline void
SeparateComponents(DestTile &dest,
                   const SrcPixelType* srcbuf,
                   TileOrientation srcOri) {
  typedef PixelSeparator<typename DestTile::PixelType,
    SrcPixelType, DestTile::NumComp> Separator;

  switch (srcOri) {
    case StartUpperLeft:
      for (uint32 sy = DestTile::TileHeight - 1; sy >= 0; --sy) {
        uint32 rowoff = sy * DestTile::TileWidth;
        for (uint32 sx = 0; sx < DestTile::TileWidth; ++sx) {
          uint32 coloff = rowoff + sx;
          Separator::Separate(dest.bufs, srcbuf, coloff);
          srcbuf += DestTile::NumComp;
        }
        // don't wrap our unsigned int
        if (sy == 0) break;
      }
      break;
    case StartLowerLeft:
      for (uint32 sy = 0 ; sy < DestTile::TileHeight; ++sy) {
        uint32 rowoff = sy * DestTile::TileWidth;
        for (uint32 sx = 0; sx < DestTile::TileWidth; sx++) {
          uint32 coloff = rowoff + sx;
          Separator::Separate(dest.bufs, srcbuf, coloff);
          srcbuf += DestTile::NumComp;
        }
      }
      break;
  }
}

template <class DestTile>
inline void
SeparateComponents(DestTile &dest, const void *srcbuf,
                   TileOrientation srcOri,
                   khTypes::StorageEnum srcType) {
  switch (srcType) {
    case khTypes::UInt8:
      SeparateComponents(dest, (const uint8 *)srcbuf, srcOri);
      break;
    case khTypes::Int8:
      SeparateComponents(dest, (const int8 *)srcbuf, srcOri);
      break;
    case khTypes::UInt16:
      SeparateComponents(dest, (const uint16 *)srcbuf, srcOri);
      break;
    case khTypes::Int16:
      SeparateComponents(dest, (const int16 *)srcbuf, srcOri);
      break;
    case khTypes::UInt32:
      SeparateComponents(dest, (const uint32 *)srcbuf, srcOri);
      break;
    case khTypes::Int32:
      SeparateComponents(dest, (const int32 *)srcbuf, srcOri);
      break;
    case khTypes::UInt64:
      SeparateComponents(dest, (const uint64 *)srcbuf, srcOri);
      break;
    case khTypes::Int64:
      SeparateComponents(dest, (const int64 *)srcbuf, srcOri);
      break;
    case khTypes::Float32:
      SeparateComponents(dest, (const float32 *)srcbuf, srcOri);
      break;
    case khTypes::Float64:
      SeparateComponents(dest, (const float64 *)srcbuf, srcOri);
      break;
  }
}


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_INTERLEAVE_H_

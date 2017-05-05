#pragma once
#include <stdint.h>

namespace SPU {

enum InterpolationType {
    NO_INTERPOLATION,
    SIMPLE_INTERPOLATION,
    GAUSS_INTERPOLATION,
    CUBIC_INTERPOLATION
};

}   // namespace SPU

#include "spu.h"

namespace SPU {

class SPUVoice;

class InterpolationBase {
public:
  virtual ~InterpolationBase() = default;

  virtual void Start() = 0;

  virtual InterpolationType GetInterpolationType() const = 0;
  bool IsSimple() const { return GetInterpolationType() == SIMPLE_INTERPOLATION; }
  bool IsGaussian() const { return GetInterpolationType() == GAUSS_INTERPOLATION; }
  bool IsCubic() const { return GetInterpolationType() == CUBIC_INTERPOLATION; }

  void SetSinc(uint32_t pitch);

  uint32_t GetSincPosition() const;
  uint32_t AddSincPosition(uint32_t dspos);
  uint32_t SubSincPosition(uint32_t dspos);
  uint32_t AdvanceSincPosition();

  void StoreValue(int fa);
  virtual int GetValue() const = 0;

protected:
  uint32_t GetSinc() const;
  virtual void storeVal(int fa) = 0;

  uint32_t spos;

private:
  uint32_t sinc;
  // int currSample; // used only for FM
};

class GaussianInterpolation: public InterpolationBase {
public:
  GaussianInterpolation() : gpos(0) {
    samples[0] = samples[1] = samples[2] = samples[3] = 0;
  }
  void Start();

  InterpolationType GetInterpolationType() const { return GAUSS_INTERPOLATION; }

protected:
  void storeVal(int fa);
  int GetValue() const;

private:
  int samples[4];
  uint32_t gpos;
};

class CubicInterpolation: public InterpolationBase {
public:
  CubicInterpolation() = default;
  void Start();

  InterpolationType GetInterpolationType() const { return CUBIC_INTERPOLATION; }

protected:
  void storeVal(int fa);
  int GetValue() const;

private:
  int samples[4];
  uint32_t gpos;
};

}   // namespace SPU

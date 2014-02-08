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

/*
  memo:

  Simple Interpolation:
    SB[29]: current sample slot
    SB[30]: next sample slot?
    SB[31]: next+1 sample slot?
    SB[32]: freq change flag

  Gauss/Cublic Interpolation:
    SB[28]: gpos?

  interpolate if spos < 0x10000L ?
*/

namespace SPU
{

class ChannelInfo;

class InterpolationBase
{
public:
    InterpolationBase();
    virtual ~InterpolationBase() {}

    virtual void Start();

    virtual InterpolationType GetInterpolationType() const = 0;
    bool IsSimple() const { return GetInterpolationType() == SIMPLE_INTERPOLATION; }
    bool IsGaussian() const { return GetInterpolationType() == GAUSS_INTERPOLATION; }
    bool IsCubic() const { return GetInterpolationType() == CUBIC_INTERPOLATION; }

    uint32_t GetSinc() const;
    void SetSinc(uint32_t pitch);

    void StoreValue(int fa);
    int GetValue();

protected:
    virtual void storeVal(int fa) = 0;
    virtual int getVal() const = 0;

public:
    uint32_t spos;

protected:
    uint32_t sinc;
    int currSample; // used only for FM
//    uint32_t nextSample;
//    uint32_t next2Sample;
};


inline uint32_t InterpolationBase::GetSinc() const { return sinc; }



class GaussianInterpolation: public InterpolationBase
{
public:
    GaussianInterpolation() {}
    void Start();

    InterpolationType GetInterpolationType() const { return GAUSS_INTERPOLATION; }

private:
    void storeVal(int fa);
    int getVal() const;

public:
    uint32_t gpos;

private:
    int samples[4];
};



class CubicInterpolation: public InterpolationBase
{
public:
    CubicInterpolation() {}
    void Start();

    InterpolationType GetInterpolationType() const { return CUBIC_INTERPOLATION; }

private:
    void storeVal(int fa);
    int getVal() const;

public:
    uint32_t gpos;

private:
    int samples[4];
};

}   // namespace SPU

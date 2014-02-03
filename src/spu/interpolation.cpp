#include "interpolation.h"


namespace {

  const int gauss_table[]={
    0x172, 0x519, 0x176, 0x000, 0x16E, 0x519, 0x17A, 0x000,
    0x16A, 0x518, 0x17D, 0x000, 0x166, 0x518, 0x181, 0x000,
    0x162, 0x518, 0x185, 0x000, 0x15F, 0x518, 0x189, 0x000,
    0x15B, 0x518, 0x18D, 0x000, 0x157, 0x517, 0x191, 0x000,
    0x153, 0x517, 0x195, 0x000, 0x150, 0x517, 0x19A, 0x000,
    0x14C, 0x516, 0x19E, 0x000, 0x148, 0x516, 0x1A2, 0x000,
    0x145, 0x515, 0x1A6, 0x000, 0x141, 0x514, 0x1AA, 0x000,
    0x13E, 0x514, 0x1AE, 0x000, 0x13A, 0x513, 0x1B2, 0x000,
    0x137, 0x512, 0x1B7, 0x001, 0x133, 0x511, 0x1BB, 0x001,
    0x130, 0x511, 0x1BF, 0x001, 0x12C, 0x510, 0x1C3, 0x001,
    0x129, 0x50F, 0x1C8, 0x001, 0x125, 0x50E, 0x1CC, 0x001,
    0x122, 0x50D, 0x1D0, 0x001, 0x11E, 0x50C, 0x1D5, 0x001,
    0x11B, 0x50B, 0x1D9, 0x001, 0x118, 0x50A, 0x1DD, 0x001,
    0x114, 0x508, 0x1E2, 0x001, 0x111, 0x507, 0x1E6, 0x002,
    0x10E, 0x506, 0x1EB, 0x002, 0x10B, 0x504, 0x1EF, 0x002,
    0x107, 0x503, 0x1F3, 0x002, 0x104, 0x502, 0x1F8, 0x002,
    0x101, 0x500, 0x1FC, 0x002, 0x0FE, 0x4FF, 0x201, 0x002,
    0x0FB, 0x4FD, 0x205, 0x003, 0x0F8, 0x4FB, 0x20A, 0x003,
    0x0F5, 0x4FA, 0x20F, 0x003, 0x0F2, 0x4F8, 0x213, 0x003,
    0x0EF, 0x4F6, 0x218, 0x003, 0x0EC, 0x4F5, 0x21C, 0x004,
    0x0E9, 0x4F3, 0x221, 0x004, 0x0E6, 0x4F1, 0x226, 0x004,
    0x0E3, 0x4EF, 0x22A, 0x004, 0x0E0, 0x4ED, 0x22F, 0x004,
    0x0DD, 0x4EB, 0x233, 0x005, 0x0DA, 0x4E9, 0x238, 0x005,
    0x0D7, 0x4E7, 0x23D, 0x005, 0x0D4, 0x4E5, 0x241, 0x005,
    0x0D2, 0x4E3, 0x246, 0x006, 0x0CF, 0x4E0, 0x24B, 0x006,
    0x0CC, 0x4DE, 0x250, 0x006, 0x0C9, 0x4DC, 0x254, 0x006,
    0x0C7, 0x4D9, 0x259, 0x007, 0x0C4, 0x4D7, 0x25E, 0x007,
    0x0C1, 0x4D5, 0x263, 0x007, 0x0BF, 0x4D2, 0x267, 0x008,
    0x0BC, 0x4D0, 0x26C, 0x008, 0x0BA, 0x4CD, 0x271, 0x008,
    0x0B7, 0x4CB, 0x276, 0x009, 0x0B4, 0x4C8, 0x27B, 0x009,
    0x0B2, 0x4C5, 0x280, 0x009, 0x0AF, 0x4C3, 0x284, 0x00A,
    0x0AD, 0x4C0, 0x289, 0x00A, 0x0AB, 0x4BD, 0x28E, 0x00A,
    0x0A8, 0x4BA, 0x293, 0x00B, 0x0A6, 0x4B7, 0x298, 0x00B,
    0x0A3, 0x4B5, 0x29D, 0x00B, 0x0A1, 0x4B2, 0x2A2, 0x00C,
    0x09F, 0x4AF, 0x2A6, 0x00C, 0x09C, 0x4AC, 0x2AB, 0x00D,
    0x09A, 0x4A9, 0x2B0, 0x00D, 0x098, 0x4A6, 0x2B5, 0x00E,
    0x096, 0x4A2, 0x2BA, 0x00E, 0x093, 0x49F, 0x2BF, 0x00F,
    0x091, 0x49C, 0x2C4, 0x00F, 0x08F, 0x499, 0x2C9, 0x00F,
    0x08D, 0x496, 0x2CE, 0x010, 0x08B, 0x492, 0x2D3, 0x010,
    0x089, 0x48F, 0x2D8, 0x011, 0x086, 0x48C, 0x2DC, 0x011,
    0x084, 0x488, 0x2E1, 0x012, 0x082, 0x485, 0x2E6, 0x013,
    0x080, 0x481, 0x2EB, 0x013, 0x07E, 0x47E, 0x2F0, 0x014,
    0x07C, 0x47A, 0x2F5, 0x014, 0x07A, 0x477, 0x2FA, 0x015,
    0x078, 0x473, 0x2FF, 0x015, 0x076, 0x470, 0x304, 0x016,
    0x075, 0x46C, 0x309, 0x017, 0x073, 0x468, 0x30E, 0x017,
    0x071, 0x465, 0x313, 0x018, 0x06F, 0x461, 0x318, 0x018,
    0x06D, 0x45D, 0x31D, 0x019, 0x06B, 0x459, 0x322, 0x01A,
    0x06A, 0x455, 0x326, 0x01B, 0x068, 0x452, 0x32B, 0x01B,
    0x066, 0x44E, 0x330, 0x01C, 0x064, 0x44A, 0x335, 0x01D,
    0x063, 0x446, 0x33A, 0x01D, 0x061, 0x442, 0x33F, 0x01E,
    0x05F, 0x43E, 0x344, 0x01F, 0x05E, 0x43A, 0x349, 0x020,
    0x05C, 0x436, 0x34E, 0x020, 0x05A, 0x432, 0x353, 0x021,
    0x059, 0x42E, 0x357, 0x022, 0x057, 0x42A, 0x35C, 0x023,
    0x056, 0x425, 0x361, 0x024, 0x054, 0x421, 0x366, 0x024,
    0x053, 0x41D, 0x36B, 0x025, 0x051, 0x419, 0x370, 0x026,
    0x050, 0x415, 0x374, 0x027, 0x04E, 0x410, 0x379, 0x028,
    0x04D, 0x40C, 0x37E, 0x029, 0x04C, 0x408, 0x383, 0x02A,
    0x04A, 0x403, 0x388, 0x02B, 0x049, 0x3FF, 0x38C, 0x02C,
    0x047, 0x3FB, 0x391, 0x02D, 0x046, 0x3F6, 0x396, 0x02E,
    0x045, 0x3F2, 0x39B, 0x02F, 0x043, 0x3ED, 0x39F, 0x030,
    0x042, 0x3E9, 0x3A4, 0x031, 0x041, 0x3E5, 0x3A9, 0x032,
    0x040, 0x3E0, 0x3AD, 0x033, 0x03E, 0x3DC, 0x3B2, 0x034,
    0x03D, 0x3D7, 0x3B7, 0x035, 0x03C, 0x3D2, 0x3BB, 0x036,
    0x03B, 0x3CE, 0x3C0, 0x037, 0x03A, 0x3C9, 0x3C5, 0x038,
    0x038, 0x3C5, 0x3C9, 0x03A, 0x037, 0x3C0, 0x3CE, 0x03B,
    0x036, 0x3BB, 0x3D2, 0x03C, 0x035, 0x3B7, 0x3D7, 0x03D,
    0x034, 0x3B2, 0x3DC, 0x03E, 0x033, 0x3AD, 0x3E0, 0x040,
    0x032, 0x3A9, 0x3E5, 0x041, 0x031, 0x3A4, 0x3E9, 0x042,
    0x030, 0x39F, 0x3ED, 0x043, 0x02F, 0x39B, 0x3F2, 0x045,
    0x02E, 0x396, 0x3F6, 0x046, 0x02D, 0x391, 0x3FB, 0x047,
    0x02C, 0x38C, 0x3FF, 0x049, 0x02B, 0x388, 0x403, 0x04A,
    0x02A, 0x383, 0x408, 0x04C, 0x029, 0x37E, 0x40C, 0x04D,
    0x028, 0x379, 0x410, 0x04E, 0x027, 0x374, 0x415, 0x050,
    0x026, 0x370, 0x419, 0x051, 0x025, 0x36B, 0x41D, 0x053,
    0x024, 0x366, 0x421, 0x054, 0x024, 0x361, 0x425, 0x056,
    0x023, 0x35C, 0x42A, 0x057, 0x022, 0x357, 0x42E, 0x059,
    0x021, 0x353, 0x432, 0x05A, 0x020, 0x34E, 0x436, 0x05C,
    0x020, 0x349, 0x43A, 0x05E, 0x01F, 0x344, 0x43E, 0x05F,
    0x01E, 0x33F, 0x442, 0x061, 0x01D, 0x33A, 0x446, 0x063,
    0x01D, 0x335, 0x44A, 0x064, 0x01C, 0x330, 0x44E, 0x066,
    0x01B, 0x32B, 0x452, 0x068, 0x01B, 0x326, 0x455, 0x06A,
    0x01A, 0x322, 0x459, 0x06B, 0x019, 0x31D, 0x45D, 0x06D,
    0x018, 0x318, 0x461, 0x06F, 0x018, 0x313, 0x465, 0x071,
    0x017, 0x30E, 0x468, 0x073, 0x017, 0x309, 0x46C, 0x075,
    0x016, 0x304, 0x470, 0x076, 0x015, 0x2FF, 0x473, 0x078,
    0x015, 0x2FA, 0x477, 0x07A, 0x014, 0x2F5, 0x47A, 0x07C,
    0x014, 0x2F0, 0x47E, 0x07E, 0x013, 0x2EB, 0x481, 0x080,
    0x013, 0x2E6, 0x485, 0x082, 0x012, 0x2E1, 0x488, 0x084,
    0x011, 0x2DC, 0x48C, 0x086, 0x011, 0x2D8, 0x48F, 0x089,
    0x010, 0x2D3, 0x492, 0x08B, 0x010, 0x2CE, 0x496, 0x08D,
    0x00F, 0x2C9, 0x499, 0x08F, 0x00F, 0x2C4, 0x49C, 0x091,
    0x00F, 0x2BF, 0x49F, 0x093, 0x00E, 0x2BA, 0x4A2, 0x096,
    0x00E, 0x2B5, 0x4A6, 0x098, 0x00D, 0x2B0, 0x4A9, 0x09A,
    0x00D, 0x2AB, 0x4AC, 0x09C, 0x00C, 0x2A6, 0x4AF, 0x09F,
    0x00C, 0x2A2, 0x4B2, 0x0A1, 0x00B, 0x29D, 0x4B5, 0x0A3,
    0x00B, 0x298, 0x4B7, 0x0A6, 0x00B, 0x293, 0x4BA, 0x0A8,
    0x00A, 0x28E, 0x4BD, 0x0AB, 0x00A, 0x289, 0x4C0, 0x0AD,
    0x00A, 0x284, 0x4C3, 0x0AF, 0x009, 0x280, 0x4C5, 0x0B2,
    0x009, 0x27B, 0x4C8, 0x0B4, 0x009, 0x276, 0x4CB, 0x0B7,
    0x008, 0x271, 0x4CD, 0x0BA, 0x008, 0x26C, 0x4D0, 0x0BC,
    0x008, 0x267, 0x4D2, 0x0BF, 0x007, 0x263, 0x4D5, 0x0C1,
    0x007, 0x25E, 0x4D7, 0x0C4, 0x007, 0x259, 0x4D9, 0x0C7,
    0x006, 0x254, 0x4DC, 0x0C9, 0x006, 0x250, 0x4DE, 0x0CC,
    0x006, 0x24B, 0x4E0, 0x0CF, 0x006, 0x246, 0x4E3, 0x0D2,
    0x005, 0x241, 0x4E5, 0x0D4, 0x005, 0x23D, 0x4E7, 0x0D7,
    0x005, 0x238, 0x4E9, 0x0DA, 0x005, 0x233, 0x4EB, 0x0DD,
    0x004, 0x22F, 0x4ED, 0x0E0, 0x004, 0x22A, 0x4EF, 0x0E3,
    0x004, 0x226, 0x4F1, 0x0E6, 0x004, 0x221, 0x4F3, 0x0E9,
    0x004, 0x21C, 0x4F5, 0x0EC, 0x003, 0x218, 0x4F6, 0x0EF,
    0x003, 0x213, 0x4F8, 0x0F2, 0x003, 0x20F, 0x4FA, 0x0F5,
    0x003, 0x20A, 0x4FB, 0x0F8, 0x003, 0x205, 0x4FD, 0x0FB,
    0x002, 0x201, 0x4FF, 0x0FE, 0x002, 0x1FC, 0x500, 0x101,
    0x002, 0x1F8, 0x502, 0x104, 0x002, 0x1F3, 0x503, 0x107,
    0x002, 0x1EF, 0x504, 0x10B, 0x002, 0x1EB, 0x506, 0x10E,
    0x002, 0x1E6, 0x507, 0x111, 0x001, 0x1E2, 0x508, 0x114,
    0x001, 0x1DD, 0x50A, 0x118, 0x001, 0x1D9, 0x50B, 0x11B,
    0x001, 0x1D5, 0x50C, 0x11E, 0x001, 0x1D0, 0x50D, 0x122,
    0x001, 0x1CC, 0x50E, 0x125, 0x001, 0x1C8, 0x50F, 0x129,
    0x001, 0x1C3, 0x510, 0x12C, 0x001, 0x1BF, 0x511, 0x130,
    0x001, 0x1BB, 0x511, 0x133, 0x001, 0x1B7, 0x512, 0x137,
    0x000, 0x1B2, 0x513, 0x13A, 0x000, 0x1AE, 0x514, 0x13E,
    0x000, 0x1AA, 0x514, 0x141, 0x000, 0x1A6, 0x515, 0x145,
    0x000, 0x1A2, 0x516, 0x148, 0x000, 0x19E, 0x516, 0x14C,
    0x000, 0x19A, 0x517, 0x150, 0x000, 0x195, 0x517, 0x153,
    0x000, 0x191, 0x517, 0x157, 0x000, 0x18D, 0x518, 0x15B,
    0x000, 0x189, 0x518, 0x15F, 0x000, 0x185, 0x518, 0x162,
    0x000, 0x181, 0x518, 0x166, 0x000, 0x17D, 0x518, 0x16A,
    0x000, 0x17A, 0x519, 0x16E, 0x000, 0x176, 0x519, 0x172
  };

}   // namespace


namespace SPU {

  InterpolationBase::InterpolationBase() {}

  void InterpolationBase::Start() {}

  void InterpolationBase::SetSinc(uint32_t pitch)
  {
    sinc = (pitch << 4);
    if (sinc == 0) sinc = 1;
  }

  void InterpolationBase::StoreValue(int fa)
  {
    if (fa > 32767) fa = 32767;
    if (fa < -32768) fa = -32768;
    storeVal(fa);
  }

  int InterpolationBase::GetValue()
  {
    return getVal();
  }


  void GaussianInterpolation::Start()
  {
    InterpolationBase::Start();
    spos = 0x30000;
    gpos = 0;
    for (int i = 0; i < 4; i++) {
      samples[i] = 0;
    }
  }


  void GaussianInterpolation::storeVal(int fa)
  {
    samples[gpos] = fa;
    gpos = (gpos+1) & 3;
  }


  int GaussianInterpolation::getVal() const
  {
    const int gval0 = samples[gpos];
    const int gval1 = samples[(gpos+1)&3];
    const int gval2 = samples[(gpos+2)&3];
    const int gval3 = samples[(gpos+3)&3];
    int vl = (spos >> 6) & ~3;
    int vr = (gauss_table[vl] * gval0) >> 9;
    vr += (gauss_table[vl+1] * gval1) >> 9;
    vr += (gauss_table[vl+2] * gval2) >> 9;
    vr += (gauss_table[vl+3] * gval3) >> 9;
    return vr >> 2;
  }


  void CubicInterpolation::Start()
  {
    InterpolationBase::Start();
    spos = 0x30000;
    gpos = 0;
    for (int i = 0; i < 4; i++) {
      samples[i] = 0;
    }
  }


  void CubicInterpolation::storeVal(int fa)
  {
    samples[gpos] = fa;
    gpos = (gpos+1) & 3;
  }


  int CubicInterpolation::getVal() const
  {
    const int gval0 = samples[gpos];
    const int gval1 = samples[(gpos+1)&3];
    const int gval2 = samples[(gpos+2)&3];
    const int gval3 = samples[(gpos+3)&3];
    const int xd = (spos >> 1) + 1;

    int fa = gval3 - 3*gval2 + 3*gval1 - gval0;
    fa *= (xd - (2<<15)) / 6;
    fa >>= 15;
    fa += gval2 - 2*gval1 + gval0;
    fa *= (xd - (1<<15)) >> 1;
    fa >>= 15;
    fa += gval1 - gval0;
    fa *= xd;
    fa >>= 15;
    fa = fa + gval0;

    return fa;
  }


}   // namespace SPU

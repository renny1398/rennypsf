#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef __WXDEBUG__
#define wxNEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define wxNEW new
#endif

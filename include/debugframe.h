#pragma once
#include <wx/frame.h>

class PSF;

class DebugFrame : public wxFrame
{
public:
    DebugFrame(wxFrame *parent, const PSF* psf);

private:
    PSF* psf_;

};

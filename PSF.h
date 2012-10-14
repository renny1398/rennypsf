#pragma once

#include "SoundFormat.h"
#include "psx/psx.h"
#include <stdint.h>

class PSF: public SoundFormat
{
    struct PSXEXEHeader {
        char signature[8];
        uint32_t text;
        uint32_t data;
        uint32_t pc0;
        uint32_t gp0;
        uint32_t text_addr;
        uint32_t text_size;
        uint32_t d_addr;
        uint32_t d_size;
        uint32_t b_addr;
        uint32_t b_size;
        // uint32_t S_addr;
        uint32_t sp0;
        uint32_t s_size;
        uint32_t SP;
        uint32_t FP;
        uint32_t GP;
        uint32_t RA;
        uint32_t S0;
        char marker[0xB4];
    };

public:
    virtual ~PSF() {}

    static void Init();
    static void Reset();

    bool Play();
    // bool Pause();
    bool Stop();
    // bool Seek();
    // bool Tell();

    friend class PSFLoader;
    friend class PSF1Loader;

private:
    PSXEXEHeader m_header;
    void *m_memory;
    PSX::R3000A::InterpreterThread *m_thread;
};

#include <wx/file.h>

class PreloadedPSF: public PreloadedSoundFormat
{
public:
    virtual ~PreloadedPSF() {}

    friend class PSFLoader;
    friend class PSF1Loader;

protected:
    wxString m_path;
    uint32_t m_version;
    wxFileOffset m_ofsReservedArea;
    wxFileOffset m_ofsBinary;
    uint32_t m_lenReservedArea;
    uint32_t m_lenBinary;
    uint32_t m_crc32Binary;
};

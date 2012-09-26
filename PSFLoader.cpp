#include "PSFLoader.h"
#include "PSF.h"
#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/file.h>
#include <wx/regex.h>
#include <cstring>


PSFLoader::~PSFLoader()
{
    if (m_preloaded_psf) {
        delete m_preloaded_psf;
    }
}

const wxString &PSFLoader::GetPath() const {
    return m_path;
}



SoundFormat *PSFLoader::Preload(const wxString& path)
{
    wxFile file(path);
    if (file.IsOpened() == false) {
        return 0;
    }

    char signature[4];
    file.Read(signature, 4);
    if (memcmp(signature, "PSF", 3) != 0) {
        signature[3] = 0;
        wxMessageOutputDebug().Printf(wxT("This file is not PSF format. (signature = ") + wxString(signature) + wxT(')'));
        return 0;
    }

    PreloadedPSF *preloaded_psf = new PreloadedPSF();
    preloaded_psf->m_version = signature[3];

    file.Read(&preloaded_psf->m_lenReservedArea, 4);
    file.Read(&preloaded_psf->m_lenBinary, 4);
    file.Read(&preloaded_psf->m_crc32Binary, 4);

    preloaded_psf->m_ofsReservedArea = file.Tell();
    file.Seek(preloaded_psf->m_lenReservedArea, wxFromCurrent);

    preloaded_psf->m_ofsBinary = file.Tell();
    file.Seek(preloaded_psf->m_lenBinary, wxFromCurrent);

    if (file.Eof() == false) {
        char strTAG[5];
        file.Read(strTAG, 5);
        if (memcmp(strTAG, "[TAG]", 5) == 0) {

                wxFileOffset len = file.Length() - file.Tell();
                char *tags = new char[len+1];
                len = file.Read(tags, len);
                tags[len] = 0;

                AddTags(preloaded_psf, tags);

 /*               wxStringTokenizer tokenizer_lf(tags, wxT('\n'));
                wxRegEx regex_tag(wxT("(\\s*)\\w+(\\s*)=(\\s*)\\w*"), wxRE_ICASE | wxRE_NOSUB);
                wxString comment;

                delete [] tags;

                while (tokenizer_lf.HasMoreTokens())
                {
                    wxString token = tokenizer_lf.GetNextToken();
                    wxMessageOutputDebug().Printf(token);
                    wxString tag = regex_tag.GetMatch(token);
                    wxMessageOutputDebug().Printf(tag);
                    wxStringTokenizer tokenizer_eq(tag, wxT('='));
                    wxString key = tokenizer_eq.NextToken();
                    if (key.IsEmpty()) continue;
                    wxString value = tokenizer_eq.NextToken();
                    if (key.CmpNoCase(wxT("comment")) == 0) {
                        comment += value + '\n';
                    } else {
                        preloaded_psf->SetTag(key, value);
                    }
                }
                */
        }
    }

    file.Close();
    m_preloaded_psf = preloaded_psf;
    m_path = path;
    return preloaded_psf;
}


void PSFLoader::AddTags(PreloadedPSF *psf, char *buff)
{
    char *p;
    const char *q;
    //            The tag is to be parsed as follows:

    //            - All characters 0x01-0x20 are considered whitespace
    //            - There must be no null (0x00) characters; behavior is undefined if a null
    //              byte is present
    //            - 0x0A is the newline character
    //            - Additional lines of the form "variable=value" may follow
    //            - Variable names are case-insensitive and must be valid C identifiers
    //            - Whitespace at the beginning/end of the line and before/after the = are
    //              ignored
    //            - Blank lines are ignored
    //            - Multiple-line variables must appear as consecutive lines using the same
    //              variable name.

    p = buff;
    while (*p != 0) {
        if (0x01 <= *p && *p < 0x20 && *p != 0x0a) *p = 0x20;
        ++p;
    }

    wxString comment;

    do {
        wxString key, value;
        // step1: skip space
        while (*buff == 0x20) ++buff;
        // step2: get key
        p = buff;
        while (*buff != 0x20 && *buff != '=') ++buff;
        q = buff;
        key.assign(p, q);
        // step3: skip space and equal
        while (*buff == 0x20 || *buff == '=') ++buff;
        // step4: get value
        p = buff;
        while (*buff != '\n' && *buff != 0) ++buff;
        q = buff;
        value.assign(p, q);
        // step5: register (key, value)
        if (key.CmpNoCase("comment") != 0) {
            psf->SetTag(key, value);
        } else {
            if (!comment.IsEmpty()) comment += '\n';
            comment.assign(value);
        }
        // step6: continue or break
        if (*buff == 0) break;
        ++buff;
    } while (*buff != 0);
    psf->SetTag(wxT("comment"), comment);
}

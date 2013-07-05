#pragma once
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <wx/vector.h>

#include <wx/event.h>
#include <wx/thread.h>
#include <wx/msgqueue.h>


// for wxThreadEvent

struct NoteInfo {
  bool is_on;
  int tone_number_;
  float pitch;
  float velocity;
};



wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_NOTE_ON, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_NOTE_OFF, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_CHANGE_TONE_NUMBER, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_CHANGE_PITCH, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_CHANGE_VELOCITY, wxCommandEvent);


class Sample {
public:
    short left;
    short right;

    Sample() : left(0), right(0) {}
    Sample(short l, short r) : left(l), right(r) {}

    operator short*() { return reinterpret_cast<short*>(this); }
};



class SoundFormat;

class SoundDriver : public wxEvtHandler
{
public:
    SoundDriver(int channelNumber);
    virtual ~SoundDriver();

    virtual bool Play(SoundFormat*);
    virtual bool Stop();

    // void WriteBuffer(unsigned char * pSound, int lBytes);
    // virtual void Write(int left, int right) = 0;
    void WriteStereo(int ch, short left, short right);
    void WriteStereo(int ch, short samples[2]);

    void Flush();

    // int GetChannelNumber() const;

    void AddListener(wxEvtHandler* listener, int ch);

    // deprecated
    virtual int GetEnvelopeVolume(int ch) const;
    virtual void SetEnvelopeVolume(int ch, int vol);

protected:
    const Sample* GetBuffer(int* size) const;

    void setChannelNumber(int number);
    void setBufferSize(int size);
    virtual void WriteToDevice() = 0;

    void OnNoteOn(wxCommandEvent& event);
    void OnNoteOff(wxCommandEvent& event);
    void OnChangeToneNumber(wxCommandEvent& event);
    void OnChangePitch(wxCommandEvent& event);
    void OnChangeVelocity(wxCommandEvent& event);
    void Notify(wxCommandEvent &event);

protected:
    SoundFormat *m_sound;

private:
    int leftSample_, rightSample_;
    Sample* chSample_;
    int *chEnvelope_;
    // int *chEnvelopeLeft_, *chEnvelopeRight_;
    int channelNumber_;
    Sample* buffer_;
    int bufferSize_;
    int bufferIndex_;

    wxVector< wxVector<wxEvtHandler*> > listeners_;
};



class WaveOutAL;
class WaveOutALCommand;

class WaveOutALThread : public wxThread {
public:
    WaveOutALThread(WaveOutAL* sound_driver)
        : wxThread(wxTHREAD_DETACHED), sound_driver_(sound_driver)
    {}

    void PostMessageQueue(WaveOutALCommand* command) {
        command_queue_.Post(command);
    }

protected:
    virtual ExitCode Entry();

private:
    WaveOutAL* const sound_driver_;
    wxMessageQueue<WaveOutALCommand*> command_queue_;
};




class WaveOutAL : public SoundDriver
{
    friend class WaveOutALThread;
    friend class WaveOutALCommand;

public:
    WaveOutAL(int channelNumber);
    ~WaveOutAL();

    void Init();
    virtual bool Stop();
    void Shutdown();

    void ThisThreadInit();
    bool ThisThreadStop();
    void ThisThreadShutdown();

    virtual void WriteToDevice();
    void ThisThreadWriteToDevice();
protected:
    void WaitForWritingToDevice();

private:
    // bool is_multithreading_; // force

    static ALCdevice *device_;
    static ALCcontext *context_;
    static int source_number_;

    static WaveOutALThread* thread_;

    ALuint buffer_, source_;
    wxMutex mutex_, mutex2_;
    wxCondition write_to_device_cond_;
    wxCondition write_to_device_cond2_;
    bool finished_writing_;
};


#include <wx/file.h>

class WaveOutDisk: public SoundDriver
{
public:
    WaveOutDisk(int channelNumber);

    bool Play(SoundFormat *);
    bool Stop();

private:
    struct WaveOutFormat
    {
        WaveOutFormat();
        char RIFF[4];
        int size;
        char WAVE[4];
        char fmt[4];
        int fmtSize;
        short formatId;
        short channelNumber;
        int samplingRate;
        int dataRate;
        short blockSize;
        short bitNumber;
        char DATA[4];
        int dataSize;
    };

    wxString fileName_;
    wxFile file_;
    WaveOutFormat format_;
};

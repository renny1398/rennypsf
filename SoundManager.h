#pragma once

class SoundFormat;

class SoundManager
{
public:
    SoundManager();

    bool Play(SoundFormat*);
    bool Stop();

private:
    SoundFormat *m_sound;
};

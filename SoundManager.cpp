#include "SoundManager.h"
#include "SoundFormat.h"

SoundManager::SoundManager(): m_sound(0)
{
}


bool SoundManager::Play(SoundFormat *sound)
{
    if (m_sound) {
        m_sound->Stop();
    }
    if (sound->Play()) {
        m_sound = sound;
        return true;
    }
    return false;
}


bool SoundManager::Stop()
{
    SoundFormat *sound = m_sound;
    if (sound == 0) return true;
    if (sound->Stop()) {
        m_sound = 0;
        return true;
    }
    return false;
}

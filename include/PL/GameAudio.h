#pragma once

#include "raylib.h"

enum class SoundCue
{
    Step, Hammer, Cracking, Collapse, Lava, Damage, Spikes, Collect,
    Achievement, Medal, Victory, Death, Respawn, Menu, Potion, Count
};

class GameAudio
{
public:
    GameAudio();
    ~GameAudio();
    GameAudio(const GameAudio& other) = delete;
    GameAudio& operator=(const GameAudio& other) = delete;

    void update(int musicVolume, int effectsVolume, bool musicMuted, bool effectsMuted);
    void play(SoundCue cue);

private:
    static constexpr int cueCount = static_cast<int>(SoundCue::Count);
    Sound sounds[cueCount] = {};
    Music music = {};
    bool ownsDevice = false;
    bool assetsLoaded = false;
    float musicGain = -1.0f;
    float effectsGain = -1.0f;

    void openDevice();
    void loadAssets();

#if defined(__EMSCRIPTEN__)
    void bindBrowserAudio();
    void unbindBrowserAudio();
    void unlockBrowserAudio();
    bool browserInputBound = false;
#endif
};

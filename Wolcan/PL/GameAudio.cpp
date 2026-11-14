#include "GameAudio.h"

#include <algorithm>
#include <string>

#if defined(__EMSCRIPTEN__)
#include <emscripten/html5.h>
#endif

GameAudio::GameAudio()
{
#if defined(__EMSCRIPTEN__)
    bindBrowserAudio();
#else
    openDevice();
#endif
}

GameAudio::~GameAudio()
{
#if defined(__EMSCRIPTEN__)
    unbindBrowserAudio();
#endif
    for (int i = 0; i < cueCount; i++)
    {
        if (IsSoundValid(sounds[i])) { UnloadSound(sounds[i]); }
    }
    if (IsMusicValid(music)) { UnloadMusicStream(music); }
    if (ownsDevice) { CloseAudioDevice(); }
}

void GameAudio::openDevice()
{
    if (IsAudioDeviceReady()) { return; }
    InitAudioDevice();
    ownsDevice = IsAudioDeviceReady();
}

#if defined(__EMSCRIPTEN__)
void GameAudio::bindBrowserAudio()
{
    // Device startup must happen during the input event itself.
    const char* target = EMSCRIPTEN_EVENT_TARGET_DOCUMENT;
    emscripten_set_mousedown_callback(target, this, true,
        [](int, const EmscriptenMouseEvent*, void* userData) -> EM_BOOL
        {
            static_cast<GameAudio*>(userData)->unlockBrowserAudio();
            return EM_FALSE;
        });
    emscripten_set_keydown_callback(target, this, true,
        [](int, const EmscriptenKeyboardEvent*, void* userData) -> EM_BOOL
        {
            static_cast<GameAudio*>(userData)->unlockBrowserAudio();
            return EM_FALSE;
        });
    emscripten_set_touchend_callback(target, this, true,
        [](int, const EmscriptenTouchEvent*, void* userData) -> EM_BOOL
        {
            static_cast<GameAudio*>(userData)->unlockBrowserAudio();
            return EM_FALSE;
        });
    browserInputBound = true;
}

void GameAudio::unbindBrowserAudio()
{
    if (!browserInputBound) { return; }
    const char* target = EMSCRIPTEN_EVENT_TARGET_DOCUMENT;
    emscripten_set_mousedown_callback(target, nullptr, true, nullptr);
    emscripten_set_keydown_callback(target, nullptr, true, nullptr);
    emscripten_set_touchend_callback(target, nullptr, true, nullptr);
    browserInputBound = false;
}

void GameAudio::unlockBrowserAudio()
{
    bool canStart = EM_ASM_INT({
        return !navigator.userActivation || navigator.userActivation.isActive;
    }) != 0;
    if (!canStart) { return; }

    openDevice();
    if (IsAudioDeviceReady()) { unbindBrowserAudio(); }
}
#endif

void GameAudio::loadAssets()
{
    constexpr const char* cueFiles[] = {
        "step.wav", "hammer.wav", "cracking.wav", "collapse.wav",
        "lava.wav", "damage.wav", "spikes.wav", "collect.wav",
        "achievement.wav", "medal.wav", "victory.wav", "death.wav",
        "respawn.wav", "menu.wav"
    };
    static_assert(sizeof(cueFiles) / sizeof(cueFiles[0]) == cueCount);
#if defined(__EMSCRIPTEN__)
    const std::string audioRoot = "/assets/audio/";
#else
    const std::string audioRoot = std::string(GetApplicationDirectory()) + "assets/audio/";
#endif
    music = LoadMusicStream((audioRoot + "music.wav").c_str());
    music.looping = true;
    for (int i = 0; i < cueCount; i++)
    {
        sounds[i] = LoadSound((audioRoot + cueFiles[i]).c_str());
    }
    assetsLoaded = true;
}

void GameAudio::update(int musicVolume, int effectsVolume, bool musicMuted, bool effectsMuted)
{
    if (!IsAudioDeviceReady()) { return; }

    if (!assetsLoaded) { loadAssets(); }
    float nextMusicGain = musicMuted ? 0.0f : static_cast<float>(std::clamp(musicVolume, 0, 100)) / 100.0f;
    float nextEffectsGain = effectsMuted ? 0.0f : static_cast<float>(std::clamp(effectsVolume, 0, 100)) / 100.0f;

    if (IsMusicValid(music))
    {
        if (musicGain != nextMusicGain) { SetMusicVolume(music, nextMusicGain); }
        UpdateMusicStream(music);
        if (!IsMusicStreamPlaying(music)) { PlayMusicStream(music); }
    }
    if (effectsGain != nextEffectsGain)
    {
        for (int i = 0; i < cueCount; i++)
        {
            if (IsSoundValid(sounds[i])) { SetSoundVolume(sounds[i], nextEffectsGain); }
        }
    }
    musicGain = nextMusicGain;
    effectsGain = nextEffectsGain;
}

void GameAudio::play(SoundCue cue)
{
    int index = static_cast<int>(cue);
    if (index < 0 || index >= cueCount || effectsGain <= 0.0f) { return; }
    if (IsSoundValid(sounds[index])) { PlaySound(sounds[index]); }
}

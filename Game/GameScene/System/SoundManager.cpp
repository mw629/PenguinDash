#include "SoundManager.h"
#include "../GameSceneManager.h"
#include <Resource/Audio.h>
#include <cmath>
#include <algorithm>

SoundManager* SoundManager::GetInstance() {
    static SoundManager instance;
    return &instance;
}

void SoundManager::Initialize() {
    if (isInitialized_) return;

    // BGMの読み込み
    bgmHandles_[static_cast<int>(BGM::Title)] = Audio::Load("Resources/Audio/BGM/BGM_Title.mp3");
    bgmHandles_[static_cast<int>(BGM::Play)]  = Audio::Load("Resources/Audio/BGM/BGM_Play.mp3");
    bgmHandles_[static_cast<int>(BGM::Boss)]  = Audio::Load("Resources/Audio/BGM/BGM_Boss.mp3");

    // SEの読み込み
    seHandles_[static_cast<int>(SE::Jump)]         = Audio::Load("Resources/Audio/SE/SE_Jump.wav");
    seHandles_[static_cast<int>(SE::Land)]         = Audio::Load("Resources/Audio/SE/SE_Land.wav");
    seHandles_[static_cast<int>(SE::Slide)]        = Audio::Load("Resources/Audio/SE/SE_Slide.wav");
    seHandles_[static_cast<int>(SE::LaneChange)]   = Audio::Load("Resources/Audio/SE/SE_LaneChange.wav");
    seHandles_[static_cast<int>(SE::ItemGet)]      = Audio::Load("Resources/Audio/SE/SE_ItemGet.wav");
    seHandles_[static_cast<int>(SE::Barrier)]      = Audio::Load("Resources/Audio/SE/SE_Barrier.wav");
    seHandles_[static_cast<int>(SE::BarrierBreak)] = Audio::Load("Resources/Audio/SE/SE_BarrierBreak.wav");
    seHandles_[static_cast<int>(SE::BonusHit)]     = Audio::Load("Resources/Audio/SE/SE_BonusHit.wav");
    seHandles_[static_cast<int>(SE::ClearBomb)]    = Audio::Load("Resources/Audio/SE/SE_ClearBomb.wav");
    seHandles_[static_cast<int>(SE::Crash)]        = Audio::Load("Resources/Audio/SE/SE_Crash.wav");
    seHandles_[static_cast<int>(SE::BossReflect)]  = Audio::Load("Resources/Audio/SE/SE_BossReflect.wav");
    seHandles_[static_cast<int>(SE::BossLand)]     = Audio::Load("Resources/Audio/SE/SE_BossLand.wav");
    seHandles_[static_cast<int>(SE::BossHit)]      = Audio::Load("Resources/Audio/SE/SE_BossDamage.wav");
    seHandles_[static_cast<int>(SE::BossDefeat)]   = Audio::Load("Resources/Audio/SE/SE_BossDefeat.wav");
    seHandles_[static_cast<int>(SE::Start)]        = Audio::Load("Resources/Audio/SE/SE_Start.wav");

    isInitialized_ = true;
}

void SoundManager::Finalize() {
    StopBGM();
    isInitialized_ = false;
}

float SoundManager::GetEffectiveBGMVolume() const {
    auto* gsm = GameSceneManager::GetInstance();
    float vol = gsm->GetMasterVolume() * gsm->GetBGMVolume();

    // ゲームプレイ中BGM（BGM::Play）は音圧が高いため、SEや他シーンとのバランスを考慮して音量を下げる
    if (currentBGM_ == BGM::Play) {
        vol *= 0.5f;
    } else if (currentBGM_ == BGM::Boss) {
        vol *= 0.7f;
    }

    return std::clamp(vol, 0.0f, 1.0f);
}

float SoundManager::GetEffectiveSEVolume(float volumeMultiplier) const {
    auto* gsm = GameSceneManager::GetInstance();
    float vol = gsm->GetMasterVolume() * gsm->GetSEVolume() * volumeMultiplier;
    return std::clamp(vol, 0.0f, 1.0f);
}

void SoundManager::PlayBGM(BGM bgm, bool loop) {
    if (!isInitialized_) Initialize();

    int bgmIdx = static_cast<int>(bgm);
    if (bgmIdx < 0 || bgmIdx >= 4) return;

    // 既に同じBGMが再生中ならそのまま継続
    if (currentBGM_ == bgm && currentBgmHandle_ >= 0 && Audio::IsPlaying(currentBgmHandle_)) {
        return;
    }

    // 既存BGM停止
    if (currentBgmHandle_ >= 0) {
        Audio::Stop(currentBgmHandle_);
    }

    currentBGM_ = bgm;
    currentBgmHandle_ = bgmHandles_[bgmIdx];

    if (currentBgmHandle_ >= 0) {
        float vol = GetEffectiveBGMVolume();
        Audio::Play(currentBgmHandle_, loop, vol);
        lastBgmVolume_ = vol;
    }
}

void SoundManager::StopBGM() {
    if (currentBgmHandle_ >= 0) {
        Audio::Stop(currentBgmHandle_);
    }
    currentBGM_ = BGM::None;
    currentBgmHandle_ = -1;
    isBgmPaused_ = false;
}

void SoundManager::PauseBGM() {
    if (currentBgmHandle_ >= 0 && !isBgmPaused_) {
        Audio::Stop(currentBgmHandle_);
        isBgmPaused_ = true;
    }
}

void SoundManager::ResumeBGM() {
    if (currentBgmHandle_ >= 0 && isBgmPaused_) {
        float vol = GetEffectiveBGMVolume();
        Audio::Play(currentBgmHandle_, true, vol);
        isBgmPaused_ = false;
    }
}

void SoundManager::PlaySE(SE se, float volumeMultiplier) {
    if (!isInitialized_) Initialize();

    int seIdx = static_cast<int>(se);
    if (seIdx < 0 || seIdx >= static_cast<int>(SE::Count)) return;

    int handle = seHandles_[seIdx];
    if (handle >= 0) {
        // スタート音は控えめで耳に優しい音量（0.2倍）に調整
        if (se == SE::Start) {
            volumeMultiplier *= 0.2f;
        }
        float vol = GetEffectiveSEVolume(volumeMultiplier);
        Audio::Play(handle, false, vol);
    }
}

void SoundManager::StopSE(SE se) {
    int seIdx = static_cast<int>(se);
    if (seIdx < 0 || seIdx >= static_cast<int>(SE::Count)) return;

    int handle = seHandles_[seIdx];
    if (handle >= 0) {
        Audio::Stop(handle);
    }
}

void SoundManager::StopAllSE() {
    for (int i = 0; i < static_cast<int>(SE::Count); ++i) {
        int handle = seHandles_[i];
        if (handle >= 0) {
            Audio::Stop(handle);
        }
    }
}

void SoundManager::StopAll() {
    StopBGM();
    StopAllSE();
}

void SoundManager::Update() {
    if (currentBgmHandle_ >= 0) {
        float currentVol = GetEffectiveBGMVolume();
        if (std::abs(currentVol - lastBgmVolume_) > 0.001f) {
            Audio::SetVolume(currentBgmHandle_, currentVol);
            lastBgmVolume_ = currentVol;
        }
    }
}

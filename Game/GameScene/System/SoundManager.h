#pragma once
#include <string>
#include <unordered_map>

class SoundManager {
public:
    enum class BGM {
        None,
        Title,
        Play,
        Boss
    };

    enum class SE {
        Jump,           // ジャンプ音
        Land,           // 雪面着地音
        Slide,          // 氷上滑走/ローリング音
        LaneChange,     // レーン移動/風切り音
        ItemGet,        // アイテム/クリスタル取得音
        Barrier,        // バリア展開音
        BarrierBreak,   // 氷バリア破砕音
        BonusHit,       // ボーナス敵ヒット音
        ClearBomb,      // クリアアイテム全画面破壊音
        Crash,          // 激突・転倒音
        BossReflect,    // ボス攻撃反射/カウンター音
        BossLand,       // ボス攻撃着地音
        BossHit,        // ボスへダメージ命中音
        BossDefeat,     // ボス撃破爆発音
        Start,          // スタート・決定音
        Count
    };

    static SoundManager* GetInstance();

    void Initialize();
    void Finalize();
    void Update();

    void PlayBGM(BGM bgm, bool loop = true);
    void StopBGM();
    void PauseBGM();
    void ResumeBGM();
    BGM GetCurrentBGM() const { return currentBGM_; }

    void PlaySE(SE se, float volumeMultiplier = 1.0f);
    void StopSE(SE se);
    void StopAllSE();
    void StopAll();

private:
    SoundManager() = default;
    ~SoundManager() = default;
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    float GetEffectiveBGMVolume() const;
    float GetEffectiveSEVolume(float volumeMultiplier = 1.0f) const;

    int bgmHandles_[4] = { -1, -1, -1, -1 };
    int seHandles_[static_cast<int>(SE::Count)] = {};
    BGM currentBGM_ = BGM::None;
    int currentBgmHandle_ = -1;
    bool isBgmPaused_ = false;
    float lastBgmVolume_ = -1.0f;
    bool isInitialized_ = false;
};

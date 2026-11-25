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
        Freeze,         // シーン遷移：凍結音
        IceCracking,    // シーン遷移：氷割れ・破砕音
        Count
    };

    /// <summary>
    /// シングルトンインスタンスの取得
    /// </summary>
    static SoundManager* GetInstance();

    /// <summary>
    /// サウンドアセット（BGM/SE）の読み込み・初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 再生中サウンドの停止と解放処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// 音量設定の毎フレーム更新
    /// </summary>
    void Update();

    /// <summary>
    /// 指定BGMの再生
    /// </summary>
    /// <param name="bgm">BGM種別</param>
    /// <param name="loop">ループ再生するかどうか</param>
    void PlayBGM(BGM bgm, bool loop = true);

    /// <summary>
    /// 現在再生中のBGMを停止
    /// </summary>
    void StopBGM();

    /// <summary>
    /// 現在再生中のBGMを一時停止
    /// </summary>
    void PauseBGM();

    /// <summary>
    /// 一時停止中のBGMを再開
    /// </summary>
    void ResumeBGM();

    /// <summary>
    /// 現在再生中のBGM種別を取得
    /// </summary>
    BGM GetCurrentBGM() const { return currentBGM_; }

    /// <summary>
    /// 効果音（SE）の再生
    /// </summary>
    /// <param name="se">SE種別</param>
    /// <param name="volumeMultiplier">音量乗数</param>
    void PlaySE(SE se, float volumeMultiplier = 1.0f);

    /// <summary>
    /// 指定SEの停止
    /// </summary>
    /// <param name="se">SE種別</param>
    void StopSE(SE se);

    /// <summary>
    /// 全SEの停止
    /// </summary>
    void StopAllSE();

    /// <summary>
    /// 全BGMおよび全SEの停止
    /// </summary>
    void StopAll();

private:
    SoundManager() = default;
    ~SoundManager() = default;
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    float GetEffectiveBGMVolume() const;
    float GetEffectiveSEVolume(float volumeMultiplier = 1.0f) const;

    static constexpr int kBgmCount = 4;
    static constexpr float kPlayBGMVolumeScale = 0.5f;
    static constexpr float kBossBGMVolumeScale = 0.7f;
    static constexpr float kStartSeVolumeMultiplier = 0.2f;

    int bgmHandles_[kBgmCount] = { -1, -1, -1, -1 };
    int seHandles_[static_cast<int>(SE::Count)] = {};
    BGM currentBGM_ = BGM::None;
    int currentBgmHandle_ = -1;
    bool isBgmPaused_ = false;
    float lastBgmVolume_ = -1.0f;
    bool isInitialized_ = false;
};

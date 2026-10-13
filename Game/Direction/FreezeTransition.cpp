#include "FreezeTransition.h"
#include <Graphics/Render/Draw.h>
#include <Core/WindowConfig.h>
#include "../GameScene/System/SoundManager.h"
#include <algorithm>
#include <cmath>
#include <random>

FreezeTransition* FreezeTransition::GetInstance() {
    static FreezeTransition instance;
    return &instance;
}

FreezeTransition::FreezeTransition() {
}

void FreezeTransition::Initialize() {
    if (isInitialized_) return;

    textureManager_ = std::make_unique<Texture>();

    // テクスチャの読み込み
    texFrostHandle_   = textureManager_->CreateTexture("Resources/Texture/Transition/ice_frost.png");
    texCrackHandle_   = textureManager_->CreateTexture("Resources/Texture/Transition/ice_crack.png");
    texSurfaceHandle_ = textureManager_->CreateTexture("Resources/Texture/Transition/ice_surface.png");

    // 1. 凍結用 PostEffect (FreezeShader)
    freezePostEffect_ = std::make_unique<PostEffect>();
    freezePostEffect_->Initialize();
    freezePostEffect_->SetActivePostEffect("FreezeShader");
    freezePostEffect_->SetTexturePath("gFrostTexture", "Resources/Texture/Transition/ice_frost.png");
    freezePostEffect_->SetColor(0.82f, 0.94f, 1.0f); // クリスタルアイスブルー
    freezePostEffect_->SetRatio(0.0f);
    freezePostEffect_->SetTime(0.0f);
    freezePostEffect_->SetValue1(0.04f); // 歪み強度

    // 2. 破砕用 PostEffect (ShatterShader)
    shatterPostEffect_ = std::make_unique<PostEffect>();
    shatterPostEffect_->Initialize();
    shatterPostEffect_->SetActivePostEffect("ShatterShader");
    shatterPostEffect_->SetTexturePath("gCrackTexture", "Resources/Texture/Transition/ice_crack.png");
    shatterPostEffect_->SetColor(0.85f, 0.95f, 1.0f); // 氷片基本色
    shatterPostEffect_->SetRatio(0.0f);
    shatterPostEffect_->SetValue1(0.0f); // 亀裂なし
    shatterPostEffect_->SetTime(0.0f);

    isInitialized_ = true;
}

void FreezeTransition::Start(float freezeDuration, float shatterDuration, std::function<void()> onMidpoint) {
    Initialize();

    phase_ = Phase::Freezing;
    timer_ = 0.0f;
    freezeDuration_ = (freezeDuration <= 0.0f) ? 0.001f : freezeDuration;
    shatterDuration_ = (shatterDuration <= 0.0f) ? 0.001f : shatterDuration;
    crackDuration_ = kDefaultCrackDuration;

    onMidpointCallback_ = onMidpoint;
    midpointExecuted_ = false;

    sparkles_.clear();

    if (freezePostEffect_) {
        freezePostEffect_->SetRatio(0.0f);
        freezePostEffect_->SetTime(0.0f);
    }
    if (shatterPostEffect_) {
        shatterPostEffect_->SetRatio(0.0f);
        shatterPostEffect_->SetValue1(0.0f);
        shatterPostEffect_->SetTime(0.0f);
    }

    // 凍結音の再生 (シュオー、キィィンという氷結音)
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Barrier);

    // 画面外周に最初の氷晶スパークルを少し散らす
    EmitSparkles(kDefaultScreenCenter, 20, 100.0f, 400.0f);
}

void FreezeTransition::Update(float deltaTime) {
    if (phase_ == Phase::None || phase_ == Phase::Finished) {
        return;
    }

    timer_ += deltaTime;
    UpdateSparkles(deltaTime);

    if (phase_ == Phase::Freezing) {
        float p = std::clamp(timer_ / freezeDuration_, 0.0f, 1.0f);

        if (freezePostEffect_) {
            freezePostEffect_->SetRatio(p);
            freezePostEffect_->SetTime(timer_);
            freezePostEffect_->Update(0.0f);
        }

        // 凍結中に時折氷晶スパークルを発生
        if (std::rand() % 5 == 0) {
            float angle = static_cast<float>(std::rand()) / RAND_MAX * 6.28318f;
            float radius = (1.0f - p * 0.7f) * kFreezeSparkleRadius;
            Vector2 sp = { kDefaultScreenCenter.x + std::cos(angle) * radius, kDefaultScreenCenter.y + std::sin(angle) * radius };
            EmitSparkles(sp, 2, 30.0f, 120.0f);
        }

        if (p >= 1.0f) {
            // 凍結完了！亀裂フェーズへ突入
            phase_ = Phase::Cracking;
            timer_ = 0.0f;

            if (shatterPostEffect_) {
                shatterPostEffect_->SetRatio(0.0f);
                shatterPostEffect_->SetValue1(0.0f);
                shatterPostEffect_->SetTime(0.0f);
                shatterPostEffect_->Update(0.0f);
            }

            // 中間コールバック（新シーンへの切り替え・初期化）を実行！
            if (onMidpointCallback_ && !midpointExecuted_) {
                onMidpointCallback_();
                midpointExecuted_ = true;
            }

            // ピキッ！と最初の亀裂が走る衝撃音
            SoundManager::GetInstance()->PlaySE(SoundManager::SE::Crash, 0.4f);

            // 亀裂の走るインパクトでスパークル
            EmitSparkles(kDefaultScreenCenter, 20, 150.0f, 450.0f);
        }
    }
    else if (phase_ == Phase::Cracking) {
        float p = std::clamp(timer_ / crackDuration_, 0.0f, 1.0f);

        if (shatterPostEffect_) {
            shatterPostEffect_->SetRatio(0.0f);
            shatterPostEffect_->SetValue1(p); // 亀裂が広がる
            shatterPostEffect_->SetTime(timer_);
            shatterPostEffect_->Update(0.0f);
        }

        // 亀裂が外側へ走るのに合わせて微小なガラス粉スパークルを散らす
        if (std::rand() % 3 == 0) {
            float angle = static_cast<float>(std::rand()) / RAND_MAX * 6.28318f;
            float radius = p * 420.0f;
            Vector2 sp = { kDefaultScreenCenter.x + std::cos(angle) * radius, kDefaultScreenCenter.y + std::sin(angle) * radius };
            EmitSparkles(sp, 2, 40.0f, 160.0f);
        }

        if (p >= 1.0f) {
            // パリーン！ガラガラガラッと砕け散る崩落フェーズ突入！
            phase_ = Phase::Shattering;
            timer_ = 0.0f;

            if (shatterPostEffect_) {
                shatterPostEffect_->SetRatio(0.0f);
                shatterPostEffect_->SetValue1(1.0f);
                shatterPostEffect_->SetTime(0.0f);
                shatterPostEffect_->Update(0.0f);
            }

            // 氷破砕音 (SE_BarrierBreak) 再生！
            SoundManager::GetInstance()->PlaySE(SoundManager::SE::BarrierBreak);

            // 画面全体で無数のガラス片・ガラス屑がガラガラと崩れ落ちる！
            EmitGlassCascade(220);
        }
    }
    else if (phase_ == Phase::Shattering) {
        float p = std::clamp(timer_ / shatterDuration_, 0.0f, 1.0f);

        if (shatterPostEffect_) {
            shatterPostEffect_->SetRatio(p); // 破片がガラガラと崩れ落ちる
            shatterPostEffect_->SetValue1(1.0f);
            shatterPostEffect_->SetTime(timer_);
            shatterPostEffect_->Update(0.0f);
        }

        // 崩落中も割れ目や上部からパラパラと降り注ぐ微小なガラス屑
        if (p < 0.65f && (std::rand() % 2 == 0)) {
            float rx = 100.0f + static_cast<float>(std::rand()) / RAND_MAX * 1080.0f;
            float ry = 30.0f + static_cast<float>(std::rand()) / RAND_MAX * 350.0f;
            EmitSparkles({ rx, ry }, 3, 20.0f, 100.0f);
        }

        if (p >= 1.0f) {
            // トランジション完全終了！
            phase_ = Phase::Finished;
            timer_ = 0.0f;
            sparkles_.clear();
        }
    }
}

void FreezeTransition::Draw(class Draw& draw) {
    if (phase_ == Phase::None || phase_ == Phase::Finished) {
        return;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE surfSrv = textureManager_->TextureData(texSurfaceHandle_);

    if (phase_ == Phase::Freezing) {
        if (freezePostEffect_) {
            draw.DrawPostEffect(surfSrv, "FreezeShader", freezePostEffect_.get(), {0}, BlendMode::kBlendModeNormal);
        }

        // 氷晶スパークルの描画
        for (const auto& sp : sparkles_) {
            Vector4 col = sp.color;
            col.w *= sp.alpha;
            draw.DrawFillRect(sp.pos, { sp.size, sp.size }, col);
        }
    }
    else if (phase_ == Phase::Cracking || phase_ == Phase::Shattering) {
        if (shatterPostEffect_) {
            draw.DrawPostEffect(surfSrv, "ShatterShader", shatterPostEffect_.get(), {0}, BlendMode::kBlendModeNormal);
        }

        // 飛び散る氷粉ダイヤモンドダスト (ガラス微粉末)
        for (const auto& sp : sparkles_) {
            Vector4 col = sp.color;
            col.w *= sp.alpha;
            draw.DrawFillRect(sp.pos, { sp.size, sp.size }, col);
        }
    }
}

void FreezeTransition::EmitSparkles(const Vector2& center, int count, float minSpeed, float maxSpeed) {
    std::mt19937 rng(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(this) ^ std::rand()));
    std::uniform_real_distribution<float> randAngle(0.0f, 6.2831853f);
    std::uniform_real_distribution<float> randSpeed(minSpeed, maxSpeed);
    std::uniform_real_distribution<float> randSize(1.5f, 6.5f);
    std::uniform_real_distribution<float> randLife(0.4f, 0.85f);
    std::uniform_real_distribution<float> randOffset(-25.0f, 25.0f);

    for (int i = 0; i < count; ++i) {
        IceSparkle sp;
        float angle = randAngle(rng);
        float speed = randSpeed(rng);

        sp.pos = { center.x + randOffset(rng), center.y + randOffset(rng) };
        sp.velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
        sp.size = randSize(rng);
        sp.life = 0.0f;
        sp.maxLife = randLife(rng);
        sp.alpha = 1.0f;

        // キラキラ光る純白〜クリスタルシアン〜シルバー (鋭いガラスの反射光)
        if (i % 4 == 0) {
            sp.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 純白の閃光
        } else if (i % 4 == 1) {
            sp.color = { 0.72f, 0.94f, 1.0f, 1.0f }; // シアン
        } else if (i % 4 == 2) {
            sp.color = { 0.88f, 0.97f, 1.0f, 1.0f }; // クリスタルガラス
        } else {
            sp.color = { 0.95f, 1.0f, 1.0f, 1.0f }; // プリズムシルバー
        }

        sparkles_.push_back(sp);
    }
}

void FreezeTransition::EmitGlassCascade(int count) {
    std::mt19937 rng(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(this) ^ std::rand()));
    std::uniform_real_distribution<float> randX(80.0f, 1200.0f);
    std::uniform_real_distribution<float> randY(40.0f, 660.0f);
    std::uniform_real_distribution<float> randVelX(-180.0f, 180.0f);
    std::uniform_real_distribution<float> randVelY(-60.0f, 260.0f);
    std::uniform_real_distribution<float> randSize(1.5f, 6.0f);
    std::uniform_real_distribution<float> randLife(0.5f, 0.95f);

    for (int i = 0; i < count; ++i) {
        IceSparkle sp;
        sp.pos = { randX(rng), randY(rng) };
        sp.velocity = { randVelX(rng), randVelY(rng) };
        sp.size = randSize(rng);
        sp.life = 0.0f;
        sp.maxLife = randLife(rng);
        sp.alpha = 1.0f;

        // キラキラ光る純白〜クリスタルシアン〜シルバー
        if (i % 4 == 0) {
            sp.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 純白
        } else if (i % 4 == 1) {
            sp.color = { 0.72f, 0.94f, 1.0f, 1.0f }; // シアン
        } else if (i % 4 == 2) {
            sp.color = { 0.88f, 0.97f, 1.0f, 1.0f }; // クリスタルガラス
        } else {
            sp.color = { 0.95f, 1.0f, 1.0f, 1.0f }; // プリズムシルバー
        }

        sparkles_.push_back(sp);
    }
}

void FreezeTransition::UpdateSparkles(float deltaTime) {
    const float gravity = 1200.0f; // ガラガラと崩落するガラス片・ガラス屑の重力加速度

    for (auto it = sparkles_.begin(); it != sparkles_.end();) {
        it->life += deltaTime;
        if (it->life >= it->maxLife) {
            it = sparkles_.erase(it);
            continue;
        }

        it->pos.x += it->velocity.x * deltaTime;
        it->pos.y += it->velocity.y * deltaTime;
        it->velocity.y += gravity * deltaTime;

        // 空気抵抗
        it->velocity.x *= 0.98f;
        it->velocity.y *= 0.98f;

        // 鋭くキラキラ瞬く (ガラス片の回転反射)
        float lifeRatio = it->life / it->maxLife;
        float twinkle = 0.6f + 0.4f * std::sin(it->life * 45.0f);
        it->alpha = (1.0f - lifeRatio) * twinkle;

        ++it;
    }
}

void FreezeTransition::Reset() {
    phase_ = Phase::None;
    timer_ = 0.0f;
    onMidpointCallback_ = nullptr;
    midpointExecuted_ = false;
    sparkles_.clear();
    if (freezePostEffect_) {
        freezePostEffect_->SetRatio(0.0f);
    }
    if (shatterPostEffect_) {
        shatterPostEffect_->SetRatio(0.0f);
        shatterPostEffect_->SetValue1(0.0f);
    }
}

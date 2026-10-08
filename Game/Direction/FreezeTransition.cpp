#include "FreezeTransition.h"
#include <Graphics/Render/Draw.h>
#include "../GameScene/System/SoundManager.h"
#include <algorithm>
#include <cmath>
#include <random>

FreezeTransition* FreezeTransition::GetInstance() {
    static FreezeTransition instance;
    return &instance;
}

FreezeTransition::FreezeTransition() {
    // コンストラクタ
}

void FreezeTransition::Initialize() {
    if (isInitialized_) return;

    textureManager_ = std::make_unique<Texture>();

    // テクスチャの読み込み
    texFrostHandle_   = textureManager_->CreateTexture("Resources/Texture/Transition/ice_frost.png");
    texCrackHandle_   = textureManager_->CreateTexture("Resources/Texture/Transition/ice_crack.png");
    texSurfaceHandle_ = textureManager_->CreateTexture("Resources/Texture/Transition/ice_surface.png");

    const Vector2 screenSize = { 1280.0f, 720.0f };
    const Vector2 centerPos  = { 640.0f, 360.0f };

    // 1. 霜スプライト 1 (正位置)
    {
        SpriteData sd{};
        sd.transform.scale = { 1.0f, 1.0f, 1.0f };
        sd.transform.translate = { centerPos.x, centerPos.y, 0.0f };
        sd.transform.rotate = { 0.0f, 0.0f, 0.0f };
        sd.size = screenSize;
        sd.pivot = { 0.5f, 0.5f };
        sd.textureArea[0] = { 0.0f, 0.0f };
        sd.textureArea[1] = { 1.0f, 1.0f };
        sd.scaleMode = SpriteScaleMode::Fit;
        sd.anchor = SpriteAnchor::None;

        frostSprite1_ = std::make_unique<Sprite>();
        frostSprite1_->Initialize(sd, texFrostHandle_);
        frostSprite1_->GetMaterial()->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
        frostSprite1_->SettingWvp();
    }

    // 2. 霜スプライト 2 (180度回転・立体感向上)
    {
        SpriteData sd{};
        sd.transform.scale = { 1.1f, 1.1f, 1.1f };
        sd.transform.translate = { centerPos.x, centerPos.y, 0.0f };
        sd.transform.rotate = { 0.0f, 0.0f, 3.14159265f };
        sd.size = screenSize;
        sd.pivot = { 0.5f, 0.5f };
        sd.textureArea[0] = { 0.0f, 0.0f };
        sd.textureArea[1] = { 1.0f, 1.0f };
        sd.scaleMode = SpriteScaleMode::Fit;
        sd.anchor = SpriteAnchor::None;

        frostSprite2_ = std::make_unique<Sprite>();
        frostSprite2_->Initialize(sd, texFrostHandle_);
        frostSprite2_->GetMaterial()->SetColor({ 0.9f, 0.95f, 1.0f, 0.0f });
        frostSprite2_->SettingWvp();
    }

    // 3. 全面氷板スプライト
    {
        SpriteData sd{};
        sd.transform.scale = { 1.0f, 1.0f, 1.0f };
        sd.transform.translate = { centerPos.x, centerPos.y, 0.0f };
        sd.transform.rotate = { 0.0f, 0.0f, 0.0f };
        sd.size = screenSize;
        sd.pivot = { 0.5f, 0.5f };
        sd.textureArea[0] = { 0.0f, 0.0f };
        sd.textureArea[1] = { 1.0f, 1.0f };
        sd.scaleMode = SpriteScaleMode::Fit;
        sd.anchor = SpriteAnchor::None;

        surfaceSprite_ = std::make_unique<Sprite>();
        surfaceSprite_->Initialize(sd, texSurfaceHandle_);
        surfaceSprite_->GetMaterial()->SetColor({ 0.85f, 0.95f, 1.0f, 0.0f });
        surfaceSprite_->SettingWvp();
    }

    // 4. ひび割れ (クラック) スプライト
    {
        SpriteData sd{};
        sd.transform.scale = { 1.0f, 1.0f, 1.0f };
        sd.transform.translate = { centerPos.x, centerPos.y, 0.0f };
        sd.transform.rotate = { 0.0f, 0.0f, 0.0f };
        sd.size = screenSize;
        sd.pivot = { 0.5f, 0.5f };
        sd.textureArea[0] = { 0.0f, 0.0f };
        sd.textureArea[1] = { 1.0f, 1.0f };
        sd.scaleMode = SpriteScaleMode::Fit;
        sd.anchor = SpriteAnchor::None;

        crackSprite_ = std::make_unique<Sprite>();
        crackSprite_->Initialize(sd, texCrackHandle_);
        crackSprite_->SetBlend(BlendMode::kBlendModeAdd);
        crackSprite_->GetMaterial()->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
        crackSprite_->SettingWvp();
    }

    // 5. 破片 (シャード) の構築
    SetupShards();

    isInitialized_ = true;
}

void FreezeTransition::SetupShards() {
    shards_.clear();

    const float screenW = 1280.0f;
    const float screenH = 720.0f;
    const float cellW = screenW / static_cast<float>(kMeshCols);
    const float cellH = screenH / static_cast<float>(kMeshRows);

    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> jitterX(-cellW * 0.38f, cellW * 0.38f);
    std::uniform_real_distribution<float> jitterY(-cellH * 0.38f, cellH * 0.38f);
    std::uniform_real_distribution<float> randColor(0.0f, 1.0f);

    // 1. 格子点の生成 (四辺・四隅は画面端にクランプして画面全体を100%隙間なく覆う)
    std::vector<std::vector<Vector2>> grid(kMeshRows + 1, std::vector<Vector2>(kMeshCols + 1));
    for (int r = 0; r <= kMeshRows; ++r) {
        for (int c = 0; c <= kMeshCols; ++c) {
            float baseX = static_cast<float>(c) * cellW;
            float baseY = static_cast<float>(r) * cellH;

            float x = baseX;
            float y = baseY;

            if (c > 0 && c < kMeshCols) {
                x += jitterX(rng);
            }
            if (r > 0 && r < kMeshRows) {
                y += jitterY(rng);
            }

            // 画面境界に固定・クランプ
            if (c == 0) x = 0.0f;
            if (c == kMeshCols) x = screenW;
            if (r == 0) y = 0.0f;
            if (r == kMeshRows) y = screenH;

            grid[r][c] = { x, y };
        }
    }

    // 三角形シャードを生成するヘルパーラムダ
    auto createTriangleShard = [&](const Vector2& A, const Vector2& B, const Vector2& C) {
        Vector2 center = { (A.x + B.x + C.x) / 3.0f, (A.y + B.y + C.y) / 3.0f };
        Vector2 localPos[3] = {
            { A.x - center.x, A.y - center.y },
            { B.x - center.x, B.y - center.y },
            { C.x - center.x, C.y - center.y }
        };
        Vector2 uvs[3] = {
            { std::clamp(A.x / screenW, 0.0f, 1.0f), std::clamp(A.y / screenH, 0.0f, 1.0f) },
            { std::clamp(B.x / screenW, 0.0f, 1.0f), std::clamp(B.y / screenH, 0.0f, 1.0f) },
            { std::clamp(C.x / screenW, 0.0f, 1.0f), std::clamp(C.y / screenH, 0.0f, 1.0f) }
        };

        SpriteData sd{};
        sd.transform.scale = { 1.0f, 1.0f, 1.0f };
        sd.transform.translate = { center.x, center.y, 0.0f };
        sd.transform.rotate = { 0.0f, 0.0f, 0.0f };
        sd.size = { 100.0f, 100.0f };
        sd.pivot = { 0.0f, 0.0f };
        sd.scaleMode = SpriteScaleMode::Fit;
        sd.anchor = SpriteAnchor::None;

        Shard shard;
        shard.sprite = std::make_unique<Sprite>();
        shard.sprite->Initialize(sd, texSurfaceHandle_);
        shard.sprite->SetCustomTriangleVertices(localPos, uvs);

        // ガラス特有の淡いクリスタルアイスブルー〜白
        float tint = randColor(rng);
        shard.baseColor = { 0.90f + tint * 0.08f, 0.95f + tint * 0.05f, 1.0f, 1.0f };
        shard.sprite->GetMaterial()->SetColor(shard.baseColor);
        shard.sprite->SettingWvp();

        shard.initialPos = center;
        shard.currentPos = center;
        shard.velocity = { 0.0f, 0.0f };
        shard.rotation = { 0.0f, 0.0f, 0.0f };
        shard.angularVelocity = { 0.0f, 0.0f, 0.0f };
        shard.scale = 1.0f;
        shard.alpha = 1.0f;

        // 破片の面積から質量を計算 (大きい破片ほど重く、小さい破片ほど激しく吹き飛ぶ)
        float area = std::abs((B.x - A.x) * (C.y - A.y) - (C.x - A.x) * (B.y - A.y)) * 0.5f;
        shard.mass = std::clamp(area / 3600.0f, 0.35f, 2.8f);

        shards_.push_back(std::move(shard));
    };

    // 2. 各セルを斜めに切って鋭利な三角形ガラス片を生成 (チェッカー交互斜線で規則性を打破)
    for (int r = 0; r < kMeshRows; ++r) {
        for (int c = 0; c < kMeshCols; ++c) {
            Vector2 v00 = grid[r][c];
            Vector2 v10 = grid[r][c + 1];
            Vector2 v01 = grid[r + 1][c];
            Vector2 v11 = grid[r + 1][c + 1];

            bool slash = ((c + r) % 2 == 0);
            if (slash) {
                createTriangleShard(v00, v10, v11);
                createTriangleShard(v00, v11, v01);
            } else {
                createTriangleShard(v00, v10, v01);
                createTriangleShard(v10, v11, v01);
            }
        }
    }

    // 3. インパクト中心付近 (640, 360) に細かな針状ガラス片 (マイクロシャード) を追加
    std::uniform_real_distribution<float> randR(15.0f, 240.0f);
    std::uniform_real_distribution<float> randAngle(0.0f, 6.2831853f);
    std::uniform_real_distribution<float> randLen(16.0f, 44.0f);
    std::uniform_real_distribution<float> randWid(5.0f, 13.0f);

    for (int i = 0; i < 28; ++i) {
        float r = randR(rng);
        float th = randAngle(rng);
        Vector2 c = { 640.0f + std::cos(th) * r, 360.0f + std::sin(th) * r };

        float len = randLen(rng);
        float wid = randWid(rng);
        float dir = th + ((i % 2 == 0) ? 0.0f : 1.5707963f);

        Vector2 forward = { std::cos(dir), std::sin(dir) };
        Vector2 perp = { -forward.y, forward.x };

        Vector2 A = { c.x + forward.x * len * 0.7f, c.y + forward.y * len * 0.7f };
        Vector2 B = { c.x - forward.x * len * 0.3f + perp.x * wid * 0.5f, c.y - forward.y * len * 0.3f + perp.y * wid * 0.5f };
        Vector2 C = { c.x - forward.x * len * 0.3f - perp.x * wid * 0.5f, c.y - forward.y * len * 0.3f - perp.y * wid * 0.5f };

        createTriangleShard(A, B, C);
    }
}

void FreezeTransition::ResetShards() {
    const Vector2 centerPos = { 640.0f, 360.0f };
    std::mt19937 rng(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(this) ^ 1337));
    std::uniform_real_distribution<float> randSpeed(580.0f, 1380.0f);
    std::uniform_real_distribution<float> randAngVelX(-14.0f, 14.0f);
    std::uniform_real_distribution<float> randAngVelY(-14.0f, 14.0f);
    std::uniform_real_distribution<float> randAngVelZ(-9.0f, 9.0f);
    std::uniform_real_distribution<float> randAngle(-0.32f, 0.32f);
    std::uniform_real_distribution<float> randUpBias(-280.0f, -60.0f);
    std::uniform_real_distribution<float> randShine(1.0f, 2.5f);

    shatterFlashTimer_ = 0.12f; // 破砕瞬間の閃光フラッシュ！

    for (auto& shard : shards_) {
        shard.currentPos = shard.initialPos;
        shard.rotation = { 0.0f, 0.0f, 0.0f };
        shard.scale = 1.0f;
        shard.alpha = 1.0f;
        shard.shineSpeed = randShine(rng);

        // 中心から外側への放射状ベクトル
        float dx = shard.initialPos.x - centerPos.x;
        float dy = shard.initialPos.y - centerPos.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < 1.0f) {
            dx = 0.0f;
            dy = -1.0f;
            dist = 1.0f;
        }

        float angle = std::atan2(dy, dx) + randAngle(rng);
        // 中心付近ほど爆発的な初速、質量が小さいほどさらに高速
        float distFactor = std::clamp(dist / 600.0f, 0.0f, 1.5f);
        float massFactor = 1.0f / std::sqrt(shard.mass);
        float speed = randSpeed(rng) * (1.15f - 0.25f * distFactor) * massFactor;

        shard.velocity.x = std::cos(angle) * speed;
        shard.velocity.y = std::sin(angle) * speed + randUpBias(rng) * massFactor;

        // 3Dタンブリング角速度 (X, Y, Zの3軸回転)
        shard.angularVelocity.x = randAngVelX(rng) * massFactor;
        shard.angularVelocity.y = randAngVelY(rng) * massFactor;
        shard.angularVelocity.z = randAngVelZ(rng) * massFactor;
    }
}

void FreezeTransition::Start(float freezeDuration, float shatterDuration, std::function<void()> onMidpoint) {
    Initialize();

    phase_ = Phase::Freezing;
    timer_ = 0.0f;
    freezeDuration_ = (freezeDuration <= 0.0f) ? 0.001f : freezeDuration;
    shatterDuration_ = (shatterDuration <= 0.0f) ? 0.001f : shatterDuration;
    crackDuration_ = 0.12f;

    onMidpointCallback_ = onMidpoint;
    midpointExecuted_ = false;

    sparkles_.clear();

    // 凍結音の再生！(シュオー、キィィンという氷結音)
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Barrier);

    // 画面外周に最初の氷晶スパークルを少し散らす
    EmitSparkles({ 640.0f, 360.0f }, 25, 100.0f, 400.0f);
}

void FreezeTransition::Update(float deltaTime) {
    if (phase_ == Phase::None || phase_ == Phase::Finished) {
        return;
    }

    timer_ += deltaTime;

    UpdateSparkles(deltaTime);

    if (phase_ == Phase::Freezing) {
        float p = std::clamp(timer_ / freezeDuration_, 0.0f, 1.0f);

        // 凍結中に時折氷晶スパークルを発生
        if (std::rand() % 4 == 0) {
            float angle = static_cast<float>(std::rand()) / RAND_MAX * 6.28318f;
            float radius = (1.0f - p * 0.7f) * 450.0f;
            Vector2 sp = { 640.0f + std::cos(angle) * radius, 360.0f + std::sin(angle) * radius };
            EmitSparkles(sp, 2, 30.0f, 120.0f);
        }

        if (p >= 1.0f) {
            // 凍結完了！亀裂フェーズへ突入
            phase_ = Phase::Cracking;
            timer_ = 0.0f;

            // 中間コールバック（新シーンへの切り替え・初期化）を実行！
            if (onMidpointCallback_ && !midpointExecuted_) {
                onMidpointCallback_();
                midpointExecuted_ = true;
            }

            // 亀裂の走るインパクトで少しスパークル
            EmitSparkles({ 640.0f, 360.0f }, 30, 200.0f, 600.0f);
        }
    }
    else if (phase_ == Phase::Cracking) {
        float p = std::clamp(timer_ / crackDuration_, 0.0f, 1.0f);

        if (p >= 1.0f) {
            // パリーン！破砕フェーズ突入！
            phase_ = Phase::Shattering;
            timer_ = 0.0f;

            // 氷破砕音 (SE_BarrierBreak) 再生！
            SoundManager::GetInstance()->PlaySE(SoundManager::SE::BarrierBreak);

            // 破片の爆発初速セットアップ
            ResetShards();

            // 大量のダイアモンドダスト (氷粉・ガラスダストスパークル) が四散！
            EmitSparkles({ 640.0f, 360.0f }, 160, 250.0f, 1300.0f);
        }
    }
    else if (phase_ == Phase::Shattering) {
        float p = std::clamp(timer_ / shatterDuration_, 0.0f, 1.0f);

        if (shatterFlashTimer_ > 0.0f) {
            shatterFlashTimer_ -= deltaTime;
        }

        // ガラス破片の物理シミュレーション (放射状飛散・3Dタンブリング・重力・閃光ハイライト・フェード)
        const float gravity = 1150.0f; // 重力加速度 (px/s^2)
        for (auto& shard : shards_) {
            shard.currentPos.x += shard.velocity.x * deltaTime;
            shard.currentPos.y += shard.velocity.y * deltaTime;
            shard.velocity.y += gravity * deltaTime;
            shard.velocity.x *= 0.985f; // 空気抵抗

            // 3Dタンブリング回転 (X, Y, Z軸の激しい立体回転)
            shard.rotation.x += shard.angularVelocity.x * deltaTime;
            shard.rotation.y += shard.angularVelocity.y * deltaTime;
            shard.rotation.z += shard.angularVelocity.z * deltaTime;

            // ガラスの反射・キラめき (Specular Glint)
            // ガラス片が3D回転して法線が特定の向きに来た瞬間に純白にフラッシュする
            float angleCos = std::abs(std::cos(shard.rotation.x) * std::cos(shard.rotation.y));
            float flash = std::pow(angleCos, 5.0f); // 鋭いハイライト

            Vector4 renderCol = shard.baseColor;
            renderCol.x = (std::min)(1.0f, renderCol.x + flash * 0.55f);
            renderCol.y = (std::min)(1.0f, renderCol.y + flash * 0.55f);
            renderCol.z = (std::min)(1.0f, renderCol.z + flash * 0.55f);

            // スケール (手前への3D飛び出し感: 割れた直後に少し手前へ拡大し、その後奥へ遠ざかる)
            if (p < 0.20f) {
                shard.scale = 1.0f + 0.15f * (p / 0.20f);
            } else {
                shard.scale = 1.15f - 0.55f * ((p - 0.20f) / 0.80f);
            }

            // 後半からアルファ減衰
            if (p > 0.40f) {
                float fadeOutP = (p - 0.40f) / 0.60f;
                shard.alpha = std::clamp(1.0f - fadeOutP, 0.0f, 1.0f);
            } else {
                shard.alpha = 1.0f;
            }
            renderCol.w = shard.alpha;

            // スプライトの姿勢更新 (3D回転反映)
            Transform t = shard.sprite->GetTransform();
            t.translate = { shard.currentPos.x, shard.currentPos.y, 0.0f };
            t.scale = { shard.scale, shard.scale, 1.0f };
            t.rotate = shard.rotation; // 3D回転！
            shard.sprite->SetTransform(t);
            shard.sprite->GetMaterial()->SetColor(renderCol);
            shard.sprite->SettingWvp();
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

    const Vector2 screenSize = { 1280.0f, 720.0f };
    const Vector2 centerPos  = { 640.0f, 360.0f };

    if (phase_ == Phase::Freezing) {
        float p = std::clamp(timer_ / freezeDuration_, 0.0f, 1.0f);
        // 滑らかなスジ立ち・急峻化イージング
        float ease = p * p * (3.0f - 2.0f * p);

        // 1. 外周の冷気グラデーション (四辺を薄いアイスブルーの矩形で覆う)
        Vector4 coldBorderColor = { 0.65f, 0.88f, 1.0f, ease * 0.45f };
        draw.DrawFillRect({ 0.0f, 0.0f }, { screenSize.x, 30.0f * ease }, coldBorderColor);
        draw.DrawFillRect({ 0.0f, screenSize.y - 30.0f * ease }, { screenSize.x, 30.0f * ease }, coldBorderColor);
        draw.DrawFillRect({ 0.0f, 0.0f }, { 40.0f * ease, screenSize.y }, coldBorderColor);
        draw.DrawFillRect({ screenSize.x - 40.0f * ease, 0.0f }, { 40.0f * ease, screenSize.y }, coldBorderColor);

        // 2. 全面の氷テクスチャ (徐々に白く半透明で現れる)
        if (surfaceSprite_) {
            float surfaceAlpha = std::clamp((p - 0.2f) / 0.8f, 0.0f, 1.0f);
            surfaceAlpha = surfaceAlpha * surfaceAlpha * 0.92f;
            Transform t = surfaceSprite_->GetTransform();
            t.translate = { centerPos.x, centerPos.y, 0.0f };
            t.scale = { 1.0f, 1.0f, 1.0f };
            surfaceSprite_->SetTransform(t);
            surfaceSprite_->GetMaterial()->SetColor({ 0.88f, 0.95f, 1.0f, surfaceAlpha });
            surfaceSprite_->SettingWvp();
            draw.DrawSprite(surfaceSprite_.get());
        }

        // 3. 画面外周から迫る霜 (正位置)
        // 初期スケール 1.35 から 0.96 へ縮小することで、中心の穴が閉じていく！
        if (frostSprite1_) {
            float scale = 1.35f - 0.39f * ease;
            float frostAlpha = std::clamp(ease * 1.2f, 0.0f, 1.0f);
            Transform t = frostSprite1_->GetTransform();
            t.translate = { centerPos.x, centerPos.y, 0.0f };
            t.scale = { scale, scale, 1.0f };
            t.rotate = { 0.0f, 0.0f, 0.0f };
            frostSprite1_->SetTransform(t);
            frostSprite1_->GetMaterial()->SetColor({ 1.0f, 1.0f, 1.0f, frostAlpha });
            frostSprite1_->SettingWvp();
            draw.DrawSprite(frostSprite1_.get());
        }

        // 4. 画面外周から迫る霜 (180度回転・少し遅れて重なる)
        if (frostSprite2_ && p > 0.15f) {
            float p2 = (p - 0.15f) / 0.85f;
            float ease2 = p2 * p2;
            float scale2 = 1.45f - 0.45f * ease2;
            float frostAlpha2 = std::clamp(ease2 * 0.85f, 0.0f, 0.85f);
            Transform t = frostSprite2_->GetTransform();
            t.translate = { centerPos.x, centerPos.y, 0.0f };
            t.scale = { scale2, scale2, 1.0f };
            t.rotate = { 0.0f, 0.0f, 3.14159265f };
            frostSprite2_->SetTransform(t);
            frostSprite2_->GetMaterial()->SetColor({ 0.85f, 0.95f, 1.0f, frostAlpha2 });
            frostSprite2_->SettingWvp();
            draw.DrawSprite(frostSprite2_.get());
        }

        // 5. 氷晶スパークルの描画
        for (const auto& sp : sparkles_) {
            Vector4 col = sp.color;
            col.w *= sp.alpha;
            draw.DrawFillRect(sp.pos, { sp.size, sp.size }, col);
        }
    }
    else if (phase_ == Phase::Cracking) {
        // 完全に氷で覆われた画面
        if (surfaceSprite_) {
            surfaceSprite_->GetMaterial()->SetColor({ 0.88f, 0.95f, 1.0f, 0.95f });
            surfaceSprite_->SettingWvp();
            draw.DrawSprite(surfaceSprite_.get());
        }
        if (frostSprite1_) {
            Transform t = frostSprite1_->GetTransform();
            t.scale = { 0.96f, 0.96f, 1.0f };
            frostSprite1_->SetTransform(t);
            frostSprite1_->GetMaterial()->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
            frostSprite1_->SettingWvp();
            draw.DrawSprite(frostSprite1_.get());
        }

        // バキッ！と走る蜘蛛の巣状亀裂
        if (crackSprite_) {
            float p = std::clamp(timer_ / crackDuration_, 0.0f, 1.0f);
            // 瞬間フラッシュしてピキッと伸びる
            float crackScale = 0.92f + 0.08f * p;
            Transform t = crackSprite_->GetTransform();
            t.translate = { centerPos.x, centerPos.y, 0.0f };
            t.scale = { crackScale, crackScale, 1.0f };
            crackSprite_->SetTransform(t);
            // 発光感のある白水色
            crackSprite_->GetMaterial()->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
            crackSprite_->SettingWvp();
            draw.DrawSprite(crackSprite_.get());
        }

        // スパークルの描画
        for (const auto& sp : sparkles_) {
            Vector4 col = sp.color;
            col.w *= sp.alpha;
            draw.DrawFillRect(sp.pos, { sp.size, sp.size }, col);
        }
    }
    else if (phase_ == Phase::Shattering) {
        // パリーン！ガラス破片群の描画
        for (const auto& shard : shards_) {
            if (shard.alpha > 0.01f && shard.sprite) {
                draw.DrawSprite(shard.sprite.get());
            }
        }

        // 飛び散る氷粉ダイヤモンドダスト (ガラス微粉末)
        for (const auto& sp : sparkles_) {
            Vector4 col = sp.color;
            col.w *= sp.alpha;
            draw.DrawFillRect(sp.pos, { sp.size, sp.size }, col);
        }

        // 破砕瞬間のホワイトフラッシュインパクト (画面全体が一瞬光る)
        if (shatterFlashTimer_ > 0.0f) {
            float flashAlpha = (shatterFlashTimer_ / 0.12f) * 0.40f;
            draw.DrawFillRect({ 0.0f, 0.0f }, screenSize, { 1.0f, 1.0f, 1.0f, flashAlpha });
        }
    }
}

void FreezeTransition::EmitSparkles(const Vector2& center, int count, float minSpeed, float maxSpeed) {
    std::mt19937 rng(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(this) ^ std::rand()));
    std::uniform_real_distribution<float> randAngle(0.0f, 6.2831853f);
    std::uniform_real_distribution<float> randSpeed(minSpeed, maxSpeed);
    std::uniform_real_distribution<float> randSize(2.5f, 6.0f);
    std::uniform_real_distribution<float> randLife(0.35f, 0.75f);
    std::uniform_real_distribution<float> randOffset(-20.0f, 20.0f);

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

        // キラキラ光る白〜クリスタルシアン〜シルバー
        if (i % 4 == 0) {
            sp.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 純白
        } else if (i % 4 == 1) {
            sp.color = { 0.75f, 0.95f, 1.0f, 1.0f }; // シアン
        } else if (i % 4 == 2) {
            sp.color = { 0.88f, 0.96f, 1.0f, 1.0f }; // アイスブルー
        } else {
            sp.color = { 0.95f, 1.0f, 1.0f, 1.0f }; // プリズムシルバー
        }

        sparkles_.push_back(sp);
    }
}

void FreezeTransition::UpdateSparkles(float deltaTime) {
    const float gravity = 400.0f; // 氷粉のふわっとした重力

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
        it->velocity.x *= 0.96f;
        it->velocity.y *= 0.96f;

        // キラキラ瞬く (正弦波で明滅)
        float lifeRatio = it->life / it->maxLife;
        float twinkle = 0.7f + 0.3f * std::sin(it->life * 25.0f);
        it->alpha = (1.0f - lifeRatio) * twinkle;

        ++it;
    }
}

void FreezeTransition::Reset() {
    phase_ = Phase::None;
    timer_ = 0.0f;
    shatterFlashTimer_ = 0.0f;
    onMidpointCallback_ = nullptr;
    midpointExecuted_ = false;
    sparkles_.clear();
}

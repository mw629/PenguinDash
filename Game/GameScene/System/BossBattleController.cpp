#include "BossBattleController.h"
#include "../Camera/GameCameraController.h"
#include "../Effect/GameSceneEffect.h"
#include "SoundManager.h"
#include <Engine.h>
#include <Entity/Boss.h>
#include <Entity/Collision.h>
#include <Input/GamePadInput.h>
#include <Input/Input.h>
#include <Stage/StageSettings.h>
#include <cmath>
#include <cstdlib>
#include <imgui.h>

void BossBattleController::Reset() {
  bossAttackTimer_ = 0.0f;
  currentReflectLane_ = kNoReflectLane;
  attackQueue_.clear();
  lastPatternIndex_ = -1;
  currentPattern_ = PatternType::Standard;
}

void BossBattleController::SpawnAttack(StageSettings *stageSettings, int lane,
                                       Obstacle::Type type) {
  if (!stageSettings)
    return;

  float targetY = kBaseSpawnY;
  if (type == Obstacle::Type::BossAttackLow) {
    targetY = 2.5f; // StageSettings::kObstacleSpawnYLow
  } else if (type == Obstacle::Type::BossAttackHigh) {
    targetY = 4.6f; // StageSettings::kObstacleSpawnYHigh
  }

  for (int j = 0; j < stageSettings->GetMaxObstacles(); j++) {
    Obstacle *obs = stageSettings->GetObstacle(j);
    if (!obs->GetIsActive()) {
      obs->SetType(type);
      obs->SetDropHeight(bossAttackDropHeight_);
      obs->SetFallDuration(bossAttackFallDuration_);
      obs->SetReflectArcHeight(bossReflectArcHeight_);
      obs->SetReflectDuration(bossReflectDuration_);
      float x = (lane - 1) * stageSettings->GetLaneWidth();
      float finalY = targetY;
      if (type == Obstacle::Type::BossAttack ||
          type == Obstacle::Type::BossAttackReflectable) {
        finalY = targetY + obs->GetCollisionHeight() * kHalfFactor;
      }
      obs->Spawn(x, finalY, bossAttackSpawnZ_);
      break;
    }
  }
}

void BossBattleController::EnqueuePattern(PatternType pattern) {
  currentPattern_ = pattern;

  switch (pattern) {
  case PatternType::Standard: {
    // 3レーンのうち、1つを安全地帯、1つを白(通常)、1つを緑(跳ね返し)にする
    int safeLane = rand() % kLaneCount;
    int greenLane = (safeLane + 1 + rand() % (kLaneCount - 1)) % kLaneCount;

    for (int i = 0; i < kLaneCount; i++) {
      if (i == safeLane)
        continue;

      Obstacle::Type type = Obstacle::Type::BossAttack;
      if (i == greenLane) {
        type = Obstacle::Type::BossAttackReflectable;
      }
      attackQueue_.push_back({0.0f, i, type});
    }
    break;
  }

  case PatternType::JumpAction: {
    // アクション強制：全レーンに低空障害物（ジャンプで飛び越える！）
    for (int i = 0; i < kLaneCount; i++) {
      attackQueue_.push_back({0.0f, i, Obstacle::Type::BossAttackLow});
    }
    // ジャンプ直後に緑の反射弾が追撃（約40フレーム遅延）
    int greenLane = rand() % kLaneCount;
    attackQueue_.push_back({40.0f, greenLane, Obstacle::Type::BossAttackReflectable});
    break;
  }

  case PatternType::SlideAction: {
    // アクション強制：全レーンに高空アーチ（スライディング/ロールでくぐる！）
    for (int i = 0; i < kLaneCount; i++) {
      attackQueue_.push_back({0.0f, i, Obstacle::Type::BossAttackHigh});
    }
    // スライディング直後に緑の反射弾が追撃（約40フレーム遅延）
    int greenLane = rand() % kLaneCount;
    attackQueue_.push_back({40.0f, greenLane, Obstacle::Type::BossAttackReflectable});
    break;
  }

  case PatternType::SequentialWave: {
    // 時間差・ウェーブ連続落下（メテオストーム）
    // 左→中→右 または 右→中→左 へ連続して着地！
    bool leftToRight = (rand() % 2 == 0);
    int lanes[3] = {0, 1, 2};
    if (!leftToRight) {
      lanes[0] = 2;
      lanes[2] = 0;
    }
    float stepDelay = 16.0f; // 約0.27秒ごとの時間差
    attackQueue_.push_back({0.0f, lanes[0], Obstacle::Type::BossAttack});
    attackQueue_.push_back({stepDelay, lanes[1], Obstacle::Type::BossAttack});
    attackQueue_.push_back({stepDelay * 2.0f, lanes[2], Obstacle::Type::BossAttackReflectable});
    break;
  }

  case PatternType::DoubleWave: {
    // 2連波状攻撃
    // 第1波: 左右に通常爆弾（中央が安全）
    attackQueue_.push_back({0.0f, 0, Obstacle::Type::BossAttack});
    attackQueue_.push_back({0.0f, 2, Obstacle::Type::BossAttack});
    // 第2波: 約32フレーム後に中央に通常爆弾、左右どちらかに緑反射爆弾
    int greenLane = (rand() % 2 == 0) ? 0 : 2;
    attackQueue_.push_back({32.0f, 1, Obstacle::Type::BossAttack});
    attackQueue_.push_back({32.0f, greenLane, Obstacle::Type::BossAttackReflectable});
    break;
  }

  default:
    break;
  }
}

BossBattleController::PatternType

BossBattleController::SelectNextPattern(int bossHP) {
  std::vector<PatternType> pool;
  if (bossHP >= 4) {
    // 序盤: 基本攻撃、時間差ウェーブ、ジャンプアクション
    pool = {PatternType::Standard, PatternType::SequentialWave,
            PatternType::JumpAction, PatternType::Standard};
  } else if (bossHP >= 2) {
    // 中盤: 全パターンがバランスよく登場
    pool = {PatternType::Standard, PatternType::JumpAction,
            PatternType::SlideAction, PatternType::SequentialWave,
            PatternType::DoubleWave};
  } else {
    // 終盤（激化）: アクション強制やウェーブ攻撃の比率が増加
    pool = {PatternType::JumpAction, PatternType::SlideAction,
            PatternType::SequentialWave, PatternType::DoubleWave};
  }

  // 直前と同じパターンが連続しにくいように選択
  PatternType selected = pool[rand() % pool.size()];
  if (pool.size() > 1 && static_cast<int>(selected) == lastPatternIndex_) {
    selected = pool[(rand() + 1) % pool.size()];
  }
  lastPatternIndex_ = static_cast<int>(selected);
  return selected;
}

void BossBattleController::Update(float timeScale, Boss *boss,
                                  StageSettings *stageSettings,
                                  GameSceneEffect *effectManager,
                                  GameCameraController *cameraController) {
  if (!boss || !stageSettings)
    return;

  // ボス戦中はボスアイテムのクールタイムを満タンに維持
  float bossItemDuration =
      stageSettings->GetItemCoolDownDuration(Obstacle::Type::BossItem);
  stageSettings->SetItemCoolDownTimer(Obstacle::Type::BossItem,
                                      bossItemDuration);

  currentReflectLane_ = kNoReflectLane;

  if (boss->GetIsActive() && boss->GetState() == BossState::Battle) {
    // 攻撃キューの更新・スポーン
    if (!attackQueue_.empty()) {
      for (auto it = attackQueue_.begin(); it != attackQueue_.end();) {
        it->delayFrames -= timeScale;
        if (it->delayFrames <= 0.0f) {
          SpawnAttack(stageSettings, it->lane, it->type);
          it = attackQueue_.erase(it);
        } else {
          ++it;
        }
      }
    }

    // キューが空のときにインターバルタイマーを進める（約2秒に1回）
    if (attackQueue_.empty()) {
      bossAttackTimer_ += timeScale;
      if (bossAttackTimer_ >= kAttackIntervalFrames) {
        bossAttackTimer_ -= kAttackIntervalFrames;
        PatternType nextPattern = SelectNextPattern(boss->GetHP());
        EnqueuePattern(nextPattern);
      }
    }

    // 跳ね返し入力判定 (キーボード 1/2/3 または コントローラー X/Y/B)
    bool push1 = Input::PushKey(DIK_1) || Input::PushKey(DIK_NUMPAD1) ||
                 GamePadInput::PushButton(XINPUT_GAMEPAD_X);
    bool push2 = Input::PushKey(DIK_2) || Input::PushKey(DIK_NUMPAD2) ||
                 GamePadInput::PushButton(XINPUT_GAMEPAD_Y);
    bool push3 = Input::PushKey(DIK_3) || Input::PushKey(DIK_NUMPAD3) ||
                 GamePadInput::PushButton(XINPUT_GAMEPAD_B);

    AABB bossAABB =
        Collision::MakeAABB(boss->GetTransform(), kBossAABBSize, kBossAABBSize,
                            kBossAABBSize);

    for (int j = 0; j < stageSettings->GetMaxObstacles(); j++) {
      Obstacle *obs = stageSettings->GetObstacle(j);
      if (!obs || !obs->GetIsActive())
        continue;

      // 着地時の砂煙エフェクト＆着地音
      if (obs->GetJustLanded()) {
        if (effectManager) {
          effectManager->EmitDust(obs->GetTransform().translate);
        }
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossLand);
      }

      if (obs->GetType() == Obstacle::Type::BossAttackReflectable &&
          !obs->GetIsReflected() && !obs->GetIsFalling()) {
        float obsX = obs->GetTransform().translate.x;
        float laneW = stageSettings->GetLaneWidth();
        int lane = 1; // 0:画面左([1]/X), 1:画面中央([2]/Y), 2:画面右([3]/B)
        // ボス戦カメラ（Y回転180度）ではワールド+Xが画面左、ワールド-Xが画面右に見える
        float halfLaneW = laneW * kHalfFactor;
        if (obsX > halfLaneW)
          lane = 0;
        else if (obsX < -halfLaneW)
          lane = 2;

        float z = obs->GetTransform().translate.z;
        if (z > bossAttackReflectMinZ_ && z < kAttackReflectMaxZ) {
          currentReflectLane_ = lane;

          if ((lane == 0 && push1) || (lane == 1 && push2) ||
              (lane == 2 && push3)) {
            obs->SetReflectArcHeight(bossReflectArcHeight_);
            obs->SetReflectDuration(bossReflectDuration_);
            obs->SetReflected(true);
            obs->SetReflectedTarget(boss->GetTransform().translate);
            SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossReflect);
            GamePadInput::Rumble(kReflectRumbleDuration, kReflectRumbleSpeed,
                                 kReflectRumbleSpeed);
            if (cameraController) {
              cameraController->ShakeReflect();
            }
          }
        }
      } else if (obs->GetIsReflected()) {
        // ボスの最新位置を追従
        obs->SetReflectedTarget(boss->GetTransform().translate);

        // 跳ね返った障害物とボスとの当たり判定
        AABB obsAABB = Collision::MakeAABB(
            obs->GetTransform(), obs->GetCollisionWidth(),
            obs->GetCollisionHeight(), obs->GetCollisionDepth());

        if ((obs->GetReflectProgress() >= kReflectHitMinProgress &&
             Collision::CheckAABB(bossAABB, obsAABB)) ||
            obs->GetReflectProgress() >= 1.0f) {
          obs->Deactivate();
          boss->OnDamage();
          SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossHit);
          if (cameraController) {
            cameraController->ShakeDefeat();
          }

          if (boss->GetState() == BossState::Defeat) {
            SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossDefeat);
            GamePadInput::Rumble(kDefeatRumbleDuration, kMaxRumbleSpeed,
                                 kMaxRumbleSpeed);
            if (cameraController) {
              cameraController->ShakeBossDefeat();
            }
            attackQueue_.clear();
            // 画面内のボス攻撃をすべて消す
            for (int k = 0; k < stageSettings->GetMaxObstacles(); k++) {
              Obstacle *o = stageSettings->GetObstacle(k);
              if (o && o->GetIsActive() && o->IsBossAttackType()) {
                o->OnBlowAway();
              }
            }
          }
        }
      }
    }
  }
}

void BossBattleController::ImGui(Boss *boss,
                                 GameCameraController *cameraController,
                                 bool isPlayingBoss,
                                 bool isCameraTransitioning) {
#ifdef _USE_IMGUI
  if (boss && boss->GetIsActive()) {
    if (ImGui::TreeNode("ボス情報")) {
      ImGui::Text("HP: %d / %d", boss->GetHP(), boss->GetMaxHP());
      ImGui::DragFloat3("ボス目標位置", &boss->GetTargetPosRef().x, 0.1f);
      ImGui::DragFloat("攻撃生成距離 Z", &bossAttackSpawnZ_, 0.5f, -100.0f,
                       0.0f, "%.1f m");
      ImGui::DragFloat("攻撃落下開始高度", &bossAttackDropHeight_, 0.5f,
                       5.0f, 40.0f, "%.1f m");
      ImGui::DragFloat("攻撃落下フレーム数", &bossAttackFallDuration_, 1.0f,
                       5.0f, 60.0f, "%.0f frames");
      ImGui::DragFloat("反射可能最小距離 Z", &bossAttackReflectMinZ_, 0.5f, -40.0f,
                       0.0f, "%.1f m");
      ImGui::DragFloat("反射円弧の高さ", &bossReflectArcHeight_, 0.2f,
                       1.0f, 25.0f, "%.1f m");
      ImGui::DragFloat("反射飛翔フレーム数", &bossReflectDuration_, 1.0f,
                       10.0f, 120.0f, "%.0f frames");
      ImGui::Text("プレイヤーまでの攻撃生成距離: %.1f m", -bossAttackSpawnZ_);

      ImGui::Separator();
      ImGui::Text("攻撃パターン:");
      const char *patternNames[] = {
          "通常 (安全1 / ボム1 / 反射弾1)",
          "ジャンプアクション (低波 + 反射弾)",
          "スライドアクション (高アーチ + 反射弾)",
          "連続ウェーブ (左->右 または 右->左)",
          "二重ウェーブ (2段メテオ)"};
      ImGui::Text("現在のパターン: %s",
                  patternNames[static_cast<int>(currentPattern_)]);
      ImGui::Text("待機中攻撃数: %zu 発", attackQueue_.size());
      ImGui::Text("攻撃間隔タイマー: %.1f / %.1f", bossAttackTimer_,
                  kAttackIntervalFrames);

      if (ImGui::Button("発動: 通常")) {
        TriggerPattern(PatternType::Standard);
      }
      ImGui::SameLine();
      if (ImGui::Button("発動: ジャンプ")) {
        TriggerPattern(PatternType::JumpAction);
      }
      if (ImGui::Button("発動: スライド")) {
        TriggerPattern(PatternType::SlideAction);
      }
      ImGui::SameLine();
      if (ImGui::Button("発動: 連続ウェーブ")) {
        TriggerPattern(PatternType::SequentialWave);
      }
      if (ImGui::Button("発動: 二重ウェーブ")) {
        TriggerPattern(PatternType::DoubleWave);
      }

      ImGui::Separator();
      ImGui::Text("ボスカメラ設定:");
      if (ImGui::DragFloat3("ボスカメラ位置", &bossCameraTranslate_.x, 0.1f)) {
        if (isPlayingBoss && !isCameraTransitioning && cameraController) {
          Transform ct = cameraController->GetTransform();
          ct.translate = bossCameraTranslate_;
          cameraController->SetTransform(ct);
        }
      }
      if (ImGui::DragFloat3("ボスカメラ回転", &bossCameraRotate_.x, 0.01f)) {
        if (isPlayingBoss && !isCameraTransitioning && cameraController) {
          Transform ct = cameraController->GetTransform();
          ct.rotate = bossCameraRotate_;
          cameraController->SetTransform(ct);
        }
      }
      ImGui::TreePop();
    }
  }
#endif
}


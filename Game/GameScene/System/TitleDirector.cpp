#include "TitleDirector.h"
#include "../Camera/GameCameraController.h"
#include "ScoreManager.h"
#include "SoundManager.h"
#include <Entity/Collision.h>
#include <Entity/Player.h>
#include <Graphics/Render/Draw.h>
#include <Stage/StageSettings.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <imgui.h>

void TitleDirector::Initialize(Texture *texture) {
  if (!texture)
    return;

  // カウントダウン用の4種スプライト (3, 2, 1, GO!)
  const char *texturePaths[4] = {
      "Resources/Texture/penguin_count_3.png",
      "Resources/Texture/penguin_count_2.png",
      "Resources/Texture/penguin_count_1.png",
      "Resources/Texture/penguin_count_go.png",
  };

  for (int i = 0; i < 4; ++i) {
    countTextureHandles_[i] = texture->CreateTexture(texturePaths[i]);

    SpriteData data;
    data.transform.scale = {1.0f, 1.0f, 1.0f};
    data.transform.translate = {kPenguinCenterPos.x, kPenguinCenterPos.y, 0.0f};
    data.transform.rotate = {0.0f, 0.0f, 0.0f};
    data.size = (i == 3) ? kGoSpriteSize : kCountSpriteSize;
    data.pivot = kCenterPivot;
    data.textureArea[0] = {0.0f, 0.0f};
    data.textureArea[1] = {1.0f, 1.0f};
    data.scaleMode = SpriteScaleMode::Fit;

    countSprites_[i] = std::make_unique<Sprite>();
    countSprites_[i]->Initialize(data, countTextureHandles_[i]);
    countSprites_[i]->GetMaterial()->SetColor(kDefaultSpriteColor);
  }
}

void TitleDirector::Reset() {
  countdownMode_ = CountdownMode::None;
  countdownTimer_ = 0.0f;
  prepDuration_ = 0.0f;
  totalDuration_ = 0.0f;
  lastCountStep_ = -1;
  titleCameraAngle_ = 0.0f;
}

void TitleDirector::StartExit(const Transform &currentCamTransform) {
  if (countdownMode_ != CountdownMode::None)
    return;

  countdownMode_ = CountdownMode::TitleExit;
  prepDuration_ = kTitlePrepDuration;  // 1.0s: カメラ復帰＆障害物クリア待ち
  totalDuration_ = kTitleExitDuration; // 4.4s: 1.0s + 2.4s(3,2,1) + 1.0s(GO)
  countdownTimer_ = 0.0f;
  lastCountStep_ = -1;
  titleExitStartCamTransform_ = currentCamTransform;
}

void TitleDirector::StartRestartCountdown() {
  countdownMode_ = CountdownMode::Restart;
  prepDuration_ = kRestartPrepDuration; // 0.6s: 画面切り替え直後にしっかり間を持たせる！
  totalDuration_ = kRestartDuration;    // 4.15s: 0.6s(準備間) + 2.55s(3,2,1) + 1.0s(GO)
  countdownTimer_ = 0.0f;
  lastCountStep_ = -1; // 準備ディレイ終了後に初めて「3」のSEを鳴らす
}

void TitleDirector::UpdateCountdown(float dt) {
  if (countdownMode_ == CountdownMode::None)
    return;

  countdownTimer_ += dt;

  // 準備期間（カメラ復帰中）を引いたアクティブタイマー
  float activeTimer = countdownTimer_ - prepDuration_;
  if (activeTimer >= 0.0f) {
    int currentStep = static_cast<int>(activeTimer / kCountdownStepDuration);
    if (currentStep != lastCountStep_) {
      if (currentStep >= 0 && currentStep <= 2) {
        // 「3」「2」「1」のカウント音
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet, 1.2f);
      } else if (currentStep == 3 && lastCountStep_ < 3) {
        // 「GO!」のスタート音
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Start, 1.4f);
      }
      lastCountStep_ = currentStep;
    }
  }

  // 演出完了
  if (countdownTimer_ >= totalDuration_) {
    countdownMode_ = CountdownMode::None;
  }
}

bool TitleDirector::IsExitComplete(StageSettings *stageSettings) const {
  if (countdownMode_ != CountdownMode::TitleExit)
    return false;

  bool hasActiveObstacles = false;
  if (stageSettings) {
    for (int i = 0; i < stageSettings->GetMaxObstacles(); i++) {
      Obstacle *obs = stageSettings->GetObstacle(i);
      if (obs && obs->GetIsActive()) {
        hasActiveObstacles = true;
        break;
      }
    }
  }

  return (!hasActiveObstacles && countdownTimer_ >= kTitleExitDuration) ||
         (countdownTimer_ >= kTitleExitDuration + kExitTimeoutOffset);
}

void TitleDirector::UpdateAutoPilot(Player *player,
                                    StageSettings *stageSettings) {
  if (!player || !stageSettings)
    return;

  int currentLane = player->GetLaneIndex();
  int targetLane = player->GetTargetLaneIndex();
  int activeLane = player->IsChangingLane() ? targetLane : currentLane;

  float laneWidth = stageSettings->GetLaneWidth();
  if (laneWidth <= 0.0f)
    laneWidth = kAiDefaultLaneWidth;

  float playerZ = player->GetTransform().translate.z;

  struct LaneInfo {
    float minDistZ = kAiMaxDetectionDist;
    Obstacle::Type minType = Obstacle::Type::Wall;
    Obstacle *minObs = nullptr;
    float minWallDistZ = kAiMaxDetectionDist;
  };
  LaneInfo lanes[kLaneCount];

  Obstacle *imminentObs = nullptr;
  float minActiveDistZ = kAiMaxDetectionDist;

  for (int i = 0; i < stageSettings->GetMaxObstacles(); i++) {
    Obstacle *obs = stageSettings->GetObstacle(i);
    if (!obs || !obs->GetIsActive() || obs->GetIsHit())
      continue;

    float relZ = obs->GetTransform().translate.z - playerZ;
    if (relZ < kAiBehindIgnoreDist)
      continue;

    float obsX = obs->GetTransform().translate.x;
    int obsLane = static_cast<int>(std::round(obsX / laneWidth));
    if (obsLane < -1)
      obsLane = -1;
    if (obsLane > 1)
      obsLane = 1;

    int idx = obsLane + 1;

    if (relZ < lanes[idx].minDistZ) {
      lanes[idx].minDistZ = relZ;
      lanes[idx].minType = obs->GetType();
      lanes[idx].minObs = obs;
    }

    if (obs->RequiresLaneChangeToAvoid()) {
      if (relZ < lanes[idx].minWallDistZ) {
        lanes[idx].minWallDistZ = relZ;
      }
    }

    if (obsLane == activeLane && relZ > kAiCloseIgnoreDist &&
        relZ < minActiveDistZ) {
      minActiveDistZ = relZ;
      imminentObs = obs;
    }
  }

  if (imminentObs) {
    Obstacle::Type type = imminentObs->GetType();

    if (type == Obstacle::Type::Low) {
      if (minActiveDistZ <= kAiJumpTriggerDistMax &&
          minActiveDistZ >= kAiJumpTriggerDistMin) {
        if (!player->GetIsJumping()) {
          player->TriggerJump();
        }
      }
    } else if (type == Obstacle::Type::High) {
      if (minActiveDistZ <= kAiRollTriggerDistMax &&
          minActiveDistZ >= kAiRollTriggerDistMin) {
        if (!player->GetIsRolling() && !player->GetIsJumping()) {
          player->TriggerRoll();
        }
      }
    } else if (type == Obstacle::Type::Wall ||
               type == Obstacle::Type::BossAttack ||
               type == Obstacle::Type::BossAttackReflectable) {
      if (minActiveDistZ <= kAiWallAvoidDist && !player->IsChangingLane() &&
          player->CanAct()) {
        if (activeLane == 0) {
          float leftWallDist = lanes[0].minWallDistZ;
          float rightWallDist = lanes[2].minWallDistZ;
          float leftMinDist = lanes[0].minDistZ;
          float rightMinDist = lanes[2].minDistZ;

          if (leftWallDist > kAiWallAvoidDist &&
              rightWallDist <= kAiWallAvoidDist) {
            player->TriggerMoveLeft();
          } else if (rightWallDist > kAiWallAvoidDist &&
                     leftWallDist <= kAiWallAvoidDist) {
            player->TriggerMoveRight();
          } else if (leftWallDist > kAiWallAvoidDist &&
                     rightWallDist > kAiWallAvoidDist) {
            if (leftMinDist >= rightMinDist && leftMinDist > kAiSafeSideDist) {
              player->TriggerMoveLeft();
            } else if (rightMinDist > kAiSafeSideDist) {
              player->TriggerMoveRight();
            } else if (leftMinDist >= rightMinDist) {
              player->TriggerMoveLeft();
            } else {
              player->TriggerMoveRight();
            }
          } else {
            if (leftWallDist > rightWallDist) {
              player->TriggerMoveLeft();
            } else {
              player->TriggerMoveRight();
            }
          }
        } else if (activeLane == -1) {
          if (lanes[1].minWallDistZ > kAiCenterLaneClearDist) {
            player->TriggerMoveRight();
          }
        } else if (activeLane == 1) {
          if (lanes[1].minWallDistZ > kAiCenterLaneClearDist) {
            player->TriggerMoveLeft();
          }
        }
      }
    }
  }

  // 中央レーンへの安全復帰
  if ((!imminentObs || minActiveDistZ > kAiReturnCenterTriggerDist) &&
      !player->IsChangingLane() && player->CanAct()) {
    if (activeLane != 0) {
      if (lanes[1].minDistZ > kAiReturnCenterMinDist &&
          lanes[1].minWallDistZ > kAiReturnCenterWallDist) {
        if (activeLane < 0) {
          player->TriggerMoveRight();
        } else {
          player->TriggerMoveLeft();
        }
      }
    }
  }
}

void TitleDirector::CheckCollisions(Player *player,
                                    StageSettings *stageSettings,
                                    ScoreManager *scoreManager) {
  if (!player || !stageSettings)
    return;

  AABB playerAABB = player->GetColliderAABB();

  for (int i = 0; i < stageSettings->GetMaxObstacles(); i++) {
    Obstacle *obstacle = stageSettings->GetObstacle(i);
    if (!obstacle || !obstacle->GetIsActive() || obstacle->GetIsHit())
      continue;

    AABB obstacleAABB = obstacle->GetColliderAABB();

    if (Collision::CheckAABB(playerAABB, obstacleAABB)) {
      if (obstacle->GetType() == Obstacle::Type::Bonus) {
        obstacle->OnBlowAway();
        if (scoreManager) {
          scoreManager->AddBonusEnemyHit();
        }
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::BonusHit);
      } else {
        // 通常の障害物に接触してもゲームオーバーにせず吹き飛ばす
        obstacle->OnBlowAway();
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::BarrierBreak);
      }
    }
  }
}

void TitleDirector::UpdateCamera(Player *player,
                                 GameCameraController *cameraController,
                                 float dt) {
  if (!player || !cameraController || !isTitleOrbitCamera_)
    return;

  Vector3 centerPos = {0.0f, player->GetBaseHeight(),
                       player->GetTransform().translate.z};
  Vector3 targetPos = {centerPos.x, centerPos.y + titleTargetOffsetY_,
                       centerPos.z};

  const Transform &defaultCamTransform =
      GameCameraController::kDefaultCameraTransform;

  Transform camTransform;

  if (countdownMode_ == CountdownMode::TitleExit) {
    const float camTransitionDuration = 1.2f;
    float progress =
        std::clamp(countdownTimer_ / camTransitionDuration, 0.0f, 1.0f);

    float easeT = (progress < 0.5f)
                      ? (2.0f * progress * progress)
                      : (1.0f - std::pow(-2.0f * progress + 2.0f, 2.0f) * 0.5f);

    camTransform.translate = Lerp(titleExitStartCamTransform_.translate,
                                  defaultCamTransform.translate, easeT);
    camTransform.scale = Lerp(titleExitStartCamTransform_.scale,
                              defaultCamTransform.scale, easeT);

    float startY = titleExitStartCamTransform_.rotate.y;
    float targetY = defaultCamTransform.rotate.y;
    float diffY = targetY - startY;
    while (diffY > kPi)
      diffY -= k2Pi;
    while (diffY < -kPi)
      diffY += k2Pi;

    camTransform.rotate.x = Lerp(titleExitStartCamTransform_.rotate.x,
                                 defaultCamTransform.rotate.x, easeT);
    camTransform.rotate.y = startY + diffY * easeT;
    camTransform.rotate.z = Lerp(titleExitStartCamTransform_.rotate.z,
                                 defaultCamTransform.rotate.z, easeT);
  } else {
    titleCameraAngle_ += titleCameraSpeed_ * dt;
    if (titleCameraAngle_ >= k2Pi) {
      titleCameraAngle_ -= k2Pi;
    } else if (titleCameraAngle_ < 0.0f) {
      titleCameraAngle_ += k2Pi;
    }

    Vector3 camPos;
    camPos.x = targetPos.x + std::sin(titleCameraAngle_) * titleCameraRadius_;
    camPos.y = targetPos.y + titleCameraHeight_;
    camPos.z = targetPos.z - std::cos(titleCameraAngle_) * titleCameraRadius_;

    Vector3 dir = {targetPos.x - camPos.x, targetPos.y - camPos.y,
                   targetPos.z - camPos.z};
    float distXZ = std::sqrt(dir.x * dir.x + dir.z * dir.z);

    camTransform.scale = {1.0f, 1.0f, 1.0f};
    camTransform.translate = camPos;
    camTransform.rotate.x = std::atan2(-dir.y, distXZ);
    camTransform.rotate.y = std::atan2(dir.x, dir.z);
    camTransform.rotate.z = 0.0f;
  }

  cameraController->SetTransform(camTransform);
}

void TitleDirector::DrawCountdown(Draw &draw) {
  if (countdownMode_ == CountdownMode::None)
    return;

  float timer = countdownTimer_;
  if (timer > totalDuration_)
    return;

  // 準備期間中 (カメラ復帰中) はまだカウントダウンを描画しない
  float activeTimer = timer - prepDuration_;
  if (activeTimer < 0.0f)
    return;

  // ステップ算出: 0="3", 1="2", 2="1", 3="GO!"
  int step = static_cast<int>(activeTimer / kCountdownStepDuration);
  if (step > 3)
    step = 3;
  float stepTimer = activeTimer - step * kCountdownStepDuration;
  float stepProgress =
      std::clamp(stepTimer / kCountdownStepDuration, 0.0f, 1.0f);

  // 1. 集中背景演出（中央を引き立てるシネマティックな暗転）
  float bgAlpha = std::clamp(activeTimer * 2.0f, 0.0f, 0.28f);
  float remaining = totalDuration_ - timer;
  if (remaining < 0.4f) {
    // 終了直前にスムーズにフェードアウト
    bgAlpha = std::clamp(remaining / 0.4f, 0.0f, 1.0f) * 0.28f;
  }
  if (bgAlpha > 0.01f) {
    draw.DrawFillRect(Vector2(0.0f, 0.0f), Vector2(1280.0f, 720.0f),
                      Vector4(0.02f, 0.04f, 0.08f, bgAlpha));
  }

  // 2. 「GO!」の瞬間のホワイトフラッシュ演出
  if (step == 3 && stepTimer < 0.25f) {
    float flashAlpha = (1.0f - (stepTimer / 0.25f)) * 0.35f;
    draw.DrawFillRect(Vector2(0.0f, 0.0f), Vector2(1280.0f, 720.0f),
                      Vector4(1.0f, 1.0f, 1.0f, flashAlpha));
  }

  // 3. ペンギンの位置・バウンス・スケール計算
  Vector2 basePos = kPenguinCenterPos;
  float jumpY = 0.0f;
  float rotZ = 0.0f;
  Vector2 scale = {1.0f, 1.0f};

  if (step < 3) {
    // カウントダウン (3, 2, 1): 各カウント開始時にピョンと跳ねる
    float jumpT = std::clamp(stepProgress * 2.5f, 0.0f, 1.0f);
    jumpY = -std::sin(jumpT * 3.14159f) * 35.0f;
    scale.y = 1.0f - jumpY * 0.003f;
    scale.x = 1.0f + jumpY * 0.002f;
  } else {
    // GO!: バナーを掲げて歓喜の大ジャンプ！
    float jumpT = std::clamp(stepProgress * 1.5f, 0.0f, 1.0f);
    jumpY = -std::sin(jumpT * 3.14159f) * 75.0f;
    rotZ = std::sin(stepTimer * 16.0f) * 0.06f;
    scale = {1.08f, 1.08f};
    if (remaining < 0.4f) {
      // 終了間際に前方に飛び出すようなスケールアップ
      float exitT = (0.4f - remaining) / 0.4f;
      scale.x += exitT * 0.35f;
      scale.y += exitT * 0.35f;
    }
  }

  // 4. ペンギンの足元にドロップシャドウ（影）
  float shadowWidth = 140.0f * (1.0f + jumpY * 0.004f);
  float shadowAlpha = std::clamp(0.45f + jumpY * 0.004f, 0.15f, 0.45f);
  if (shadowWidth > 40.0f) {
    draw.DrawFillRect(
        Vector2(basePos.x - shadowWidth * 0.5f, basePos.y + 130.0f),
        Vector2(shadowWidth, 14.0f),
        Vector4(0.01f, 0.02f, 0.05f, shadowAlpha));
  }

  // 5. ペンギンスプライトの描画 (3, 2, 1, GO! それぞれの画像)
  Sprite *activePenguin = countSprites_[step].get();
  if (activePenguin) {
    Transform pt;
    pt.scale = {scale.x, scale.y, 1.0f};
    pt.rotate = {0.0f, 0.0f, rotZ};
    pt.translate = {basePos.x, basePos.y + jumpY, 0.0f};
    activePenguin->SetTransform(pt);
    activePenguin->SettingWvp();
    draw.DrawSprite(activePenguin);
  }
}

void TitleDirector::ImGui() {
#ifdef _USE_IMGUI
  if (ImGui::CollapsingHeader("Title Orbit Camera Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Enable Orbit Camera", &isTitleOrbitCamera_);
    ImGui::SliderFloat("Orbit Speed", &titleCameraSpeed_, -2.0f, 2.0f,
                       "%.2f rad/s");
    ImGui::DragFloat("Orbit Radius", &titleCameraRadius_, 0.2f, 3.0f, 40.0f);
    ImGui::DragFloat("Orbit Height", &titleCameraHeight_, 0.2f, -10.0f, 20.0f);
    ImGui::DragFloat("Target Offset Y", &titleTargetOffsetY_, 0.1f, -5.0f,
                     10.0f);
    ImGui::SliderAngle("Current Angle", &titleCameraAngle_);
  }
#endif
}

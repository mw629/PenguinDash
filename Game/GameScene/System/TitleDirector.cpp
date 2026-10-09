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
  if (!texture) return;

  titlePenguinTextureHandle_ = texture->CreateTexture("Resources/Texture/penguin_run.png");
  for (int i = 0; i < kTitlePenguinCount; ++i) {
    SpriteData penguinSpriteData;
    penguinSpriteData.transform.scale = {1.0f, 1.0f, 1.0f};
    penguinSpriteData.transform.translate = kPenguinInitialPos;
    penguinSpriteData.transform.rotate = {0.0f, 0.0f, 0.0f};
    penguinSpriteData.size = kPenguinSpriteSize;
    penguinSpriteData.pivot = kCenterPivot;
    penguinSpriteData.textureArea[0] = {0.0f, 0.0f};
    penguinSpriteData.textureArea[1] = {0.5f, 1.0f};
    penguinSpriteData.scaleMode = SpriteScaleMode::Fit;

    titlePenguinSprites_[i] = std::make_unique<Sprite>();
    titlePenguinSprites_[i]->Initialize(penguinSpriteData, titlePenguinTextureHandle_);
    titlePenguinSprites_[i]->GetMaterial()->SetColor(kDefaultSpriteColor);
  }
}

void TitleDirector::Reset() {
  isTitleExiting_ = false;
  titleExitTimer_ = 0.0f;
  titleCameraAngle_ = 0.0f;

  for (int i = 0; i < kTitlePenguinCount; ++i) {
    if (titlePenguinSprites_[i]) {
      Transform pt = titlePenguinSprites_[i]->GetTransform();
      pt.translate = kPenguinInitialPos;
      pt.rotate = {0.0f, 0.0f, 0.0f};
      titlePenguinSprites_[i]->SetTransform(pt);
      titlePenguinSprites_[i]->SettingWvp();
    }
  }
}

void TitleDirector::StartExit(const Transform &currentCamTransform) {
  if (isTitleExiting_) return;

  isTitleExiting_ = true;
  titleExitTimer_ = 0.0f;
  titleExitStartCamTransform_ = currentCamTransform;

  SoundManager::GetInstance()->PlaySE(SoundManager::SE::Start);
}

void TitleDirector::UpdateExit(float dt) {
  if (isTitleExiting_) {
    titleExitTimer_ += dt;
  }
}

bool TitleDirector::IsExitComplete(StageSettings *stageSettings) const {
  if (!isTitleExiting_) return false;

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

  return (!hasActiveObstacles && titleExitTimer_ >= kTitleExitDuration) ||
         (titleExitTimer_ >= kTitleExitDuration + kExitTimeoutOffset);
}

void TitleDirector::UpdateAutoPilot(Player *player, StageSettings *stageSettings) {
  if (!player || !stageSettings) return;

  int currentLane = player->GetLaneIndex();
  int targetLane = player->GetTargetLaneIndex();
  int activeLane = player->IsChangingLane() ? targetLane : currentLane;

  float laneWidth = stageSettings->GetLaneWidth();
  if (laneWidth <= 0.0f) laneWidth = kAiDefaultLaneWidth;

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
    if (!obs || !obs->GetIsActive() || obs->GetIsHit()) continue;

    float relZ = obs->GetTransform().translate.z - playerZ;
    if (relZ < kAiBehindIgnoreDist) continue;

    float obsX = obs->GetTransform().translate.x;
    int obsLane = static_cast<int>(std::round(obsX / laneWidth));
    if (obsLane < -1) obsLane = -1;
    if (obsLane > 1) obsLane = 1;

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

    if (obsLane == activeLane && relZ > kAiCloseIgnoreDist && relZ < minActiveDistZ) {
      minActiveDistZ = relZ;
      imminentObs = obs;
    }
  }

  if (imminentObs) {
    Obstacle::Type type = imminentObs->GetType();

    if (type == Obstacle::Type::Low) {
      if (minActiveDistZ <= kAiJumpTriggerDistMax && minActiveDistZ >= kAiJumpTriggerDistMin) {
        if (!player->GetIsJumping()) {
          player->TriggerJump();
        }
      }
    } else if (type == Obstacle::Type::High) {
      if (minActiveDistZ <= kAiRollTriggerDistMax && minActiveDistZ >= kAiRollTriggerDistMin) {
        if (!player->GetIsRolling() && !player->GetIsJumping()) {
          player->TriggerRoll();
        }
      }
    } else if (type == Obstacle::Type::Wall ||
               type == Obstacle::Type::BossAttack ||
               type == Obstacle::Type::BossAttackReflectable) {
      if (minActiveDistZ <= kAiWallAvoidDist && !player->IsChangingLane() && player->CanAct()) {
        if (activeLane == 0) {
          float leftWallDist = lanes[0].minWallDistZ;
          float rightWallDist = lanes[2].minWallDistZ;
          float leftMinDist = lanes[0].minDistZ;
          float rightMinDist = lanes[2].minDistZ;

          if (leftWallDist > kAiWallAvoidDist && rightWallDist <= kAiWallAvoidDist) {
            player->TriggerMoveLeft();
          } else if (rightWallDist > kAiWallAvoidDist && leftWallDist <= kAiWallAvoidDist) {
            player->TriggerMoveRight();
          } else if (leftWallDist > kAiWallAvoidDist && rightWallDist > kAiWallAvoidDist) {
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
  if ((!imminentObs || minActiveDistZ > kAiReturnCenterTriggerDist) && !player->IsChangingLane() && player->CanAct()) {
    if (activeLane != 0) {
      if (lanes[1].minDistZ > kAiReturnCenterMinDist && lanes[1].minWallDistZ > kAiReturnCenterWallDist) {
        if (activeLane < 0) {
          player->TriggerMoveRight();
        } else {
          player->TriggerMoveLeft();
        }
      }
    }
  }
}

void TitleDirector::CheckCollisions(Player *player, StageSettings *stageSettings,
                                    ScoreManager *scoreManager) {
  if (!player || !stageSettings) return;

  AABB playerAABB = player->GetColliderAABB();

  for (int i = 0; i < stageSettings->GetMaxObstacles(); i++) {
    Obstacle *obstacle = stageSettings->GetObstacle(i);
    if (!obstacle || !obstacle->GetIsActive() || obstacle->GetIsHit()) continue;

    AABB obstacleAABB = Collision::MakeAABB(
        obstacle->GetTransform(), obstacle->GetCollisionWidth(),
        obstacle->GetCollisionHeight(), obstacle->GetCollisionDepth());

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
  if (!player || !cameraController || !isTitleOrbitCamera_) return;

  Vector3 centerPos = {0.0f, player->GetBaseHeight(),
                       player->GetTransform().translate.z};
  Vector3 targetPos = {centerPos.x, centerPos.y + titleTargetOffsetY_,
                       centerPos.z};

  const Transform &defaultCamTransform = GameCameraController::kDefaultCameraTransform;

  Transform camTransform;

  if (isTitleExiting_) {
    float progress = (kTitleExitDuration > 0.0f) ? (titleExitTimer_ / kTitleExitDuration) : 1.0f;
    if (progress > 1.0f) progress = 1.0f;

    float easeT = (progress < 0.5f)
                      ? (2.0f * progress * progress)
                      : (1.0f - std::pow(-2.0f * progress + 2.0f, 2.0f) * 0.5f);

    camTransform.translate =
        Lerp(titleExitStartCamTransform_.translate, defaultCamTransform.translate, easeT);
    camTransform.scale =
        Lerp(titleExitStartCamTransform_.scale, defaultCamTransform.scale, easeT);

    float startY = titleExitStartCamTransform_.rotate.y;
    float targetY = defaultCamTransform.rotate.y;
    float diffY = targetY - startY;
    while (diffY > kPi) diffY -= k2Pi;
    while (diffY < -kPi) diffY += k2Pi;

    camTransform.rotate.x =
        Lerp(titleExitStartCamTransform_.rotate.x, defaultCamTransform.rotate.x, easeT);
    camTransform.rotate.y = startY + diffY * easeT;
    camTransform.rotate.z =
        Lerp(titleExitStartCamTransform_.rotate.z, defaultCamTransform.rotate.z, easeT);
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

void TitleDirector::DrawPenguins(Draw &draw) {
  if (!isTitleExiting_ || !titlePenguinSprites_[0]) return;

  float progress = std::clamp(titleExitTimer_ / kTitleExitDuration, 0.0f, 1.0f);
  float runProgress = progress * progress * (3.0f - 2.0f * progress) * 0.15f + progress * 0.85f;
  float runStartX = kStampedeStartX;
  float runEndX = kStampedeEndX;
  float leaderX = runStartX + (runEndX - runStartX) * runProgress;

  const float baseY = kStampedeBaseY;

  struct FormationParam {
    float offsetX;
    float offsetY;
    float animTimeOffset;
    float scale;
  };

  const float stepY = kStampedeStepY;
  const float slopeX = kStampedeSlopeX;

  FormationParam formations[kTitlePenguinCount];
  for (int i = 0; i < kTitlePenguinCount; ++i) {
    int distFromCenter = std::abs(i - 7);
    float offsetY = (i - 7) * stepY;
    float offsetX = -distFromCenter * slopeX;

    float depth = kStampedeBaseDepth + (offsetY / kStampedeMaxOffsetY) * kStampedeDepthScale;
    float scale = (i == 7) ? kStampedeLeaderScale : (1.00f * depth);
    float animTimeOffset = distFromCenter * kStampedeAnimTimeStep;

    formations[i] = {offsetX, offsetY, animTimeOffset, scale};
  }

  for (int idx = 0; idx < kTitlePenguinCount; ++idx) {
    if (!titlePenguinSprites_[idx]) continue;

    const auto &form = formations[idx];
    float posX = leaderX + form.offsetX;
    float posY = baseY + form.offsetY;

    float timerWithOffset = titleExitTimer_ + form.animTimeOffset;
    int animFrame = (static_cast<int>(timerWithOffset * kStampedeAnimFps)) % 2;
    Vector2 uvArea[2];
    if (animFrame == 0) {
      uvArea[0] = {0.0f, 0.0f};
      uvArea[1] = {0.5f, 1.0f};
    } else {
      uvArea[0] = {0.5f, 0.0f};
      uvArea[1] = {1.0f, 1.0f};
    }

    float bobbing = (animFrame == 1) ? -kStampedeBobbingHeight * (form.scale / 1.00f) : 0.0f;
    float tilt = kStampedeTiltAngle;

    Transform pTransform;
    pTransform.scale = {form.scale, form.scale, 1.0f};
    pTransform.rotate = {0.0f, 0.0f, tilt};
    pTransform.translate = {posX, posY + bobbing, 0.0f};

    titlePenguinSprites_[idx]->SetTransform(pTransform);
    titlePenguinSprites_[idx]->SetTextureArea(uvArea);
    titlePenguinSprites_[idx]->UpdateVertexBuffer();
    titlePenguinSprites_[idx]->SettingWvp();

    if (posX > 0.0f && posX < kStampedeMaxDrawX) {
      float trailAlpha = std::clamp(runProgress * kStampedeTrailAlphaScale, 0.0f, kStampedeTrailMaxAlpha) * form.scale;
      draw.DrawFillRect(Vector2(posX + kStampedeTrailOffset.x * form.scale, posY + kStampedeTrailOffset.y + bobbing),
                        Vector2(kStampedeTrailSize.x * form.scale, kStampedeTrailSize.y),
                        Vector4(0.85f, 0.95f, 1.0f, trailAlpha * kStampedeTrailAlphaMultiplier));
    }

    draw.DrawSprite(titlePenguinSprites_[idx].get());
  }
}

void TitleDirector::ImGui() {
#ifdef _USE_IMGUI
  if (ImGui::CollapsingHeader("Title Orbit Camera Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Enable Orbit Camera", &isTitleOrbitCamera_);
    ImGui::SliderFloat("Orbit Speed", &titleCameraSpeed_, -2.0f, 2.0f, "%.2f rad/s");
    ImGui::DragFloat("Orbit Radius", &titleCameraRadius_, 0.2f, 3.0f, 40.0f);
    ImGui::DragFloat("Orbit Height", &titleCameraHeight_, 0.2f, -10.0f, 20.0f);
    ImGui::DragFloat("Target Offset Y", &titleTargetOffsetY_, 0.1f, -5.0f, 10.0f);
    ImGui::SliderAngle("Current Angle", &titleCameraAngle_);
  }
#endif
}

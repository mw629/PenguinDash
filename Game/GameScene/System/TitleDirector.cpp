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
    penguinSpriteData.transform.translate = {-300.0f, 360.0f, 0.0f};
    penguinSpriteData.transform.rotate = {0.0f, 0.0f, 0.0f};
    penguinSpriteData.size = {130.0f, 130.0f};
    penguinSpriteData.pivot = {0.5f, 0.5f};
    penguinSpriteData.textureArea[0] = {0.0f, 0.0f};
    penguinSpriteData.textureArea[1] = {0.5f, 1.0f};
    penguinSpriteData.scaleMode = SpriteScaleMode::Fit;

    titlePenguinSprites_[i] = std::make_unique<Sprite>();
    titlePenguinSprites_[i]->Initialize(penguinSpriteData, titlePenguinTextureHandle_);
    titlePenguinSprites_[i]->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
  }
}

void TitleDirector::Reset() {
  isTitleExiting_ = false;
  titleExitTimer_ = 0.0f;
  titleCameraAngle_ = 0.0f;

  for (int i = 0; i < kTitlePenguinCount; ++i) {
    if (titlePenguinSprites_[i]) {
      Transform pt = titlePenguinSprites_[i]->GetTransform();
      pt.translate = {-300.0f, 360.0f, 0.0f};
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
         (titleExitTimer_ >= kTitleExitDuration + 3.0f);
}

void TitleDirector::UpdateAutoPilot(Player *player, StageSettings *stageSettings) {
  if (!player || !stageSettings) return;

  int currentLane = player->GetLaneIndex();
  int targetLane = player->GetTargetLaneIndex();
  int activeLane = player->IsChangingLane() ? targetLane : currentLane;

  float laneWidth = stageSettings->GetLaneWidth();
  if (laneWidth <= 0.0f) laneWidth = 2.0f;

  float playerZ = player->GetTransform().translate.z;

  struct LaneInfo {
    float minDistZ = 999.0f;
    Obstacle::Type minType = Obstacle::Type::Wall;
    Obstacle *minObs = nullptr;
    float minWallDistZ = 999.0f;
  };
  LaneInfo lanes[3];

  Obstacle *imminentObs = nullptr;
  float minActiveDistZ = 999.0f;

  for (int i = 0; i < stageSettings->GetMaxObstacles(); i++) {
    Obstacle *obs = stageSettings->GetObstacle(i);
    if (!obs || !obs->GetIsActive() || obs->GetIsHit()) continue;

    float relZ = obs->GetTransform().translate.z - playerZ;
    if (relZ < -0.8f) continue;

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

    if (obs->GetType() == Obstacle::Type::Wall ||
        obs->GetType() == Obstacle::Type::BossAttack ||
        obs->GetType() == Obstacle::Type::BossAttackReflectable) {
      if (relZ < lanes[idx].minWallDistZ) {
        lanes[idx].minWallDistZ = relZ;
      }
    }

    if (obsLane == activeLane && relZ > -0.5f && relZ < minActiveDistZ) {
      minActiveDistZ = relZ;
      imminentObs = obs;
    }
  }

  if (imminentObs) {
    Obstacle::Type type = imminentObs->GetType();

    if (type == Obstacle::Type::Low) {
      if (minActiveDistZ <= 3.6f && minActiveDistZ >= 0.8f) {
        if (!player->GetIsJumping()) {
          player->TriggerJump();
        }
      }
    } else if (type == Obstacle::Type::High) {
      if (minActiveDistZ <= 3.8f && minActiveDistZ >= 0.8f) {
        if (!player->GetIsRolling() && !player->GetIsJumping()) {
          player->TriggerRoll();
        }
      }
    } else if (type == Obstacle::Type::Wall ||
               type == Obstacle::Type::BossAttack ||
               type == Obstacle::Type::BossAttackReflectable) {
      if (minActiveDistZ <= 15.0f && !player->IsChangingLane() && player->CanAct()) {
        if (activeLane == 0) {
          float leftWallDist = lanes[0].minWallDistZ;
          float rightWallDist = lanes[2].minWallDistZ;
          float leftMinDist = lanes[0].minDistZ;
          float rightMinDist = lanes[2].minDistZ;

          if (leftWallDist > 15.0f && rightWallDist <= 15.0f) {
            player->TriggerMoveLeft();
          } else if (rightWallDist > 15.0f && leftWallDist <= 15.0f) {
            player->TriggerMoveRight();
          } else if (leftWallDist > 15.0f && rightWallDist > 15.0f) {
            if (leftMinDist >= rightMinDist && leftMinDist > 5.0f) {
              player->TriggerMoveLeft();
            } else if (rightMinDist > 5.0f) {
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
          if (lanes[1].minWallDistZ > 4.0f) {
            player->TriggerMoveRight();
          }
        } else if (activeLane == 1) {
          if (lanes[1].minWallDistZ > 4.0f) {
            player->TriggerMoveLeft();
          }
        }
      }
    }
  }

  // 中央レーンへの安全復帰
  if ((!imminentObs || minActiveDistZ > 20.0f) && !player->IsChangingLane() && player->CanAct()) {
    if (activeLane != 0) {
      if (lanes[1].minDistZ > 22.0f && lanes[1].minWallDistZ > 30.0f) {
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

  const Transform &playerTransform = player->GetTransform();
  float playerHeight = player->GetIsRolling() ? 0.5f : 1.5f;
  AABB playerAABB = Collision::MakeAABB(playerTransform, 0.8f, playerHeight, 0.8f);

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

  Transform defaultCamTransform;
  defaultCamTransform.scale = {1.0f, 1.0f, 1.0f};
  defaultCamTransform.rotate = {0.3f, 0.0f, 0.0f};
  defaultCamTransform.translate = {0.0f, 8.0f, -15.0f};

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
    while (diffY > 3.14159265f) diffY -= 6.2831853f;
    while (diffY < -3.14159265f) diffY += 6.2831853f;

    camTransform.rotate.x =
        Lerp(titleExitStartCamTransform_.rotate.x, defaultCamTransform.rotate.x, easeT);
    camTransform.rotate.y = startY + diffY * easeT;
    camTransform.rotate.z =
        Lerp(titleExitStartCamTransform_.rotate.z, defaultCamTransform.rotate.z, easeT);
  } else {
    titleCameraAngle_ += titleCameraSpeed_ * dt;
    if (titleCameraAngle_ >= 6.2831853f) {
      titleCameraAngle_ -= 6.2831853f;
    } else if (titleCameraAngle_ < 0.0f) {
      titleCameraAngle_ += 6.2831853f;
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
  float runStartX = -100.0f;
  float runEndX = 1620.0f;
  float leaderX = runStartX + (runEndX - runStartX) * runProgress;

  const float baseY = 360.0f;

  struct FormationParam {
    float offsetX;
    float offsetY;
    float animTimeOffset;
    float scale;
  };

  const float stepY = 41.5f;
  const float slopeX = 26.0f;

  FormationParam formations[kTitlePenguinCount];
  for (int i = 0; i < kTitlePenguinCount; ++i) {
    int distFromCenter = std::abs(i - 7);
    float offsetY = (i - 7) * stepY;
    float offsetX = -distFromCenter * slopeX;

    float depth = 0.92f + (offsetY / 290.5f) * 0.10f;
    float scale = (i == 7) ? 1.10f : (1.00f * depth);
    float animTimeOffset = distFromCenter * 0.09f;

    formations[i] = {offsetX, offsetY, animTimeOffset, scale};
  }

  for (int idx = 0; idx < kTitlePenguinCount; ++idx) {
    if (!titlePenguinSprites_[idx]) continue;

    const auto &form = formations[idx];
    float posX = leaderX + form.offsetX;
    float posY = baseY + form.offsetY;

    float timerWithOffset = titleExitTimer_ + form.animTimeOffset;
    int animFrame = (static_cast<int>(timerWithOffset * 10.0f)) % 2;
    Vector2 uvArea[2];
    if (animFrame == 0) {
      uvArea[0] = {0.0f, 0.0f};
      uvArea[1] = {0.5f, 1.0f};
    } else {
      uvArea[0] = {0.5f, 0.0f};
      uvArea[1] = {1.0f, 1.0f};
    }

    float bobbing = (animFrame == 1) ? -4.0f * (form.scale / 1.00f) : 0.0f;
    float tilt = -0.05f;

    Transform pTransform;
    pTransform.scale = {form.scale, form.scale, 1.0f};
    pTransform.rotate = {0.0f, 0.0f, tilt};
    pTransform.translate = {posX, posY + bobbing, 0.0f};

    titlePenguinSprites_[idx]->SetTransform(pTransform);
    titlePenguinSprites_[idx]->SetTextureArea(uvArea);
    titlePenguinSprites_[idx]->UpdateVertexBuffer();
    titlePenguinSprites_[idx]->SettingWvp();

    if (posX > 0.0f && posX < 1360.0f) {
      float trailAlpha = std::clamp(runProgress * 1.5f, 0.0f, 0.65f) * form.scale;
      draw.DrawFillRect(Vector2(posX - 65.0f * form.scale, posY + 5.0f + bobbing),
                        Vector2(55.0f * form.scale, 2.5f),
                        Vector4(0.85f, 0.95f, 1.0f, trailAlpha * 0.7f));
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

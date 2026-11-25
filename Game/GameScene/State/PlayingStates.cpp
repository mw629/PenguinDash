#include "PlayingStates.h"
#include "../GameScene.h"
#include "../System/SoundManager.h"

// ==========================================
// IPlayingState デフォルト実装
// ==========================================

void IPlayingState::OnCameraTransitionComplete(GameScene &scene) {
  if (auto *stage = scene.GetStageSettings()) {
    stage->SetSpawningPaused(false);
  }
}

// ==========================================
// ThreeLanePlayingState
// ==========================================

void ThreeLanePlayingState::Enter(GameScene &scene, PlayingState prevState,
                                  Transform &outTarget, int &outLaneCount) {
  outTarget = GameCameraController::kDefaultCameraTransform;
  outLaneCount = GameScene::kThreeLaneCount;

  if (auto *boss = scene.GetBoss()) {
    boss->Reset();
  }
  if (auto *bossCtrl = scene.GetBossBattleController()) {
    bossCtrl->Reset();
  }

  if (prevState == PlayingState::Boss) {
    needsBossItemCooldownReset_ = true;
  }

  if (prevState == PlayingState::OneLane) {
    if (auto *stage = scene.GetStageSettings()) {
      for (int j = 0; j < stage->GetMaxObstacles(); j++) {
        Obstacle *obs = stage->GetObstacle(j);
        if (obs && obs->GetIsActive() && !obs->GetIsHit() &&
            obs->GetTransform().translate.z > 15.0f) {
          obs->Deactivate();
        }
      }
    }
  }
}

void ThreeLanePlayingState::OnCameraTransitionComplete(GameScene &scene) {
  if (auto *stage = scene.GetStageSettings()) {
    stage->SetSpawningPaused(false);
    if (needsBossItemCooldownReset_) {
      needsBossItemCooldownReset_ = false;
      float bossItemDuration =
          stage->GetItemCoolDownDuration(Obstacle::Type::BossItem);
      stage->SetItemCoolDownTimer(Obstacle::Type::BossItem, bossItemDuration);
    }
  }
}

// ==========================================
// OneLanePlayingState
// ==========================================

void OneLanePlayingState::Enter(GameScene &scene, PlayingState /*prevState*/,
                                Transform &outTarget, int &outLaneCount) {
  outTarget = GameCameraController::kDefaultRightSideViewTransform;
  outLaneCount = GameScene::kOneLaneCount;
  rightSideDistance_ = 0.0f;

  if (auto *boss = scene.GetBoss()) {
    boss->Reset();
  }
  if (auto *bossCtrl = scene.GetBossBattleController()) {
    bossCtrl->Reset();
  }
  if (auto *stage = scene.GetStageSettings()) {
    stage->SetLaneCount(GameScene::kOneLaneCount);
    stage->SetupOneLaneInitialObstacles();
  }
  if (auto *cam = scene.GetCameraController()) {
    cam->ShakeImpact();
  }
}

std::unique_ptr<IPlayingState>
OneLanePlayingState::Update(GameScene &scene, float timeScale,
                            float scoreScrollSpeed) {
  auto *cam = scene.GetCameraController();
  // カメラトランジション中および待機中は距離を加算しない
  if (cam && !cam->IsTransitioning() && !cam->IsTransitionPending()) {
    rightSideDistance_ += scoreScrollSpeed * timeScale;
    if (rightSideDistance_ >= kOneLaneTotalDistance) {
      scene.AddOneLaneClearCount();
      return std::make_unique<ThreeLanePlayingState>(false);
    }
  }
  return nullptr;
}

// ==========================================
// BossPlayingState
// ==========================================

void BossPlayingState::Enter(GameScene &scene, PlayingState /*prevState*/,
                             Transform &outTarget, int &outLaneCount) {
  outTarget.scale = {1.0f, 1.0f, 1.0f};
  if (auto *bossCtrl = scene.GetBossBattleController()) {
    outTarget.rotate = bossCtrl->GetBossCameraRotate();
    outTarget.translate = bossCtrl->GetBossCameraTranslate();
    bossCtrl->Reset();
  }
  outLaneCount = GameScene::kThreeLaneCount;

  if (auto *stage = scene.GetStageSettings()) {
    stage->SetSpawningPaused(true);
  }
  if (auto *boss = scene.GetBoss()) {
    boss->Reset();
  }
  if (auto *cam = scene.GetCameraController()) {
    cam->ShakeImpact();
  }
  SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Boss);
}

void BossPlayingState::Exit(GameScene &scene, PlayingState nextState) {
  if (nextState != PlayingState::Boss) {
    SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
    if (auto *stage = scene.GetStageSettings()) {
      float bossItemDuration =
          stage->GetItemCoolDownDuration(Obstacle::Type::BossItem);
      stage->SetItemCoolDownTimer(Obstacle::Type::BossItem, bossItemDuration);
    }
  }
}

std::unique_ptr<IPlayingState>
BossPlayingState::Update(GameScene &scene, float timeScale,
                         float /*scoreScrollSpeed*/) {
  auto *boss = scene.GetBoss();
  auto *cam = scene.GetCameraController();
  if (boss && !boss->GetIsActive() && cam && !cam->IsTransitioning() &&
      !cam->IsTransitionPending()) {
    return std::make_unique<ThreeLanePlayingState>(true);
  }

  if (auto *bossCtrl = scene.GetBossBattleController()) {
    bossCtrl->Update(timeScale, boss, scene.GetStageSettings(),
                     scene.GetEffectManager(), cam);
  }
  return nullptr;
}

void BossPlayingState::OnCameraTransitionComplete(GameScene &scene) {
  if (auto *boss = scene.GetBoss()) {
    boss->Spawn();
  }
}

bool BossPlayingState::ShouldInvertControls(const GameScene &scene) const {
  const auto *cam = scene.GetCameraController();
  return cam && cam->GetTransform().rotate.y > kInvertedCameraThresholdRotateY;
}

// ==========================================
// Factory
// ==========================================

std::unique_ptr<IPlayingState> CreatePlayingState(PlayingState state,
                                                  PlayingState prevState) {
  switch (state) {
  case PlayingState::ThreeLane:
    return std::make_unique<ThreeLanePlayingState>(prevState ==
                                                   PlayingState::Boss);
  case PlayingState::OneLane:
    return std::make_unique<OneLanePlayingState>();
  case PlayingState::Boss:
    return std::make_unique<BossPlayingState>();
  default:
    return std::make_unique<ThreeLanePlayingState>(false);
  }
}


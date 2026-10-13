#include "GameScene.h"
#include "../../Editer/EditorManager.h"
#include "AssetManager.h"
#include "GameSceneManager.h"
#include "Graphics/Render/Draw.h"
#include "System/SoundManager.h"
#include "Network/Firebase/FirebaseRankingManager.h"
#include <Engine.h>
#include <GameObjects/Object/3d/Model.h>
#include <Math/Calculation.h>
#include <System/CollisionManager.h>
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <memory>

GameScene::~GameScene() {
  EditorManager::SetGameViewDrawCallback(nullptr, nullptr);
  EditorManager::SetSaveCallback(nullptr);
  EditorManager::SetLoadCallback(nullptr);
  EditorManager::SetFileDropCallback(nullptr);
  EditorManager::ClearSceneOverlayCallback();
}

void GameScene::Initialize() {
  sceneID_ = SceneID::Game;
  fade_->Initialize();
  resultTransition_ = ResultTransition::None;

  titleDirector_->Initialize(texture_.get());
  crashEffect_->Initialize(texture_.get());

  if (!EditorManager::IsPlaying()) {
    gameState_ = GameState::Editor;
    cameraController_->SetDebugCamera(true);
    SoundManager::GetInstance()->StopBGM();
  } else {
    gameState_ = GameState::Title;
    cameraController_->SetDebugCamera(false);
  }
  sceneChangeRequest_ = false;

  effectManager_->Initialize();
  collisionManager_ = std::make_unique<CollisionManager>();
  cameraController_->Initialize();

  // Game View 描画コールバックの登録
  EditorManager::SetGameViewDrawCallback(
      [this](class Draw &draw) {
        Matrix4x4 gameViewMat = cameraController_->GetGameViewMatrix();

        Transform originalSkyBoxT = skyBox_->GetTransform();
        Transform gameSkyBoxT = originalSkyBoxT;
        gameSkyBoxT.translate = cameraController_->GetGameCamera()->GetTransform().translate;
        skyBox_->SetTransform(gameSkyBoxT);

        ObjectBase::SetWvpIndex(1);
        EffectDefinition::SetWvpIndex(1);

        gameObjectManager_->UpdateAll(gameViewMat, 0.0f);
        stageSettings_->EditorUpdate(gameViewMat);
        effectManager_->EditorUpdate(gameViewMat);

        draw.SetCamera(cameraController_->GetGameCamera());
        draw.SetEnvironmentTexture(skyBoxTexture_);
        gameObjectManager_->DrawAll(draw);
        stageSettings_->Draw(draw);
        effectManager_->Draw(draw);

        skyBox_->SetTransform(originalSkyBoxT);

        ObjectBase::SetWvpIndex(0);
        EffectDefinition::SetWvpIndex(0);
      },
      [this](class Draw &draw) {
        DrawUI(draw);
      });

  // スカイボックスの初期化
  skyBoxTexture_ = texture_->CreateTexture("Resources/DDS/SnowWorld.dds");
  skyBox_->Initialize(skyBoxTexture_);
  skyBox_->SetShader("SkyBoxShader");
  skyBox_->SetCullMode(kCullModeFront);
  skyBox_->SetFrustumCullingEnabled(false);
  skyBox_->SetLighting(false);
  skyBox_->SetTransform(skyBoxTransform_);
  skyBox_->SetName("SkyBox");

  // プレイヤーの初期化
  ModelData playerModelData =
      AssimpLoadObjFile("Resources/gltf/Penguin", "RunPenguin.gltf");
  player_->Initialize(playerModelData);

  // ボスの初期化
  ModelData bossModelData = AssetManager::LoadModel(
      "Resources/Model/StylizedIceKing", "StylizedIceKing.obj");
  boss_->Initialize(bossModelData);
  boss_->SetName("Boss");

  // オブジェクトマネージャーへの登録
  gameObjectManager_->Clear();
  auto skyboxRenderObj = std::make_shared<RenderObject>(skyBox_);
  skyboxRenderObj->SetName("SkyBox");
  skyboxRenderObj->SetFrustumCullingEnabled(false);
  gameObjectManager_->AddObject(skyboxRenderObj);
  player_->SetName("Player");
  gameObjectManager_->AddObject(player_);
  gameObjectManager_->AddObject(boss_);

  // エディターコールバック登録
  EditorManager::SetSaveCallback([this](const std::string &filePath) {
    gameObjectManager_->SaveScene(filePath);
  });
  EditorManager::SetLoadCallback([this](const std::string &filePath) {
    gameObjectManager_->LoadScene(filePath);
  });
  EditorManager::SetFileDropCallback([this](const std::string &dropPath) {
    size_t lastSlash = dropPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
      std::string dirPath = dropPath.substr(0, lastSlash);
      std::string fileName = dropPath.substr(lastSlash + 1);

      size_t extPos = fileName.find_last_of(".");
      if (extPos != std::string::npos) {
        std::string ext = fileName.substr(extPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".obj" || ext == ".gltf") {
          try {
            ModelData modelData = AssetManager::LoadModel(dirPath, fileName);
            auto model = std::make_shared<Model>();
            model->Initialize(modelData);
            model->SetName(fileName);

            auto renderObj = std::make_shared<RenderObject>(model);
            renderObj->SetName(fileName);
            gameObjectManager_->AddObject(renderObj);
          } catch (const std::exception &) {
          }
        }
      }
    }
  });

  // ステージの初期化
  ModelData roadModelData =
      AssetManager::LoadModel("Resources/Model/Ground", "Ground.obj");
  ModelData fallenTreeModel =
      AssetManager::LoadModel("Resources/Model/FallenTree", "FallenTree.obj");
  ModelData iceArchwayModel =
      AssetManager::LoadModel("Resources/Model/IceArchway", "IceArchway.obj");
  ModelData iceWallModel =
      AssetManager::LoadModel("Resources/Model/IceWall", "IceWall.obj");
  ModelData iceBomModel =
      AssetManager::LoadModel("Resources/Model/IceBom", "IceBom.obj");
  ModelData reflectingAttackModel = AssetManager::LoadModel(
      "Resources/Model/ReflectingAttack", "ReflectingAttack.obj");
  ModelData bonusEnemyModel = AssetManager::LoadModel(
      "Resources/Model/StylizedSnowman", "StylizedSnowman.obj");
  stageSettings_->Initialize(roadModelData, fallenTreeModel, iceArchwayModel,
                             iceWallModel, bonusEnemyModel, iceBomModel,
                             reflectingAttackModel, gameObjectManager_.get());

  gameObjectManager_->LoadScene(initialSceneJson_);

  if (gameState_ == GameState::Title) {
    ReturnToTitle();
  } else {
    ResetGame();
  }

  SoundManager::GetInstance()->Initialize();
  if (gameState_ == GameState::Title) {
    SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Title);
  }
}

void GameScene::ResetGame() {
  stageSettings_->Reset();
  player_->Reset();
  boss_->Reset();
  bossBattleController_->Reset();
  effectManager_->ClearBarrier();
  scoreManager_->Reset();
  crashEffect_->Reset();
  wasBossBattle_ = false;

  cameraController_->Reset(GameCameraController::kDefaultCameraTransform);

  playingState_ = PlayingState::ThreeLane;
  isRightSideMode_ = false;
  rightSideDistance_ = 0.0f;
  stageSettings_->SetLaneCountImmediate(kThreeLaneCount);
  stageSettings_->SetSpawningPaused(false);
  player_->SetInvertedControls(false);
  GameSceneManager::GetInstance()->SetInBossBattle(false);
}

void GameScene::StartGame() {
  if (titleDirector_->IsExiting()) return;
  titleDirector_->StartExit(cameraController_->GetTransform());
  stageSettings_->SetSpawningPaused(true);
}

void GameScene::StartPlaying() {
  gameState_ = GameState::Playing;

  cameraController_->Reset(GameCameraController::kDefaultCameraTransform);

  player_->SetAutoPilot(false);
  player_->SetInvertedControls(false);

  stageSettings_->ClearObstacles(kStartObstacleClearDistance);
  stageSettings_->SetSpawningPaused(false);
  stageSettings_->SetItemSpawnChance(kStartItemSpawnChance);
  stageSettings_->SetBaseScrollSpeed(kPlayBaseScrollSpeed);
  stageSettings_->SetScrollAcceleration(kPlayScrollAcceleration);

  scoreManager_->Reset();
  wasBossBattle_ = false;
  bossBattleController_->Reset();
  boss_->Reset();

  effectManager_->ClearBarrier();
  SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
}

void GameScene::ReturnToTitle() {
  ResetGame();
  player_->SetAutoPilot(true);
  stageSettings_->SetItemSpawnChance(0.0f);
  stageSettings_->SetBaseScrollSpeed(kTitleBaseScrollSpeed);
  stageSettings_->SetScrollAcceleration(0.0f);
  stageSettings_->SetSpawningPaused(false);
  gameState_ = GameState::Title;

  titleDirector_->Reset();
  SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Title);
}

void GameScene::StartResultTransition(ResultTransition target) {
  if (FreezeTransition::GetInstance()->IsActive() || resultTransition_ != ResultTransition::None) {
    return;
  }
  resultTransition_ = target;
  FreezeTransition::GetInstance()->Start(kResultFreezeDuration, kResultShatterDuration, [this, target]() {
    if (target == ResultTransition::Restart) {
      ResetGame();
      gameState_ = GameState::Playing;
      SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
      fade_->StartFadeIn(fadeDuration_);
    } else if (target == ResultTransition::Title) {
      ReturnToTitle();
      fade_->StartFadeIn(fadeDuration_);
    }
    resultTransition_ = ResultTransition::None;
  });
}

void GameScene::Update() {
  ObjectBase::SetWvpIndex(0);
  EffectDefinition::SetWvpIndex(0);
  uiTimer_ += Engine::kDefaultDeltaTime;
  SoundManager::GetInstance()->Update();

  // フェードの更新
  if (fade_) {
    fade_->Update(Engine::kDefaultDeltaTime);
    if (resultTransition_ != ResultTransition::None && fade_->IsFadeOutFinished()) {
      if (resultTransition_ == ResultTransition::Restart) {
        ResetGame();
        gameState_ = GameState::Playing;
        SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
        fade_->StartFadeIn(fadeDuration_);
      } else if (resultTransition_ == ResultTransition::Title) {
        ReturnToTitle();
        fade_->StartFadeIn(fadeDuration_);
      }
      resultTransition_ = ResultTransition::None;
    }
  }

  // タイトル開始演出タイマー更新
  titleDirector_->UpdateExit(Engine::kDefaultDeltaTime);

#ifdef _DEBUG
  if (Input::PushKey(DIK_Q)) {
    ChangePlayingState(PlayingState::ThreeLane);
  }
  if (Input::PushKey(DIK_E)) {
    ChangePlayingState(PlayingState::OneLane);
  }
  if (Input::PushKey(DIK_F1)) {
    ColliderComponent::ToggleDrawDebug();
  }
#endif

  // 一人称切り替え (F2 / BACK / RTHUMB)
  if (Input::PushKey(DIK_F2) || GamePadInput::PushButton(XINPUT_GAMEPAD_BACK) ||
      GamePadInput::PushButton(XINPUT_GAMEPAD_RIGHT_THUMB)) {
    cameraController_->ToggleFirstPerson(player_.get());
  }

  // F3キーで氷結・破砕トランジションテスト
  if (Input::PushKey(DIK_F3)) {
    if (!FreezeTransition::GetInstance()->IsActive()) {
      FreezeTransition::GetInstance()->Start();
    }
  }

  // Engine側のPlay/Stop状態に同期
  bool isEnginePlaying = EditorManager::IsPlaying();
  if (isEnginePlaying && gameState_ == GameState::Editor) {
    ReturnToTitle();
    cameraController_->SetDebugCamera(false);
  } else if (!isEnginePlaying && gameState_ != GameState::Editor) {
    gameState_ = GameState::Editor;
    cameraController_->SetDebugCamera(true);
    SoundManager::GetInstance()->StopBGM();
    SoundManager::GetInstance()->StopAllSE();
  }

  // カメラトランジションの更新
  cameraController_->UpdateTransition(stageSettings_.get(), [this]() {
    if (playingState_ != PlayingState::Boss) {
      stageSettings_->SetSpawningPaused(false);
      if (wasBossBattle_) {
        wasBossBattle_ = false;
        float bossItemDuration =
            stageSettings_->GetItemCoolDownDuration(Obstacle::Type::BossItem);
        stageSettings_->SetItemCoolDownTimer(Obstacle::Type::BossItem, bossItemDuration);
      }
    } else {
      boss_->Spawn();
    }
  });

  // カメラの更新 (FPV or タイトルオービット)
  float timeScale = EditorManager::GetPlaySpeed();
  if (cameraController_->IsFirstPerson()) {
    cameraController_->UpdateFirstPersonCamera(player_.get());
  } else if (gameState_ == GameState::Title) {
    titleDirector_->UpdateCamera(player_.get(), cameraController_.get(), Engine::kDefaultDeltaTime * timeScale);
  }

  cameraController_->Update();

  // SkyBoxをカメラの位置に追従
  Transform skyboxTransform = skyBox_->GetTransform();
  skyboxTransform.translate = cameraController_->GetCamera()->GetTransform().translate;
  skyBox_->SetTransform(skyboxTransform);

  // 各ステート別の更新
  if (gameState_ == GameState::Title) {
    TitleUpdate();
  } else if (gameState_ == GameState::Playing) {
    PlayingUpdate();
    if (Input::PushKey(DIK_ESCAPE) || GamePadInput::PushButton(XINPUT_GAMEPAD_START)) {
      gameState_ = GameState::Paused;
      SoundManager::GetInstance()->PauseBGM();
    }
  } else if (gameState_ == GameState::Paused) {
    PausedUpdate();
    effectManager_->EditorUpdate(cameraController_->GetViewMatrix());
    if (Input::PushKey(DIK_ESCAPE) || GamePadInput::PushButton(XINPUT_GAMEPAD_START)) {
      gameState_ = GameState::Playing;
      SoundManager::GetInstance()->ResumeBGM();
    }
  } else if (gameState_ == GameState::PlayerHit) {
    PlayerHitUpdate();
  } else if (gameState_ == GameState::GameOver) {
    effectManager_->PlayerHitUpdate(cameraController_->GetViewMatrix());
    if (resultTransition_ == ResultTransition::None && !fade_->IsFading() &&
        !FreezeTransition::GetInstance()->IsActive()) {
      if (Input::PushKey(DIK_1) || GamePadInput::PushButton(XINPUT_GAMEPAD_A)) {
        StartResultTransition(ResultTransition::Restart);
      }
      if (Input::PushKey(DIK_2) || GamePadInput::PushButton(XINPUT_GAMEPAD_B)) {
        StartResultTransition(ResultTransition::Title);
      }
    }
  } else if (gameState_ == GameState::Editor) {
    EditorUpdate();
  }

  // FPV時のプレイヤー移動後同期
  if (cameraController_->IsFirstPerson()) {
    cameraController_->UpdateFirstPersonCamera(player_.get());
    cameraController_->Update();
    skyboxTransform = skyBox_->GetTransform();
    skyboxTransform.translate = cameraController_->GetCamera()->GetTransform().translate;
    skyBox_->SetTransform(skyboxTransform);
  }
}

void GameScene::PlayingUpdate() {
  GameSceneManager::GetInstance()->SetInBossBattle(playingState_ == PlayingState::Boss);

  float timeScale = EditorManager::GetPlaySpeed();
  float speedMultiplier = 1.0f;
  if (stageSettings_->GetBaseScrollSpeed() > 0.0f) {
    speedMultiplier =
        stageSettings_->GetScrollSpeed() / stageSettings_->GetBaseScrollSpeed();
  }

  scoreManager_->Update(stageSettings_->GetScrollSpeed(), timeScale);
  CheckKeepRolling();

  // 1レーン（右サイドカメラ）モードの走行距離加算
  if (isRightSideMode_) {
    rightSideDistance_ += stageSettings_->GetScrollSpeed() * timeScale;
    if (rightSideDistance_ >= kOneLaneTotalDistance) {
      ChangePlayingState(PlayingState::ThreeLane);
    }
  }

  // ボス戦の更新
  if (playingState_ == PlayingState::Boss) {
    if (!boss_->GetIsActive() && !cameraController_->IsTransitioning() &&
        !cameraController_->IsTransitionPending()) {
      ChangePlayingState(PlayingState::ThreeLane);
    } else {
      bossBattleController_->Update(timeScale, boss_.get(), stageSettings_.get(),
                                    effectManager_.get());
    }
  }

  player_->SetLaneLimits(stageSettings_->GetMinLaneIndex(),
                         stageSettings_->GetMaxLaneIndex(),
                         stageSettings_->GetLaneWidth());

  bool isCameraInverted = (playingState_ == PlayingState::Boss &&
                           cameraController_->GetTransform().rotate.y > kInvertedCameraThresholdRotateY);
  player_->SetInvertedControls(isCameraInverted);

  const Matrix4x4 &view = cameraController_->GetViewMatrix();
  gameObjectManager_->UpdateAll(view, speedMultiplier * timeScale);

  float activeWaterTime = cameraController_->GetGameCamera()
                              ? cameraController_->GetGameCamera()->GetTotalTime()
                              : cameraController_->GetCamera()->GetTotalTime();
  stageSettings_->Update(view, timeScale, activeWaterTime);

  CheckCollisions();
  collisionManager_->UpdateCollisions(gameObjectManager_.get());

  effectManager_->PlayingUpdate(view, player_->GetTransform().translate);

  if (!player_->GetIsRolling() && player_->GetTransform().translate.y <= kDustGroundThresholdY) {
    effectManager_->EmitDust(player_->GetTransform().translate);
  }
}

void GameScene::TitleUpdate() {
  float timeScale = EditorManager::GetPlaySpeed();
  float speedMultiplier = 1.0f;

  stageSettings_->SetBaseScrollSpeed(kTitleBaseScrollSpeed);
  stageSettings_->SetScrollAcceleration(0.0f);
  stageSettings_->SetSpawningPaused(titleDirector_->IsExiting());

  titleDirector_->UpdateAutoPilot(player_.get(), stageSettings_.get());

  player_->SetLaneLimits(stageSettings_->GetMinLaneIndex(),
                         stageSettings_->GetMaxLaneIndex(),
                         stageSettings_->GetLaneWidth());
  player_->SetInvertedControls(false);

  CheckKeepRolling();

  const Matrix4x4 &view = cameraController_->GetViewMatrix();
  gameObjectManager_->UpdateAll(view, speedMultiplier * timeScale);

  float activeWaterTime = cameraController_->GetGameCamera()
                              ? cameraController_->GetGameCamera()->GetTotalTime()
                              : cameraController_->GetCamera()->GetTotalTime();
  stageSettings_->Update(view, timeScale, activeWaterTime);

  titleDirector_->CheckCollisions(player_.get(), stageSettings_.get(),
                                  scoreManager_.get());
  effectManager_->PlayingUpdate(view, player_->GetTransform().translate);

  if (!player_->GetIsRolling() && player_->GetTransform().translate.y <= kDustGroundThresholdY) {
    effectManager_->EmitDust(player_->GetTransform().translate);
  }

  if (titleDirector_->IsExiting()) {
    if (titleDirector_->IsExitComplete(stageSettings_.get())) {
      StartPlaying();
    }
  } else {
    if ((!fade_ || !fade_->IsFading()) &&
        (Input::PushKey(DIK_SPACE) || GamePadInput::PushButton(XINPUT_GAMEPAD_A) ||
         GamePadInput::PushButton(XINPUT_GAMEPAD_START))) {
      StartGame();
    }
  }
}

void GameScene::PausedUpdate() {
  if (Input::PushKey(DIK_1) || GamePadInput::PushButton(XINPUT_GAMEPAD_A)) {
    ResetGame();
    gameState_ = GameState::Playing;
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Start);
    SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
  }
  if (Input::PushKey(DIK_2) || GamePadInput::PushButton(XINPUT_GAMEPAD_B)) {
    ReturnToTitle();
  }
}

void GameScene::PlayerHitUpdate() {
  crashEffect_->Update(Engine::kDefaultDeltaTime);
  effectManager_->PlayerHitUpdate(cameraController_->GetViewMatrix());
  player_->Update(cameraController_->GetViewMatrix(), 1.0f);

  if (player_->IsHitAnimationFinished()) {
    gameState_ = GameState::GameOver;
  }
}

void GameScene::EditorUpdate() {
  const Matrix4x4 &view = cameraController_->GetViewMatrix();
  gameObjectManager_->UpdateAll(view, 0.0f);
  float activeWaterTime = cameraController_->GetCamera()->GetTotalTime();
  stageSettings_->EditorUpdate(view, activeWaterTime);
  effectManager_->EditorUpdate(view);
}

void GameScene::CheckCollisions() {
  AABB playerAABB = player_->GetColliderAABB();

  for (int i = 0; i < stageSettings_->GetMaxObstacles(); i++) {
    Obstacle *obstacle = stageSettings_->GetObstacle(i);
    if (!obstacle->GetIsActive() || obstacle->GetIsHit()) continue;

    AABB obstacleAABB = Collision::MakeAABB(
        obstacle->GetTransform(), obstacle->GetCollisionWidth(),
        obstacle->GetCollisionHeight(), obstacle->GetCollisionDepth());

    if (Collision::CheckAABB(playerAABB, obstacleAABB)) {
      if (obstacle->OnPlayerCollision(this, player_.get())) {
        break;
      }
      continue;
    }
  }
}

void GameScene::TriggerPlayerHit(bool isTrip) {
  gameState_ = GameState::PlayerHit;
  crashEffect_->Trigger();
  SoundManager::GetInstance()->PlaySE(SoundManager::SE::Crash);
  GamePadInput::Rumble(kHitRumbleDuration, kHitRumbleMotorSpeed, kHitRumbleMotorSpeed);

  if (playingState_ == PlayingState::Boss && boss_->GetIsActive()) {
    boss_->ChangeState(BossState::Victory);
  }

  stageSettings_->SetGameOver(true);
  player_->OnHit(isTrip);

  scoreManager_->RecordCurrentScores();
  scoreManager_->SendOnlineScoreAndFetchRanking();
}

void GameScene::CheckKeepRolling() {
  bool keepRolling = false;
  if (player_->GetIsRolling()) {
    const Transform &playerTransform = player_->GetTransform();
    AABB standingAABB = player_->GetStandingColliderAABB();

    for (int i = 0; i < stageSettings_->GetMaxObstacles(); i++) {
      Obstacle *obstacle = stageSettings_->GetObstacle(i);
      if (!obstacle->GetIsActive() || obstacle->GetIsHit()) continue;

      if (obstacle->CanRollUnder()) {
        AABB obstacleAABB = Collision::MakeAABB(
            obstacle->GetTransform(), obstacle->GetCollisionWidth(),
            obstacle->GetCollisionHeight(), obstacle->GetCollisionDepth());

        if (Collision::CheckAABB(standingAABB, obstacleAABB)) {
          if (playerTransform.translate.z > obstacle->GetTransform().translate.z) {
            keepRolling = true;
            break;
          }
        }
      }
    }
  }
  player_->SetKeepRolling(keepRolling);
}

void GameScene::ChangePlayingState(PlayingState newState, bool force) {
  if (!force && playingState_ == newState) return;

  PlayingState prevState = playingState_;
  playingState_ = newState;

  if (prevState == PlayingState::Boss && newState != PlayingState::Boss) {
    wasBossBattle_ = true;
    SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
    float bossItemDuration =
        stageSettings_->GetItemCoolDownDuration(Obstacle::Type::BossItem);
    stageSettings_->SetItemCoolDownTimer(Obstacle::Type::BossItem, bossItemDuration);
  }

  Transform target;
  int laneCount = kThreeLaneCount;

  switch (playingState_) {
  case PlayingState::ThreeLane:
    target = GameCameraController::kDefaultCameraTransform;
    laneCount = kThreeLaneCount;
    isRightSideMode_ = false;
    rightSideDistance_ = 0.0f;
    boss_->Reset();
    bossBattleController_->Reset();
    break;

  case PlayingState::OneLane:
    target = GameCameraController::kDefaultRightSideViewTransform;
    laneCount = kOneLaneCount;
    isRightSideMode_ = true;
    rightSideDistance_ = 0.0f;
    boss_->Reset();
    bossBattleController_->Reset();
    break;

  case PlayingState::Boss:
    target.scale = {1.0f, 1.0f, 1.0f};
    target.rotate = bossBattleController_->GetBossCameraRotate();
    target.translate = bossBattleController_->GetBossCameraTranslate();
    laneCount = kThreeLaneCount;
    isRightSideMode_ = false;
    rightSideDistance_ = 0.0f;
    stageSettings_->SetSpawningPaused(true);
    bossBattleController_->Reset();
    boss_->Reset();
    SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Boss);
    break;
  }

  cameraController_->StartTransition(target, laneCount, stageSettings_.get());
}

void GameScene::Draw(class Draw &draw) {
  draw_ = &draw;
  draw.SetCamera(cameraController_->GetCamera());
  draw.SetEnvironmentTexture(skyBoxTexture_);

  gameObjectManager_->DrawAll(draw);
  stageSettings_->Draw(draw);
  effectManager_->Draw(draw);
}

void GameScene::DrawUI(class Draw &draw) {
  draw_ = &draw;

  GameHUDContext ctx;
  ctx.gameState = static_cast<int>(gameState_);
  ctx.playingState = static_cast<int>(playingState_);
  ctx.currentDistance = scoreManager_->GetCurrentDistance();
  ctx.currentScore = scoreManager_->GetCurrentScore();
  ctx.bonusEnemyHitCount = scoreManager_->GetBonusEnemyHitCount();
  ctx.scrollSpeed = stageSettings_->GetScrollSpeed();
  ctx.baseScrollSpeed = stageSettings_->GetBaseScrollSpeed();
  ctx.maxScrollSpeed = stageSettings_->GetMaxScrollSpeed();
  ctx.topRankings = scoreManager_->GetTopRankings();
  ctx.topScoreRankings = scoreManager_->GetTopScoreRankings();
  ctx.isOneLaneMode = isRightSideMode_;
  ctx.rightSideDistance = rightSideDistance_;
  ctx.boss = boss_.get();
  ctx.stageSettings = stageSettings_.get();
  ctx.reflectLane = bossBattleController_->GetReflectLane();
  ctx.uiTimer = uiTimer_;
  ctx.isTitleExiting = titleDirector_->IsExiting();
  ctx.titleExitTimer = titleDirector_->GetExitTimer();
  ctx.titleExitDuration = titleDirector_->GetExitDuration();
  ctx.isFirstPersonView = cameraController_->IsFirstPerson();
  ctx.crashSprite = crashEffect_->GetSprite();
  ctx.fade = fade_.get();

  auto rankingMgr = FirebaseRankingManager::GetInstance();
  ctx.onlineRankings = &rankingMgr->GetCachedRankings();
  ctx.isOnlineRankingSending = rankingMgr->IsSending();
  ctx.isOnlineRankingFetching = rankingMgr->IsFetching();
  ctx.isOnlineRankingConfigured = rankingMgr->IsConfigured();
  ctx.onlineRankingStatus = rankingMgr->GetLastStatusMessage();
  ctx.playerName = scoreManager_->GetPlayerName();

  gameHUD_->DrawHUD(draw, ctx);

  // タイトル演出ペンギン疾走の描画
  titleDirector_->DrawPenguins(draw);
}

void GameScene::ImGui() {
#ifdef _USE_IMGUI
  ImGui::Begin("GameScene");

  // ゲームオーバー時の表示
  if (gameState_ == GameState::GameOver) {
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    bool open = ImGui::CollapsingHeader("=== GAME OVER ===",
                                        ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor(2);

    if (open) {
      ImGui::Text("Your Distance: %.2f m", scoreManager_->GetCurrentDistance());
      ImGui::Text("Your Score: %.0f", scoreManager_->GetCurrentScore());
      ImGui::Separator();

      scoreManager_->ImGui();
      ImGui::Separator();

      if (ImGui::Button("Restart (1)", ImVec2(160, 35))) {
        StartResultTransition(ResultTransition::Restart);
      }
      ImGui::SameLine();
      if (ImGui::Button("Return to Title (2)", ImVec2(160, 35))) {
        StartResultTransition(ResultTransition::Title);
      }
      ImGui::SliderFloat("Fade Duration", &fadeDuration_, 0.1f, 2.0f, "%.2f s");
      if (ImGui::Button("Test Freeze & Shatter Transition", ImVec2(240, 32))) {
        FreezeTransition::GetInstance()->Start(kResultFreezeDuration, kResultShatterDuration, nullptr);
      }
      ImGui::Separator();
    }
  }

  // カメラ設定
  cameraController_->ImGui(player_.get(), [this](int presetIndex) {
    if (presetIndex == 0) ChangePlayingState(PlayingState::ThreeLane);
    else if (presetIndex == 1) ChangePlayingState(PlayingState::OneLane);
    else if (presetIndex == 2) ChangePlayingState(PlayingState::Boss, true);
  });

  // タイトル設定
  titleDirector_->ImGui();

  // サウンド設定
  if (ImGui::CollapsingHeader("Sound Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto *gsm = GameSceneManager::GetInstance();
    float masterVol = gsm->GetMasterVolume();
    float bgmVol = gsm->GetBGMVolume();
    float seVol = gsm->GetSEVolume();

    if (ImGui::SliderFloat("Master Volume", &masterVol, 0.0f, 1.0f, "%.2f")) {
      gsm->SetMasterVolume(masterVol);
    }
    if (ImGui::SliderFloat("BGM Volume", &bgmVol, 0.0f, 1.0f, "%.2f")) {
      gsm->SetBGMVolume(bgmVol);
    }
    if (ImGui::SliderFloat("SE Volume", &seVol, 0.0f, 1.0f, "%.2f")) {
      gsm->SetSEVolume(seVol);
    }

    ImGui::Separator();
    ImGui::Text("BGM Test:");
    if (ImGui::Button("Title BGM")) {
      SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Title);
    }
    ImGui::SameLine();
    if (ImGui::Button("Play BGM (BlackDiamond)")) {
      SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
    }
    ImGui::SameLine();
    if (ImGui::Button("Boss BGM")) {
      SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Boss);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop BGM")) {
      SoundManager::GetInstance()->StopBGM();
    }

    ImGui::Text("SE Test:");
    if (ImGui::Button("Jump"))
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Jump);
    ImGui::SameLine();
    if (ImGui::Button("Land"))
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Land);
    ImGui::SameLine();
    if (ImGui::Button("Slide"))
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Slide);
    ImGui::SameLine();
    if (ImGui::Button("Barrier"))
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Barrier);
    ImGui::SameLine();
    if (ImGui::Button("Break"))
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::BarrierBreak);

    if (ImGui::Button("BonusHit"))
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::BonusHit);
    ImGui::SameLine();
    if (ImGui::Button("Reflect"))
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossReflect);
    ImGui::SameLine();
    if (ImGui::Button("Defeat"))
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossDefeat);
    ImGui::SameLine();
    if (ImGui::Button("Crash"))
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Crash);
  }

  // ゲームステート
  if (ImGui::CollapsingHeader("Game State", ImGuiTreeNodeFlags_DefaultOpen)) {
    const char *stateNames[] = {"Title",     "Playing",  "Paused", "PlayerHit",
                                "GameClear", "GameOver", "Editor"};
    ImGui::Text("Game State: %s", stateNames[gameState_]);

    const char *playingStateNames[] = {"ThreeLane", "OneLane", "Boss"};
    int currentStateIdx = static_cast<int>(playingState_);
    if (ImGui::Combo("Playing State", &currentStateIdx, playingStateNames, 3)) {
      PlayingState newState = static_cast<PlayingState>(currentStateIdx);
      ChangePlayingState(newState);
    }

    ImGui::Separator();

    // プレイヤー情報
    if (ImGui::TreeNode("Player Info")) {
      const Transform &playerTransform = player_->GetTransform();
      ImGui::Text("Position: (%.2f, %.2f, %.2f)", playerTransform.translate.x,
                  playerTransform.translate.y, playerTransform.translate.z);
      ImGui::Text("Rolling: %s", player_->GetIsRolling() ? "YES" : "NO");
      ImGui::Text("Jumping: %s", player_->GetIsJumping() ? "YES" : "NO");

      float jumpPower = player_->GetJumpPower();
      if (ImGui::SliderFloat("Jump Power", &jumpPower, 0.10f, 0.40f, "%.3f")) {
        player_->SetJumpPower(jumpPower);
      }
      float gravity = player_->GetGravity();
      if (ImGui::SliderFloat("Gravity", &gravity, 0.005f, 0.040f, "%.4f")) {
        player_->SetGravity(gravity);
      }
      float laneSpeed = player_->GetLaneChangeSpeed();
      if (ImGui::SliderFloat("Lane Speed", &laneSpeed, 0.05f, 0.50f, "%.2f")) {
        player_->SetLaneChangeSpeed(laneSpeed);
      }
      float rollDuration = player_->GetRollDuration();
      if (ImGui::SliderFloat("Roll Duration", &rollDuration, 10.0f, 60.0f, "%.0f f")) {
        player_->SetRollDuration(rollDuration);
      }

      float estAirFrames = gravity > 0.0f ? ((2.0f * jumpPower / gravity) + 1.0f) : 0.0f;
      float estMaxHeight = gravity > 0.0f ? ((jumpPower * jumpPower) / (2.0f * gravity)) : 0.0f;
      ImGui::Text("Jump Air Time: %.0f frames (%.2f s)", estAirFrames, estAirFrames / 60.0f);
      ImGui::Text("Max Jump Height: +%.2f m", estMaxHeight);
      ImGui::Text("Lane Move Time: %.0f frames", laneSpeed > 0.0f ? (1.0f / laneSpeed) : 0.0f);
      ImGui::Text("Roll Duration: %.0f frames (%.2f s)", rollDuration, rollDuration / 60.0f);

      ImGui::TreePop();
    }

    // ボス情報
    if (playingState_ == PlayingState::Boss && boss_->GetIsActive()) {
      bossBattleController_->ImGui(boss_.get(), cameraController_.get(),
                                   playingState_ == PlayingState::Boss,
                                   cameraController_->IsTransitioning());
    }

    // ステージ情報
    if (ImGui::TreeNode("Stage Info")) {
      ImGui::Text("Scroll Speed: %.3f", stageSettings_->GetScrollSpeed());
      ImGui::Text("Base Scroll Speed: %.3f", stageSettings_->GetBaseScrollSpeed());
      ImGui::Text("Max Scroll Speed: %.3f", stageSettings_->GetMaxScrollSpeed());
      ImGui::Text("Lane Index: Min=%d, Max=%d",
                  stageSettings_->GetMinLaneIndex(), stageSettings_->GetMaxLaneIndex());
      ImGui::Text("Lane Width: %.2f (Effective: %.2f)",
                  stageSettings_->GetLaneWidth(), stageSettings_->GetEffectiveLaneWidth());
      ImGui::Text("Chunks: %d (Behind: %d, Ahead: %d)",
                  stageSettings_->GetChunkCount(), stageSettings_->GetBackwardChunks(),
                  stageSettings_->GetForwardChunks());
      ImGui::TreePop();
    }

    ImGui::Separator();

    // スコア表示
    scoreManager_->ImGui();

    ImGui::Separator();

    if (ImGui::Button("Reset Game", ImVec2(120, 0))) {
      ResetGame();
      gameState_ = GameState::Playing;
      cameraController_->SetDebugCamera(false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Go to Title", ImVec2(120, 0))) {
      ReturnToTitle();
      cameraController_->SetDebugCamera(false);
    }
  }

  // フォント設定
  if (draw_ && ImGui::CollapsingHeader("Font / Text Settings")) {
    float boldness = draw_->GetTextBaseBoldness();
    if (ImGui::SliderFloat("Global Boldness", &boldness, -0.05f, 0.20f, "%.3f")) {
      draw_->SetTextBaseBoldness(boldness);
    }
    ImGui::TextDisabled("Default: 0.070 (Thick / Bold)");
  }

  // ステージデバッグパネル
  if (ImGui::CollapsingHeader("Stage Settings Debug")) {
    int currentLane = stageSettings_->GetLaneCount();
    ImGui::Text("Current (Player Foot) Lane: %d", currentLane);

    int targetLane = stageSettings_->GetTargetLaneCount();
    if (ImGui::SliderInt("Target Lane Count", &targetLane, 1, 11)) {
      stageSettings_->SetLaneCount(targetLane);
    }

    float laneWidth = stageSettings_->GetLaneWidth();
    if (ImGui::SliderFloat("Lane Width", &laneWidth, 1.0f, 10.0f)) {
      stageSettings_->SetLaneWidth(laneWidth);
    }

    float oneLaneMult = stageSettings_->GetOneLaneWidthMultiplier();
    if (ImGui::SliderFloat("One Lane Width Multiplier", &oneLaneMult, 1.0f, 3.0f, "%.2f")) {
      stageSettings_->SetOneLaneWidthMultiplier(oneLaneMult);
    }

    float baseSpeed = stageSettings_->GetBaseScrollSpeed();
    if (ImGui::SliderFloat("Base Scroll Speed", &baseSpeed, 0.0f, 1.0f)) {
      stageSettings_->SetBaseScrollSpeed(baseSpeed);
    }

    float maxSpeed = stageSettings_->GetMaxScrollSpeed();
    if (ImGui::SliderFloat("Max Scroll Speed", &maxSpeed, baseSpeed, 2.0f)) {
      stageSettings_->SetMaxScrollSpeed(maxSpeed);
    }

    float accel = stageSettings_->GetScrollAcceleration();
    if (ImGui::SliderFloat("Scroll Acceleration", &accel, 0.0f, 0.01f)) {
      stageSettings_->SetScrollAcceleration(accel);
    }

    float minDistance = stageSettings_->GetMinObstacleDistance();
    float maxDistance = stageSettings_->GetMaxObstacleDistance();
    if (ImGui::SliderFloat("Min Obstacle Distance", &minDistance, 3.0f, 30.0f, "%.1f m")) {
      if (minDistance > maxDistance) maxDistance = minDistance;
      stageSettings_->SetMinObstacleDistance(minDistance);
      stageSettings_->SetMaxObstacleDistance(maxDistance);
    }
    if (ImGui::SliderFloat("Max Obstacle Distance", &maxDistance, 5.0f, 60.0f, "%.1f m")) {
      if (maxDistance < minDistance) minDistance = maxDistance;
      stageSettings_->SetMinObstacleDistance(minDistance);
      stageSettings_->SetMaxObstacleDistance(maxDistance);
    }

    float baseActionFrames = stageSettings_->GetBaseActionFrames();
    if (ImGui::SliderFloat("Base Action Frames", &baseActionFrames, 0.0f, 60.0f, "%.0f f")) {
      stageSettings_->SetBaseActionFrames(baseActionFrames);
    }

    float minGrace = stageSettings_->GetMinGraceFrames();
    float maxGrace = stageSettings_->GetMaxGraceFrames();
    if (ImGui::SliderFloat("Min Grace Frames", &minGrace, 5.0f, 60.0f, "%.0f f")) {
      if (minGrace > maxGrace) maxGrace = minGrace;
      stageSettings_->SetMinGraceFrames(minGrace);
      stageSettings_->SetMaxGraceFrames(maxGrace);
    }
    if (ImGui::SliderFloat("Max Grace Frames", &maxGrace, 5.0f, 80.0f, "%.0f f")) {
      if (maxGrace < minGrace) minGrace = maxGrace;
      stageSettings_->SetMinGraceFrames(minGrace);
      stageSettings_->SetMaxGraceFrames(maxGrace);
    }

    float noSpawnChance = stageSettings_->GetNoSpawnChance() * 100.0f;
    if (ImGui::SliderFloat("No Spawn Chance (%)", &noSpawnChance, 0.0f, 30.0f, "%.1f %%")) {
      stageSettings_->SetNoSpawnChance(noSpawnChance / 100.0f);
    }

    float currentSpeed = stageSettings_->GetScrollSpeed();
    float curInterval = stageSettings_->GetObstacleInterval();
    float estFrames = currentSpeed > 0.0f ? (curInterval / currentSpeed) : 0.0f;
    ImGui::Text("Next Spawn: %.1f m (approx. %.0f frames / %.2f s)",
                curInterval, estFrames, estFrames / 60.0f);
    ImGui::Text("Dodgeable Range: %.1f m ~ %.1f m", minDistance, maxDistance);

    Vector4 roadColor = stageSettings_->GetRoadColor();
    float roadColorArr[4] = {roadColor.x, roadColor.y, roadColor.z, roadColor.w};
    if (ImGui::ColorEdit4("Ground Color", roadColorArr)) {
      stageSettings_->SetRoadColor(
          {roadColorArr[0], roadColorArr[1], roadColorArr[2], roadColorArr[3]});
    }

    bool driftIceEnabled = stageSettings_->GetDriftIceEnabled();
    if (ImGui::Checkbox("Enable Drift Ice (流氷)", &driftIceEnabled)) {
      stageSettings_->SetDriftIceEnabled(driftIceEnabled);
    }
    if (driftIceEnabled) {
      Vector4 driftIceColor = stageSettings_->GetDriftIceColor();
      float driftIceColorArr[4] = {driftIceColor.x, driftIceColor.y, driftIceColor.z, driftIceColor.w};
      if (ImGui::ColorEdit4("Drift Ice Color", driftIceColorArr)) {
        stageSettings_->SetDriftIceColor(
            {driftIceColorArr[0], driftIceColorArr[1], driftIceColorArr[2], driftIceColorArr[3]});
      }
      float bobbingScale = stageSettings_->GetDriftIceBobbingScale();
      if (ImGui::SliderFloat("Drift Ice Bobbing Scale (揺れ幅)", &bobbingScale, 0.0f, 3.0f, "%.2f")) {
        stageSettings_->SetDriftIceBobbingScale(bobbingScale);
      }
      float bobbingSpeed = stageSettings_->GetDriftIceBobbingSpeedScale();
      if (ImGui::SliderFloat("Drift Ice Bobbing Speed (揺れ速度)", &bobbingSpeed, 0.0f, 3.0f, "%.2f")) {
        stageSettings_->SetDriftIceBobbingSpeedScale(bobbingSpeed);
      }
      float sizeScale = stageSettings_->GetDriftIceSizeScale();
      if (ImGui::SliderFloat("Drift Ice Size Scale (大きさ倍率)", &sizeScale, 0.1f, 3.0f, "%.2f")) {
        stageSettings_->SetDriftIceSizeScale(sizeScale);
      }
      float thicknessScale = stageSettings_->GetDriftIceThicknessScale();
      if (ImGui::SliderFloat("Drift Ice Thickness (厚み・縦の太さ)", &thicknessScale, 0.5f, 6.0f, "%.2f")) {
        stageSettings_->SetDriftIceThicknessScale(thicknessScale);
      }
      float heightOffset = stageSettings_->GetDriftIceHeightOffset();
      if (ImGui::SliderFloat("Drift Ice Height (浮遊高さ)", &heightOffset, -1.0f, 5.0f, "%.2f m")) {
        stageSettings_->SetDriftIceHeightOffset(heightOffset);
      }
      float distOffset = stageSettings_->GetDriftIceDistanceOffset();
      if (ImGui::SliderFloat("Drift Ice Distance (地面からの離れ具合)", &distOffset, 0.0f, 20.0f, "%.1f m")) {
        stageSettings_->SetDriftIceDistanceOffset(distOffset);
      }
      bool lighting = stageSettings_->GetDriftIceLighting();
      if (ImGui::Checkbox("Drift Ice Lighting (ライティング)", &lighting)) {
        stageSettings_->SetDriftIceLighting(lighting);
      }
      float sealSpin = stageSettings_->GetBabySealSpinSpeed();
      if (ImGui::SliderFloat("Baby Seal Spin Speed (アザラシ回転速度)", &sealSpin, 0.0f, 10.0f, "%.1f rad/s")) {
        stageSettings_->SetBabySealSpinSpeed(sealSpin);
      }
    }

    ImGui::Separator();
    ImGui::Text("Road Backwards: %d chunks (-%.0f m)",
                stageSettings_->GetBackwardChunks(),
                static_cast<float>(stageSettings_->GetBackwardChunks()) * stageSettings_->GetChunkLength());
    float waterForwardExt = stageSettings_->GetWaterForwardExtension();
    if (ImGui::SliderFloat("Water Forward Ext (波の奥への拡張)", &waterForwardExt, 50.0f, 1200.0f, "%.0f m")) {
      stageSettings_->SetWaterForwardExtension(waterForwardExt);
    }
    float waterBackwardExt = stageSettings_->GetWaterBackwardExtension();
    if (ImGui::SliderFloat("Water Backward Ext (波の手前への拡張)", &waterBackwardExt, 50.0f, 800.0f, "%.0f m")) {
      stageSettings_->SetWaterBackwardExtension(waterBackwardExt);
    }
    float waterWidthScale = stageSettings_->GetWaterWidthScale();
    if (ImGui::SliderFloat("Water Width Scale (波の横幅)", &waterWidthScale, 50.0f, 400.0f, "%.0f m")) {
      stageSettings_->SetWaterWidthScale(waterWidthScale);
    }
  }

  // アイテムクールタイム設定
  if (ImGui::CollapsingHeader("Item CoolDowns (アイテムクールタイム設定)",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    float spawnChance = stageSettings_->GetItemSpawnChance() * 100.0f;
    if (ImGui::SliderFloat("Item Spawn Chance (%)", &spawnChance, 0.0f, 100.0f, "%.1f %%")) {
      stageSettings_->SetItemSpawnChance(spawnChance / 100.0f);
    }
    ImGui::Separator();

    struct ItemInfo {
      Obstacle::Type type;
      const char *name;
      const char *desc;
    };
    ItemInfo items[] = {
        {Obstacle::Type::Bonus, "Bonus (ボーナス)", "スコア加算"},
        {Obstacle::Type::BarrierItem, "Barrier (バリア)", "ミスを1回防御"},
        {Obstacle::Type::ClearItem, "Clear (障害物全消去)", "画面内の障害物を一掃"},
        {Obstacle::Type::CameraItem, "Camera (視点切替)", "1レーンモードへ移行"},
        {Obstacle::Type::BossItem, "Boss (ボス戦突入)", "ボスバトル開始"},
    };

    for (const auto &item : items) {
      ImGui::PushID(static_cast<int>(item.type));
      float duration = stageSettings_->GetItemCoolDownDuration(item.type);
      float current = stageSettings_->GetItemCoolDownTimer(item.type);

      ImGui::Text("%s - %s", item.name, item.desc);
      if (ImGui::SliderFloat("CoolTime (s)", &duration, 0.0f, 120.0f, "%.1f s")) {
        stageSettings_->SetItemCoolDownDuration(item.type, duration);
      }

      if (current <= 0.0f) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "  Status: READY (出現可能)");
      } else {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "  Status: COOLDOWN (残り %.1f s)", current);
      }

      ImGui::SameLine();
      if (ImGui::SmallButton("Reset CD (即時可能)")) {
        stageSettings_->SetItemCoolDownTimer(item.type, 0.0f);
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("Trigger CD")) {
        stageSettings_->SetItemCoolDownTimer(item.type, duration);
      }
      ImGui::Separator();
      ImGui::PopID();
    }
  }

  // アイテム効果のデバッグ
  if (ImGui::CollapsingHeader("Item Debug / Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
    bool hasBarrier = player_->GetHasBarrier();
    ImGui::Text("Player Barrier Status: %s", hasBarrier ? "ACTIVE (ON)" : "INACTIVE (OFF)");

    if (ImGui::Button("Give Barrier (バリア付与)")) {
      player_->SetHasBarrier(true);
      effectManager_->EmitBarrier(player_->GetTransform().translate);
    }
    ImGui::SameLine();
    if (ImGui::Button("Break Barrier (バリア破壊/解除)")) {
      player_->SetHasBarrier(false);
      effectManager_->BreakBarrier(player_->GetTransform().translate);
    }

    ImGui::Separator();
    if (ImGui::Button("Clear All Obstacles (全障害物吹き飛ばし)")) {
      for (int j = 0; j < stageSettings_->GetMaxObstacles(); j++) {
        Obstacle *obs = stageSettings_->GetObstacle(j);
        if (obs && obs->GetIsActive() && obs->IsRegularObstacle()) {
          obs->OnBlowAway();
        }
      }
    }

    if (ImGui::Button("Trigger Camera Item (視点切り替え)")) {
      ChangePlayingState(PlayingState::OneLane);
    }
    ImGui::SameLine();
    if (ImGui::Button("Trigger Boss Item (ボス戦移行)")) {
      for (int j = 0; j < stageSettings_->GetMaxObstacles(); j++) {
        Obstacle *obs = stageSettings_->GetObstacle(j);
        if (obs && obs->GetIsActive() && obs->IsRegularObstacle()) {
          obs->OnBlowAway();
        }
      }
      ChangePlayingState(PlayingState::Boss, true);
    }

    ImGui::Separator();
    ImGui::Text("Spawn Billboard Items Ahead (テスト生成):");
    auto spawnItemAhead = [this](Obstacle::Type itemType) {
      for (int j = 0; j < stageSettings_->GetMaxObstacles(); j++) {
        Obstacle *obs = stageSettings_->GetObstacle(j);
        if (obs && !obs->GetIsActive()) {
          obs->SetType(itemType);
          obs->Spawn(player_->GetTransform().translate.x, 2.5f,
                     player_->GetTransform().translate.z + 20.0f);
          break;
        }
      }
    };
    if (ImGui::Button("Spawn Shield (バリア)")) spawnItemAhead(Obstacle::Type::BarrierItem);
    ImGui::SameLine();
    if (ImGui::Button("Spawn Boss (王冠)")) spawnItemAhead(Obstacle::Type::BossItem);
    ImGui::SameLine();
    if (ImGui::Button("Spawn Bomb (ボム)")) spawnItemAhead(Obstacle::Type::ClearItem);
    ImGui::SameLine();
    if (ImGui::Button("Spawn OneLane (1)")) spawnItemAhead(Obstacle::Type::CameraItem);

    ImGui::Separator();
    ImGui::Text("Effect Triggers:");
    if (ImGui::Button("Emit Dust")) {
      effectManager_->EmitDust(player_->GetTransform().translate);
    }
    ImGui::SameLine();
    if (ImGui::Button("Trigger Crash Effect")) {
      gameState_ = GameState::PlayerHit;
      crashEffect_->Trigger();
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Crash);
      player_->OnHit(false);
    }

    crashEffect_->ImGui();
  }

  effectManager_->ImGui();
  ImGui::End();

  Matrix4x4 projection = MakePerspectiveFovMatrix(
      Camera::kDefaultFovY, Camera::kDefaultAspectRatio,
      Camera::kDefaultNearClip, Camera::kDefaultFarClip);
  editorUI_->Draw(gameObjectManager_.get(), cameraController_->GetViewMatrix(), projection);

#ifdef _USE_IMGUI
  if (!EditorManager::IsPlaying()) {
    EditorManager::SetSceneOverlayCallback([this]() {
      Matrix4x4 proj = MakePerspectiveFovMatrix(
          Camera::kDefaultFovY, Camera::kDefaultAspectRatio,
          Camera::kDefaultNearClip, Camera::kDefaultFarClip);
      editorUI_->DrawGizmoInScene(cameraController_->GetViewMatrix(), proj);
    });
  } else {
    EditorManager::ClearSceneOverlayCallback();
  }
#endif

  // ポーズメニュー
  if (gameState_ == GameState::Paused) {
    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Pause Menu", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoCollapse);
    ImGui::Text("PAUSED");
    ImGui::Separator();
    if (ImGui::Button("Resume (ESC)", ImVec2(200, 40))) {
      gameState_ = GameState::Playing;
    }
    if (ImGui::Button("Return to Title", ImVec2(200, 40))) {
      ReturnToTitle();
    }
    ImGui::End();
  }

  // タイトル中のImGui
  if (gameState_ == GameState::Title) {
    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.85f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Title Menu", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoCollapse);
    if (ImGui::Button("Start Game (SPACE)", ImVec2(220, 45))) {
      StartGame();
    }
    ImGui::End();
  }
#endif
}

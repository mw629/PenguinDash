#include "GameScene.h"
#include "../../Editer/EditorManager.h"
#include "AssetManager.h"
#include "GameSceneManager.h"
#include "Graphics/Render/Draw.h"
#include "Network/Firebase/FirebaseRankingManager.h"
#include "System/SoundManager.h"
#include <Engine.h>
#include <GameObjects/Object/3d/Model.h>
#include <Math/Calculation.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <memory>
#include <nlohmann/json.hpp>


GameScene::~GameScene() {
  EditorManager::SetGameViewDrawCallback(nullptr, nullptr);
  EditorManager::SetSaveCallback(nullptr);
  EditorManager::SetLoadCallback(nullptr);
  EditorManager::SetFileDropCallback(nullptr);
  EditorManager::ClearSceneOverlayCallback();
}

void GameScene::Initialize() {
  sceneID_ = SceneID::Game;
  FreezeTransition::GetInstance()->Initialize();
  resultTransition_ = ResultTransition::None;

  titleDirector_->Initialize(texture_.get());
  crashEffect_->Initialize(texture_.get());
  gameHUD_->Initialize(texture_.get());

  if (!EditorManager::IsPlaying()) {
    gameState_ = GameState::Editor;
    SoundManager::GetInstance()->StopBGM();
  } else {
    gameState_ = GameState::Title;
  }
  cameraController_->SetDebugCamera(true);
  sceneChangeRequest_ = false;

  effectManager_->Initialize();
  cameraController_->Initialize();

  // Game View 描画コールバックの登録
  EditorManager::SetGameViewDrawCallback(
      [this](class Draw &draw) {
        Matrix4x4 gameViewMat = cameraController_->GetGameViewMatrix();

        Transform originalSkyBoxT = skyBox_->GetTransform();
        Transform gameSkyBoxT = originalSkyBoxT;
        gameSkyBoxT.translate =
            cameraController_->GetGameCamera()->GetTransform().translate;
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
      [this](class Draw &draw) { DrawUI(draw); });

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

  if (gameState_ == GameState::Title) {
    ReturnToTitle();
  } else {
    ResetGame();
  }

  SoundManager::GetInstance()->Initialize();
  if (gameState_ == GameState::Title) {
    SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Title);
  }

  // 保存された設定ファイルが存在する場合は自動読み込み
  std::filesystem::path defaultSettingsPath = "Resources/Json/GameScene/GameSceneSettings.json";
  if (std::filesystem::exists(defaultSettingsPath)) {
    LoadSettingsFromJson("GameSceneSettings");
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
  if (titleDirector_->IsExiting())
    return;
  // タイトルの障害物を一瞬で消さず、新規スポーンのみ停止して手前の障害物がGOまでに自然に捌けるようにする
  stageSettings_->SetSpawningPaused(true);

  // 遠すぎる障害物（Z > 28.0f）のみ消去。手前〜中距離の障害物は自然に通り過ぎていき、
  // 3.4秒のカウントダウン（3, 2, 1）中にすべて画面外へ捌けてGOの瞬間にコースが何もない状態になる
  for (int i = 0; i < stageSettings_->GetMaxObstacles(); i++) {
    Obstacle *obs = stageSettings_->GetObstacle(i);
    if (obs && obs->GetIsActive()) {
      if (obs->GetTransform().translate.z > 28.0f) {
        obs->Deactivate();
      }
    }
  }

  titleDirector_->StartExit(cameraController_->GetTransform());
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

  titleSubView_ = TitleSubView::None;
  titleMenuIndex_ = TitleMenuItem::GameStart;
  if (scoreManager_) {
    titlePlayerName_ = scoreManager_->GetPlayerName();
  }

  titleDirector_->Reset();
  SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Title);
}

void GameScene::StartResultTransition(ResultTransition target) {
  if (FreezeTransition::GetInstance()->IsActive() ||
      resultTransition_ != ResultTransition::None) {
    return;
  }
  resultTransition_ = target;
  FreezeTransition::GetInstance()->Start(
      kResultFreezeDuration, kResultShatterDuration, [this, target]() {
        if (target == ResultTransition::Restart) {
          ResetGame();
          gameState_ = GameState::Playing;
          cameraController_->Reset(GameCameraController::kDefaultCameraTransform);
          player_->SetAutoPilot(false);
          player_->SetInvertedControls(false);
          player_->SetInputEnabled(false);
          stageSettings_->ClearObstacles(kStartObstacleClearDistance);
          stageSettings_->SetSpawningPaused(true);
          stageSettings_->SetBaseScrollSpeed(kPlayBaseScrollSpeed);
          stageSettings_->SetScrollAcceleration(0.0f);
          titleDirector_->StartRestartCountdown();
        } else if (target == ResultTransition::Title) {
          ReturnToTitle();
        }
        resultTransition_ = ResultTransition::None;
      });
}

void GameScene::Update() {
  ObjectBase::SetWvpIndex(0);
  EffectDefinition::SetWvpIndex(0);
  uiTimer_ += Engine::kDefaultDeltaTime;
  SoundManager::GetInstance()->Update();

  // タイトル開始演出タイマー更新
  titleDirector_->UpdateExit(Engine::kDefaultDeltaTime);

  // 設定保存・読み込み通知メッセージのタイマー更新
  if (settingsStatusTimer_ > 0.0f) {
    settingsStatusTimer_ -= Engine::kDefaultDeltaTime;
  }

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
        stageSettings_->SetItemCoolDownTimer(Obstacle::Type::BossItem,
                                             bossItemDuration);
      }
    } else {
      boss_->Spawn();
    }
  });

  // カメラの更新 (タイトルオービット or プレイ中追従。FPVはプレイヤー移動後に1回のみ更新)
  float timeScale = EditorManager::GetPlaySpeed();
  if (!cameraController_->IsFirstPerson()) {
    if (gameState_ == GameState::Title) {
      titleDirector_->UpdateCamera(player_.get(), cameraController_.get(),
                                   Engine::kDefaultDeltaTime * timeScale);
    } else if (gameState_ == GameState::Playing ||
               gameState_ == GameState::Editor) {
      cameraController_->UpdateGameplayCamera(
          player_.get(), Engine::kDefaultDeltaTime * timeScale);
    }

    cameraController_->Update();

    // SkyBoxをカメラの位置に追従
    Transform skyboxTransform = skyBox_->GetTransform();
    skyboxTransform.translate =
        cameraController_->GetCamera()->GetTransform().translate;
    skyBox_->SetTransform(skyboxTransform);
  }

  // 各ステート別の更新
  if (gameState_ == GameState::Title) {
    TitleUpdate();
  } else if (gameState_ == GameState::Playing) {
    PlayingUpdate();
    if (Input::PushKey(DIK_ESCAPE) ||
        GamePadInput::PushButton(XINPUT_GAMEPAD_START)) {
      gameState_ = GameState::Paused;
      SoundManager::GetInstance()->PauseBGM();
    }
  } else if (gameState_ == GameState::Paused) {
    PausedUpdate();
    effectManager_->EditorUpdate(cameraController_->GetViewMatrix());
    if (Input::PushKey(DIK_ESCAPE) ||
        GamePadInput::PushButton(XINPUT_GAMEPAD_START)) {
      gameState_ = GameState::Playing;
      SoundManager::GetInstance()->ResumeBGM();
    }
  } else if (gameState_ == GameState::PlayerHit) {
    PlayerHitUpdate();
  } else if (gameState_ == GameState::GameOver) {
    effectManager_->PlayerHitUpdate(cameraController_->GetViewMatrix());
    if (resultTransition_ == ResultTransition::None &&
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
    Transform skyboxTransform = skyBox_->GetTransform();
    skyboxTransform.translate =
        cameraController_->GetCamera()->GetTransform().translate;
    skyBox_->SetTransform(skyboxTransform);
  }
}

void GameScene::UpdateWorldEntities(float timeScale, float speedMultiplier) {
  const Matrix4x4 &view = cameraController_->GetViewMatrix();
  gameObjectManager_->UpdateAll(view, speedMultiplier * timeScale);

  float activeWaterTime =
      cameraController_->GetGameCamera()
          ? cameraController_->GetGameCamera()->GetTotalTime()
          : cameraController_->GetCamera()->GetTotalTime();
  stageSettings_->Update(view, timeScale, activeWaterTime);

  effectManager_->PlayingUpdate(view, player_->GetTransform().translate);

  if (!player_->GetIsRolling() &&
      player_->GetTransform().translate.y <= kDustGroundThresholdY) {
    effectManager_->EmitDust(player_->GetTransform().translate);
  }
}

void GameScene::PlayingUpdate() {
  // リスタート等のカウントダウン演出中の制御（走っている感を出してGOで加速・操作解禁）
  if (titleDirector_->IsCountdownActive() &&
      titleDirector_->GetCountdownMode() == TitleDirector::CountdownMode::Restart) {
    if (titleDirector_->IsInputAllowed()) {
      // GO! に達したら加速・スポーン・操作解禁・BGM開始
      if (stageSettings_->GetScrollAcceleration() == 0.0f) {
        stageSettings_->SetScrollAcceleration(kPlayScrollAcceleration);
        stageSettings_->ClearObstacles(kStartObstacleClearDistance);
        stageSettings_->SetSpawningPaused(false);
        player_->SetInputEnabled(true);
        SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
      }
    } else {
      // 3, 2, 1 の間はスポーン停止＆プレイヤー入力無効（走るアニメーションと地面・流氷スクロールは維持）
      stageSettings_->SetSpawningPaused(true);
      player_->SetInputEnabled(false);
      player_->ClearInputBuffer();
    }
  }

  GameSceneManager::GetInstance()->SetInBossBattle(playingState_ ==
                                                   PlayingState::Boss);

  float timeScale = EditorManager::GetPlaySpeed();
  float speedMultiplier = 1.0f;
  if (stageSettings_->GetBaseScrollSpeed() > 0.0f) {
    speedMultiplier =
        stageSettings_->GetScrollSpeed() / stageSettings_->GetBaseScrollSpeed();
  }

  float scoreScrollSpeed = stageSettings_->GetScrollSpeed();
  if (titleDirector_->IsCountdownActive() && !titleDirector_->IsInputAllowed()) {
    scoreScrollSpeed = 0.0f; // GO! まではスコア・走行距離の加算を保留
  }

  scoreManager_->Update(scoreScrollSpeed, timeScale);
  CheckKeepRolling();

  // 1レーン（右サイドカメラ）モードの走行距離加算
  if (isRightSideMode_) {
    rightSideDistance_ += scoreScrollSpeed * timeScale;
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
      bossBattleController_->Update(timeScale, boss_.get(),
                                    stageSettings_.get(), effectManager_.get(),
                                    cameraController_.get());
    }
  }

  player_->SetLaneLimits(stageSettings_->GetMinLaneIndex(),
                         stageSettings_->GetMaxLaneIndex(),
                         stageSettings_->GetLaneWidth());

  bool isCameraInverted = (playingState_ == PlayingState::Boss &&
                           cameraController_->GetTransform().rotate.y >
                               kInvertedCameraThresholdRotateY);
  player_->SetInvertedControls(isCameraInverted);

  UpdateWorldEntities(timeScale, speedMultiplier);
  CheckCollisions();
}

void GameScene::UpdateTitleTextInput() {
  bool isShift = Input::PressKey(DIK_LSHIFT) || Input::PressKey(DIK_RSHIFT);

  // A - Z
  static const struct {
    uint32_t key;
    char lower;
    char upper;
  } kAlphabetKeys[] = {
    {DIK_A, 'a', 'A'}, {DIK_B, 'b', 'B'}, {DIK_C, 'c', 'C'}, {DIK_D, 'd', 'D'},
    {DIK_E, 'e', 'E'}, {DIK_F, 'f', 'F'}, {DIK_G, 'g', 'G'}, {DIK_H, 'h', 'H'},
    {DIK_I, 'i', 'I'}, {DIK_J, 'j', 'J'}, {DIK_K, 'k', 'K'}, {DIK_L, 'l', 'L'},
    {DIK_M, 'm', 'M'}, {DIK_N, 'n', 'N'}, {DIK_O, 'o', 'O'}, {DIK_P, 'p', 'P'},
    {DIK_Q, 'q', 'Q'}, {DIK_R, 'r', 'R'}, {DIK_S, 's', 'S'}, {DIK_T, 't', 'T'},
    {DIK_U, 'u', 'U'}, {DIK_V, 'v', 'V'}, {DIK_W, 'w', 'W'}, {DIK_X, 'x', 'X'},
    {DIK_Y, 'y', 'Y'}, {DIK_Z, 'z', 'Z'}
  };

  // 0 - 9
  static const struct {
    uint32_t key;
    char num;
  } kNumberKeys[] = {
    {DIK_0, '0'}, {DIK_1, '1'}, {DIK_2, '2'}, {DIK_3, '3'}, {DIK_4, '4'},
    {DIK_5, '5'}, {DIK_6, '6'}, {DIK_7, '7'}, {DIK_8, '8'}, {DIK_9, '9'},
    {DIK_NUMPAD0, '0'}, {DIK_NUMPAD1, '1'}, {DIK_NUMPAD2, '2'}, {DIK_NUMPAD3, '3'},
    {DIK_NUMPAD4, '4'}, {DIK_NUMPAD5, '5'}, {DIK_NUMPAD6, '6'}, {DIK_NUMPAD7, '7'},
    {DIK_NUMPAD8, '8'}, {DIK_NUMPAD9, '9'}
  };

  constexpr size_t kMaxNameLength = 14;

  bool textChanged = false;
  if (titlePlayerName_.size() < kMaxNameLength) {
    for (const auto &entry : kAlphabetKeys) {
      if (Input::PushKey(entry.key)) {
        titlePlayerName_.push_back(isShift ? entry.upper : entry.lower);
        textChanged = true;
      }
    }
    for (const auto &entry : kNumberKeys) {
      if (Input::PushKey(entry.key)) {
        titlePlayerName_.push_back(entry.num);
        textChanged = true;
      }
    }
    if (Input::PushKey(DIK_MINUS)) {
      titlePlayerName_.push_back(isShift ? '_' : '-');
      textChanged = true;
    }
    if (Input::PushKey(DIK_SPACE)) {
      titlePlayerName_.push_back(' ');
      textChanged = true;
    }
  }

  // Backspace で1文字削除
  if (Input::PushKey(DIK_BACKSPACE) && !titlePlayerName_.empty()) {
    titlePlayerName_.pop_back();
    textChanged = true;
  }

  if (textChanged && scoreManager_) {
    scoreManager_->SetPlayerName(titlePlayerName_);
  }
}

void GameScene::TitleUpdate() {
  float timeScale = EditorManager::GetPlaySpeed();
  float speedMultiplier = 1.0f;
  stageSettings_->SetSpawningPaused(titleDirector_->IsExiting());

  titleDirector_->UpdateAutoPilot(player_.get(), stageSettings_.get());

  player_->SetLaneLimits(stageSettings_->GetMinLaneIndex(),
                         stageSettings_->GetMaxLaneIndex(),
                         stageSettings_->GetLaneWidth());
  player_->SetInvertedControls(false);

  CheckKeepRolling();

  UpdateWorldEntities(timeScale, speedMultiplier);
  titleDirector_->CheckCollisions(player_.get(), stageSettings_.get(),
                                  scoreManager_.get());

  if (titleDirector_->IsExiting()) {
    // GO!の瞬間に直ちにゲームプレイを開始し、スタート演出からシームレスにゲームへ繋げる
    if (titleDirector_->IsGoStarted()) {
      StartPlaying();
    }
    return;
  }

  // ==========================================================
  // モーダル表示中（ランキング または 操作説明）の入力処理
  // ==========================================================
  if (titleSubView_ != TitleSubView::None) {
    bool closeTriggered =
        Input::PushKey(DIK_ESCAPE) || Input::PushKey(DIK_SPACE) ||
        Input::PushKey(DIK_RETURN) || Input::PushKey(DIK_NUMPADENTER) ||
        GamePadInput::PushButton(XINPUT_GAMEPAD_B) ||
        GamePadInput::PushButton(XINPUT_GAMEPAD_A);

    if (closeTriggered) {
      titleSubView_ = TitleSubView::None;
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Land);
    }
    return;
  }

  // ==========================================================
  // タイトル画面のメニュー選択・名前登録入力処理
  // ==========================================================
  // 1. 名前登録バー選択中の文字入力
  if (titleMenuIndex_ == TitleMenuItem::PlayerName) {
    UpdateTitleTextInput();
  }

  // 2. 上下カーソル移動
  bool moveUp = false;
  bool moveDown = false;

  if (titleMenuIndex_ == TitleMenuItem::PlayerName) {
    // 名前入力中は、W/Sは文字入力に使うため、矢印キーやパッド、Enterで移動
    moveUp = Input::PushKey(DIK_UP) ||
             GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_UP) ||
             GamePadInput::PushLeftStickUp();
    moveDown = Input::PushKey(DIK_DOWN) || Input::PushKey(DIK_RETURN) ||
               Input::PushKey(DIK_NUMPADENTER) ||
               GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_DOWN) ||
               GamePadInput::PushLeftStickDown();
  } else {
    // 通常メニュー項目選択中は、W/S や 矢印キー、パッドで移動可能
    moveUp = Input::PushKey(DIK_UP) || Input::PushKey(DIK_W) ||
             GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_UP) ||
             GamePadInput::PushLeftStickUp();
    moveDown = Input::PushKey(DIK_DOWN) || Input::PushKey(DIK_S) ||
               GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_DOWN) ||
               GamePadInput::PushLeftStickDown();
  }

  constexpr int kMenuCount = static_cast<int>(TitleMenuItem::Count);
  if (moveUp) {
    int cur = static_cast<int>(titleMenuIndex_);
    cur = (cur - 1 + kMenuCount) % kMenuCount;
    titleMenuIndex_ = static_cast<TitleMenuItem>(cur);
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
  } else if (moveDown) {
    int cur = static_cast<int>(titleMenuIndex_);
    cur = (cur + 1) % kMenuCount;
    titleMenuIndex_ = static_cast<TitleMenuItem>(cur);
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
  }

  // 3. 決定操作
  bool selectTriggered = false;
  if (titleMenuIndex_ == TitleMenuItem::PlayerName) {
    // 名前入力バーでは Enter または Aボタン で確定して GameStart へ進む
    if (Input::PushKey(DIK_RETURN) || Input::PushKey(DIK_NUMPADENTER) ||
        GamePadInput::PushButton(XINPUT_GAMEPAD_A)) {
      if (titlePlayerName_.empty()) {
        titlePlayerName_ = "Player";
        if (scoreManager_) {
          scoreManager_->SetPlayerName(titlePlayerName_);
        }
      }
      titleMenuIndex_ = TitleMenuItem::GameStart;
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Land);
    }
  } else {
    // メニュー項目上では SPACE, ENTER, ゲームパッドA, START で実行
    selectTriggered = Input::PushKey(DIK_SPACE) || Input::PushKey(DIK_RETURN) ||
                      Input::PushKey(DIK_NUMPADENTER) ||
                      GamePadInput::PushButton(XINPUT_GAMEPAD_A) ||
                      GamePadInput::PushButton(XINPUT_GAMEPAD_START);
  }

  if (selectTriggered) {
    // 名前が空ならデフォルトに設定
    if (titlePlayerName_.empty()) {
      titlePlayerName_ = "Player";
      if (scoreManager_) {
        scoreManager_->SetPlayerName(titlePlayerName_);
      }
    }

    switch (titleMenuIndex_) {
    case TitleMenuItem::GameStart:
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Start);
      StartGame();
      break;

    case TitleMenuItem::Ranking:
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);
      FirebaseRankingManager::GetInstance()->FetchBothRankingsAsync(10);
      titleSubView_ = TitleSubView::Ranking;
      break;

    case TitleMenuItem::HowToPlay:
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);
      titleSubView_ = TitleSubView::HowToPlay;
      break;

    case TitleMenuItem::Exit:
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Start);
      PostQuitMessage(0);
      break;

    default:
      break;
    }
  }
}

void GameScene::PausedUpdate() {
  if (resultTransition_ == ResultTransition::None &&
      !FreezeTransition::GetInstance()->IsActive()) {
    if (Input::PushKey(DIK_1) || GamePadInput::PushButton(XINPUT_GAMEPAD_A)) {
      StartResultTransition(ResultTransition::Restart);
    }
    if (Input::PushKey(DIK_2) || GamePadInput::PushButton(XINPUT_GAMEPAD_B)) {
      StartResultTransition(ResultTransition::Title);
    }
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
  if (titleDirector_->IsCountdownActive() && !titleDirector_->IsInputAllowed()) {
    return; // カウントダウン中（GO前）は衝突判定を行わない安全ガード
  }

  AABB playerAABB = player_->GetColliderAABB();

  for (int i = 0; i < stageSettings_->GetMaxObstacles(); i++) {
    Obstacle *obstacle = stageSettings_->GetObstacle(i);
    if (!obstacle->GetIsActive() || obstacle->GetIsHit())
      continue;

    AABB obstacleAABB = obstacle->GetColliderAABB();

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
  cameraController_->ShakeImpact();
  SoundManager::GetInstance()->PlaySE(SoundManager::SE::Crash);
  GamePadInput::Rumble(kHitRumbleDuration, kHitRumbleMotorSpeed,
                       kHitRumbleMotorSpeed);

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
      if (!obstacle->GetIsActive() || obstacle->GetIsHit())
        continue;

      if (obstacle->CanRollUnder()) {
        AABB obstacleAABB = obstacle->GetColliderAABB();

        if (Collision::CheckAABB(standingAABB, obstacleAABB)) {
          if (playerTransform.translate.z >
              obstacle->GetTransform().translate.z) {
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
  if (!force && playingState_ == newState)
    return;

  PlayingState prevState = playingState_;
  playingState_ = newState;

  if (prevState == PlayingState::Boss && newState != PlayingState::Boss) {
    wasBossBattle_ = true;
    SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
    float bossItemDuration =
        stageSettings_->GetItemCoolDownDuration(Obstacle::Type::BossItem);
    stageSettings_->SetItemCoolDownTimer(Obstacle::Type::BossItem,
                                         bossItemDuration);
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

  auto rankingMgr = FirebaseRankingManager::GetInstance();
  ctx.onlineScoreRankings = &rankingMgr->GetCachedScoreRankings();
  ctx.onlineDistanceRankings = &rankingMgr->GetCachedDistanceRankings();
  ctx.lastSubmittedDocId = &rankingMgr->GetLastSubmittedDocId();
  ctx.playerName = &scoreManager_->GetPlayerName();
  ctx.localDistanceRank = scoreManager_->GetLastDistanceRank();
  ctx.localScoreRank = scoreManager_->GetLastScoreRank();
  ctx.isOnlineRankingSending = rankingMgr->IsSending();
  ctx.isOnlineRankingFetching = rankingMgr->IsFetching();
  ctx.isOnlineRankingConfigured = rankingMgr->IsConfigured();

  // タイトル画面メニュー＆名前登録
  ctx.titleMenuIndex = static_cast<int>(titleMenuIndex_);
  ctx.titleSubView = static_cast<int>(titleSubView_);
  ctx.titlePlayerName = &titlePlayerName_;

  gameHUD_->DrawHUD(draw, ctx);

  // タイトル演出ペンギン疾走の描画
  titleDirector_->DrawPenguins(draw);

  // 最前面に氷結・破片トランジションを描画（GameView & メイン画面共通）
  FreezeTransition::GetInstance()->Draw(draw);
}

void GameScene::ImGui() {
#ifdef _USE_IMGUI
  if (ImGui::Begin("ゲームシーン###GameScene")) {

    // 設定プリセット (JSON 保存 / 読み込み)
    if (ImGui::CollapsingHeader("設定プリセット (JSON 保存 / 読み込み)",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::InputText("設定ファイル名", settingsFileName_, sizeof(settingsFileName_));

      std::filesystem::path dir = "Resources/Json/GameScene";
      std::vector<std::string> jsonFiles;
      if (std::filesystem::exists(dir)) {
        for (const auto &entry : std::filesystem::directory_iterator(dir)) {
          if (entry.path().extension() == ".json") {
            jsonFiles.push_back(entry.path().filename().string());
          }
        }
      }

      if (ImGui::BeginCombo("保存済み設定一覧", settingsFileName_)) {
        for (const auto &file : jsonFiles) {
          std::string nameWithoutExt = file;
          size_t extPos = nameWithoutExt.find(".json");
          if (extPos != std::string::npos) {
            nameWithoutExt = nameWithoutExt.substr(0, extPos);
          }

          bool is_selected = (std::string(settingsFileName_) == nameWithoutExt);
          if (ImGui::Selectable(file.c_str(), is_selected)) {
            strncpy_s(settingsFileName_, sizeof(settingsFileName_),
                      nameWithoutExt.c_str(), _TRUNCATE);
          }
          if (is_selected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      if (ImGui::Button("JSONに保存 (Save)", ImVec2(140, 0))) {
        SaveSettingsToJson(settingsFileName_);
      }
      ImGui::SameLine();
      if (ImGui::Button("JSONから読み込み (Load)", ImVec2(160, 0))) {
        LoadSettingsFromJson(settingsFileName_);
      }
      ImGui::SameLine();
      if (ImGui::Button("初期設定を復元", ImVec2(120, 0))) {
        ResetSettingsToDefault();
      }

      if (settingsStatusTimer_ > 0.0f) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "%s",
                           settingsStatusMessage_.c_str());
      }
      ImGui::Separator();
    }

    // ゲームオーバー時の表示
    if (gameState_ == GameState::GameOver) {
      ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
      bool open = ImGui::CollapsingHeader("=== ゲームオーバー ===",
                                          ImGuiTreeNodeFlags_DefaultOpen);
      ImGui::PopStyleColor(2);

      if (open) {
        ImGui::Text("走行距離: %.2f m",
                    scoreManager_->GetCurrentDistance());
        ImGui::Text("スコア: %.0f", scoreManager_->GetCurrentScore());
        ImGui::Separator();

        scoreManager_->ImGui();
        ImGui::Separator();

        if (ImGui::Button("リスタート (1)", ImVec2(160, 35))) {
          StartResultTransition(ResultTransition::Restart);
        }
        ImGui::SameLine();
        if (ImGui::Button("タイトルへ戻る (2)", ImVec2(160, 35))) {
          StartResultTransition(ResultTransition::Title);
        }
        ImGui::Separator();
      }
    }

    // カメラ設定
    cameraController_->ImGui(player_.get(), [this](int presetIndex) {
      if (presetIndex == 0)
        ChangePlayingState(PlayingState::ThreeLane);
      else if (presetIndex == 1)
        ChangePlayingState(PlayingState::OneLane);
      else if (presetIndex == 2)
        ChangePlayingState(PlayingState::Boss, true);
    });

    // タイトル設定
    titleDirector_->ImGui();

    // タイトルメニュー・プレイヤー名デバッグ設定
    if (gameState_ == GameState::Title &&
        ImGui::CollapsingHeader("タイトルメニュー・プレイヤー名設定",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      char nameBuf[32];
      snprintf(nameBuf, sizeof(nameBuf), "%s", titlePlayerName_.c_str());
      if (ImGui::InputText("登録プレイヤー名", nameBuf, sizeof(nameBuf))) {
        titlePlayerName_ = nameBuf;
        if (scoreManager_) {
          scoreManager_->SetPlayerName(titlePlayerName_);
        }
      }

      const char *menuNames[] = {
          "0: 名前登録バー", "1: ゲームスタート", "2: ランキング確認",
          "3: 操作説明", "4: ゲーム終了"};
      int curMenuIdx = static_cast<int>(titleMenuIndex_);
      if (ImGui::Combo("選択中メニュー項目", &curMenuIdx, menuNames,
                       IM_ARRAYSIZE(menuNames))) {
        titleMenuIndex_ = static_cast<TitleMenuItem>(curMenuIdx);
      }

      const char *subViewNames[] = {"なし", "オンラインランキング", "操作説明"};
      int curSubView = static_cast<int>(titleSubView_);
      if (ImGui::Combo("モーダルサブビュー", &curSubView, subViewNames,
                       IM_ARRAYSIZE(subViewNames))) {
        titleSubView_ = static_cast<TitleSubView>(curSubView);
      }
    }

    // サウンド設定
    if (ImGui::CollapsingHeader("サウンド設定",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      auto *gsm = GameSceneManager::GetInstance();
      float masterVol = gsm->GetMasterVolume();
      float bgmVol = gsm->GetBGMVolume();
      float seVol = gsm->GetSEVolume();

      if (ImGui::SliderFloat("マスター音量", &masterVol, 0.0f, 1.0f, "%.2f")) {
        gsm->SetMasterVolume(masterVol);
      }
      if (ImGui::SliderFloat("BGM音量", &bgmVol, 0.0f, 1.0f, "%.2f")) {
        gsm->SetBGMVolume(bgmVol);
      }
      if (ImGui::SliderFloat("SE音量", &seVol, 0.0f, 1.0f, "%.2f")) {
        gsm->SetSEVolume(seVol);
      }

      ImGui::Separator();
      ImGui::Text("BGMテスト再生:");
      if (ImGui::Button("タイトルBGM")) {
        SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Title);
      }
      ImGui::SameLine();
      if (ImGui::Button("プレイBGM (BlackDiamond)")) {
        SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
      }
      ImGui::SameLine();
      if (ImGui::Button("ボスBGM")) {
        SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Boss);
      }
      ImGui::SameLine();
      if (ImGui::Button("BGM停止")) {
        SoundManager::GetInstance()->StopBGM();
      }

      ImGui::Text("SEテスト再生:");
      if (ImGui::Button("ジャンプ##SE"))
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Jump);
      ImGui::SameLine();
      if (ImGui::Button("着地##SE"))
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Land);
      ImGui::SameLine();
      if (ImGui::Button("スライディング##SE"))
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Slide);
      ImGui::SameLine();
      if (ImGui::Button("バリア##SE"))
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Barrier);
      ImGui::SameLine();
      if (ImGui::Button("バリア破壊##SE"))
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::BarrierBreak);

      if (ImGui::Button("ボーナス獲得##SE"))
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::BonusHit);
      ImGui::SameLine();
      if (ImGui::Button("ボス弾反射##SE"))
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossReflect);
      ImGui::SameLine();
      if (ImGui::Button("ボス撃破##SE"))
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossDefeat);
      ImGui::SameLine();
      if (ImGui::Button("クラッシュ##SE"))
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Crash);
    }

    // ゲームステート
    if (ImGui::CollapsingHeader("ゲームステート", ImGuiTreeNodeFlags_DefaultOpen)) {
      const char *stateNames[] = {"タイトル",     "プレイ中",   "一時停止",
                                  "被弾", "ゲームクリア", "ゲームオーバー",
                                  "エディター"};
      ImGui::Text("現在のゲーム状態: %s", stateNames[gameState_]);

      const char *playingStateNames[] = {"3レーン", "1レーン", "ボス戦"};
      int currentStateIdx = static_cast<int>(playingState_);
      if (ImGui::Combo("プレイモード", &currentStateIdx, playingStateNames,
                       3)) {
        PlayingState newState = static_cast<PlayingState>(currentStateIdx);
        ChangePlayingState(newState);
      }

      ImGui::Separator();

      // プレイヤー情報
      if (ImGui::TreeNode("プレイヤー情報")) {
        const Transform &playerTransform = player_->GetTransform();
        ImGui::Text("位置: (%.2f, %.2f, %.2f)", playerTransform.translate.x,
                    playerTransform.translate.y, playerTransform.translate.z);
        ImGui::Text("ローリング状態: %s", player_->GetIsRolling() ? "YES" : "NO");
        ImGui::Text("ジャンプ状態: %s", player_->GetIsJumping() ? "YES" : "NO");

        float jumpPower = player_->GetJumpPower();
        if (ImGui::SliderFloat("ジャンプ力", &jumpPower, 0.10f, 0.40f,
                               "%.3f")) {
          player_->SetJumpPower(jumpPower);
        }
        float gravity = player_->GetGravity();
        if (ImGui::SliderFloat("重力加速度", &gravity, 0.005f, 0.040f, "%.4f")) {
          player_->SetGravity(gravity);
        }
        float laneSpeed = player_->GetLaneChangeSpeed();
        if (ImGui::SliderFloat("レーン移動速度", &laneSpeed, 0.05f, 0.50f,
                               "%.2f")) {
          player_->SetLaneChangeSpeed(laneSpeed);
        }
        float rollDuration = player_->GetRollDuration();
        if (ImGui::SliderFloat("ローリング時間", &rollDuration, 10.0f, 60.0f,
                               "%.0f f")) {
          player_->SetRollDuration(rollDuration);
        }

        float estAirFrames =
            gravity > 0.0f ? ((2.0f * jumpPower / gravity) + 1.0f) : 0.0f;
        float estMaxHeight = gravity > 0.0f
                                 ? ((jumpPower * jumpPower) / (2.0f * gravity))
                                 : 0.0f;
        ImGui::Text("ジャンプ滞空時間: %.0f フレーム (%.2f 秒)", estAirFrames,
                    estAirFrames / 60.0f);
        ImGui::Text("最大ジャンプ高度: +%.2f m", estMaxHeight);
        ImGui::Text("レーン移動所要時間: %.0f フレーム",
                    laneSpeed > 0.0f ? (1.0f / laneSpeed) : 0.0f);
        ImGui::Text("ローリング持続時間: %.0f フレーム (%.2f 秒)", rollDuration,
                    rollDuration / 60.0f);

        ImGui::TreePop();
      }

      // ボス情報
      if (playingState_ == PlayingState::Boss && boss_->GetIsActive()) {
        bossBattleController_->ImGui(boss_.get(), cameraController_.get(),
                                     playingState_ == PlayingState::Boss,
                                     cameraController_->IsTransitioning());
      }

      // ステージ情報
      if (ImGui::TreeNode("ステージ情報")) {
        ImGui::Text("スクロール速度: %.3f", stageSettings_->GetScrollSpeed());
        ImGui::Text("基本スクロール速度: %.3f",
                    stageSettings_->GetBaseScrollSpeed());
        ImGui::Text("最高スクロール速度: %.3f",
                    stageSettings_->GetMaxScrollSpeed());
        ImGui::Text("レーン番号範囲: 最小=%d, 最大=%d",
                    stageSettings_->GetMinLaneIndex(),
                    stageSettings_->GetMaxLaneIndex());
        ImGui::Text("レーン幅: %.2f (実効幅: %.2f)",
                    stageSettings_->GetLaneWidth(),
                    stageSettings_->GetEffectiveLaneWidth());
        ImGui::Text("チャンク数: %d (後方: %d, 前方: %d)",
                    stageSettings_->GetChunkCount(),
                    stageSettings_->GetBackwardChunks(),
                    stageSettings_->GetForwardChunks());

        ImGui::Separator();
        float spawnZ = stageSettings_->GetSpawnObstacleZ();
        if (ImGui::SliderFloat("障害物スポーンZ距離", &spawnZ, 20.0f, 160.0f, "%.1f m")) {
          stageSettings_->SetSpawnObstacleZ(spawnZ);
        }
        float curSpd = stageSettings_->GetScrollSpeed();
        float etaSec = curSpd > 0.0f ? (spawnZ / curSpd / 60.0f) : 0.0f;
        ImGui::Text("到達予測時間: 約 %.1f 秒 (@ 速度 %.3f)", etaSec, curSpd);
        float remSpawnDist = (std::max)(0.0f, stageSettings_->GetObstacleInterval() - stageSettings_->GetDistanceSinceLastSpawn());
        ImGui::Text("次回スポーンまで: %.1f m (%s)", remSpawnDist,
                    stageSettings_->GetIsSpawningPaused() ? "一時停止中" : "進行中");
        ImGui::TreePop();
      }

      ImGui::Separator();

      // スコア表示
      scoreManager_->ImGui();

      ImGui::Separator();

      if (ImGui::Button("ゲームリセット", ImVec2(120, 0))) {
        ResetGame();
        gameState_ = GameState::Playing;
      }
      ImGui::SameLine();
      if (ImGui::Button("タイトルへ戻る", ImVec2(120, 0))) {
        ReturnToTitle();
      }
    }

    // フォント設定
    if (draw_ && ImGui::CollapsingHeader("フォント / テキスト設定")) {
      float boldness = draw_->GetTextBaseBoldness();
      if (ImGui::SliderFloat("フォントの太さ", &boldness, -0.05f, 0.20f,
                             "%.3f")) {
        draw_->SetTextBaseBoldness(boldness);
      }
      ImGui::TextDisabled("デフォルト値: 0.070 (太字)");
    }

    // ステージ・アイテム設定
    stageSettings_->ImGui();

    // アイテム効果のデバッグ
    if (ImGui::CollapsingHeader("アイテムデバッグ / エフェクト",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      bool hasBarrier = player_->GetHasBarrier();
      ImGui::Text("プレイヤーバリア状態: %s",
                  hasBarrier ? "有効 (ON)" : "無効 (OFF)");

      if (ImGui::Button("バリア付与")) {
        player_->SetHasBarrier(true);
        effectManager_->EmitBarrier(player_->GetTransform().translate);
      }
      ImGui::SameLine();
      if (ImGui::Button("バリア破壊/解除")) {
        player_->SetHasBarrier(false);
        effectManager_->BreakBarrier(player_->GetTransform().translate);
      }

      ImGui::Separator();
      if (ImGui::Button("全障害物吹き飛ばし")) {
        for (int j = 0; j < stageSettings_->GetMaxObstacles(); j++) {
          Obstacle *obs = stageSettings_->GetObstacle(j);
          if (obs && obs->GetIsActive() && obs->IsRegularObstacle()) {
            obs->OnBlowAway();
          }
        }
      }

      if (ImGui::Button("視点切り替えアイテム発動 (1レーン)")) {
        ChangePlayingState(PlayingState::OneLane);
      }
      ImGui::SameLine();
      if (ImGui::Button("ボス戦アイテム発動 (ボス戦へ)")) {
        for (int j = 0; j < stageSettings_->GetMaxObstacles(); j++) {
          Obstacle *obs = stageSettings_->GetObstacle(j);
          if (obs && obs->GetIsActive() && obs->IsRegularObstacle()) {
            obs->OnBlowAway();
          }
        }
        ChangePlayingState(PlayingState::Boss, true);
      }

      ImGui::Separator();
      ImGui::Text("前方にアイテムをテスト生成:");
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
      if (ImGui::Button("バリア生成"))
        spawnItemAhead(Obstacle::Type::BarrierItem);
      ImGui::SameLine();
      if (ImGui::Button("王冠(ボス)生成"))
        spawnItemAhead(Obstacle::Type::BossItem);
      ImGui::SameLine();
      if (ImGui::Button("ボム生成"))
        spawnItemAhead(Obstacle::Type::ClearItem);
      ImGui::SameLine();
      if (ImGui::Button("1レーン生成"))
        spawnItemAhead(Obstacle::Type::CameraItem);

      ImGui::Separator();
      ImGui::Text("エフェクトトリガー:");
      if (ImGui::Button("土煙エフェクト発生")) {
        effectManager_->EmitDust(player_->GetTransform().translate);
      }
      ImGui::SameLine();
      if (ImGui::Button("クラッシュ演出発動")) {
        gameState_ = GameState::PlayerHit;
        crashEffect_->Trigger();
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Crash);
        player_->OnHit(false);
      }

      crashEffect_->ImGui();
    }

    effectManager_->ImGui();

    ImGui::Separator();
    if (ImGui::Button("画面遷移演出テスト (氷結トランジション)",
                      ImVec2(-1, 32))) {
      FreezeTransition::GetInstance()->Start();
    }
  }
  ImGui::End();

  Matrix4x4 projection = MakePerspectiveFovMatrix(
      Camera::kDefaultFovY, Camera::kDefaultAspectRatio,
      Camera::kDefaultNearClip, Camera::kDefaultFarClip);
  editorUI_->Draw(gameObjectManager_.get(), cameraController_->GetViewMatrix(),
                  projection);

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


#endif
}

bool GameScene::SaveSettingsToJson(const std::string &fileName) {
  try {
    std::filesystem::path dir = "Resources/Json/GameScene";
    if (!std::filesystem::exists(dir)) {
      std::filesystem::create_directories(dir);
    }

    std::string actualFileName = fileName;
    if (actualFileName.empty()) {
      actualFileName = "GameSceneSettings";
    }
    if (actualFileName.find(".json") == std::string::npos) {
      actualFileName += ".json";
    }
    std::string filePath = (dir / actualFileName).string();

    nlohmann::json root;

    // 1. サウンド設定
    auto *gsm = GameSceneManager::GetInstance();
    root["sound"]["masterVolume"] = gsm->GetMasterVolume();
    root["sound"]["bgmVolume"] = gsm->GetBGMVolume();
    root["sound"]["seVolume"] = gsm->GetSEVolume();

    // 2. カメラ設定
    const Transform &camT = cameraController_->GetTransform();
    root["camera"]["transform"]["translate"] = {camT.translate.x, camT.translate.y, camT.translate.z};
    root["camera"]["transform"]["rotate"] = {camT.rotate.x, camT.rotate.y, camT.rotate.z};
    root["camera"]["transform"]["scale"] = {camT.scale.x, camT.scale.y, camT.scale.z};
    root["camera"]["isFollowPlayer"] = cameraController_->IsFollowPlayer();
    root["camera"]["followSmoothSpeed"] = cameraController_->GetFollowSmoothSpeed();
    root["camera"]["followWeightX"] = cameraController_->GetFollowWeightX();
    root["camera"]["followWeightY"] = cameraController_->GetFollowWeightY();
    root["camera"]["bankRollMultiplier"] = cameraController_->GetBankRollMultiplier();

    // 3. タイトル設定
    root["title"]["isTitleOrbitCamera"] = titleDirector_->GetTitleOrbitCamera();
    root["title"]["titleCameraSpeed"] = titleDirector_->GetTitleCameraSpeed();
    root["title"]["titleCameraRadius"] = titleDirector_->GetTitleCameraRadius();
    root["title"]["titleCameraHeight"] = titleDirector_->GetTitleCameraHeight();
    root["title"]["titleTargetOffsetY"] = titleDirector_->GetTitleTargetOffsetY();

    // 4. プレイヤー設定
    root["player"]["jumpPower"] = player_->GetJumpPower();
    root["player"]["gravity"] = player_->GetGravity();
    root["player"]["laneChangeSpeed"] = player_->GetLaneChangeSpeed();
    root["player"]["rollDuration"] = player_->GetRollDuration();

    // 5. ステージ設定
    root["stage"]["laneCount"] = stageSettings_->GetTargetLaneCount();
    root["stage"]["laneWidth"] = stageSettings_->GetLaneWidth();
    root["stage"]["oneLaneWidthMultiplier"] = stageSettings_->GetOneLaneWidthMultiplier();
    root["stage"]["baseScrollSpeed"] = stageSettings_->GetBaseScrollSpeed();
    root["stage"]["maxScrollSpeed"] = stageSettings_->GetMaxScrollSpeed();
    root["stage"]["scrollAcceleration"] = stageSettings_->GetScrollAcceleration();
    root["stage"]["minObstacleDistance"] = stageSettings_->GetMinObstacleDistance();
    root["stage"]["maxObstacleDistance"] = stageSettings_->GetMaxObstacleDistance();
    root["stage"]["baseActionFrames"] = stageSettings_->GetBaseActionFrames();
    root["stage"]["minGraceFrames"] = stageSettings_->GetMinGraceFrames();
    root["stage"]["maxGraceFrames"] = stageSettings_->GetMaxGraceFrames();
    root["stage"]["noSpawnChance"] = stageSettings_->GetNoSpawnChance();
    root["stage"]["spawnObstacleZ"] = stageSettings_->GetSpawnObstacleZ();

    Vector4 roadCol = stageSettings_->GetRoadColor();
    root["stage"]["roadColor"] = {roadCol.x, roadCol.y, roadCol.z, roadCol.w};

    root["stage"]["driftIce"]["enabled"] = stageSettings_->GetDriftIceEnabled();
    Vector4 iceCol = stageSettings_->GetDriftIceColor();
    root["stage"]["driftIce"]["color"] = {iceCol.x, iceCol.y, iceCol.z, iceCol.w};
    root["stage"]["driftIce"]["bobbingScale"] = stageSettings_->GetDriftIceBobbingScale();
    root["stage"]["driftIce"]["bobbingSpeedScale"] = stageSettings_->GetDriftIceBobbingSpeedScale();
    root["stage"]["driftIce"]["sizeScale"] = stageSettings_->GetDriftIceSizeScale();
    root["stage"]["driftIce"]["thicknessScale"] = stageSettings_->GetDriftIceThicknessScale();
    root["stage"]["driftIce"]["heightOffset"] = stageSettings_->GetDriftIceHeightOffset();
    root["stage"]["driftIce"]["distanceOffset"] = stageSettings_->GetDriftIceDistanceOffset();
    root["stage"]["driftIce"]["lighting"] = stageSettings_->GetDriftIceLighting();
    root["stage"]["driftIce"]["babySealSpinSpeed"] = stageSettings_->GetBabySealSpinSpeed();

    root["stage"]["water"]["forwardExtension"] = stageSettings_->GetWaterForwardExtension();
    root["stage"]["water"]["backwardExtension"] = stageSettings_->GetWaterBackwardExtension();
    root["stage"]["water"]["widthScale"] = stageSettings_->GetWaterWidthScale();

    root["stage"]["items"]["spawnChance"] = stageSettings_->GetItemSpawnChance();
    root["stage"]["items"]["cooldowns"]["bonus"] = stageSettings_->GetItemCoolDownDuration(Obstacle::Type::Bonus);
    root["stage"]["items"]["cooldowns"]["barrier"] = stageSettings_->GetItemCoolDownDuration(Obstacle::Type::BarrierItem);
    root["stage"]["items"]["cooldowns"]["clear"] = stageSettings_->GetItemCoolDownDuration(Obstacle::Type::ClearItem);
    root["stage"]["items"]["cooldowns"]["camera"] = stageSettings_->GetItemCoolDownDuration(Obstacle::Type::CameraItem);
    root["stage"]["items"]["cooldowns"]["boss"] = stageSettings_->GetItemCoolDownDuration(Obstacle::Type::BossItem);

    // 6. ボス設定
    root["boss"]["attackSpawnZ"] = bossBattleController_->GetAttackSpawnZ();
    root["boss"]["attackDropHeight"] = bossBattleController_->GetAttackDropHeight();
    root["boss"]["attackFallDuration"] = bossBattleController_->GetAttackFallDuration();
    root["boss"]["attackReflectMinZ"] = bossBattleController_->GetAttackReflectMinZ();
    root["boss"]["reflectArcHeight"] = bossBattleController_->GetReflectArcHeight();
    root["boss"]["reflectDuration"] = bossBattleController_->GetReflectDuration();
    const Vector3 &bossCamTr = bossBattleController_->GetBossCameraTranslate();
    root["boss"]["cameraTranslate"] = {bossCamTr.x, bossCamTr.y, bossCamTr.z};
    const Vector3 &bossCamRot = bossBattleController_->GetBossCameraRotate();
    root["boss"]["cameraRotate"] = {bossCamRot.x, bossCamRot.y, bossCamRot.z};

    // 7. クラッシュ演出設定
    const Vector2 &crashPos = crashEffect_->GetCrashPosition();
    root["crashEffect"]["position"] = {crashPos.x, crashPos.y};
    root["crashEffect"]["scaleDuration"] = crashEffect_->GetCrashScaleDuration();
    const Vector2 &crashBaseSize = crashEffect_->GetCrashBaseSize();
    root["crashEffect"]["baseSize"] = {crashBaseSize.x, crashBaseSize.y};

    // 8. バリア演出設定
    const Vector3 &barrOff = effectManager_->GetBarrierOffset();
    root["barrierEffect"]["offset"] = {barrOff.x, barrOff.y, barrOff.z};
    const Vector3 &barrScale = effectManager_->GetBarrierBaseScale();
    root["barrierEffect"]["baseScale"] = {barrScale.x, barrScale.y, barrScale.z};
    const Vector4 &barrCol = effectManager_->GetBarrierBaseColor();
    root["barrierEffect"]["baseColor"] = {barrCol.x, barrCol.y, barrCol.z, barrCol.w};
    const Vector4 &flashCol = effectManager_->GetBarrierFlashColor();
    root["barrierEffect"]["flashColor"] = {flashCol.x, flashCol.y, flashCol.z, flashCol.w};
    root["barrierEffect"]["radius"] = effectManager_->GetBarrierRadius();
    root["barrierEffect"]["pointyTopped"] = effectManager_->GetBarrierPointyTopped();
    root["barrierEffect"]["shape"] = effectManager_->GetBarrierShape();

    // 9. フォント設定
    if (draw_) {
      root["font"]["boldness"] = draw_->GetTextBaseBoldness();
    }

    std::ofstream ofs(filePath);
    if (!ofs.is_open()) {
      settingsStatusMessage_ = "保存失敗: ファイルを開けませんでした (" + filePath + ")";
      settingsStatusTimer_ = 3.0f;
      return false;
    }

    ofs << root.dump(4);
    ofs.close();

    settingsStatusMessage_ = "JSON設定を保存しました: " + actualFileName;
    settingsStatusTimer_ = 3.0f;
    return true;
  } catch (const std::exception &e) {
    settingsStatusMessage_ = std::string("保存例外エラー: ") + e.what();
    settingsStatusTimer_ = 3.0f;
    return false;
  }
}

bool GameScene::LoadSettingsFromJson(const std::string &fileName) {
  try {
    std::filesystem::path dir = "Resources/Json/GameScene";
    std::string actualFileName = fileName;
    if (actualFileName.empty()) {
      actualFileName = "GameSceneSettings";
    }
    if (actualFileName.find(".json") == std::string::npos) {
      actualFileName += ".json";
    }
    std::string filePath = (dir / actualFileName).string();

    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
      settingsStatusMessage_ = "読み込み失敗: ファイルが見つかりません (" + filePath + ")";
      settingsStatusTimer_ = 3.0f;
      return false;
    }

    nlohmann::json root;
    ifs >> root;
    ifs.close();

    // 1. サウンド設定
    if (root.contains("sound")) {
      const auto &s = root["sound"];
      auto *gsm = GameSceneManager::GetInstance();
      if (s.contains("masterVolume")) gsm->SetMasterVolume(s["masterVolume"].get<float>());
      if (s.contains("bgmVolume")) gsm->SetBGMVolume(s["bgmVolume"].get<float>());
      if (s.contains("seVolume")) gsm->SetSEVolume(s["seVolume"].get<float>());
    }

    // 2. カメラ設定
    if (root.contains("camera")) {
      const auto &c = root["camera"];
      if (c.contains("transform")) {
        Transform camT = cameraController_->GetTransform();
        if (c["transform"].contains("translate") && c["transform"]["translate"].is_array() && c["transform"]["translate"].size() >= 3) {
          camT.translate = {c["transform"]["translate"][0].get<float>(), c["transform"]["translate"][1].get<float>(), c["transform"]["translate"][2].get<float>()};
        }
        if (c["transform"].contains("rotate") && c["transform"]["rotate"].is_array() && c["transform"]["rotate"].size() >= 3) {
          camT.rotate = {c["transform"]["rotate"][0].get<float>(), c["transform"]["rotate"][1].get<float>(), c["transform"]["rotate"][2].get<float>()};
        }
        if (c["transform"].contains("scale") && c["transform"]["scale"].is_array() && c["transform"]["scale"].size() >= 3) {
          camT.scale = {c["transform"]["scale"][0].get<float>(), c["transform"]["scale"][1].get<float>(), c["transform"]["scale"][2].get<float>()};
        }
        cameraController_->SetTransform(camT);
        if (cameraController_->GetCamera()) cameraController_->GetCamera()->SetTransform(camT);
        if (cameraController_->GetGameCamera()) cameraController_->GetGameCamera()->SetTransform(camT);
      }
      if (c.contains("isFollowPlayer")) cameraController_->SetFollowPlayer(c["isFollowPlayer"].get<bool>());
      if (c.contains("followSmoothSpeed")) cameraController_->SetFollowSmoothSpeed(c["followSmoothSpeed"].get<float>());
      if (c.contains("followWeightX")) cameraController_->SetFollowWeightX(c["followWeightX"].get<float>());
      if (c.contains("followWeightY")) cameraController_->SetFollowWeightY(c["followWeightY"].get<float>());
      if (c.contains("bankRollMultiplier")) cameraController_->SetBankRollMultiplier(c["bankRollMultiplier"].get<float>());
    }

    // 3. タイトル設定
    if (root.contains("title")) {
      const auto &t = root["title"];
      if (t.contains("isTitleOrbitCamera")) titleDirector_->SetTitleOrbitCamera(t["isTitleOrbitCamera"].get<bool>());
      if (t.contains("titleCameraSpeed")) titleDirector_->SetTitleCameraSpeed(t["titleCameraSpeed"].get<float>());
      if (t.contains("titleCameraRadius")) titleDirector_->SetTitleCameraRadius(t["titleCameraRadius"].get<float>());
      if (t.contains("titleCameraHeight")) titleDirector_->SetTitleCameraHeight(t["titleCameraHeight"].get<float>());
      if (t.contains("titleTargetOffsetY")) titleDirector_->SetTitleTargetOffsetY(t["titleTargetOffsetY"].get<float>());
    }

    // 4. プレイヤー設定
    if (root.contains("player")) {
      const auto &p = root["player"];
      if (p.contains("jumpPower")) player_->SetJumpPower(p["jumpPower"].get<float>());
      if (p.contains("gravity")) player_->SetGravity(p["gravity"].get<float>());
      if (p.contains("laneChangeSpeed")) player_->SetLaneChangeSpeed(p["laneChangeSpeed"].get<float>());
      if (p.contains("rollDuration")) player_->SetRollDuration(p["rollDuration"].get<float>());
    }

    // 5. ステージ設定
    if (root.contains("stage")) {
      const auto &st = root["stage"];
      if (st.contains("laneCount")) stageSettings_->SetLaneCount(st["laneCount"].get<int>());
      if (st.contains("laneWidth")) stageSettings_->SetLaneWidth(st["laneWidth"].get<float>());
      if (st.contains("oneLaneWidthMultiplier")) stageSettings_->SetOneLaneWidthMultiplier(st["oneLaneWidthMultiplier"].get<float>());
      if (st.contains("baseScrollSpeed")) stageSettings_->SetBaseScrollSpeed(st["baseScrollSpeed"].get<float>());
      if (st.contains("maxScrollSpeed")) stageSettings_->SetMaxScrollSpeed(st["maxScrollSpeed"].get<float>());
      if (st.contains("scrollAcceleration")) stageSettings_->SetScrollAcceleration(st["scrollAcceleration"].get<float>());
      if (st.contains("minObstacleDistance")) stageSettings_->SetMinObstacleDistance(st["minObstacleDistance"].get<float>());
      if (st.contains("maxObstacleDistance")) stageSettings_->SetMaxObstacleDistance(st["maxObstacleDistance"].get<float>());
      if (st.contains("baseActionFrames")) stageSettings_->SetBaseActionFrames(st["baseActionFrames"].get<float>());
      if (st.contains("minGraceFrames")) stageSettings_->SetMinGraceFrames(st["minGraceFrames"].get<float>());
      if (st.contains("maxGraceFrames")) stageSettings_->SetMaxGraceFrames(st["maxGraceFrames"].get<float>());
      if (st.contains("noSpawnChance")) stageSettings_->SetNoSpawnChance(st["noSpawnChance"].get<float>());
      if (st.contains("spawnObstacleZ")) stageSettings_->SetSpawnObstacleZ(st["spawnObstacleZ"].get<float>());
      if (st.contains("roadColor") && st["roadColor"].is_array() && st["roadColor"].size() >= 4) {
        stageSettings_->SetRoadColor({st["roadColor"][0].get<float>(), st["roadColor"][1].get<float>(), st["roadColor"][2].get<float>(), st["roadColor"][3].get<float>()});
      }

      if (st.contains("driftIce")) {
        const auto &di = st["driftIce"];
        if (di.contains("enabled")) stageSettings_->SetDriftIceEnabled(di["enabled"].get<bool>());
        if (di.contains("color") && di["color"].is_array() && di["color"].size() >= 4) {
          stageSettings_->SetDriftIceColor({di["color"][0].get<float>(), di["color"][1].get<float>(), di["color"][2].get<float>(), di["color"][3].get<float>()});
        }
        if (di.contains("bobbingScale")) stageSettings_->SetDriftIceBobbingScale(di["bobbingScale"].get<float>());
        if (di.contains("bobbingSpeedScale")) stageSettings_->SetDriftIceBobbingSpeedScale(di["bobbingSpeedScale"].get<float>());
        if (di.contains("sizeScale")) stageSettings_->SetDriftIceSizeScale(di["sizeScale"].get<float>());
        if (di.contains("thicknessScale")) stageSettings_->SetDriftIceThicknessScale(di["thicknessScale"].get<float>());
        if (di.contains("heightOffset")) stageSettings_->SetDriftIceHeightOffset(di["heightOffset"].get<float>());
        if (di.contains("distanceOffset")) stageSettings_->SetDriftIceDistanceOffset(di["distanceOffset"].get<float>());
        if (di.contains("lighting")) stageSettings_->SetDriftIceLighting(di["lighting"].get<bool>());
        if (di.contains("babySealSpinSpeed")) stageSettings_->SetBabySealSpinSpeed(di["babySealSpinSpeed"].get<float>());
      }

      if (st.contains("water")) {
        const auto &w = st["water"];
        if (w.contains("forwardExtension")) stageSettings_->SetWaterForwardExtension(w["forwardExtension"].get<float>());
        if (w.contains("backwardExtension")) stageSettings_->SetWaterBackwardExtension(w["backwardExtension"].get<float>());
        if (w.contains("widthScale")) stageSettings_->SetWaterWidthScale(w["widthScale"].get<float>());
      }

      if (st.contains("items")) {
        const auto &it = st["items"];
        if (it.contains("spawnChance")) stageSettings_->SetItemSpawnChance(it["spawnChance"].get<float>());
        if (it.contains("cooldowns")) {
          const auto &cd = it["cooldowns"];
          if (cd.contains("bonus")) stageSettings_->SetItemCoolDownDuration(Obstacle::Type::Bonus, cd["bonus"].get<float>());
          if (cd.contains("barrier")) stageSettings_->SetItemCoolDownDuration(Obstacle::Type::BarrierItem, cd["barrier"].get<float>());
          if (cd.contains("clear")) stageSettings_->SetItemCoolDownDuration(Obstacle::Type::ClearItem, cd["clear"].get<float>());
          if (cd.contains("camera")) stageSettings_->SetItemCoolDownDuration(Obstacle::Type::CameraItem, cd["camera"].get<float>());
          if (cd.contains("boss")) stageSettings_->SetItemCoolDownDuration(Obstacle::Type::BossItem, cd["boss"].get<float>());
        }
      }
    }

    // 6. ボス設定
    if (root.contains("boss")) {
      const auto &b = root["boss"];
      if (b.contains("attackSpawnZ")) bossBattleController_->SetAttackSpawnZ(b["attackSpawnZ"].get<float>());
      if (b.contains("attackDropHeight")) bossBattleController_->SetAttackDropHeight(b["attackDropHeight"].get<float>());
      if (b.contains("attackFallDuration")) bossBattleController_->SetAttackFallDuration(b["attackFallDuration"].get<float>());
      if (b.contains("attackReflectMinZ")) bossBattleController_->SetAttackReflectMinZ(b["attackReflectMinZ"].get<float>());
      if (b.contains("reflectArcHeight")) bossBattleController_->SetReflectArcHeight(b["reflectArcHeight"].get<float>());
      if (b.contains("reflectDuration")) bossBattleController_->SetReflectDuration(b["reflectDuration"].get<float>());
      if (b.contains("cameraTranslate") && b["cameraTranslate"].is_array() && b["cameraTranslate"].size() >= 3) {
        bossBattleController_->SetBossCameraTranslate({b["cameraTranslate"][0].get<float>(), b["cameraTranslate"][1].get<float>(), b["cameraTranslate"][2].get<float>()});
      }
      if (b.contains("cameraRotate") && b["cameraRotate"].is_array() && b["cameraRotate"].size() >= 3) {
        bossBattleController_->SetBossCameraRotate({b["cameraRotate"][0].get<float>(), b["cameraRotate"][1].get<float>(), b["cameraRotate"][2].get<float>()});
      }
    }

    // 7. クラッシュ演出設定
    if (root.contains("crashEffect")) {
      const auto &ce = root["crashEffect"];
      if (ce.contains("position") && ce["position"].is_array() && ce["position"].size() >= 2) {
        crashEffect_->SetCrashPosition({ce["position"][0].get<float>(), ce["position"][1].get<float>()});
      }
      if (ce.contains("scaleDuration")) crashEffect_->SetCrashScaleDuration(ce["scaleDuration"].get<float>());
      if (ce.contains("baseSize") && ce["baseSize"].is_array() && ce["baseSize"].size() >= 2) {
        crashEffect_->SetCrashBaseSize({ce["baseSize"][0].get<float>(), ce["baseSize"][1].get<float>()});
      }
    }

    // 8. バリア演出設定
    if (root.contains("barrierEffect")) {
      const auto &be = root["barrierEffect"];
      if (be.contains("offset") && be["offset"].is_array() && be["offset"].size() >= 3) {
        effectManager_->SetBarrierOffset({be["offset"][0].get<float>(), be["offset"][1].get<float>(), be["offset"][2].get<float>()});
      }
      if (be.contains("baseScale") && be["baseScale"].is_array() && be["baseScale"].size() >= 3) {
        effectManager_->SetBarrierBaseScale({be["baseScale"][0].get<float>(), be["baseScale"][1].get<float>(), be["baseScale"][2].get<float>()});
      }
      if (be.contains("baseColor") && be["baseColor"].is_array() && be["baseColor"].size() >= 4) {
        effectManager_->SetBarrierBaseColor({be["baseColor"][0].get<float>(), be["baseColor"][1].get<float>(), be["baseColor"][2].get<float>(), be["baseColor"][3].get<float>()});
      }
      if (be.contains("flashColor") && be["flashColor"].is_array() && be["flashColor"].size() >= 4) {
        effectManager_->SetBarrierFlashColor({be["flashColor"][0].get<float>(), be["flashColor"][1].get<float>(), be["flashColor"][2].get<float>(), be["flashColor"][3].get<float>()});
      }
      if (be.contains("radius")) effectManager_->SetBarrierRadius(be["radius"].get<float>());
      if (be.contains("pointyTopped")) effectManager_->SetBarrierPointyTopped(be["pointyTopped"].get<bool>());
      if (be.contains("shape")) effectManager_->SetBarrierShape(be["shape"].get<int>());
    }

    // 9. フォント設定
    if (root.contains("font") && draw_) {
      const auto &f = root["font"];
      if (f.contains("boldness")) draw_->SetTextBaseBoldness(f["boldness"].get<float>());
    }

    stageSettings_->UpdateSidePlanesTransform();

    settingsStatusMessage_ = "JSON設定を読み込みました: " + actualFileName;
    settingsStatusTimer_ = 3.0f;
    return true;
  } catch (const std::exception &e) {
    settingsStatusMessage_ = std::string("読み込み例外エラー: ") + e.what();
    settingsStatusTimer_ = 3.0f;
    return false;
  }
}

void GameScene::ResetSettingsToDefault() {
  // 1. サウンド設定
  auto *gsm = GameSceneManager::GetInstance();
  gsm->SetMasterVolume(0.5f);
  gsm->SetBGMVolume(0.35f);
  gsm->SetSEVolume(1.0f);

  // 2. カメラ設定
  cameraController_->Reset();
  cameraController_->SetFollowPlayer(true);
  cameraController_->SetFollowSmoothSpeed(10.0f);
  cameraController_->SetFollowWeightX(0.45f);
  cameraController_->SetFollowWeightY(0.25f);
  cameraController_->SetBankRollMultiplier(0.0f);

  // 3. タイトル設定
  titleDirector_->SetTitleOrbitCamera(true);
  titleDirector_->SetTitleCameraSpeed(0.35f);
  titleDirector_->SetTitleCameraRadius(14.0f);
  titleDirector_->SetTitleCameraHeight(4.0f);
  titleDirector_->SetTitleTargetOffsetY(1.2f);

  // 4. プレイヤー設定
  player_->SetJumpPower(0.21f);
  player_->SetGravity(0.014f);
  player_->SetLaneChangeSpeed(0.2f);
  player_->SetRollDuration(30.0f);

  // 5. ステージ設定
  stageSettings_->SetLaneCount(3);
  stageSettings_->SetLaneWidth(2.0f);
  stageSettings_->SetOneLaneWidthMultiplier(1.0f);
  stageSettings_->SetBaseScrollSpeed(0.2f);
  stageSettings_->SetMaxScrollSpeed(1.0f);
  stageSettings_->SetScrollAcceleration(0.0001f);
  stageSettings_->SetMinObstacleDistance(6.0f);
  stageSettings_->SetMaxObstacleDistance(45.0f);
  stageSettings_->SetBaseActionFrames(30.0f);
  stageSettings_->SetMinGraceFrames(5.0f);
  stageSettings_->SetMaxGraceFrames(15.0f);
  stageSettings_->SetNoSpawnChance(0.05f);
  stageSettings_->SetSpawnObstacleZ(105.0f);
  stageSettings_->SetRoadColor({1.0f, 1.0f, 1.0f, 1.0f});

  stageSettings_->SetDriftIceEnabled(true);
  stageSettings_->SetDriftIceColor({1.0f, 1.0f, 1.0f, 1.0f});
  stageSettings_->SetDriftIceBobbingScale(1.0f);
  stageSettings_->SetDriftIceBobbingSpeedScale(1.0f);
  stageSettings_->SetDriftIceSizeScale(1.0f);
  stageSettings_->SetDriftIceThicknessScale(1.0f);
  stageSettings_->SetDriftIceHeightOffset(0.0f);
  stageSettings_->SetDriftIceDistanceOffset(0.0f);
  stageSettings_->SetDriftIceLighting(false);
  stageSettings_->SetBabySealSpinSpeed(3.5f);

  stageSettings_->SetWaterForwardExtension(500.0f);
  stageSettings_->SetWaterBackwardExtension(200.0f);
  stageSettings_->SetWaterWidthScale(120.0f);

  stageSettings_->SetItemSpawnChance(0.15f);
  stageSettings_->SetItemCoolDownDuration(Obstacle::Type::Bonus, 5.0f);
  stageSettings_->SetItemCoolDownDuration(Obstacle::Type::BarrierItem, 15.0f);
  stageSettings_->SetItemCoolDownDuration(Obstacle::Type::ClearItem, 20.0f);
  stageSettings_->SetItemCoolDownDuration(Obstacle::Type::CameraItem, 30.0f);
  stageSettings_->SetItemCoolDownDuration(Obstacle::Type::BossItem, 45.0f);

  // 6. ボス設定
  bossBattleController_->SetAttackSpawnZ(-35.0f);
  bossBattleController_->SetAttackDropHeight(18.0f);
  bossBattleController_->SetAttackFallDuration(28.0f);
  bossBattleController_->SetAttackReflectMinZ(-15.0f);
  bossBattleController_->SetReflectArcHeight(7.0f);
  bossBattleController_->SetReflectDuration(36.0f);
  bossBattleController_->SetBossCameraTranslate({0.0f, 7.0f, 14.0f});
  bossBattleController_->SetBossCameraRotate({0.24f, 3.25f, 0.0f});

  // 7. クラッシュ演出設定
  crashEffect_->SetCrashPosition({640.0f, 320.0f});
  crashEffect_->SetCrashScaleDuration(0.35f);
  crashEffect_->SetCrashBaseSize({580.0f, 580.0f * (569.0f / 1024.0f)});

  // 8. バリア演出設定
  effectManager_->SetBarrierOffset({0.0f, 0.0f, 1.2f});
  effectManager_->SetBarrierBaseScale({0.6f, 0.6f, 0.6f});
  effectManager_->SetBarrierBaseColor({0.25f, 0.8f, 1.0f, 0.85f});
  effectManager_->SetBarrierFlashColor({1.0f, 1.0f, 1.0f, 0.95f});
  effectManager_->SetBarrierRadius(0.5f);
  effectManager_->SetBarrierPointyTopped(false);
  effectManager_->SetBarrierShape(1);

  // 9. フォント設定
  if (draw_) {
    draw_->SetTextBaseBoldness(0.070f);
  }

  stageSettings_->UpdateSidePlanesTransform();

  settingsStatusMessage_ = "初期設定に戻しました";
  settingsStatusTimer_ = 3.0f;
}


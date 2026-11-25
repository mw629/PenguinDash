#include "GameSceneStates.h"
#include "../../../Editer/EditorManager.h"
#include "../GameScene.h"
#include "../GameSceneManager.h"
#include "../System/SoundManager.h"
#include "Network/Firebase/FirebaseRankingManager.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// ==========================================
// IGameSceneState デフォルト実装
// ==========================================

float IGameSceneState::GetSunLightDeltaTime(float timeScale) const {
  return Engine::kDefaultDeltaTime * timeScale;
}

bool IGameSceneState::ShouldRenderRankingCutIn(const GameScene &scene) const {
  const auto *cutin = scene.GetRankingCutinEffect();
  return cutin && cutin->IsActive();
}

// ==========================================
// TitleSceneState
// ==========================================

void TitleSceneState::UpdateCamera(GameScene &scene, float dt) {
  auto *cam = scene.GetCameraController();
  auto *titleDir = scene.GetTitleDirector();
  auto *player = scene.GetPlayer();
  if (!cam) {
    return;
  }

  if (scene.GetPlayingState() == PlayingState::ThreeLane &&
      !cam->IsTransitioning() && !cam->IsTransitionPending() && titleDir) {
    titleDir->UpdateCamera(player, cam, dt);
  } else {
    cam->UpdateGameplayCamera(player, dt);
  }
}

std::unique_ptr<IGameSceneState> TitleSceneState::Update(GameScene &scene) {
  auto *stage = scene.GetStageSettings();
  auto *titleDir = scene.GetTitleDirector();
  auto *player = scene.GetPlayer();
  auto *scoreMgr = scene.GetScoreManager();
  auto *achMgr = scene.GetAchievementManager();

  // --- タイトル背景のオートパイロット走行とエンティティ・衝突判定の更新 ---
  float timeScale = EditorManager::GetPlaySpeed();
  float speedMultiplier = 1.0f;
  stage->SetSpawningPaused(titleDir->IsExiting());

  titleDir->UpdateAutoPilot(player, stage);

  player->SetLaneLimits(stage->GetMinLaneIndex(), stage->GetMaxLaneIndex(),
                        stage->GetLaneWidth());
  player->SetInvertedControls(false);

  scene.CheckKeepRolling();
  scene.UpdateWorldEntities(timeScale, speedMultiplier);
  titleDir->CheckCollisions(player, stage, scoreMgr);

  // --- ゲーム開始演出中（GO!）のゲームプレイ移行判定 ---
  if (titleDir->IsExiting()) {
    if (titleDir->IsGoStarted()) {
      scene.StartPlaying();
    }
    return nullptr;
  }

  // ==========================================================
  // モーダル表示中（ランキング / 操作説明 / 実績一覧 / 設定）の入力処理
  // ==========================================================
  GameScene::TitleSubView subView = scene.GetTitleSubView();
  if (subView != GameScene::TitleSubView::None) {
    if (subView == GameScene::TitleSubView::Settings) {
      scene.UpdateSettingsInput();
      return nullptr;
    }

    if (subView == GameScene::TitleSubView::Achievements && achMgr) {
      constexpr int kItemsPerPage = 10;
      int totalCount = achMgr->GetVisibleTotalCount();
      int totalPages =
          (std::max)(1, (totalCount + kItemsPerPage - 1) / kItemsPerPage);

      bool prevPage = Input::PushKey(DIK_LEFT) || Input::PushKey(DIK_A) ||
                      Input::PushKey(DIK_UP) || Input::PushKey(DIK_W) ||
                      GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_LEFT) ||
                      GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_UP) ||
                      GamePadInput::PushLeftStickLeft() ||
                      GamePadInput::PushLeftStickUp();
      bool nextPage = Input::PushKey(DIK_RIGHT) || Input::PushKey(DIK_D) ||
                      Input::PushKey(DIK_DOWN) || Input::PushKey(DIK_S) ||
                      GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_RIGHT) ||
                      GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_DOWN) ||
                      GamePadInput::PushLeftStickRight() ||
                      GamePadInput::PushLeftStickDown();

      if (totalPages > 1) {
        int page = scene.GetTitleAchievementPage();
        if (prevPage) {
          scene.SetTitleAchievementPage((page - 1 + totalPages) % totalPages);
          SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
        } else if (nextPage) {
          scene.SetTitleAchievementPage((page + 1) % totalPages);
          SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
        }
      }
    }

    bool closeTriggered =
        Input::PushKey(DIK_ESCAPE) || Input::PushKey(DIK_SPACE) ||
        Input::PushKey(DIK_RETURN) || Input::PushKey(DIK_NUMPADENTER) ||
        GamePadInput::PushButton(XINPUT_GAMEPAD_B) ||
        GamePadInput::PushButton(XINPUT_GAMEPAD_A);

    if (closeTriggered) {
      scene.SetTitleSubView(GameScene::TitleSubView::None);
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Land);
    }
    return nullptr;
  }

  // ==========================================================
  // タイトル画面のメニュー選択・名前登録入力処理
  // ==========================================================
  if (scene.IsEditingTitlePlayerName()) {
    bool confirmInput = Input::PushKey(DIK_RETURN) ||
                        Input::PushKey(DIK_NUMPADENTER) ||
                        GamePadInput::PushButton(XINPUT_GAMEPAD_A) ||
                        GamePadInput::PushButton(XINPUT_GAMEPAD_START);
    bool cancelInput = Input::PushKey(DIK_ESCAPE);

    if (confirmInput || cancelInput) {
      if (scene.GetTitlePlayerName().empty()) {
        scene.SetTitlePlayerName("Player");
      }
      if (scoreMgr) {
        scoreMgr->SetPlayerName(scene.GetTitlePlayerName());
      }
      scene.SetEditingTitlePlayerName(false);
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Land);
      return nullptr;
    }

    scene.UpdateTitleTextInput();
    return nullptr;
  }

  // 上下カーソル移動
  bool moveUp = Input::PushKey(DIK_UP) || Input::PushKey(DIK_W) ||
                GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_UP) ||
                GamePadInput::PushLeftStickUp();
  bool moveDown = Input::PushKey(DIK_DOWN) || Input::PushKey(DIK_S) ||
                  GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_DOWN) ||
                  GamePadInput::PushLeftStickDown();

  constexpr int kMenuCount = static_cast<int>(GameScene::TitleMenuItem::Count);
  if (moveUp) {
    int cur = static_cast<int>(scene.GetTitleMenuIndex());
    cur = (cur - 1 + kMenuCount) % kMenuCount;
    scene.SetTitleMenuIndex(static_cast<GameScene::TitleMenuItem>(cur));
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
  } else if (moveDown) {
    int cur = static_cast<int>(scene.GetTitleMenuIndex());
    cur = (cur + 1) % kMenuCount;
    scene.SetTitleMenuIndex(static_cast<GameScene::TitleMenuItem>(cur));
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
  }

  // 決定・アクション操作
  if (scene.GetTitleMenuIndex() == GameScene::TitleMenuItem::PlayerName) {
    if (Input::PushKey(DIK_SPACE) || Input::PushKey(DIK_RETURN) ||
        Input::PushKey(DIK_NUMPADENTER) ||
        GamePadInput::PushButton(XINPUT_GAMEPAD_A)) {
      scene.SetEditingTitlePlayerName(true);
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);
    }
  } else {
    bool selectTriggered = Input::PushKey(DIK_SPACE) ||
                           Input::PushKey(DIK_RETURN) ||
                           Input::PushKey(DIK_NUMPADENTER) ||
                           GamePadInput::PushButton(XINPUT_GAMEPAD_A) ||
                           GamePadInput::PushButton(XINPUT_GAMEPAD_START);

    if (selectTriggered) {
      if (scene.GetTitlePlayerName().empty()) {
        scene.SetTitlePlayerName("Player");
        if (scoreMgr) {
          scoreMgr->SetPlayerName(scene.GetTitlePlayerName());
        }
      }

      switch (scene.GetTitleMenuIndex()) {
      case GameScene::TitleMenuItem::GameStart:
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Start);
        scene.StartGame();
        break;

      case GameScene::TitleMenuItem::Ranking:
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);
        FirebaseRankingManager::GetInstance()->FetchBothRankingsAsync(10);
        scene.SetTitleSubView(GameScene::TitleSubView::Ranking);
        break;

      case GameScene::TitleMenuItem::Achievements:
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);
        scene.SetTitleAchievementPage(0);
        scene.SetTitleSubView(GameScene::TitleSubView::Achievements);
        break;

      case GameScene::TitleMenuItem::HowToPlay:
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);
        scene.SetTitleSubView(GameScene::TitleSubView::HowToPlay);
        break;

      case GameScene::TitleMenuItem::Settings:
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);
        scene.SetSettingsMenuIndex(0);
        scene.SetTitleSubView(GameScene::TitleSubView::Settings);
        break;

      case GameScene::TitleMenuItem::Exit:
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Start);
        PostQuitMessage(0);
        break;

      default:
        break;
      }
    }
  }

  return nullptr;
}

// ==========================================
// PlayingSceneState
// ==========================================

void PlayingSceneState::UpdateCamera(GameScene &scene, float dt) {
  if (auto *cam = scene.GetCameraController()) {
    cam->UpdateGameplayCamera(scene.GetPlayer(), dt);
  }
}

std::unique_ptr<IGameSceneState> PlayingSceneState::Update(GameScene &scene) {
  auto *titleDir = scene.GetTitleDirector();
  auto *stage = scene.GetStageSettings();
  auto *player = scene.GetPlayer();
  auto *scoreMgr = scene.GetScoreManager();

  // --- リスタートカウントダウン演出中の進行制御 ---
  if (titleDir->IsCountdownActive() &&
      titleDir->GetCountdownMode() == TitleDirector::CountdownMode::Restart) {
    if (titleDir->IsInputAllowed()) {
      if (stage->GetScrollAcceleration() == 0.0f) {
        stage->SetScrollAcceleration(GameScene::kPlayScrollAcceleration);
        stage->ClearObstacles(GameScene::kStartObstacleClearDistance);
        stage->SetSpawningPaused(false);
        player->SetInputEnabled(true);
        SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
      }
    } else {
      stage->SetSpawningPaused(true);
      player->SetInputEnabled(false);
      player->ClearInputBuffer();
    }
  }

  // --- ボス戦状態の同期 ---
  GameSceneManager::GetInstance()->SetInBossBattle(scene.GetPlayingState() ==
                                                   PlayingState::Boss);

  // --- スクロール速度および加算用速度の算出 ---
  float timeScale = EditorManager::GetPlaySpeed();
  float speedMultiplier = 1.0f;
  if (stage->GetBaseScrollSpeed() > 0.0f) {
    speedMultiplier = stage->GetScrollSpeed() / stage->GetBaseScrollSpeed();
  }

  float scoreScrollSpeed = stage->GetScrollSpeed();
  if (titleDir->IsCountdownActive() && !titleDir->IsInputAllowed()) {
    scoreScrollSpeed = 0.0f;
  }

  // --- スコア・走行距離マネージャーの更新 ---
  scoreMgr->Update(scoreScrollSpeed, timeScale);

  // --- スライディング継続判定 ---
  scene.CheckKeepRolling();

  // --- プレイモード（3レーン / 1レーン / ボス戦）固有の更新 ---
  scene.UpdatePlayingState(timeScale, scoreScrollSpeed);

  // --- プレイヤーのレーン移動制限と操作反転の設定 ---
  player->SetLaneLimits(stage->GetMinLaneIndex(), stage->GetMaxLaneIndex(),
                        stage->GetLaneWidth());
  if ((stage->GetLaneCountAtZ(0.0f) == 1 ||
       stage->GetLaneCountAtZ(8.0f) == 1) &&
      (player->GetTargetLaneIndex() < 0 || player->GetTargetLaneIndex() > 0)) {
    player->StartForceToCenter(12.0f);
  }

  player->SetInvertedControls(scene.ShouldInvertControls());

  // --- ワールド内エンティティの更新と障害物との衝突判定 ---
  scene.UpdateWorldEntities(timeScale, speedMultiplier);
  scene.CheckCollisions();

  // CheckCollisions 内で被弾して既に別ステートへ遷移していればここで終了
  if (scene.GetGameState() != GameState::Playing) {
    return nullptr;
  }

  // --- 実績トラッキング更新 ---
  scene.UpdateAchievementTracking(timeScale);

  // --- ポーズ遷移入力 ---
  if (Input::PushKey(DIK_ESCAPE) ||
      GamePadInput::PushButton(XINPUT_GAMEPAD_START)) {
    return std::make_unique<PausedSceneState>();
  }

  return nullptr;
}

// ==========================================
// PausedSceneState
// ==========================================

void PausedSceneState::Enter(GameScene & /*scene*/) {
  isSettingsOpen_ = false;
  SoundManager::GetInstance()->PauseBGM();
}

void PausedSceneState::Exit(GameScene & /*scene*/, GameState nextState) {
  isSettingsOpen_ = false;
  if (nextState == GameState::Playing) {
    SoundManager::GetInstance()->ResumeBGM();
  }
}

float PausedSceneState::GetSunLightDeltaTime(float /*timeScale*/) const {
  return 0.0f;
}

std::unique_ptr<IGameSceneState> PausedSceneState::Update(GameScene &scene) {
  if (isSettingsOpen_) {
    scene.UpdateSettingsInput();
    return nullptr;
  }

  // --- ポーズ中のリスタート / タイトル復帰 / 音量設定の入力受付 ---
  if (!scene.IsResultTransitioning() &&
      !FreezeTransition::GetInstance()->IsActive()) {
    if (Input::PushKey(DIK_1) || GamePadInput::PushButton(XINPUT_GAMEPAD_A)) {
      scene.StartResultTransition(GameScene::ResultTransition::Restart);
    }
    if (Input::PushKey(DIK_2) || GamePadInput::PushButton(XINPUT_GAMEPAD_B)) {
      scene.StartResultTransition(GameScene::ResultTransition::Title);
    }
    if (Input::PushKey(DIK_3) || GamePadInput::PushButton(XINPUT_GAMEPAD_X)) {
      isSettingsOpen_ = true;
      scene.SetSettingsMenuIndex(0);
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);
    }
  }

  if (auto *effectMgr = scene.GetEffectManager()) {
    effectMgr->EditorUpdate(scene.GetCameraController()->GetViewMatrix());
  }

  if (Input::PushKey(DIK_ESCAPE) ||
      GamePadInput::PushButton(XINPUT_GAMEPAD_START)) {
    return std::make_unique<PlayingSceneState>();
  }

  return nullptr;
}

// ==========================================
// PlayerHitSceneState
// ==========================================

std::unique_ptr<IGameSceneState> PlayerHitSceneState::Update(GameScene &scene) {
  auto *crash = scene.GetCrashEffect();
  auto *effectMgr = scene.GetEffectManager();
  auto *cam = scene.GetCameraController();
  auto *player = scene.GetPlayer();
  auto *stage = scene.GetStageSettings();
  auto *scoreMgr = scene.GetScoreManager();

  // --- 被弾演出（クラッシュ・エフェクト・被弾アニメーション）の更新 ---
  crash->Update(Engine::kDefaultDeltaTime);
  effectMgr->PlayerHitUpdate(cam->GetViewMatrix());
  if (player->GetIsTrip() && effectMgr) {
    effectMgr->EmitDust(player->GetTransform().translate);
  }
  player->Update(cam->GetViewMatrix(), 1.0f);

  // === 雪玉状態時の障害物吹き飛ばし判定＆被弾中障害物の物理・描画更新 ===
  if (stage) {
    const Matrix4x4 &view = cam->GetViewMatrix();
    bool isSnowballRolling = player->GetIsTrip() &&
                             player->GetIsSnowballActive() &&
                             (player->GetSnowballScale() > 0.1f);

    BoundingSphere snowballSphere{};
    std::shared_ptr<SphereColliderComponent> snowballCollider;
    if (isSnowballRolling) {
      snowballSphere.center = player->GetSnowballTransform().translate;
      snowballSphere.radius =
          (std::max)(player->GetSnowballScale() * 1.15f, 1.2f);
      snowballCollider = player->GetSnowballCollider();
    }

    for (int i = 0; i < stage->GetMaxObstacles(); i++) {
      Obstacle *obs = stage->GetObstacle(i);
      if (!obs || !obs->GetIsActive()) {
        continue;
      }

      if (isSnowballRolling && !obs->GetIsHit()) {
        if (obs->GetType() == Obstacle::Type::GuideFloor) {
          continue;
        }

        auto obsCollider = obs->GetComponent<ColliderComponent>();
        bool hit = (snowballCollider && obsCollider)
                       ? snowballCollider->IntersectsWith(obsCollider.get())
                       : Collision::CheckAABBSphere(obs->GetColliderAABB(),
                                                    snowballSphere);
        if (hit) {
          if (obs->IsItem()) {
            obs->OnHit();
            SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);
            continue;
          }

          Vector3 obsPos = obs->GetTransform().translate;
          float diffX = obsPos.x - snowballSphere.center.x;
          float blowDirX = (std::abs(diffX) > 0.1f)
                               ? (diffX > 0.0f ? 0.35f : -0.35f)
                               : ((rand() % 2 == 0) ? 0.3f : -0.3f);
          float blowVelX =
              blowDirX + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.2f;
          float blowVelY =
              0.55f + (static_cast<float>(rand()) / RAND_MAX) * 0.25f;
          float blowVelZ =
              0.60f + (static_cast<float>(rand()) / RAND_MAX) * 0.30f;

          obs->OnBlowAway({blowVelX, blowVelY, blowVelZ});
          SoundManager::GetInstance()->PlaySE(SoundManager::SE::BonusHit);
          GamePadInput::Rumble(0.2f, 32000, 32000);

          if (effectMgr) {
            effectMgr->EmitDust(obsPos);
          }
          if (cam) {
            cam->ShakeImpact();
          }
        }
      }

      obs->StageUpdate(view, 0.0f);
    }
  }

  // --- 被弾アニメーション終了後のリザルト演出判定 ---
  if (player->IsHitAnimationFinished()) {
    bool isScoreFirst = scoreMgr->IsScoreFirstPlace();
    bool isDistFirst = scoreMgr->IsDistanceFirstPlace();

    if (scene.IsForceFirstPlaceCutInTest() || isScoreFirst || isDistFirst) {
      RankingCutinEffect::CutinType type = RankingCutinEffect::CutinType::Score;
      if (isScoreFirst && isDistFirst) {
        type = RankingCutinEffect::CutinType::Double;
      } else if (isDistFirst) {
        type = RankingCutinEffect::CutinType::Distance;
      } else if (isScoreFirst) {
        type = RankingCutinEffect::CutinType::Score;
      } else {
        int testType = scene.GetTestCutinType();
        if (testType == 1) {
          type = RankingCutinEffect::CutinType::Distance;
        } else if (testType == 2) {
          type = RankingCutinEffect::CutinType::Double;
        } else {
          type = RankingCutinEffect::CutinType::Score;
        }
      }

      return std::make_unique<FirstPlaceCutInSceneState>(type);
    }
    return std::make_unique<GameOverSceneState>();
  }

  return nullptr;
}

// ==========================================
// FirstPlaceCutInSceneState
// ==========================================

void FirstPlaceCutInSceneState::Enter(GameScene &scene) {
  if (autoTriggerOnEnter_) {
    if (auto *cutin = scene.GetRankingCutinEffect()) {
      cutin->Trigger(cutinType_);
    }
  }
}

std::unique_ptr<IGameSceneState>
FirstPlaceCutInSceneState::Update(GameScene &scene) {
  auto *effectMgr = scene.GetEffectManager();
  auto *cam = scene.GetCameraController();
  auto *cutin = scene.GetRankingCutinEffect();

  effectMgr->PlayerHitUpdate(cam->GetViewMatrix());
  cutin->Update(Engine::kDefaultDeltaTime);

  // スキップ入力受付
  if (Input::PushKey(DIK_SPACE) || Input::PushKey(DIK_RETURN) ||
      GamePadInput::PushButton(XINPUT_GAMEPAD_A) ||
      GamePadInput::PushButton(XINPUT_GAMEPAD_B)) {
    cutin->RequestSkip();
  }

  // 演出完了時のリザルト画面移行
  if (cutin->IsFinished()) {
    return std::make_unique<GameOverSceneState>();
  }

  return nullptr;
}

// ==========================================
// GameOverSceneState
// ==========================================

void GameOverSceneState::Enter(GameScene & /*scene*/) {
  resultTimer_ = 0.0f;
  resultAchievementScrollOffset_ = 0;
  resultScrollRepeatTimer_ = 0.0f;
}

std::unique_ptr<IGameSceneState> GameOverSceneState::Update(GameScene &scene) {
  resultTimer_ += Engine::kDefaultDeltaTime;
  scene.GetEffectManager()->PlayerHitUpdate(
      scene.GetCameraController()->GetViewMatrix());

  if (!scene.IsResultTransitioning() &&
      !FreezeTransition::GetInstance()->IsActive()) {
    // リザルトパネル落下中の誤入力を防止するため、一定時間経過後に入力を受付
    if (resultTimer_ >= 0.45f) {
      if (Input::PushKey(DIK_1) || GamePadInput::PushButton(XINPUT_GAMEPAD_A)) {
        scene.StartResultTransition(GameScene::ResultTransition::Restart);
      }
      if (Input::PushKey(DIK_2) || GamePadInput::PushButton(XINPUT_GAMEPAD_B)) {
        scene.StartResultTransition(GameScene::ResultTransition::Title);
      }

      // リザルト獲得実績リストのスクロール入力処理
      auto *achMgr = scene.GetAchievementManager();
      int unlockedCount = achMgr ? achMgr->GetUnlockedCount() : 0;
      int maxScroll =
          (std::max)(0, unlockedCount - GameHUD::kResultAchMaxVisibleItems);
      resultAchievementScrollOffset_ =
          std::clamp(resultAchievementScrollOffset_, 0, maxScroll);

      if (maxScroll > 0) {
        bool pushUp = Input::PushKey(DIK_UP) || Input::PushKey(DIK_W) ||
                      GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_UP) ||
                      GamePadInput::PushLeftStickUp() ||
                      GamePadInput::PushRightStickUp();
        bool pushDown = Input::PushKey(DIK_DOWN) || Input::PushKey(DIK_S) ||
                        GamePadInput::PushButton(XINPUT_GAMEPAD_DPAD_DOWN) ||
                        GamePadInput::PushLeftStickDown() ||
                        GamePadInput::PushRightStickDown();

        bool holdUp = Input::PressKey(DIK_UP) || Input::PressKey(DIK_W) ||
                      GamePadInput::PressButton(XINPUT_GAMEPAD_DPAD_UP) ||
                      (GamePadInput::GetLeftStickY() > 0.5f) ||
                      (GamePadInput::GetRightStickY() > 0.5f);
        bool holdDown = Input::PressKey(DIK_DOWN) || Input::PressKey(DIK_S) ||
                        GamePadInput::PressButton(XINPUT_GAMEPAD_DPAD_DOWN) ||
                        (GamePadInput::GetLeftStickY() < -0.5f) ||
                        (GamePadInput::GetRightStickY() < -0.5f);

        int scrollDelta = 0;
        int wheel = Input::GetMouseWheel();
        if (wheel > 0) {
          scrollDelta -= 1;
        } else if (wheel < 0) {
          scrollDelta += 1;
        }

        if (pushUp) {
          scrollDelta -= 1;
          resultScrollRepeatTimer_ = 0.28f;
        } else if (pushDown) {
          scrollDelta += 1;
          resultScrollRepeatTimer_ = 0.28f;
        } else if (holdUp || holdDown) {
          resultScrollRepeatTimer_ -= Engine::kDefaultDeltaTime;
          if (resultScrollRepeatTimer_ <= 0.0f) {
            scrollDelta += holdUp ? -1 : 1;
            resultScrollRepeatTimer_ = 0.07f;
          }
        } else {
          resultScrollRepeatTimer_ = 0.0f;
        }

        if (scrollDelta != 0) {
          int nextOffset = std::clamp(
              resultAchievementScrollOffset_ + scrollDelta, 0, maxScroll);
          if (nextOffset != resultAchievementScrollOffset_) {
            resultAchievementScrollOffset_ = nextOffset;
            SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
          }
        }
      }
    }
  }

  return nullptr;
}

// ==========================================
// EditorSceneState
// ==========================================

void EditorSceneState::Enter(GameScene &scene) {
  if (auto *cam = scene.GetCameraController()) {
    cam->SetDebugCamera(true);
  }
  SoundManager::GetInstance()->StopBGM();
  SoundManager::GetInstance()->StopAllSE();
}

float EditorSceneState::GetSunLightDeltaTime(float /*timeScale*/) const {
  return Engine::kDefaultDeltaTime;
}

void EditorSceneState::UpdateCamera(GameScene &scene, float dt) {
  if (auto *cam = scene.GetCameraController()) {
    cam->UpdateGameplayCamera(scene.GetPlayer(), dt);
  }
}

std::unique_ptr<IGameSceneState> EditorSceneState::Update(GameScene &scene) {
  const Matrix4x4 &view = scene.GetCameraController()->GetViewMatrix();
  scene.GetGameObjectManager()->UpdateAll(view, 0.0f);
  float activeWaterTime =
      scene.GetCameraController()->GetCamera()->GetTotalTime();
  scene.GetStageSettings()->EditorUpdate(view, activeWaterTime);
  scene.GetEffectManager()->EditorUpdate(view);
  return nullptr;
}

// ==========================================
// Factory
// ==========================================

std::unique_ptr<IGameSceneState> CreateGameSceneState(GameState state) {
  switch (state) {
  case GameState::Title:
    return std::make_unique<TitleSceneState>();
  case GameState::Playing:
    return std::make_unique<PlayingSceneState>();
  case GameState::Paused:
    return std::make_unique<PausedSceneState>();
  case GameState::PlayerHit:
    return std::make_unique<PlayerHitSceneState>();
  case GameState::FirstPlaceCutIn:
    return std::make_unique<FirstPlaceCutInSceneState>();
  case GameState::GameOver:
    return std::make_unique<GameOverSceneState>();
  case GameState::Editor:
    return std::make_unique<EditorSceneState>();
  default:
    return std::make_unique<TitleSceneState>();
  }
}

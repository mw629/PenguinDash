#include "GameScene.h"
#include "../../Editer/EditorManager.h"
#include "AssetManager.h"
#include "GameSceneManager.h"
#include "Graphics/Font/TextRenderer.h"
#include "Graphics/Render/Draw.h"
#include "System/SoundManager.h"
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
      ImGui::Text("Your Distance: %.2f m", currentDistance_);
      ImGui::Text("Your Score: %.0f", currentScore_);
      ImGui::Separator();

      ImGui::Text("--- DISTANCE TOP 3 RANKING ---");
      for (int i = 0; i < 3; i++) {
        if (topRankings_[i] > 0.0f) {
          ImGui::Text("  %d. %.2f m", i + 1, topRankings_[i]);
        } else {
          ImGui::Text("  %d. ---", i + 1);
        }
      }
      ImGui::Separator();

      ImGui::Text("--- SCORE TOP 3 RANKING ---");
      for (int i = 0; i < 3; i++) {
        if (topScoreRankings_[i] > 0.0f) {
          ImGui::Text("  %d. %.0f", i + 1, topScoreRankings_[i]);
        } else {
          ImGui::Text("  %d. ---", i + 1);
        }
      }
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
        FreezeTransition::GetInstance()->Start(0.75f, 0.75f, nullptr);
      }
      ImGui::Separator();
    }
  }

  camera_.get()->ImGui();

  if (ImGui::CollapsingHeader("GameScene Camera Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    bool cameraChanged = false;
    if (ImGui::DragFloat3("Translate", &cameraTransform_.translate.x, 0.1f))
      cameraChanged = true;
    if (ImGui::DragFloat3("Rotate", &cameraTransform_.rotate.x, 0.01f))
      cameraChanged = true;
    if (ImGui::DragFloat3("Scale", &cameraTransform_.scale.x, 0.01f))
      cameraChanged = true;

    if (cameraChanged) {
      camera_->SetTransform(cameraTransform_);
      gameCamera_->SetTransform(cameraTransform_);
    }

    ImGui::Separator();
    ImGui::Text("Presets:");
    if (ImGui::Button("Behind View")) {
      ChangePlayingState(PlayingState::ThreeLane);
    }
    ImGui::SameLine();
    if (ImGui::Button("Left Side View")) {
      cameraTransform_.scale = {1.0f, 1.0f, 1.0f};
      cameraTransform_.rotate = {0.3f, 1.0472f, 0.0f};
      cameraTransform_.translate = {-20.0f, 8.0f, -5.0f};
      camera_->SetTransform(cameraTransform_);
      gameCamera_->SetTransform(cameraTransform_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Right Side View")) {
      ChangePlayingState(PlayingState::OneLane);
    }
    ImGui::SameLine();
    if (ImGui::Button("Boss View")) {
      ChangePlayingState(PlayingState::Boss, true);
    }
    ImGui::SameLine();
    if (ImGui::Button(isFirstPersonView_ ? "Disable FPV (F2)" : "First Person View (F2)")) {
      isFirstPersonView_ = !isFirstPersonView_;
      if (!isFirstPersonView_) {
        camera_->SetTransform(cameraTransform_);
        gameCamera_->SetTransform(cameraTransform_);
        player_->SetVisible(true);
      }
    }

    ImGui::Separator();
    if (ImGui::Button("Reset Debug Camera to Game Camera")) {
      camera_->ResetDebugCamera(cameraTransform_);
    }
  }

  if (ImGui::CollapsingHeader("First Person Camera Settings (F2)",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Checkbox("Enable First Person View (F2)", &isFirstPersonView_)) {
      if (!isFirstPersonView_) {
        camera_->SetTransform(cameraTransform_);
        gameCamera_->SetTransform(cameraTransform_);
        player_->SetVisible(true);
      }
    }
    ImGui::Checkbox("Draw Player Body in First Person", &drawPlayerInFirstPerson_);
    ImGui::DragFloat3("FPV Camera Offset", &firstPersonOffset_.x, 0.02f);
    ImGui::DragFloat3("FPV Camera Rotate", &firstPersonRotate_.x, 0.01f);
  }

  if (ImGui::CollapsingHeader("Title Orbit Camera Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Enable Orbit Camera", &isTitleOrbitCamera_);
    ImGui::SliderFloat("Orbit Speed", &titleCameraSpeed_, -2.0f, 2.0f, "%.2f rad/s");
    ImGui::DragFloat("Orbit Radius", &titleCameraRadius_, 0.2f, 3.0f, 40.0f);
    ImGui::DragFloat("Orbit Height", &titleCameraHeight_, 0.2f, -10.0f, 20.0f);
    ImGui::DragFloat("Target Offset Y", &titleTargetOffsetY_, 0.1f, -5.0f, 10.0f);
    ImGui::SliderAngle("Current Angle", &titleCameraAngle_);
  }

  if (ImGui::CollapsingHeader("Sound Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
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
      if (ImGui::SliderFloat("Roll Duration", &rollDuration, 10.0f, 60.0f,
                             "%.0f f")) {
        player_->SetRollDuration(rollDuration);
      }

      float estAirFrames =
          gravity > 0.0f ? ((2.0f * jumpPower / gravity) + 1.0f) : 0.0f;
      float estMaxHeight =
          gravity > 0.0f ? ((jumpPower * jumpPower) / (2.0f * gravity)) : 0.0f;
      ImGui::Text("Jump Air Time: %.0f frames (%.2f s)", estAirFrames,
                  estAirFrames / 60.0f);
      ImGui::Text("Max Jump Height: +%.2f m", estMaxHeight);
      ImGui::Text("Lane Move Time: %.0f frames",
                  laneSpeed > 0.0f ? (1.0f / laneSpeed) : 0.0f);
      ImGui::Text("Roll Duration: %.0f frames (%.2f s)", rollDuration,
                  rollDuration / 60.0f);

      ImGui::TreePop();
    }

    // ボス情報
    if (playingState_ == PlayingState::Boss && boss_->GetIsActive()) {
      if (ImGui::TreeNode("Boss Info")) {
        ImGui::Text("HP: %d / %d", boss_->GetHP(), boss_->GetMaxHP());
        ImGui::DragFloat3("Target Pos", &boss_->GetTargetPosRef().x, 0.1f);
        ImGui::DragFloat("Attack Spawn Z", &bossAttackSpawnZ_, 0.5f, -100.0f,
                         0.0f, "%.1f m");
        ImGui::DragFloat("Attack Drop Height", &bossAttackDropHeight_, 0.5f,
                         5.0f, 40.0f, "%.1f m");
        ImGui::DragFloat("Attack Fall Frames", &bossAttackFallDuration_, 1.0f,
                         5.0f, 60.0f, "%.0f frames");
        ImGui::DragFloat("Reflect Min Z", &bossAttackReflectMinZ_, 0.5f, -40.0f,
                         0.0f, "%.1f m");
        ImGui::DragFloat("Reflect Arc Height", &bossReflectArcHeight_, 0.2f,
                         1.0f, 25.0f, "%.1f m");
        ImGui::DragFloat("Reflect Duration", &bossReflectDuration_, 1.0f,
                         10.0f, 120.0f, "%.0f frames");
        ImGui::Text("Attack Distance to Player: %.1f m", -bossAttackSpawnZ_);
        ImGui::Separator();
        ImGui::Text("Boss Camera Settings:");
        if (ImGui::DragFloat3("Boss Cam Pos", &bossCameraTranslate_.x, 0.1f)) {
          if (playingState_ == PlayingState::Boss && !isCameraTransitioning_) {
            cameraTransform_.translate = bossCameraTranslate_;
            camera_->SetTransform(cameraTransform_);
            gameCamera_->SetTransform(cameraTransform_);
          }
        }
        if (ImGui::DragFloat3("Boss Cam Rot", &bossCameraRotate_.x, 0.01f)) {
          if (playingState_ == PlayingState::Boss && !isCameraTransitioning_) {
            cameraTransform_.rotate = bossCameraRotate_;
            camera_->SetTransform(cameraTransform_);
            gameCamera_->SetTransform(cameraTransform_);
          }
        }
        ImGui::TreePop();
      }
    }

    // ステージ情報
    if (ImGui::TreeNode("Stage Info")) {
      ImGui::Text("Scroll Speed: %.3f", stageSettings_->GetScrollSpeed());
      ImGui::Text("Base Scroll Speed: %.3f",
                  stageSettings_->GetBaseScrollSpeed());
      ImGui::Text("Max Scroll Speed: %.3f",
                  stageSettings_->GetMaxScrollSpeed());
      ImGui::Text("Lane Index: Min=%d, Max=%d",
                  stageSettings_->GetMinLaneIndex(),
                  stageSettings_->GetMaxLaneIndex());
      ImGui::Text("Lane Width: %.2f (Effective: %.2f)",
                  stageSettings_->GetLaneWidth(),
                  stageSettings_->GetEffectiveLaneWidth());
      ImGui::Text("Chunks: %d (Behind: %d, Ahead: %d)",
                  stageSettings_->GetChunkCount(),
                  stageSettings_->GetBackwardChunks(),
                  stageSettings_->GetForwardChunks());
      ImGui::TreePop();
    }

    ImGui::Separator();

    // スコアとランキング
    ImGui::Text("Current Distance: %.2f m", currentDistance_);
    if (ImGui::TreeNode("Distance Top 3 Ranking")) {
      for (int i = 0; i < 3; i++) {
        if (topRankings_[i] > 0.0f) {
          ImGui::Text("Rank %d: %.2f m", i + 1, topRankings_[i]);
        } else {
          ImGui::Text("Rank %d: ---", i + 1);
        }
      }
      ImGui::TreePop();
    }

    ImGui::Text("Current Score: %.0f", currentScore_);
    if (ImGui::TreeNode("Score Top 3 Ranking")) {
      for (int i = 0; i < 3; i++) {
        if (topScoreRankings_[i] > 0.0f) {
          ImGui::Text("Rank %d: %.0f", i + 1, topScoreRankings_[i]);
        } else {
          ImGui::Text("Rank %d: ---", i + 1);
        }
      }
      ImGui::TreePop();
    }

    ImGui::Separator();

    if (ImGui::Button("Reset Game", ImVec2(120, 0))) {
      ResetGame();
      gameState_ = GameState::Playing;
      isTitleExiting_ = false;
      camera_->SetDebugCamera(false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Go to Title", ImVec2(120, 0))) {
      ReturnToTitle();
      camera_->SetDebugCamera(false);
    }
  }

  // フォント / 文字の太さ設定パネル
  if (draw_ && ImGui::CollapsingHeader("Font / Text Settings")) {
    float boldness = draw_->GetTextBaseBoldness();
    if (ImGui::SliderFloat("Global Boldness", &boldness, -0.05f, 0.20f,
                           "%.3f")) {
      draw_->SetTextBaseBoldness(boldness);
    }
    ImGui::TextDisabled("Default: 0.070 (Thick / Bold)");
  }

  // ステージ設定のデバッグパネル
  if (ImGui::CollapsingHeader("Stage Settings Debug")) {
    int currentLane = stageSettings_->GetLaneCount();
    ImGui::Text("Current (Player Foot) Lane: %d", currentLane);

    int targetLane = stageSettings_->GetTargetLaneCount();
    if (ImGui::SliderInt("Target Lane Count", &targetLane, 1, 11)) {
      // Ensure it's preferably an odd number, or just pass it to the setter
      stageSettings_->SetLaneCount(targetLane);
    }

    float laneWidth = stageSettings_->GetLaneWidth();
    if (ImGui::SliderFloat("Lane Width", &laneWidth, 1.0f, 10.0f)) {
      stageSettings_->SetLaneWidth(laneWidth);
    }

    float oneLaneMult = stageSettings_->GetOneLaneWidthMultiplier();
    if (ImGui::SliderFloat("One Lane Width Multiplier", &oneLaneMult, 1.0f,
                           3.0f, "%.2f")) {
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

    float minDistance =
        stageSettings_
            ->GetMinObstacleDistance(); // 確実に避けられる最小間隔距離
    float maxDistance =
        stageSettings_->GetMaxObstacleDistance(); // 避けられる最大間隔距離
    if (ImGui::SliderFloat("Min Obstacle Distance", &minDistance, 3.0f, 30.0f,
                           "%.1f m")) {
      if (minDistance > maxDistance)
        maxDistance = minDistance;
      stageSettings_->SetMinObstacleDistance(minDistance);
      stageSettings_->SetMaxObstacleDistance(maxDistance);
    }
    if (ImGui::SliderFloat("Max Obstacle Distance", &maxDistance, 5.0f, 60.0f,
                           "%.1f m")) {
      if (maxDistance < minDistance)
        minDistance = maxDistance;
      stageSettings_->SetMinObstacleDistance(minDistance);
      stageSettings_->SetMaxObstacleDistance(maxDistance);
    }

    float baseActionFrames =
        stageSettings_->GetBaseActionFrames(); // 回避アクション所要フレーム数
    if (ImGui::SliderFloat("Base Action Frames", &baseActionFrames, 0.0f, 60.0f,
                           "%.0f f")) {
      stageSettings_->SetBaseActionFrames(baseActionFrames);
    }

    float minGrace = stageSettings_->GetMinGraceFrames(); // 最小猶予フレーム数
    float maxGrace = stageSettings_->GetMaxGraceFrames(); // 最大猶予フレーム数
    if (ImGui::SliderFloat("Min Grace Frames", &minGrace, 5.0f, 60.0f,
                           "%.0f f")) {
      if (minGrace > maxGrace)
        maxGrace = minGrace;
      stageSettings_->SetMinGraceFrames(minGrace);
      stageSettings_->SetMaxGraceFrames(maxGrace);
    }
    if (ImGui::SliderFloat("Max Grace Frames", &maxGrace, 5.0f, 80.0f,
                           "%.0f f")) {
      if (maxGrace < minGrace)
        minGrace = maxGrace;
      stageSettings_->SetMinGraceFrames(minGrace);
      stageSettings_->SetMaxGraceFrames(maxGrace);
    }

    float noSpawnChance =
        stageSettings_->GetNoSpawnChance() * 100.0f; // 障害物が出ない確率（%）
    if (ImGui::SliderFloat("No Spawn Chance (%)", &noSpawnChance, 0.0f, 30.0f,
                           "%.1f %%")) {
      stageSettings_->SetNoSpawnChance(noSpawnChance / 100.0f);
    }

    float currentSpeed =
        stageSettings_->GetScrollSpeed(); // 現在のスクロール速度
    float curInterval =
        stageSettings_->GetObstacleInterval(); // 現在の次回生成間隔
    float estFrames = currentSpeed > 0.0f ? (curInterval / currentSpeed)
                                          : 0.0f; // 到達までの推定フレーム数
    ImGui::Text("Next Spawn: %.1f m (approx. %.0f frames / %.2f s)",
                curInterval, estFrames, estFrames / 60.0f);
    ImGui::Text("Dodgeable Range: %.1f m ~ %.1f m", minDistance, maxDistance);

    Vector4 roadColor = stageSettings_->GetRoadColor();
    float roadColorArr[4] = {roadColor.x, roadColor.y, roadColor.z, roadColor.w};
    if (ImGui::ColorEdit4("Ground Color", roadColorArr)) {
      stageSettings_->SetRoadColor(
          {roadColorArr[0], roadColorArr[1], roadColorArr[2], roadColorArr[3]});
    }
  }

  // アイテムクールタイム設定パネル
  if (ImGui::CollapsingHeader("Item CoolDowns (アイテムクールタイム設定)",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    float spawnChance = stageSettings_->GetItemSpawnChance() * 100.0f;
    if (ImGui::SliderFloat("Item Spawn Chance (%)", &spawnChance, 0.0f, 100.0f,
                           "%.1f %%")) {
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
        {Obstacle::Type::ClearItem, "Clear (障害物全消去)",
         "画面内の障害物を一掃"},
        {Obstacle::Type::CameraItem, "Camera (視点切替)",
         "1レーンモードへ移行"},
        {Obstacle::Type::BossItem, "Boss (ボス戦突入)", "ボスバトル開始"},
    };

    for (const auto &item : items) {
      ImGui::PushID(static_cast<int>(item.type));
      float duration = stageSettings_->GetItemCoolDownDuration(item.type);
      float current = stageSettings_->GetItemCoolDownTimer(item.type);

      ImGui::Text("%s - %s", item.name, item.desc);
      if (ImGui::SliderFloat("CoolTime (s)", &duration, 0.0f, 120.0f,
                             "%.1f s")) {
        stageSettings_->SetItemCoolDownDuration(item.type, duration);
      }

      if (current <= 0.0f) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                           "  Status: READY (出現可能)");
      } else {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                           "  Status: COOLDOWN (残り %.1f s)", current);
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

  // アイテム効果のデバッグパネル
  if (ImGui::CollapsingHeader("Item Debug / Effects",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    bool hasBarrier = player_->GetHasBarrier();
    ImGui::Text("Player Barrier Status: %s",
                hasBarrier ? "ACTIVE (ON)" : "INACTIVE (OFF)");

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
        if (obs && obs->GetIsActive() &&
            (obs->GetType() == Obstacle::Type::Low ||
             obs->GetType() == Obstacle::Type::High ||
             obs->GetType() == Obstacle::Type::Wall)) {
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
        if (obs->GetIsActive() && (obs->GetType() == Obstacle::Type::Low ||
                                   obs->GetType() == Obstacle::Type::High ||
                                   obs->GetType() == Obstacle::Type::Wall)) {
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
    if (ImGui::Button("Spawn Shield (バリア)")) {
      spawnItemAhead(Obstacle::Type::BarrierItem);
    }
    ImGui::SameLine();
    if (ImGui::Button("Spawn Boss (王冠)")) {
      spawnItemAhead(Obstacle::Type::BossItem);
    }
    ImGui::SameLine();
    if (ImGui::Button("Spawn Bomb (ボム)")) {
      spawnItemAhead(Obstacle::Type::ClearItem);
    }
    ImGui::SameLine();
    if (ImGui::Button("Spawn OneLane (1)")) {
      spawnItemAhead(Obstacle::Type::CameraItem);
    }

    ImGui::Separator();
    ImGui::Text("Effect Triggers:");
    if (ImGui::Button("Emit Dust")) {
      effectManager_->EmitDust(player_->GetTransform().translate);
    }
    ImGui::SameLine();
    if (ImGui::Button("Trigger Crash Effect")) {
      gameState_ = GameState::PlayerHit;
      crashTimer_ = 0.0f;
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Crash);
      player_->OnHit(false);
      if (crashSprite_) {
        Transform ct = crashSpriteData_.transform;
        ct.scale = {0.0f, 0.0f, 1.0f};
        ct.translate = {crashPosition_.x, crashPosition_.y, 0.0f};
        crashSprite_->SetTransform(ct);
        crashSprite_->SettingWvp();
      }
    }
    if (ImGui::TreeNode("Crash Effect Settings")) {
      ImGui::DragFloat2("Position", &crashPosition_.x, 1.0f, 0.0f, 1280.0f);
      ImGui::DragFloat("Scale Duration", &crashScaleDuration_, 0.02f, 0.05f, 2.0f);
      if (ImGui::DragFloat2("Base Size", &crashBaseSize_.x, 1.0f, 50.0f, 1280.0f)) {
        if (crashSprite_) {
          crashSprite_->SetSize(crashBaseSize_);
          crashSprite_->UpdateVertexBuffer();
        }
      }
      ImGui::TreePop();
    }
  }

  effectManager_->ImGui();

  ImGui::End();

  Matrix4x4 projection = MakePerspectiveFovMatrix(
      0.45f, float(1280.0f) / float(720.0f), 0.1f, 10000.0f);
  editorUI_->Draw(gameObjectManager_.get(), view, projection);

  // Stopモードの時だけギズモ描画コールバックをSceneウィンドウに登録する
#ifdef _USE_IMGUI
  if (!EditorManager::IsPlaying()) {
    EditorManager::SetSceneOverlayCallback([this]() {
      Matrix4x4 proj = MakePerspectiveFovMatrix(
          0.45f, float(1280.0f) / float(720.0f), 0.1f, 10000.0f);
      editorUI_->DrawGizmoInScene(view, proj);
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

#endif // _USE_IMGUI
}

void GameScene::ResetGame() {
  stageSettings_->Reset();
  player_->Reset();
  boss_->Reset();
  bossAttackTimer_ = 0.0f;
  effectManager_->ClearBarrier();
  currentDistance_ = 0.0f;
  currentScore_ = 0.0f;
  bonusEnemyHitCount_ = 0;
  wasBossBattle_ = false;

  // カメラと遷移状態の即座初期化
  isCameraTransitionPending_ = false;
  isCameraTransitioning_ = false;
  cameraTransitionTimer_ = 0.0f;

  cameraTransform_.scale = {1.0f, 1.0f, 1.0f};
  cameraTransform_.rotate = {0.3f, 0.0f, 0.0f};
  cameraTransform_.translate = {0.0f, 8.0f, -15.0f};
  camera_->SetTransform(cameraTransform_);
  gameCamera_->SetTransform(cameraTransform_);

  playingState_ = PlayingState::ThreeLane;
  isRightSideMode_ = false;
  rightSideDistance_ = 0.0f;
  stageSettings_->SetLaneCountImmediate(3);
  stageSettings_->SetSpawningPaused(false);
  player_->SetInvertedControls(false);
  GameSceneManager::GetInstance()->SetInBossBattle(false);

  crashTimer_ = 0.0f;
  if (crashSprite_) {
    Transform ct = crashSpriteData_.transform;
    ct.scale = {0.0f, 0.0f, 1.0f};
    ct.translate = {crashPosition_.x, crashPosition_.y, 0.0f};
    crashSprite_->SetTransform(ct);
    crashSprite_->SettingWvp();
  }
}

void GameScene::StartGame() {
  if (isTitleExiting_) return;

  // 1. 開始演出フェーズへ移行（まだ GameState::Title のまま、文字上昇開始）
  isTitleExiting_ = true;
  titleExitTimer_ = 0.0f;
  titleExitStartCamTransform_ = cameraTransform_;

  // 2. 障害物の新規生成をストップ（今出ている障害物は消さずにそのまま流れる）
  stageSettings_->SetSpawningPaused(true);

  // 3. スタート決定音を再生
  SoundManager::GetInstance()->PlaySE(SoundManager::SE::Start);
}

void GameScene::StartPlaying() {
  // 障害物が完全になくなったタイミングで呼ばれる本編ゲームスタート！
  gameState_ = GameState::Playing;
  isTitleExiting_ = false;
  titleExitTimer_ = 0.0f;

  // 本編ゲーム用の通常カメラをセット
  cameraTransform_.scale = {1.0f, 1.0f, 1.0f};
  cameraTransform_.rotate = {0.3f, 0.0f, 0.0f};
  cameraTransform_.translate = {0.0f, 8.0f, -15.0f};
  camera_->SetTransform(cameraTransform_);
  gameCamera_->SetTransform(cameraTransform_);

  // 1. オートパイロット解除（プレイヤー手動操作へ移行）
  player_->SetAutoPilot(false);
  player_->SetInvertedControls(false);

  // 2. 通常プレイ用の障害物スポーン再開＆設定（助走区間12m）
  stageSettings_->ClearObstacles(12.0f);
  stageSettings_->SetSpawningPaused(false);
  stageSettings_->SetItemSpawnChance(0.15f);
  stageSettings_->SetBaseScrollSpeed(0.2f);
  stageSettings_->SetScrollAcceleration(0.0001f);

  // 3. スコア・距離などのプレイデータを0から開始
  currentDistance_ = 0.0f;
  currentScore_ = 0.0f;
  bonusEnemyHitCount_ = 0;
  wasBossBattle_ = false;
  bossAttackTimer_ = 0.0f;
  boss_->Reset();

  // 4. エフェクトのクリア
  effectManager_->ClearBarrier();

  // 5. プレイ用BGM開始
  SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
}

void GameScene::StartResultTransition(ResultTransition target) {
  if (FreezeTransition::GetInstance()->IsActive() || resultTransition_ != ResultTransition::None) {
    return;
  }
  resultTransition_ = target;
  FreezeTransition::GetInstance()->Start(0.75f, 0.75f, [this, target]() {
    if (target == ResultTransition::Restart) {
      ResetGame();
      gameState_ = GameState::Playing;
      isTitleExiting_ = false;
      SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
    } else if (target == ResultTransition::Title) {
      ReturnToTitle();
    }
    resultTransition_ = ResultTransition::None;
  });
}

void GameScene::ReturnToTitle() {
  ResetGame();
  player_->SetAutoPilot(true);
  stageSettings_->SetItemSpawnChance(0.0f);
  stageSettings_->SetBaseScrollSpeed(0.22f);
  stageSettings_->SetScrollAcceleration(0.0f);
  stageSettings_->SetSpawningPaused(false);
  gameState_ = GameState::Title;
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
  SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Title);
}

void GameScene::Initialize() {
  sceneID_ = SceneID::Game;
  fade_->Initialize();
  resultTransition_ = ResultTransition::None;

  // タイトル用ペンギン走りスプライトの生成 (15体 画面上から下の＞の字フォーメーション用)
  titlePenguinTextureHandle_ = texture_->CreateTexture("Resources/Texture/penguin_run.png");
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

  // クラッシュ演出用スプライトの生成
  crashTextureHandle_ = texture_->CreateTexture("Resources/Texture/Crash.png");
  crashSpriteData_.transform.scale = {0.0f, 0.0f, 1.0f};
  crashSpriteData_.transform.translate = {crashPosition_.x, crashPosition_.y, 0.0f};
  crashSpriteData_.transform.rotate = {0.0f, 0.0f, 0.0f};
  crashSpriteData_.size = crashBaseSize_;
  crashSpriteData_.pivot = {0.5f, 0.5f};
  crashSpriteData_.textureArea[0] = {0.0f, 0.0f};
  crashSpriteData_.textureArea[1] = {1.0f, 1.0f};
  crashSpriteData_.scaleMode = SpriteScaleMode::Fit;
  crashSpriteData_.anchor = SpriteAnchor::None;
  crashSprite_ = std::make_unique<Sprite>();
  crashSprite_->Initialize(crashSpriteData_, crashTextureHandle_);
  crashSprite_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
  crashSprite_->SettingWvp();

  if (!EditorManager::IsPlaying()) {
    gameState_ = GameState::Editor;
    camera_->SetDebugCamera(true);
    SoundManager::GetInstance()->StopBGM();
  } else {
    gameState_ = GameState::Title;
    camera_->SetDebugCamera(false);
  }
  sceneChangeRequest_ = false;

  // パーティクルマネージャーの初期化
  effectManager_->Initialize();

  // コリジョンマネージャーの初期化
  collisionManager_ = std::make_unique<CollisionManager>();

  // camera_->SetDebugCamera() は上記で設定済み
  camera_->SetTransform(cameraTransform_);
  camera_->Update();

  gameCamera_->SetDebugCamera(
      false); // Game Cameraは常にDebug操作を受け付けない
  gameCamera_->SetTransform(cameraTransform_);
  gameCamera_->Update();

  // Game
  // View描画コールバックの登録（3D描画とUI描画を分離してポストエフェクトが正しく適用されるようにする）
  EditorManager::SetGameViewDrawCallback(
      [this](class Draw &draw) {
        Matrix4x4 gameViewMat = gameCamera_->GetViewMatrix();

        // 一時的にSkyBoxをGameCameraの位置へ移動
        Transform originalSkyBoxT = skyBox_->GetTransform();
        Transform gameSkyBoxT = originalSkyBoxT;
        gameSkyBoxT.translate = gameCamera_->GetTransform().translate;
        skyBox_->SetTransform(gameSkyBoxT);

        // ゲームカメラのView行列でオブジェクトのWVPを更新 (speedMultiplier=0.0f
        // でアニメーションは進めない)
        ObjectBase::SetWvpIndex(1);
        EffectDefinition::SetWvpIndex(1);

        gameObjectManager_->UpdateAll(gameViewMat, 0.0f);
        stageSettings_->EditorUpdate(gameViewMat);
        effectManager_->EditorUpdate(gameViewMat);

        draw.SetCamera(gameCamera_.get());
        draw.SetEnvironmentTexture(skyBoxTexture_);
        gameObjectManager_->DrawAll(draw);
        stageSettings_->Draw(draw);
        effectManager_->Draw(draw);

        // SkyBoxの位置を元に戻す
        skyBox_->SetTransform(originalSkyBoxT);

        ObjectBase::SetWvpIndex(0);
        EffectDefinition::SetWvpIndex(0);
      },
      [this](class Draw &draw) {
        // Game View 内でも ポストエフェクト後に HUD (動的MSDFテキスト) を描画
        DrawHUD(draw);
      });

  // スカイボックスの初期化
  skyBoxTexture_ = texture_.get()->CreateTexture("Resources/DDS/SnowWorld.dds");
  skyBox_.get()->Initialize(skyBoxTexture_);
  skyBox_.get()->SetShader("SkyBoxShader");
  skyBox_.get()->SetCullMode(kCullModeFront);
  skyBox_.get()->SetFrustumCullingEnabled(false);
  skyBox_.get()->SetLighting(false);
  skyBox_.get()->SetTransform(skyBoxTransform_);
  skyBox_.get()->name_ = "SkyBox";

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

  // エディターでの保存・読み込み先をJsonSceneに設定
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
            model->name_ = fileName;

            auto renderObj = std::make_shared<RenderObject>(model);
            renderObj->SetName(fileName);
            gameObjectManager_->AddObject(renderObj);
          } catch (const std::exception &e) {
            // Error handling if loading fails
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

  // 指定したJsonファイルを初期シーンとして読み込む
  gameObjectManager_->LoadScene(initialSceneJson_);

  // ゲーム状態・カメラ・ステージの初期化（カメラ遷移アニメーションを起こさず即座に初期状態にする）
  if (gameState_ == GameState::Title) {
    ReturnToTitle();
  } else {
    ResetGame();
  }

  // サウンドマネージャー初期化＆タイトルBGM再生
  SoundManager::GetInstance()->Initialize();
  if (gameState_ == GameState::Title) {
    SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Title);
  }
}

void GameScene::Update() {
  ObjectBase::SetWvpIndex(0);
  EffectDefinition::SetWvpIndex(0);
  uiTimer_ += 1.0f / 60.0f;
  SoundManager::GetInstance()->Update();

  // フェードの更新
  if (fade_) {
    fade_->Update(1.0f / 60.0f);

    // リザルトからのフェードアウト完了時の遷移処理
    if (resultTransition_ != ResultTransition::None && fade_->IsFadeOutFinished()) {
      if (resultTransition_ == ResultTransition::Restart) {
        ResetGame();
        gameState_ = GameState::Playing;
        isTitleExiting_ = false;
        SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
        fade_->StartFadeIn(fadeDuration_);
      } else if (resultTransition_ == ResultTransition::Title) {
        ReturnToTitle();
        fade_->StartFadeIn(fadeDuration_);
      }
      resultTransition_ = ResultTransition::None;
    }
  }

  // タイトル文字の退出（上へ流れる）アニメーション更新
  if (isTitleExiting_) {
    titleExitTimer_ += 1.0f / 60.0f;
  }

#ifdef _DEBUG
  if (Input::PushKey(DIK_Q)) {
    ChangePlayingState(PlayingState::ThreeLane);
  }
  if (Input::PushKey(DIK_E)) {
    ChangePlayingState(PlayingState::OneLane);
  }
  if (Input::PushKey(DIK_F1)) {
    ColliderComponent::s_isDrawDebug_ = !ColliderComponent::s_isDrawDebug_;
  }
#endif // _DEBUG

  // F2キー または コントローラーBACKボタン/右スティック押し込みでPlayerの一人称固定モードの切り替え
  if (Input::PushKey(DIK_F2) || GamePadInput::PushButton(XINPUT_GAMEPAD_BACK) || GamePadInput::PushButton(XINPUT_GAMEPAD_RIGHT_THUMB)) {
    isFirstPersonView_ = !isFirstPersonView_;
    if (!isFirstPersonView_) {
      camera_->SetTransform(cameraTransform_);
      gameCamera_->SetTransform(cameraTransform_);
      player_->SetVisible(true);
    }
  }

  // F3キーでいつでも氷結・破砕トランジションをテスト発動
  if (Input::PushKey(DIK_F3)) {
    if (!FreezeTransition::GetInstance()->IsActive()) {
      FreezeTransition::GetInstance()->Start(0.75f, 0.75f, nullptr);
    }
  }

  // PostEffect::SetActivePostEffect(PostEffect::Type::GaussianFilter);

  // Engine側のPlay/Stop状態に同期してゲームステートを切り替え
  bool isEnginePlaying = EditorManager::IsPlaying();
  if (isEnginePlaying && gameState_ == GameState::Editor) {
    ReturnToTitle();
    camera_->SetDebugCamera(false);
  } else if (!isEnginePlaying && gameState_ != GameState::Editor) {
    gameState_ = GameState::Editor;
    camera_->SetDebugCamera(true);
    SoundManager::GetInstance()->StopBGM();
    SoundManager::GetInstance()->StopAllSE();
  }

  UpdateCameraTransition();

  if (isFirstPersonView_) {
    UpdateFirstPersonCamera();
  } else if (gameState_ == GameState::Title && isTitleOrbitCamera_) {
    UpdateTitleCamera();
  }

  camera_->Update();
  view = camera_->GetViewMatrix();

  gameCamera_->Update();

  // SkyBoxをカメラの位置に追従させる（無限遠の背景として機能させるため）
  Transform skyboxTransform = skyBox_->GetTransform();
  skyboxTransform.translate = camera_->GetTransform().translate;
  skyBox_->SetTransform(skyboxTransform);

  // skyBox_ is now updated in gameObjectManager_

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
    if (Input::PushKey(DIK_ESCAPE) || GamePadInput::PushButton(XINPUT_GAMEPAD_START)) {
      gameState_ = GameState::Playing;
      SoundManager::GetInstance()->ResumeBGM();
    }
  } else if (gameState_ == GameState::PlayerHit) {
    PlayerHitUpdate();
  } else if (gameState_ == GameState::GameOver) {
    if (resultTransition_ == ResultTransition::None && !fade_->IsFading() && !FreezeTransition::GetInstance()->IsActive()) {
      // 1キー または ゲームパッドAボタンでリスタート
      if (Input::PushKey(DIK_1) || GamePadInput::PushButton(XINPUT_GAMEPAD_A)) {
        StartResultTransition(ResultTransition::Restart);
      }
      // 2キー または ゲームパッドBボタンでタイトルへ
      if (Input::PushKey(DIK_2) || GamePadInput::PushButton(XINPUT_GAMEPAD_B)) {
        StartResultTransition(ResultTransition::Title);
      }
    }
  } else if (gameState_ == GameState::Editor) {
    EditorUpdate();
  }

  // 一人称視点時はプレイヤーの移動後にもカメラ位置を最新状態に同期
  if (isFirstPersonView_) {
    UpdateFirstPersonCamera();
    camera_->Update();
    view = camera_->GetViewMatrix();
    gameCamera_->Update();

    skyboxTransform = skyBox_->GetTransform();
    skyboxTransform.translate = camera_->GetTransform().translate;
    skyBox_->SetTransform(skyboxTransform);
  }
}

void GameScene::Draw(class Draw &draw) {
  draw_ = &draw;
  // カメラの設定
  draw.SetCamera(camera_.get());
  // 背景の設定
  draw.SetEnvironmentTexture(skyBoxTexture_);

  // オブジェクトの一括描画（SkyBox, Player など）
  gameObjectManager_->DrawAll(draw);

  // ステージ描画（道路 + 障害物）
  stageSettings_->Draw(draw);

  // ポーズ中の描画
  if (gameState_ == GameState::Paused) {
    pauseSystem_->Draw(draw);
  }

  // ヒットエフェクトの描画
  effectManager_->Draw(draw);
}

void GameScene::DrawUI(class Draw &draw) {
  draw_ = &draw;
  // HUDの描画（ポストエフェクト後に描画することでUIにエフェクトがかからないようにする）
  DrawHUD(draw);
}

void GameScene::DrawHUD(class Draw &draw) {
  // 初回呼び出し時に頻出文字列をアトラスへ一括プリロード
  static bool s_preloaded = false;
  if (!s_preloaded && draw.GetTextRenderer() &&
      draw.GetTextRenderer()->GetAtlas()) {
    s_preloaded = true;
    draw.GetTextRenderer()->GetAtlas()->PreloadString(
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz:/"
        ".mkmhpt%+-[]()!★◆▼▲●■░|【】①②③※・「」←→↑↓=<>,！、 "
        "ぁあぃいぅうぇえぉおかがきぎくぐけげこごさざしじすずせぜそぞただちぢっ"
        "つづてでとど"
        "なにぬねのはばぱひびぴふぶぷへべぺほぼぽまみむめもゃやゅゆょよらりるれ"
        "ろゎわゐゑをん"
        "アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモ"
        "ヤユヨラリルレロワヲンッャュョー"
        "一二三四五六七八九十百千万到達距離スコア速度最高記録ベストゲームオーバ"
        "ーリスタートリザルトへ戻る一時停止中現在獲得順位反撃チャンス左中央右打"
        "ち返せ跳ね返しボーナス敵撃破モードシールドバリアアクティブジャンプスラ"
        "イディング走るポーズキーもう一度遊ぶプレイメニュー"
        "ペンギンダッシュ―—"
        "操作方法十字説明攻略倒し方緑赤色迫る直撃減少命中削切回避手前当てろ避け"
        "ろ戦指令障害物魚押飛進残移動来固定再開初終体視替人称接続");
  }

  if (gameState_ == GameState::Title) {
    DrawTitleHUD(draw);
  } else if (gameState_ == GameState::GameOver) {
    DrawGameOverHUD(draw);
  } else if (gameState_ == GameState::Paused) {
    DrawPlayingHUD(draw);
    DrawPauseHUD(draw);
  } else if (gameState_ == GameState::PlayerHit) {
    DrawPlayingHUD(draw);
    // 衝突演出（Crash.png）
    if (crashSprite_) {
      draw.DrawSprite(crashSprite_.get());
    }
  } else if (gameState_ == GameState::Playing) {
    DrawPlayingHUD(draw);
    if (playingState_ == PlayingState::Boss && boss_->GetIsActive()) {
      DrawBossHUD(draw);
    }
    DrawControlsGuide(draw);
    if (isTitleExiting_) {
      DrawTitleHUD(draw);
    }
  } else if (gameState_ == GameState::Editor) {
    draw.DrawFillRect(Vector2(20.0f, 20.0f), Vector2(300.0f, 75.0f),
                      Vector4(0.04f, 0.06f, 0.1f, 0.8f));
    char distBuf[64];
    snprintf(distBuf, sizeof(distBuf), "[EDITOR] 距離: %.1f m",
             currentDistance_);
    draw.DrawMSDFString(distBuf, Vector2(30.0f, 28.0f), 22.0f,
                        Vector4(0.9f, 0.9f, 0.9f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
    char scoreBuf[64];
    snprintf(scoreBuf, sizeof(scoreBuf), "スコア: %.0f pt", currentScore_);
    draw.DrawMSDFString(scoreBuf, Vector2(30.0f, 58.0f), 20.0f,
                        Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  }

  // 一人称視点（FPV）アクティブ時のバッジ表示
  if (isFirstPersonView_ && gameState_ != GameState::Editor) {
    const float fpx = 20.0f;
    const float fpy = 675.0f;
    const float fpw = 175.0f;
    const float fph = 30.0f;
    draw.DrawFillRect(Vector2(fpx, fpy), Vector2(fpw, fph),
                      Vector4(0.04f, 0.10f, 0.18f, 0.85f));
    draw.DrawFillRect(Vector2(fpx, fpy), Vector2(3.0f, fph),
                      Vector4(0.3f, 0.85f, 1.0f, 1.0f));
    draw.DrawMSDFString("[F2] 1ST PERSON", Vector2(fpx + 10.0f, fpy + 6.0f), 16.0f,
                        Vector4(0.4f, 0.9f, 1.0f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.08f);
  }

  // 最前面にフェード描画
  if (fade_) {
    fade_->Draw(draw);
  }
}

void GameScene::DrawPlayingHUD(class Draw &draw) {
  // --- 1. 左上: メインステータスパネル (テキスト幅に合わせて横の余白を最適化)
  // ---
  const float panelX = 20.0f;
  const float panelY = 15.0f;
  const float panelW = 265.0f; // 元の390pxから余分な横の余白を詰める
  const float panelH = 145.0f;

  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, panelH),
                    Vector4(0.04f, 0.07f, 0.12f, 0.82f));
  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, 3.0f),
                    Vector4(0.2f, 0.6f, 0.9f, 0.9f));

  const float textX = panelX + 12.0f;

  char distBuf[64];
  snprintf(distBuf, sizeof(distBuf), "距離: %.1f m", currentDistance_);
  draw.DrawMSDFString(distBuf, Vector2(textX, panelY + 9.0f), 30.0f,
                      Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                      Vector4(0.05f, 0.15f, 0.25f, 1.0f), 0.08f);

  char scoreBuf[96];
  if (bonusEnemyHitCount_ > 0) {
    snprintf(scoreBuf, sizeof(scoreBuf), "スコア: %.0f pt  (+ボーナスx%d)",
             currentScore_, bonusEnemyHitCount_);
  } else {
    snprintf(scoreBuf, sizeof(scoreBuf), "スコア: %.0f pt", currentScore_);
  }
  draw.DrawMSDFString(scoreBuf, Vector2(textX, panelY + 47.0f), 24.0f,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.15f, 0.10f, 0.0f, 1.0f), 0.08f);

  // スピードメーター（時速換算とプログレスゲージ）
  float currentSpeed = stageSettings_->GetScrollSpeed();
  float baseSpeed = stageSettings_->GetBaseScrollSpeed();
  float maxSpeed = stageSettings_->GetMaxScrollSpeed();
  float speedKm = currentSpeed * 300.0f;
  float speedRatio = (maxSpeed > baseSpeed)
                         ? ((currentSpeed - baseSpeed) / (maxSpeed - baseSpeed))
                         : 0.0f;
  if (speedRatio < 0.0f)
    speedRatio = 0.0f;
  if (speedRatio > 1.0f)
    speedRatio = 1.0f;

  std::string speedBar = "[";
  int totalSegments = 10;
  int filledSegments = static_cast<int>(speedRatio * totalSegments + 0.5f);
  for (int s = 0; s < totalSegments; s++) {
    if (s < filledSegments)
      speedBar += "■";
    else
      speedBar += "░";
  }
  speedBar += "]";

  char speedText[64];
  snprintf(speedText, sizeof(speedText), "速度: %.0f km/h  %s", speedKm,
           speedBar.c_str());
  Vector4 speedColor = Lerp(Vector4{0.3f, 0.9f, 1.0f, 1.0f},
                            Vector4{1.0f, 0.4f, 0.2f, 1.0f}, speedRatio);
  draw.DrawMSDFString(speedText, Vector2(textX, panelY + 81.0f), 19.0f,
                      speedColor, true, Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  // ベスト記録（距離 / スコア）
  char bestText[96];
  snprintf(bestText, sizeof(bestText), "BEST: %.1f m  /  %.0f pt",
           topRankings_[0], topScoreRankings_[0]);
  draw.DrawMSDFString(bestText, Vector2(textX, panelY + 111.0f), 17.0f,
                      Vector4(0.8f, 0.85f, 0.9f, 0.85f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.06f);

  // --- 2. 右上: 1レーン時のみ残り距離を表示（横の余白を詰めたコンパクト設計）
  // ---
  if (playingState_ == PlayingState::OneLane) {
    float remainDist = (200.0f - rightSideDistance_ > 0.0f)
                           ? (200.0f - rightSideDistance_)
                           : 0.0f;
    char oneLaneBuf[64];
    snprintf(oneLaneBuf, sizeof(oneLaneBuf), "[ 1-LANE DASH: 残り %.0f m ]",
             remainDist);

    const float rightW = 275.0f;
    const float rightH = 46.0f;
    const float rightX = 1260.0f - rightW; // 画面右端から20pxマージン
    const float rightY = 15.0f;

    draw.DrawFillRect(Vector2(rightX, rightY), Vector2(rightW, rightH),
                      Vector4(0.04f, 0.07f, 0.12f, 0.82f));
    draw.DrawFillRect(Vector2(rightX, rightY), Vector2(rightW, 3.0f),
                      Vector4(1.0f, 0.45f, 0.9f, 0.9f));
    draw.DrawMSDFString(oneLaneBuf, Vector2(rightX + 12.0f, rightY + 10.0f),
                        22.0f, Vector4(1.0f, 0.45f, 0.9f, 1.0f), true,
                        Vector4(0.35f, 0.0f, 0.35f, 1.0f), 0.08f);
  }
}

void GameScene::DrawBossHUD(class Draw &draw) {
  // 中央上部: ボスパネル背景
  const float panelX = 320.0f;
  const float panelY = 15.0f;
  const float panelW = 640.0f;
  const float panelH = 115.0f;

  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, panelH),
                    Vector4(0.10f, 0.03f, 0.03f, 0.85f));
  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, 3.0f),
                    Vector4(0.9f, 0.2f, 0.2f, 0.9f));

  auto *tr = draw.GetTextRenderer();

  // ボスヘッダー (パネル中央揃え)
  const char *titleStr = "=== BOSS: ICE KING ===";
  const float titleSize = 28.0f;
  float titleW = tr ? tr->MeasureString(titleStr, titleSize).x : 307.0f;
  float titleX = panelX + (panelW - titleW) * 0.5f;
  draw.DrawMSDFString(titleStr, Vector2(titleX, 24.0f), titleSize,
                      Vector4(0.8f, 0.95f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.2f, 0.5f, 1.0f), 0.08f);

  // ボスHPゲージ
  int maxHp = boss_->GetMaxHP();
  int hp = boss_->GetHP();
  if (hp < 0)
    hp = 0;
  if (hp > maxHp)
    hp = maxHp;

  std::string hpGauge = "[";
  for (int i = 0; i < maxHp; i++) {
    if (i < hp)
      hpGauge += "■";
    else
      hpGauge += "░";
  }
  hpGauge += "]";

  char hpText[128];
  snprintf(hpText, sizeof(hpText), "HP %s  %d / %d", hpGauge.c_str(), hp,
           maxHp);
  const float hpSize = 22.0f;
  float hpW = tr ? tr->MeasureString(hpText, hpSize).x : 567.0f;
  float hpX = panelX + (panelW - hpW) * 0.5f;
  draw.DrawMSDFString(hpText, Vector2(hpX, 58.0f), hpSize,
                      Vector4(1.0f, 0.45f, 0.45f, 1.0f), true,
                      Vector4(0.2f, 0.0f, 0.0f, 1.0f), 0.07f);

  // 反撃（跳ね返し）基本操作ガイド (パネル中央揃え)
  bool isPad = GamePadInput::IsConnected();
  const char *guideStr = isPad
      ? "[X] 左レーン  |  [Y] 中央レーン  |  [B] 右レーン (魚を押して敵へ飛ばす)"
      : "[1] 左レーン  |  [2] 中央レーン  |  [3] 右レーン (魚を押して敵へ飛ばす)";
  const float guideSize = 20.0f;
  float guideW = tr ? tr->MeasureString(guideStr, guideSize).x : 680.0f;
  float guideX = panelX + (panelW - guideW) * 0.5f;
  draw.DrawMSDFString(guideStr, Vector2(guideX, 92.0f), guideSize,
                      Vector4(0.8f, 0.95f, 0.5f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  // 緑攻撃（跳ね返し可能弾）が反撃有効範囲（z: -15.0f
  // 〜 15.0f）にあるかチェック
  int reflectLane = -1;
  for (int j = 0; j < stageSettings_->GetMaxObstacles(); j++) {
    Obstacle *obs = stageSettings_->GetObstacle(j);
    if (!obs || !obs->GetIsActive() || obs->GetIsReflected())
      continue;

    if (obs->GetType() == Obstacle::Type::BossAttackReflectable) {
      float z = obs->GetTransform().translate.z;
      if (z > bossAttackReflectMinZ_ && z < 15.0f) {
        float obsX = obs->GetTransform().translate.x;
        float laneW = stageSettings_->GetLaneWidth();
        int lane = 1;
        // ボス戦カメラ（Y回転180度）ではワールド+Xが画面左（[1]キー）、ワールド-Xが画面右（[3]キー）に見える
        if (obsX > laneW / 2.0f)
          lane = 0; // 画面左レーン（[1]キー / Xボタン）
        else if (obsX < -laneW / 2.0f)
          lane = 2; // 画面右レーン（[3]キー / Bボタン）
        reflectLane = lane;
        break;
      }
    }
  }

  // 反撃チャンスのアラート点滅表示 (中央揃え)
  if (reflectLane != -1) {
    const char *keyName = "";
    if (isPad) {
      keyName = (reflectLane == 0)   ? "Xボタン"
                : (reflectLane == 1) ? "Yボタン"
                                     : "Bボタン";
    } else {
      keyName = (reflectLane == 0)   ? "1キー"
                : (reflectLane == 1) ? "2キー"
                                     : "3キー";
    }
    const char *laneName = (reflectLane == 0)   ? "左レーン"
                           : (reflectLane == 1) ? "中央レーン"
                                                : "右レーン";
    char alertBuf[96];
    snprintf(alertBuf, sizeof(alertBuf),
             ">>> 反撃チャンス！ [%s] で%sの魚を敵に飛ばせ！ <<<", keyName,
             laneName);

    float pulseScale = 0.8f + 0.2f * std::sin(uiTimer_ * 12.0f);
    const float alertW = 740.0f;
    const float alertX = 640.0f - alertW * 0.5f;
    draw.DrawFillRect(Vector2(alertX, 125.0f), Vector2(alertW, 40.0f),
                      Vector4(0.2f, 0.1f, 0.0f, 0.85f));
    float alertTextW = tr ? tr->MeasureString(alertBuf, 25.0f).x : 670.0f;
    float alertTextX = alertX + (alertW - alertTextW) * 0.5f;
    draw.DrawMSDFString(alertBuf, Vector2(alertTextX, 130.0f), 25.0f,
                        Vector4(1.0f, 0.95f, 0.15f, pulseScale), true,
                        Vector4(0.5f, 0.1f, 0.0f, 1.0f), 0.09f);
  }

  // --- 右下: ボス倒し方（攻略ガイド）パネル ---
  {
    const float guideX = 880.0f;
    const float guideY = 455.0f;
    const float guideW = 380.0f;
    const float guideH = 205.0f;

    // パネル背景（ダーククリムゾン）
    draw.DrawFillRect(Vector2(guideX, guideY), Vector2(guideW, guideH),
                      Vector4(0.08f, 0.03f, 0.04f, 0.88f));
    // 上部アクセントバー（ゴールド/オレンジ）
    draw.DrawFillRect(Vector2(guideX, guideY), Vector2(guideW, 3.0f),
                      Vector4(1.0f, 0.45f, 0.2f, 0.95f));

    // ヘッダー
    draw.DrawMSDFString("【 ボスの倒し方 】",
                        Vector2(guideX + 16.0f, guideY + 12.0f), 22.0f,
                        Vector4(1.0f, 0.88f, 0.25f, 1.0f), true,
                        Vector4(0.3f, 0.05f, 0.0f, 1.0f), 0.12f, 0.08f);
    draw.DrawMSDFString("HOW TO DEFEAT",
                        Vector2(guideX + 225.0f, guideY + 16.0f), 15.0f,
                        Vector4(0.85f, 0.65f, 0.5f, 0.85f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.08f, 0.05f);

    // 区切り線
    draw.DrawFillRect(Vector2(guideX + 14.0f, guideY + 42.0f),
                      Vector2(guideW - 28.0f, 1.0f),
                      Vector4(0.5f, 0.25f, 0.2f, 0.65f));

    // ステップ1: 障害物をよける
    draw.DrawMSDFString("① 障害物をよけて進め！",
                        Vector2(guideX + 16.0f, guideY + 52.0f), 19.0f,
                        Vector4(1.0f, 0.5f, 0.5f, 1.0f), true,
                        Vector4(0.2f, 0.0f, 0.0f, 1.0f), 0.10f, 0.06f);
    draw.DrawMSDFString("   移動で回避",
                        Vector2(guideX + 16.0f, guideY + 77.0f), 16.0f,
                        Vector4(0.9f, 0.9f, 0.9f, 0.9f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.08f, 0.05f);

    // ステップ2: 魚がいるレーンを押す
    draw.DrawMSDFString("② 魚がいるレーンを押せ！",
                        Vector2(guideX + 16.0f, guideY + 104.0f), 19.0f,
                        Vector4(0.3f, 1.0f, 0.6f, 1.0f), true,
                        Vector4(0.0f, 0.2f, 0.1f, 1.0f), 0.10f, 0.06f);
    const char *step2Str = isPad
        ? "   魚が手前に来たら [X]左 / [Y]中 / [B]右"
        : "   魚が手前に来たら [1]左 / [2]中 / [3]右";
    draw.DrawMSDFString(step2Str,
                        Vector2(guideX + 16.0f, guideY + 129.0f), 16.0f,
                        Vector4(1.0f, 0.95f, 0.65f, 0.95f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.08f, 0.05f);

    // ステップ3: 魚を敵に飛ばして撃破
    draw.DrawMSDFString("③ 魚を敵に飛ばして撃破！",
                        Vector2(guideX + 16.0f, guideY + 156.0f), 19.0f,
                        Vector4(1.0f, 0.8f, 0.25f, 1.0f), true,
                        Vector4(0.2f, 0.1f, 0.0f, 1.0f), 0.10f, 0.06f);
    draw.DrawMSDFString("   飛ばした魚を当ててHPを削り切れ！",
                        Vector2(guideX + 16.0f, guideY + 181.0f), 16.0f,
                        Vector4(0.95f, 0.95f, 0.95f, 0.9f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.08f, 0.05f);
  }

  // ボス登場時の大迫力作戦指令バナー (Appearance演出中)
  if (boss_->GetState() == BossState::Appearance) {
    float alpha = std::clamp(boss_->GetStateTimer() * 2.0f, 0.0f, 1.0f);
    draw.DrawFillRect(Vector2(260.0f, 240.0f), Vector2(760.0f, 190.0f),
                      Vector4(0.06f, 0.02f, 0.02f, 0.94f * alpha));
    draw.DrawFillRect(Vector2(260.0f, 240.0f), Vector2(760.0f, 4.0f),
                      Vector4(1.0f, 0.8f, 0.2f, 0.95f * alpha));
    draw.DrawFillRect(Vector2(260.0f, 426.0f), Vector2(760.0f, 4.0f),
                      Vector4(1.0f, 0.8f, 0.2f, 0.95f * alpha));

    draw.DrawMSDFString("★ MISSION: ボス ICE KING を撃破せよ！ ★",
                        Vector2(300.0f, 255.0f), 30.0f,
                        Vector4(1.0f, 0.9f, 0.2f, alpha), true,
                        Vector4(0.3f, 0.0f, 0.0f, alpha), 0.12f, 0.08f);

    draw.DrawMSDFString("① 障害物をよけろ！ (移動で回避)",
                        Vector2(290.0f, 305.0f), 21.0f,
                        Vector4(1.0f, 0.5f, 0.5f, alpha), true,
                        Vector4(0.2f, 0.0f, 0.0f, alpha), 0.10f, 0.06f);

    const char *mission2Str = isPad
        ? "② 魚がいるレーンを手前で [X] [Y] [B] ボタンで押せ！"
        : "② 魚がいるレーンを手前で [1] [2] [3] キーで押せ！";
    draw.DrawMSDFString(mission2Str,
                        Vector2(290.0f, 345.0f), 21.0f,
                        Vector4(0.3f, 1.0f, 0.6f, alpha), true,
                        Vector4(0.0f, 0.2f, 0.1f, alpha), 0.10f, 0.06f);

    draw.DrawMSDFString("③ 魚を敵に飛ばして命中させ、HP を削り切れ！",
                        Vector2(290.0f, 385.0f), 21.0f,
                        Vector4(1.0f, 0.85f, 0.3f, alpha), true,
                        Vector4(0.2f, 0.1f, 0.0f, alpha), 0.10f, 0.06f);
  }

  // ボス撃破時の演出バナー
  if (boss_->GetState() == BossState::Defeat) {
    draw.DrawFillRect(Vector2(360.0f, 170.0f), Vector2(560.0f, 95.0f),
                      Vector4(0.05f, 0.12f, 0.05f, 0.9f));
    draw.DrawFillRect(Vector2(360.0f, 170.0f), Vector2(560.0f, 3.0f),
                      Vector4(1.0f, 0.8f, 0.2f, 0.95f));
    draw.DrawMSDFString("★ BOSS DEFEATED! ★", Vector2(430.0f, 185.0f), 38.0f,
                        Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.10f);
    draw.DrawMSDFString("撃破ボーナス獲得！", Vector2(510.0f, 230.0f), 24.0f,
                        Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  }
}

void GameScene::DrawControlsGuide(class Draw &draw, float alpha) {
  if (alpha <= 0.01f) return;

  bool isPad = GamePadInput::IsConnected();

  draw.DrawFillRect(Vector2(20.0f, 672.0f), Vector2(1240.0f, 36.0f),
                    Vector4(0.03f, 0.06f, 0.11f, 0.88f * alpha));
  draw.DrawFillRect(Vector2(20.0f, 672.0f), Vector2(1240.0f, 2.0f),
                    Vector4(0.3f, 0.6f, 0.9f, 0.85f * alpha));

  // 左端のバッジ [ PAD ] または [ KEYBOARD ]
  const char *badgeText = isPad ? "PAD" : "KEYBOARD";
  float badgeW = isPad ? 72.0f : 110.0f;
  // 外枠 (1px)
  draw.DrawFillRect(Vector2(25.0f, 675.0f), Vector2(badgeW + 2.0f, 28.0f),
                    Vector4(0.35f, 0.40f, 0.48f, 0.75f * alpha));
  // 内側背景（視認性の高いダークチャコール）
  draw.DrawFillRect(Vector2(26.0f, 676.0f), Vector2(badgeW, 26.0f),
                    Vector4(0.07f, 0.09f, 0.13f, 0.95f * alpha));

  Vector4 badgeColor = isPad ? Vector4(0.35f, 1.0f, 0.75f, alpha)
                             : Vector4(1.0f, 0.88f, 0.35f, alpha);
  float badgeTextX = isPad ? 43.0f : 32.0f;
  draw.DrawMSDFString(badgeText, Vector2(badgeTextX, 680.0f), 15.5f,
                      badgeColor, true,
                      Vector4(0.0f, 0.0f, 0.0f, alpha), 0.08f);

  const char *guideText = "";
  if (isPad) {
    if (playingState_ == PlayingState::ThreeLane || gameState_ == GameState::Title) {
      guideText = "移動: [Lスティック / 十字]    ジャンプ: [Aボタン]    スライド: [Bボタン]    ポーズ: [START]";
    } else if (playingState_ == PlayingState::OneLane) {
      guideText = "ジャンプ: [Aボタン]    スライド: [Bボタン]    ポーズ: [START]    (※1レーン固定中)";
    } else if (playingState_ == PlayingState::Boss) {
      guideText = "【ボス反撃】 魚跳ね返し: [X]左  [Y]中  [B]右    |    移動: [Lスティック]    |    ポーズ: [START]";
    }
  } else {
    if (playingState_ == PlayingState::ThreeLane || gameState_ == GameState::Title) {
      guideText = "移動: [A/D / ← →]    ジャンプ: [SPACE / W]    スライド: [S / ↓]    ポーズ: [ESC]";
    } else if (playingState_ == PlayingState::OneLane) {
      guideText = "ジャンプ: [SPACE / W]    スライド: [S / ↓]    ポーズ: [ESC]    (※1レーン固定中)";
    } else if (playingState_ == PlayingState::Boss) {
      guideText = "【ボス反撃】 魚跳ね返し: [1]左  [2]中  [3]右    |    移動: [A/D]    |    ポーズ: [ESC]";
    }
  }

  float guideTextX = 26.0f + badgeW + 14.0f;
  draw.DrawMSDFString(guideText, Vector2(guideTextX, 680.0f), 17.5f,
                      Vector4(0.92f, 0.96f, 1.0f, 0.95f * alpha), true,
                      Vector4(0.0f, 0.0f, 0.0f, alpha), 0.06f);
}

void GameScene::DrawPauseHUD(class Draw &draw) {
  // 全画面暗転オーバーレイ
  draw.DrawFillRect(Vector2(0.0f, 0.0f), Vector2(1280.0f, 720.0f),
                    Vector4(0.0f, 0.0f, 0.0f, 0.65f));

  // 中央モーダルカード
  draw.DrawFillRect(Vector2(360.0f, 160.0f), Vector2(560.0f, 410.0f),
                    Vector4(0.06f, 0.08f, 0.12f, 0.94f));
  draw.DrawFillRect(Vector2(360.0f, 160.0f), Vector2(560.0f, 4.0f),
                    Vector4(1.0f, 0.8f, 0.2f, 0.95f));

  draw.DrawMSDFString("=== PAUSE ===", Vector2(490.0f, 190.0f), 52.0f,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.10f);

  draw.DrawMSDFString("ゲーム一時停止中", Vector2(540.0f, 255.0f), 22.0f,
                      Vector4(0.85f, 0.85f, 0.85f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  draw.DrawFillRect(Vector2(400.0f, 290.0f), Vector2(480.0f, 2.0f),
                    Vector4(0.3f, 0.4f, 0.5f, 0.7f));

  char pDistBuf[64];
  snprintf(pDistBuf, sizeof(pDistBuf), "現在の到達距離:  %.1f m",
           currentDistance_);
  draw.DrawMSDFString(pDistBuf, Vector2(430.0f, 315.0f), 26.0f,
                      Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  char pScoreBuf[64];
  snprintf(pScoreBuf, sizeof(pScoreBuf), "現在のスコア:    %.0f pt",
           currentScore_);
  draw.DrawMSDFString(pScoreBuf, Vector2(430.0f, 355.0f), 26.0f,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  draw.DrawFillRect(Vector2(400.0f, 400.0f), Vector2(480.0f, 2.0f),
                    Vector4(0.3f, 0.4f, 0.5f, 0.7f));

  bool isPausePad = GamePadInput::IsConnected();
  if (isPausePad) {
    draw.DrawMSDFString("[ START ] ゲームを再開 (RESUME)", Vector2(410.0f, 425.0f),
                        24.0f, Vector4(0.3f, 0.9f, 1.0f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
    draw.DrawMSDFString("[ Aボタン ] 最初からリスタート (RESTART)",
                        Vector2(410.0f, 465.0f), 24.0f,
                        Vector4(0.9f, 0.9f, 0.9f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
    draw.DrawMSDFString("[ Bボタン ] タイトルへ戻る (TITLE)", Vector2(410.0f, 505.0f),
                        24.0f, Vector4(0.9f, 0.9f, 0.9f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  } else {
    draw.DrawMSDFString("[ ESC ] ゲームを再開 (RESUME)", Vector2(410.0f, 425.0f),
                        24.0f, Vector4(0.3f, 0.9f, 1.0f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
    draw.DrawMSDFString("[  1  ] 最初からリスタート (RESTART)",
                        Vector2(410.0f, 465.0f), 24.0f,
                        Vector4(0.9f, 0.9f, 0.9f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
    draw.DrawMSDFString("[  2  ] タイトルへ戻る (TITLE)", Vector2(410.0f, 505.0f),
                        24.0f, Vector4(0.9f, 0.9f, 0.9f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  }
}

void GameScene::DrawGameOverHUD(class Draw &draw) {
  // 全画面暗転オーバーレイ
  draw.DrawFillRect(Vector2(0.0f, 0.0f), Vector2(1280.0f, 720.0f),
                    Vector4(0.0f, 0.0f, 0.0f, 0.72f));

  // 中央モーダルカード
  draw.DrawFillRect(Vector2(200.0f, 60.0f), Vector2(880.0f, 570.0f),
                    Vector4(0.06f, 0.08f, 0.12f, 0.95f));
  draw.DrawFillRect(Vector2(200.0f, 60.0f), Vector2(880.0f, 4.0f),
                    Vector4(0.9f, 0.2f, 0.2f, 0.95f));

  draw.DrawMSDFString("GAME OVER", Vector2(480.0f, 85.0f), 56.0f,
                      Vector4(1.0f, 0.25f, 0.25f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.10f);

  bool isNewDistRecord =
      (topRankings_[0] > 0.0f && currentDistance_ >= topRankings_[0]);
  bool isNewScoreRecord =
      (topScoreRankings_[0] > 0.0f && currentScore_ >= topScoreRankings_[0]);

  char distBuf[96];
  snprintf(distBuf, sizeof(distBuf), "到達距離: %.1f m  %s", currentDistance_,
           isNewDistRecord ? "★ NEW RECORD! ★" : "");
  draw.DrawMSDFString(distBuf, Vector2(260.0f, 160.0f), 26.0f,
                      isNewDistRecord ? Vector4(1.0f, 0.9f, 0.2f, 1.0f)
                                      : Vector4(1.0f, 1.0f, 1.0f, 1.0f),
                      true, Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  char scoreBuf[96];
  snprintf(scoreBuf, sizeof(scoreBuf), "最終スコア: %.0f pt  %s", currentScore_,
           isNewScoreRecord ? "★ NEW RECORD! ★" : "");
  draw.DrawMSDFString(scoreBuf, Vector2(260.0f, 200.0f), 26.0f,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  char bonusBuf[64];
  snprintf(bonusBuf, sizeof(bonusBuf), "ボーナス敵撃破: %d 体",
           bonusEnemyHitCount_);
  draw.DrawMSDFString(bonusBuf, Vector2(260.0f, 240.0f), 22.0f,
                      Vector4(0.6f, 0.9f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.06f);

  // 区切りライン
  draw.DrawFillRect(Vector2(240.0f, 280.0f), Vector2(800.0f, 2.0f),
                    Vector4(0.3f, 0.4f, 0.5f, 0.7f));

  // ランキング 2カラム表示
  // 左カラム: 距離ランキング TOP 3
  draw.DrawMSDFString("【 距離ランキング TOP 3 】", Vector2(260.0f, 305.0f),
                      22.0f, Vector4(0.4f, 0.85f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  for (int i = 0; i < 3; i++) {
    char rBuf[64];
    if (topRankings_[i] > 0.0f) {
      snprintf(rBuf, sizeof(rBuf), " %d位: %.1f m", i + 1, topRankings_[i]);
    } else {
      snprintf(rBuf, sizeof(rBuf), " %d位: ---", i + 1);
    }
    Vector4 rankColor = (i == 0)   ? Vector4(1.0f, 0.9f, 0.2f, 1.0f)
                        : (i == 1) ? Vector4(0.85f, 0.85f, 0.9f, 1.0f)
                                   : Vector4(0.85f, 0.65f, 0.45f, 1.0f);
    draw.DrawMSDFString(rBuf, Vector2(270.0f, 345.0f + i * 35.0f), 20.0f,
                        rankColor, true, Vector4(0.0f, 0.0f, 0.0f, 1.0f),
                        0.06f);
  }

  // 右カラム: スコアランキング TOP 3
  draw.DrawMSDFString("【 スコアランキング TOP 3 】", Vector2(660.0f, 305.0f),
                      22.0f, Vector4(1.0f, 0.85f, 0.3f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  for (int i = 0; i < 3; i++) {
    char rBuf[64];
    if (topScoreRankings_[i] > 0.0f) {
      snprintf(rBuf, sizeof(rBuf), " %d位: %.0f pt", i + 1,
               topScoreRankings_[i]);
    } else {
      snprintf(rBuf, sizeof(rBuf), " %d位: ---", i + 1);
    }
    Vector4 rankColor = (i == 0)   ? Vector4(1.0f, 0.9f, 0.2f, 1.0f)
                        : (i == 1) ? Vector4(0.85f, 0.85f, 0.9f, 1.0f)
                                   : Vector4(0.85f, 0.65f, 0.45f, 1.0f);
    draw.DrawMSDFString(rBuf, Vector2(670.0f, 345.0f + i * 35.0f), 20.0f,
                        rankColor, true, Vector4(0.0f, 0.0f, 0.0f, 1.0f),
                        0.06f);
  }

  // 区切りライン
  draw.DrawFillRect(Vector2(240.0f, 470.0f), Vector2(800.0f, 2.0f),
                    Vector4(0.3f, 0.4f, 0.5f, 0.7f));

  bool isOverPad = GamePadInput::IsConnected();
  if (isOverPad) {
    draw.DrawMSDFString("[ Aボタン ] もう一度プレイ (RESTART)   |   [ Bボタン ] "
                        "タイトルへ戻る (TITLE)",
                        Vector2(245.0f, 505.0f), 22.0f,
                        Vector4(0.9f, 0.95f, 1.0f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  } else {
    draw.DrawMSDFString("[ 1キー ] もう一度プレイ (RESTART)   |   [ 2キー ] "
                        "タイトルへ戻る (TITLE)",
                        Vector2(245.0f, 505.0f), 22.0f,
                        Vector4(0.9f, 0.95f, 1.0f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  }
}

void GameScene::DrawTitleHUD(class Draw &draw) {
  // 画像は出さないでください（titleSprite_ の描画は行わない）

  // タイトル文字およびアニメーション計算
  const std::string titleText = "ペンギンダッシュ";
  const std::string subTitleText = "― PENGUIN DASH ―";
  bool isTitlePad = GamePadInput::IsConnected();
  const std::string startText = isTitlePad ? "PRESS [A] BUTTON TO START" : "PRESS SPACE TO START";

  const float fontSize = 112.0f;
  const float subFontSize = 32.0f;
  const float startFontSize = 34.0f;

  // テキストサイズの計測とセンタリング
  float titleWidth = 0.0f;
  float subWidth = 0.0f;
  float startWidth = 0.0f;
  if (draw.GetTextRenderer()) {
    titleWidth = draw.GetTextRenderer()->MeasureString(titleText, fontSize).x;
    subWidth =
        draw.GetTextRenderer()->MeasureString(subTitleText, subFontSize).x;
    startWidth =
        draw.GetTextRenderer()->MeasureString(startText, startFontSize).x;
  }
  if (titleWidth <= 0.0f)
    titleWidth = 760.0f;
  if (subWidth <= 0.0f)
    subWidth = 350.0f;
  if (startWidth <= 0.0f)
    startWidth = 640.0f;

  float titleX = (1280.0f - titleWidth) * 0.5f;
  float subX = (1280.0f - subWidth) * 0.5f;
  float startX = (1280.0f - startWidth) * 0.5f;

  // 基準Y座標
  const float baseTitleY = 170.0f;
  const float baseSubY = baseTitleY + 125.0f;
  const float baseStartY = 570.0f;

  float currentTitleY = baseTitleY;
  float currentSubY = baseSubY;
  float currentStartY = baseStartY;

  float mainAlpha = 1.0f;
  float subAlpha = 0.95f;
  float startAlpha = 1.0f;

  if (isTitleExiting_) {
    // スタート時：文字がゆったりと上へ昇っていく演出（ゆっくり上昇）
    float progress =
        std::clamp(titleExitTimer_ / kTitleExitDuration_, 0.0f, 1.0f);
    float ease = 1.0f - std::pow(1.0f - progress, 2.5f);
    float flyOffset = ease * 380.0f;

    currentTitleY = baseTitleY - flyOffset;
    currentSubY = baseSubY - flyOffset;
    currentStartY = baseStartY - ease * 250.0f;

    mainAlpha = (progress < 0.65f) ? 1.0f : std::clamp(1.0f - (progress - 0.65f) / 0.35f, 0.0f, 1.0f);
    subAlpha = (progress < 0.55f) ? 0.95f : std::clamp(0.95f - (progress - 0.55f) / 0.45f, 0.0f, 1.0f);
    startAlpha = std::clamp(1.0f - progress * 3.5f, 0.0f, 1.0f);
  } else {
    // タイトル待機中：心地よい上下の浮遊モーション
    float hover = sinf(uiTimer_ * 2.2f) * 6.0f;
    currentTitleY = baseTitleY + hover;
    currentSubY = baseSubY + hover;

    // スタート案内の点滅表示
    float blink = sinf(uiTimer_ * 5.0f);
    startAlpha = (blink > -0.2f) ? 1.0f : 0.0f;
  }

  // タイトル文字の描画（背景が3Dの雪景色でもクッキリ見えるよう、シャドウ＋アウトライン太字で描画）
  if (mainAlpha > 0.01f) {
    // ドロップシャドウ
    Vector4 shadowColor = Vector4(0.01f, 0.04f, 0.12f, 0.8f * mainAlpha);
    draw.DrawMSDFString(titleText, Vector2(titleX + 5.0f, currentTitleY + 6.0f),
                        fontSize, shadowColor, false, {0.0f, 0.0f, 0.0f, 0.0f},
                        0.0f, 0.18f);

    // メインテキスト（清涼感のあるアイスホワイト & 濃紺アウトライン）
    Vector4 textColor = Vector4(0.95f, 0.98f, 1.0f, mainAlpha);
    Vector4 outlineColor = Vector4(0.06f, 0.16f, 0.35f, mainAlpha);
    draw.DrawMSDFString(titleText, Vector2(titleX, currentTitleY), fontSize,
                        textColor, true, outlineColor, 0.22f, 0.15f);
  }

  // 英語サブタイトル
  if (subAlpha > 0.01f) {
    Vector4 subColor = Vector4(0.65f, 0.88f, 1.0f, subAlpha);
    Vector4 subOutline = Vector4(0.04f, 0.1f, 0.24f, subAlpha);
    draw.DrawMSDFString(subTitleText, Vector2(subX, currentSubY), subFontSize,
                        subColor, true, subOutline, 0.18f, 0.08f);
  }

  // スタート案内テキスト
  if (startAlpha > 0.02f) {
    draw.DrawMSDFString(startText, Vector2(startX, currentStartY),
                        startFontSize, Vector4(1.0f, 1.0f, 1.0f, startAlpha),
                        true, Vector4(0.0f, 0.0f, 0.0f, startAlpha), 0.15f,
                        0.08f);
  }

  // --- タイトル操作方法バー（ゲームシーンと統一された下部バー） ---
  float guideAlpha = isTitleExiting_ ? std::clamp(1.0f - (titleExitTimer_ / 0.4f), 0.0f, 1.0f) : mainAlpha;
  DrawControlsGuide(draw, guideAlpha);

  // --- スタート時のペンギン疾走演出 (15体 画面上から下への＞の字フォーメーション) ---
  if (isTitleExiting_ && titlePenguinSprites_[0]) {
    float progress = std::clamp(titleExitTimer_ / kTitleExitDuration_, 0.0f, 1.0f);

    // ゆっくり等速〜なだらかなイーズで心地よいスピード感で右へ進む
    float runProgress = progress * progress * (3.0f - 2.0f * progress) * 0.15f + progress * 0.85f;
    float runStartX = -100.0f; // 先頭の開始X（一番後ろのペンギンも画面左外からスタート）
    float runEndX = 1620.0f;   // 先頭の終了X（一番後ろのペンギンも画面右外へ完全に抜ける）
    float leaderX = runStartX + (runEndX - runStartX) * runProgress;

    const float baseY = 360.0f; // 画面中央(縦720pxの真ん中)

    // 画面上から下への「＞」の字フォーメーション (15体: i=0 最上段 〜 i=7 中央先頭 〜 i=14 最下段)
    struct FormationParam {
      float offsetX;
      float offsetY;
      float animTimeOffset; // コマ送りの時間ズレ（群れとしての自然な躍動感）
      float scale;          // 遠近感スケール (等倍比率)
    };

    const float stepY = 41.5f;  // 画面の上端(Y≈70px)から下端(Y≈650px)まで均等に展開
    const float slopeX = 26.0f; // 中央先頭から上下斜め後ろへの「＞」の傾き

    FormationParam formations[kTitlePenguinCount];
    for (int i = 0; i < kTitlePenguinCount; ++i) {
      int distFromCenter = std::abs(i - 7); // 0(中央先頭) 〜 7(上下端)
      float offsetY = (i - 7) * stepY;
      float offsetX = -distFromCenter * slopeX;

      // 奥(上部)から手前(下部)への自然な遠近感、中央リーダーは少し大きめ
      float depth = 0.92f + (offsetY / 290.5f) * 0.10f;
      float scale = (i == 7) ? 1.10f : (1.00f * depth);

      // 中央先頭から外側へ波打つように走るアニメーションオフセット
      float animTimeOffset = distFromCenter * 0.09f;

      formations[i] = { offsetX, offsetY, animTimeOffset, scale };
    }

    // 描画: 奥(上側 i=0)から手前(下側 i=14)へ順に描画して自然な重なりにする
    for (int idx = 0; idx < kTitlePenguinCount; ++idx) {
      if (!titlePenguinSprites_[idx]) continue;

      const auto& form = formations[idx];
      float posX = leaderX + form.offsetX;
      float posY = baseY + form.offsetY;

      // アニメーションコマ送り (約1秒間に10回切り替え = 少しゆったりトコトコ走る)
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

      // 上下のボビング（跳ね）と少しの前傾姿勢
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

      // 各ペンギンの背後に風の疾走ラインを描画（画面内にいる時）
      if (posX > 0.0f && posX < 1360.0f) {
        float trailAlpha = std::clamp(runProgress * 1.5f, 0.0f, 0.65f) * form.scale;
        draw.DrawFillRect(Vector2(posX - 65.0f * form.scale, posY + 5.0f + bobbing),
                          Vector2(55.0f * form.scale, 2.5f),
                          Vector4(0.85f, 0.95f, 1.0f, trailAlpha * 0.7f));
      }

      // ペンギンスプライトを描画
      draw.DrawSprite(titlePenguinSprites_[idx].get());
    }
  }
}

void GameScene::PlayerHitUpdate() {
  crashTimer_ += (1.0f / 60.0f);

  // Crash.png をサイズ0から勢いよく大きく拡大するアニメーション
  if (crashSprite_) {
    float t = std::clamp(crashTimer_ / crashScaleDuration_, 0.0f, 1.0f);
    // EaseOutBack: 0から勢いよく拡大して少しバウンドする迫力ある出現演出
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float s = (t >= 1.0f) ? 1.0f : (1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f));
    if (s < 0.0f) s = 0.0f;

    Transform ct = crashSpriteData_.transform;
    ct.scale = {s, s, 1.0f};
    ct.translate = {crashPosition_.x, crashPosition_.y, 0.0f};
    crashSprite_->SetTransform(ct);
    crashSprite_->SettingWvp();
  }

  // カメラやビューの更新は GameScene::Update で行われている
  effectManager_->PlayerHitUpdate(view);

  // プレイヤーのノックバックアニメーションを更新
  // (GameScene側の全体更新は停止し、プレイヤーのみ更新)
  player_->Update(view, 1.0f);

  if (player_->IsHitAnimationFinished()) {
    gameState_ = GameState::GameOver;
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

  currentDistance_ += stageSettings_->GetScrollSpeed() * timeScale;
  currentScore_ = currentDistance_ + (bonusEnemyHitCount_ * 200.0f *
                                      stageSettings_->GetScrollSpeed());

  CheckKeepRolling();

  // 右サイドモード（カメラアイテム取得後）の更新
  if (isRightSideMode_) {
    rightSideDistance_ += stageSettings_->GetScrollSpeed() * timeScale;
    if (rightSideDistance_ >= 200.0f) {
      ChangePlayingState(PlayingState::ThreeLane);
    }
  }

  // ボス戦の更新
  if (playingState_ == PlayingState::Boss) {
    // ボス戦中はボスアイテムのクールタイムを満タンに維持（ボス戦終了後にクールタイムを開始させるため）
    float bossItemDuration =
        stageSettings_->GetItemCoolDownDuration(Obstacle::Type::BossItem);
    stageSettings_->SetItemCoolDownTimer(Obstacle::Type::BossItem,
                                         bossItemDuration);

    if (!boss_->GetIsActive() && !isCameraTransitioning_ &&
        !isCameraTransitionPending_) {
      ChangePlayingState(PlayingState::ThreeLane);
    } else if (boss_->GetIsActive() && boss_->GetState() == BossState::Battle) {
      bossAttackTimer_ += timeScale;
      // 攻撃の生成（約2秒に1回）
      if (bossAttackTimer_ >= 120.0f) {
        bossAttackTimer_ -= 120.0f;

        // 3レーンのうち、1つを安全地帯、1つを白、1つを緑にする
        int safeLane = rand() % 3;
        int greenLane = (safeLane + 1 + rand() % 2) % 3;

        for (int i = 0; i < 3; i++) {
          if (i == safeLane)
            continue;

          Obstacle::Type type = Obstacle::Type::BossAttack;
          if (i == greenLane) {
            type = Obstacle::Type::BossAttackReflectable;
          }

          // 未使用のObstacleを探す
          for (int j = 0; j < stageSettings_->GetMaxObstacles(); j++) {
            Obstacle *obs = stageSettings_->GetObstacle(j);
            if (!obs->GetIsActive()) {
              obs->SetType(type);
              obs->SetDropHeight(bossAttackDropHeight_);
              obs->SetFallDuration(bossAttackFallDuration_);
              obs->SetReflectArcHeight(bossReflectArcHeight_);
              obs->SetReflectDuration(bossReflectDuration_);
              float x = (i - 1) * stageSettings_->GetLaneWidth();
              obs->Spawn(
                  x, 2.0f + obs->GetCollisionHeight() * 0.5f,
                  bossAttackSpawnZ_); // ボスの攻撃を奥の上空から降らせて出現させる
              break;
            }
          }
        }
      }

      // 跳ね返し入力判定 (キーボード 1/2/3 または コントローラー X/Y/B)
      // 画面左: 1キー または Xボタン
      // 画面中央: 2キー または Yボタン
      // 画面右: 3キー または Bボタン
      bool push1 = Input::PushKey(DIK_1) || Input::PushKey(DIK_NUMPAD1) ||
                   GamePadInput::PushButton(XINPUT_GAMEPAD_X);
      bool push2 = Input::PushKey(DIK_2) || Input::PushKey(DIK_NUMPAD2) ||
                   GamePadInput::PushButton(XINPUT_GAMEPAD_Y);
      bool push3 = Input::PushKey(DIK_3) || Input::PushKey(DIK_NUMPAD3) ||
                   GamePadInput::PushButton(XINPUT_GAMEPAD_B);

      AABB bossAABB =
          Collision::MakeAABB(boss_->GetTransform(), 5.0f, 5.0f, 5.0f);

      for (int j = 0; j < stageSettings_->GetMaxObstacles(); j++) {
        Obstacle *obs = stageSettings_->GetObstacle(j);
        if (!obs->GetIsActive())
          continue;

        // 着地時の砂煙エフェクト
        if (obs->GetJustLanded()) {
          effectManager_->EmitDust(obs->GetTransform().translate);
          SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossLand);
        }

        if (obs->GetType() == Obstacle::Type::BossAttackReflectable &&
            !obs->GetIsReflected() && !obs->GetIsFalling()) {
          float obsX = obs->GetTransform().translate.x;
          float laneW = stageSettings_->GetLaneWidth();
          int lane =
              1; // 0:画面左([1]/X), 1:画面中央([2]/Y), 2:画面右([3]/B)
          // ボス戦カメラ（Y回転180度）ではワールド+Xが画面左、ワールド-Xが画面右に見える
          if (obsX > laneW / 2.0f)
            lane = 0; // 画面左（[1]キー / Xボタン対応）
          else if (obsX < -laneW / 2.0f)
            lane = 2; // 画面右（[3]キー / Bボタン対応）

          // プレイヤーの手前にいる時に跳ね返せる
          float z = obs->GetTransform().translate.z;
          if (z > bossAttackReflectMinZ_ && z < 15.0f) {
            if ((lane == 0 && push1) || (lane == 1 && push2) ||
                (lane == 2 && push3)) {
              obs->SetReflectArcHeight(bossReflectArcHeight_);
              obs->SetReflectDuration(bossReflectDuration_);
              obs->SetReflected(true);
              obs->SetReflectedTarget(boss_->GetTransform().translate);
              SoundManager::GetInstance()->PlaySE(
                  SoundManager::SE::BossReflect);
              GamePadInput::Rumble(0.18f, 32000, 32000); // 跳ね返しフィードバック振動
            }
          }
        } else if (obs->GetIsReflected()) {
          // ボスの最新位置を追従（浮遊アニメーションにも追従）
          obs->SetReflectedTarget(boss_->GetTransform().translate);

          // 跳ね返った障害物とボスとの当たり判定
          AABB obsAABB = Collision::MakeAABB(
              obs->GetTransform(), obs->GetCollisionWidth(),
              obs->GetCollisionHeight(), obs->GetCollisionDepth());
          // 弧の降下フェーズ（進行度70%以上）でボスAABBに接触、または終点(100%)到達でヒット
          if ((obs->GetReflectProgress() >= 0.7f &&
               Collision::CheckAABB(bossAABB, obsAABB)) ||
              obs->GetReflectProgress() >= 1.0f) {
            obs->Deactivate(); // 障害物を消す
            boss_->OnDamage();
            SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossHit);

            if (boss_->GetState() == BossState::Defeat) {
              SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossDefeat);
              GamePadInput::Rumble(0.5f, 65535, 65535); // 撃破の大迫力振動
              // 画面内のボス攻撃をすべて消す
              for (int k = 0; k < stageSettings_->GetMaxObstacles(); k++) {
                Obstacle *o = stageSettings_->GetObstacle(k);
                if (o->GetIsActive() &&
                    (o->GetType() == Obstacle::Type::BossAttack ||
                     o->GetType() == Obstacle::Type::BossAttackReflectable)) {
                  o->OnBlowAway();
                }
              }
            }
          }
        }
      }
    }
  }

  // Update Player lane constraints
  player_->SetLaneLimits(stageSettings_->GetMinLaneIndex(),
                         stageSettings_->GetMaxLaneIndex(),
                         stageSettings_->GetLaneWidth());

  // カメラが反転視点（ボス戦等でY回転が約90度以上反転している状態）の時は、
  // 画面の見た目通りに動くようにプレイヤーの左右操作を反転させる
  bool isCameraInverted = (playingState_ == PlayingState::Boss &&
                           cameraTransform_.rotate.y > 1.57f);
  player_->SetInvertedControls(isCameraInverted);

  // オブジェクトの一括更新
  gameObjectManager_->UpdateAll(view, speedMultiplier * timeScale);

  stageSettings_->Update(view, timeScale);

  // 以前の当たり判定チェック
  CheckCollisions();

  // Componentベースの当たり判定チェック
  collisionManager_->UpdateCollisions(gameObjectManager_.get());

  effectManager_->PlayingUpdate(view, player_->GetTransform().translate);

  // 走っている間（転がっていなくて地面にいる時）	//
  // プレイヤーの足元に砂埃エフェクトを生成
  if (!player_->GetIsRolling() &&
      player_->GetTransform().translate.y <= 3.01f) {
    effectManager_->EmitDust(player_->GetTransform().translate);
  }
}

void GameScene::TitleUpdate() {
  float timeScale = EditorManager::GetPlaySpeed();
  float speedMultiplier = 1.0f;

  // タイトル中は心地よい見やすい一定スクロール速度をキープ
  stageSettings_->SetBaseScrollSpeed(0.22f);
  stageSettings_->SetScrollAcceleration(0.0f);

  // スタート移行中（isTitleExiting_）なら障害物の新規生成を停止、待機中は生成する
  stageSettings_->SetSpawningPaused(isTitleExiting_);

  // 1. オートパイロットAIによる回避・アクション判断
  UpdateTitleAutoPilot();

  // 2. プレイヤーのレーン制限を更新
  player_->SetLaneLimits(stageSettings_->GetMinLaneIndex(),
                         stageSettings_->GetMaxLaneIndex(),
                         stageSettings_->GetLaneWidth());
  player_->SetInvertedControls(false);

  // しゃがみ中に頭上に障害物がある場合はしゃがみを維持
  CheckKeepRolling();

  // 3. 3Dオブジェクトの一括更新（プレイヤーのアニメーションと移動を前進させる）
  gameObjectManager_->UpdateAll(view, speedMultiplier * timeScale);

  // 4. ステージ・障害物のスクロール更新
  stageSettings_->Update(view, timeScale);

  // 5. タイトル専用の衝突判定（ゲームオーバーにならず、雪だるまは吹っ飛び、通常障害物も安全処理）
  CheckTitleCollisions();

  // 6. エフェクト更新
  effectManager_->PlayingUpdate(view, player_->GetTransform().translate);

  // 7. プレイヤーの足元に砂煙エフェクトを発生（走っている躍動感）
  if (!player_->GetIsRolling() && player_->GetTransform().translate.y <= 3.01f) {
    effectManager_->EmitDust(player_->GetTransform().translate);
  }

  // 8. 開始演出中の完了判定、またはキー入力受付
  if (isTitleExiting_) {
    // 画面内にアクティブな障害物が残っているかチェック
    bool hasActiveObstacles = false;
    for (int i = 0; i < stageSettings_->GetMaxObstacles(); i++) {
      Obstacle* obs = stageSettings_->GetObstacle(i);
      if (obs && obs->GetIsActive()) {
        hasActiveObstacles = true;
        break;
      }
    }

    // 障害物が完全になくなったタイミングで正式にゲームスタート！
    // （※ペンギンが右端まで走り抜けきる titleExitTimer_ >= kTitleExitDuration_、またはタイムアウト時）
    if ((!hasActiveObstacles && titleExitTimer_ >= kTitleExitDuration_) || (titleExitTimer_ >= kTitleExitDuration_ + 3.0f)) {
      StartPlaying();
    }
  } else {
    // スペースキーまたはゲームパッドAボタン・STARTボタンでゲーム開始シーケンス突入 (フェード中は誤操作防止)
    if ((!fade_ || !fade_->IsFading()) &&
        (Input::PushKey(DIK_SPACE) ||
         GamePadInput::PushButton(XINPUT_GAMEPAD_A) ||
         GamePadInput::PushButton(XINPUT_GAMEPAD_START))) {
      StartGame();
    }
  }
}

void GameScene::UpdateTitleAutoPilot() {
  if (!player_) return;

  int currentLane = player_->GetLaneIndex();
  int targetLane = player_->GetTargetLaneIndex();
  // レーン移動中の場合は目標レーンを基準に障害物を判断
  int activeLane = player_->IsChangingLane() ? targetLane : currentLane;

  float laneWidth = stageSettings_->GetLaneWidth();
  if (laneWidth <= 0.0f) laneWidth = 2.0f;

  float playerZ = player_->GetTransform().translate.z;

  // 各レーン（index 0: -1(Left), 1: 0(Center), 2: 1(Right)）の情報
  struct LaneInfo {
    float minDistZ = 999.0f;       // すべての障害物の中で直近のrelZ
    Obstacle::Type minType = Obstacle::Type::Wall;
    Obstacle* minObs = nullptr;

    float minWallDistZ = 999.0f;   // 回避不能障害物（Wall等）の中で直近のrelZ
  };
  LaneInfo lanes[3];

  Obstacle* imminentObs = nullptr;
  float minActiveDistZ = 999.0f;

  for (int i = 0; i < stageSettings_->GetMaxObstacles(); i++) {
    Obstacle* obs = stageSettings_->GetObstacle(i);
    if (!obs || !obs->GetIsActive() || obs->GetIsHit()) continue;

    float relZ = obs->GetTransform().translate.z - playerZ;
    // プレイヤーの当たり判定（背面側約-0.8m）を完全に通り過ぎたものは除外
    if (relZ < -0.8f) continue;

    float obsX = obs->GetTransform().translate.x;
    int obsLane = static_cast<int>(std::round(obsX / laneWidth));
    if (obsLane < -1) obsLane = -1;
    if (obsLane > 1) obsLane = 1;

    int idx = obsLane + 1; // 0, 1, 2

    // 直近障害物の更新
    if (relZ < lanes[idx].minDistZ) {
      lanes[idx].minDistZ = relZ;
      lanes[idx].minType = obs->GetType();
      lanes[idx].minObs = obs;
    }

    // 壁系（回避不能障害物）の直近距離の更新
    if (obs->GetType() == Obstacle::Type::Wall ||
        obs->GetType() == Obstacle::Type::BossAttack ||
        obs->GetType() == Obstacle::Type::BossAttackReflectable) {
      if (relZ < lanes[idx].minWallDistZ) {
        lanes[idx].minWallDistZ = relZ;
      }
    }

    // プレイヤーが現在いる（または移動中の）レーンにある直近障害物
    if (obsLane == activeLane && relZ > -0.5f && relZ < minActiveDistZ) {
      minActiveDistZ = relZ;
      imminentObs = obs;
    }
  }

  // 直近に迫っている障害物の回避アクション
  if (imminentObs) {
    Obstacle::Type type = imminentObs->GetType();

    // Low（倒木・ジャンプで飛び越える）
    // 早すぎると着地時に障害物に激突するため、十分に引きつけてから跳ぶ（距離約3.6m）
    if (type == Obstacle::Type::Low) {
      if (minActiveDistZ <= 3.6f && minActiveDistZ >= 0.8f) {
        if (!player_->GetIsJumping()) {
          player_->TriggerJump();
        }
      }
    }
    // High（氷のアーチ・スライディングで潜り抜ける）
    // 早すぎると立った瞬間に障害物に激突するため、十分に引きつけてから潜る（距離約3.8m）
    else if (type == Obstacle::Type::High) {
      if (minActiveDistZ <= 3.8f && minActiveDistZ >= 0.8f) {
        if (!player_->GetIsRolling() && !player_->GetIsJumping()) {
          player_->TriggerRoll();
        }
      }
    }
    // Wall（壁）またはその他の避けられない障害物（レーン移動で回避）
    else if (type == Obstacle::Type::Wall ||
             type == Obstacle::Type::BossAttack ||
             type == Obstacle::Type::BossAttackReflectable) {
      if (minActiveDistZ <= 15.0f && !player_->IsChangingLane() && player_->CanAct()) {
        if (activeLane == 0) {
          // 中央にいる場合: 左(-1, idx 0) と 右(+1, idx 2) の安全度を比較
          float leftWallDist = lanes[0].minWallDistZ;
          float rightWallDist = lanes[2].minWallDistZ;
          float leftMinDist = lanes[0].minDistZ;
          float rightMinDist = lanes[2].minDistZ;

          // 1. 壁の有無を優先比較（壁がない、または壁が遠い方を優先）
          if (leftWallDist > 15.0f && rightWallDist <= 15.0f) {
            // 左は壁がなく安全、右は壁がある -> 左へ
            player_->TriggerMoveLeft();
          } else if (rightWallDist > 15.0f && leftWallDist <= 15.0f) {
            // 右は壁がなく安全、左は壁がある -> 右へ
            player_->TriggerMoveRight();
          } else if (leftWallDist > 15.0f && rightWallDist > 15.0f) {
            // どちらも壁はない -> より距離が開いている（何もない）方へ移動
            if (leftMinDist >= rightMinDist && leftMinDist > 5.0f) {
              player_->TriggerMoveLeft();
            } else if (rightMinDist > 5.0f) {
              player_->TriggerMoveRight();
            } else if (leftMinDist >= rightMinDist) {
              player_->TriggerMoveLeft();
            } else {
              player_->TriggerMoveRight();
            }
          } else {
            // 両方に壁がある場合 -> 壁までの距離がより遠い方へ逃げる
            if (leftWallDist > rightWallDist) {
              player_->TriggerMoveLeft();
            } else {
              player_->TriggerMoveRight();
            }
          }
        } else if (activeLane == -1) {
          // 左にいる場合: 中央(0, idx 1)へ移動
          // 中央の壁が目前（4.0m未満）でなければ、左の壁を避けるために中央へ移動
          if (lanes[1].minWallDistZ > 4.0f) {
            player_->TriggerMoveRight();
          }
        } else if (activeLane == 1) {
          // 右にいる場合: 中央(0, idx 1)へ移動
          // 中央の壁が目前（4.0m未満）でなければ、右の壁を避けるために中央へ移動
          if (lanes[1].minWallDistZ > 4.0f) {
            player_->TriggerMoveLeft();
          }
        }
      }
    }
  }

  // 目の前に差し迫った危険がなく、中央レーンが十分に安全な場合のみ中央（0）へ戻る
  if ((!imminentObs || minActiveDistZ > 20.0f) && !player_->IsChangingLane() && player_->CanAct()) {
    if (activeLane != 0) {
      // 中央レーンに 22m 以内に障害物がなく、かつ 30m 以内に壁がない場合のみ復帰
      if (lanes[1].minDistZ > 22.0f && lanes[1].minWallDistZ > 30.0f) {
        if (activeLane < 0) {
          player_->TriggerMoveRight();
        } else {
          player_->TriggerMoveLeft();
        }
      }
    }
  }
}

void GameScene::CheckTitleCollisions() {
  const Transform& playerTransform = player_->GetTransform();
  float playerHeight = player_->GetIsRolling() ? 0.5f : 1.5f;
  AABB playerAABB = Collision::MakeAABB(playerTransform, 0.8f, playerHeight, 0.8f);

  for (int i = 0; i < stageSettings_->GetMaxObstacles(); i++) {
    Obstacle* obstacle = stageSettings_->GetObstacle(i);
    if (!obstacle || !obstacle->GetIsActive() || obstacle->GetIsHit()) continue;

    AABB obstacleAABB = Collision::MakeAABB(
        obstacle->GetTransform(), obstacle->GetCollisionWidth(),
        obstacle->GetCollisionHeight(), obstacle->GetCollisionDepth());

    if (Collision::CheckAABB(playerAABB, obstacleAABB)) {
      if (obstacle->GetType() == Obstacle::Type::Bonus) {
        // 雪だるまに当たった！吹き飛ばしてボーナス演出
        obstacle->OnBlowAway();
        bonusEnemyHitCount_++;
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::BonusHit);
      } else {
        // 万が一通常の障害物に接触してもゲームオーバーにせず吹き飛ばす（セーフティネット）
        obstacle->OnBlowAway();
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::BarrierBreak);
      }
    }
  }
}

void GameScene::PausedUpdate() {
  pauseSystem_->Update();
  if (Input::PushKey(DIK_1) || GamePadInput::PushButton(XINPUT_GAMEPAD_A)) {
    ResetGame();
    gameState_ = GameState::Playing;
    isTitleExiting_ = false;
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Start);
    SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Play);
  }
  if (Input::PushKey(DIK_2) || GamePadInput::PushButton(XINPUT_GAMEPAD_B)) {
    ReturnToTitle();
  }
}

void GameScene::EditorUpdate() {
  // Editor mode doesn't progress the game scroll or obstacle positions.
  // But we still want to update objects (like their transforms).
  gameObjectManager_->UpdateAll(view, 0.0f);
  stageSettings_->EditorUpdate(view);

  // パーティクルがデバッグカメラに対応するように、EditorUpdate() を呼び出す
  effectManager_->EditorUpdate(view);
}

void GameScene::CheckCollisions() {
  // プレイヤーのAABBを生成
  const Transform &playerTransform = player_->GetTransform();
  float playerHeight =
      player_->GetIsRolling() ? 0.5f : 1.5f; // 転がり中は低くなる
  AABB playerAABB =
      Collision::MakeAABB(playerTransform, 0.8f, playerHeight, 0.8f);

  // 全障害物との当たり判定
  for (int i = 0; i < stageSettings_->GetMaxObstacles(); i++) {
    Obstacle *obstacle = stageSettings_->GetObstacle(i);
    if (!obstacle->GetIsActive() || obstacle->GetIsHit())
      continue;

    AABB obstacleAABB = Collision::MakeAABB(
        obstacle->GetTransform(), obstacle->GetCollisionWidth(),
        obstacle->GetCollisionHeight(), obstacle->GetCollisionDepth());

    if (Collision::CheckAABB(playerAABB, obstacleAABB)) {
      // 誘導床（GuideFloor）の判定
      if (obstacle->GetType() == Obstacle::Type::GuideFloor) {
        // プレイヤーを滑らかに中央へ誘導 (30フレーム)
        player_->StartForceToCenter(30.0f);
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Slide);

        continue; // ゲームオーバーにはならない
      }

      if (obstacle->GetType() == Obstacle::Type::Bonus) {
        // ボーナスエネミーに当たった場合の処理（吹き飛ばす）
        obstacle->OnBlowAway();
        bonusEnemyHitCount_++; // スコア（距離）ボーナス
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::BonusHit);

        continue; // ゲームオーバーにはならず、次の判定へ
      }

      if (obstacle->GetType() == Obstacle::Type::CameraItem) {
        obstacle->OnHit();
        ChangePlayingState(PlayingState::OneLane);
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);

        continue; // ゲームオーバーにはならず、次の判定へ
      }

      if (obstacle->GetType() == Obstacle::Type::BarrierItem) {
        obstacle->OnHit();
        player_->SetHasBarrier(true);
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Barrier);
        effectManager_->EmitBarrier(player_->GetTransform().translate);
        continue;
      }

      if (obstacle->GetType() == Obstacle::Type::ClearItem) {
        obstacle->OnHit();
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::ClearBomb);

        // 画面内の障害物を吹き飛ばす
        for (int j = 0; j < stageSettings_->GetMaxObstacles(); j++) {
          Obstacle *obs = stageSettings_->GetObstacle(j);
          if (obs->GetIsActive() && (obs->GetType() == Obstacle::Type::Low ||
                                     obs->GetType() == Obstacle::Type::High ||
                                     obs->GetType() == Obstacle::Type::Wall)) {
            obs->OnBlowAway();
          }
        }
        continue;
      }

      if (obstacle->GetType() == Obstacle::Type::BossItem) {
        obstacle->OnHit();
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Barrier);
        // 画面内の通常障害物を吹き飛ばしてボス戦へスムーズに移行
        for (int j = 0; j < stageSettings_->GetMaxObstacles(); j++) {
          Obstacle *obs = stageSettings_->GetObstacle(j);
          if (obs->GetIsActive() && (obs->GetType() == Obstacle::Type::Low ||
                                     obs->GetType() == Obstacle::Type::High ||
                                     obs->GetType() == Obstacle::Type::Wall)) {
            obs->OnBlowAway();
          }
        }
        ChangePlayingState(PlayingState::Boss);
        continue;
      }

      if (obstacle->GetType() == Obstacle::Type::BossAttack ||
          obstacle->GetType() == Obstacle::Type::BossAttackReflectable) {
        // 跳ね返されている緑障害物はプレイヤーに当たらない
        if (obstacle->GetIsReflected())
          continue;
        // 落下中（上空にいる間）はプレイヤーに当たらない
        if (obstacle->GetIsFalling())
          continue;
      }

      // プレイヤーがバリアを持っている場合は消費して防ぐ
      if (player_->GetHasBarrier()) {
        player_->SetHasBarrier(false);
        obstacle->OnBlowAway();
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::BarrierBreak);
        effectManager_->BreakBarrier(player_->GetTransform().translate);
        continue; // ゲームオーバーにならず次へ
      }

      // 衝突！ヒット演出へ移行
      gameState_ = GameState::PlayerHit;
      crashTimer_ = 0.0f;
      if (crashSprite_) {
        Transform ct = crashSpriteData_.transform;
        ct.scale = {0.0f, 0.0f, 1.0f};
        ct.translate = {crashPosition_.x, crashPosition_.y, 0.0f};
        crashSprite_->SetTransform(ct);
        crashSprite_->SettingWvp();
      }
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Crash);
      GamePadInput::Rumble(0.35f, 48000, 48000); // 被弾時の衝撃振動
      if (playingState_ == PlayingState::Boss && boss_->GetIsActive()) {
        boss_->ChangeState(BossState::Victory);
      }

      stageSettings_->SetGameOver(true);
      // PostEffect::SetActivePostEffect(PostEffect::Type::GrayScale);

      // プレイヤーのヒットアニメーション開始（Low障害物なら前へ転がる）
      bool isTrip = (obstacle->GetType() == Obstacle::Type::Low);
      player_->OnHit(isTrip);


      // ランキング更新
      UpdateRanking();
      UpdateScoreRanking();

      break;
    }
  }
}

void GameScene::UpdateRanking() {
  // 降順ソートでトップ3を保持
  for (int i = 0; i < 3; i++) {
    if (currentDistance_ > topRankings_[i]) {
      // シフト
      for (int j = 2; j > i; j--) {
        topRankings_[j] = topRankings_[j - 1];
      }
      topRankings_[i] = currentDistance_;
      break;
    }
  }
}

void GameScene::UpdateScoreRanking() {
  // 降順ソートでトップ3を保持
  for (int i = 0; i < 3; i++) {
    if (currentScore_ > topScoreRankings_[i]) {
      // シフト
      for (int j = 2; j > i; j--) {
        topScoreRankings_[j] = topScoreRankings_[j - 1];
      }
      topScoreRankings_[i] = currentScore_;
      break;
    }
  }
}

void GameScene::CheckKeepRolling() {
  bool keepRolling = false;
  if (player_->GetIsRolling()) {
    // Calculate a "standing up" AABB for the player
    const Transform &playerTransform = player_->GetTransform();
    AABB standingAABB = Collision::MakeAABB(playerTransform, 0.8f, 1.5f, 0.8f);

    for (int i = 0; i < stageSettings_->GetMaxObstacles(); i++) {
      Obstacle *obstacle = stageSettings_->GetObstacle(i);
      if (!obstacle->GetIsActive() || obstacle->GetIsHit())
        continue;

      if (obstacle->GetType() == Obstacle::Type::High) {
        AABB obstacleAABB = Collision::MakeAABB(
            obstacle->GetTransform(), obstacle->GetCollisionWidth(),
            obstacle->GetCollisionHeight(), obstacle->GetCollisionDepth());

        // もし立ち上がったら当たる位置にいるか？
        if (Collision::CheckAABB(standingAABB, obstacleAABB)) {
          // プレイヤーの中心が障害物の中心より奥（Z座標が大きい）なら
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
    // ボス戦終了時：ボスアイテムのクールタイムを満タンに設定
    float bossItemDuration =
        stageSettings_->GetItemCoolDownDuration(Obstacle::Type::BossItem);
    stageSettings_->SetItemCoolDownTimer(Obstacle::Type::BossItem,
                                         bossItemDuration);
  }

  Transform target;
  target.scale = {1.0f, 1.0f, 1.0f};
  int laneCount = 3;

  switch (playingState_) {
  case PlayingState::ThreeLane:
    target.rotate = {0.3f, 0.0f, 0.0f};
    target.translate = {0.0f, 8.0f, -15.0f};
    laneCount = 3;
    isRightSideMode_ = false;
    rightSideDistance_ = 0.0f;
    boss_->Reset();
    bossAttackTimer_ = 0.0f;
    break;

  case PlayingState::OneLane:
    target.rotate = {0.3f, -1.0472f, 0.0f};
    target.translate = {30.0f, 15.0f, -5.0f};
    laneCount = 1;
    isRightSideMode_ = true;
    rightSideDistance_ = 0.0f;
    boss_->Reset();
    bossAttackTimer_ = 0.0f;
    break;

  case PlayingState::Boss:
    // ボス用のカメラ位置・角度 (bossCameraTranslate_, bossCameraRotate_)
    target.rotate = bossCameraRotate_;
    target.translate = bossCameraTranslate_;
    laneCount = 3;
    isRightSideMode_ = false;
    rightSideDistance_ = 0.0f;
    stageSettings_->SetSpawningPaused(true);
    bossAttackTimer_ = 0.0f;
    boss_->Reset();
    SoundManager::GetInstance()->PlayBGM(SoundManager::BGM::Boss);
    break;
  }

  StartCameraTransition(target, laneCount);
}

void GameScene::StartCameraTransition(const Transform &targetTransform,
                                      int laneCount) {
  // 障害物の生成を即座に停止
  stageSettings_->SetSpawningPaused(true);

  // トランジションの予約を行う（奥から新レーンが出現するように設定）
  stageSettings_->SetLaneCount(laneCount);
  isCameraTransitionPending_ = true;
  pendingCameraTargetTransform_ = targetTransform;
  pendingLaneCount_ = laneCount;
}

void GameScene::UpdateCameraTransition() {
  if (isCameraTransitionPending_) {
    // アクティブな障害物が残っているかチェック（ヒット済みの障害物は無視する）
    bool hasActiveObstacles = false;
    for (int i = 0; i < stageSettings_->GetMaxObstacles(); i++) {
      Obstacle *obstacle = stageSettings_->GetObstacle(i);
      if (obstacle->GetIsActive() && !obstacle->GetIsHit()) {
        hasActiveObstacles = true;
        break;
      }
    }

    // 障害物が全て消えたら、実際のトランジションを開始する
    if (!hasActiveObstacles) {
      isCameraTransitionPending_ = false;
      isCameraTransitioning_ = true;
      cameraTransitionTimer_ = 0.0f;
      startCameraTransform_ = cameraTransform_;
      targetCameraTransform_ = pendingCameraTargetTransform_;
      stageSettings_->SetLaneCount(pendingLaneCount_);
    }
  }

  if (!isCameraTransitioning_)
    return;

  cameraTransitionTimer_ +=
      1.0f / 60.0f; // 毎フレームの時間を加算 (FPS固定なら)
  float t = cameraTransitionTimer_ / cameraTransitionDuration_;

  if (t >= 1.0f) {
    t = 1.0f;
    isCameraTransitioning_ = false;
    if (playingState_ != PlayingState::Boss) {
      stageSettings_->SetSpawningPaused(false);
      // ボス戦から通常走行へ復帰した瞬間からクールタイムを改めて開始
      if (wasBossBattle_) {
        wasBossBattle_ = false;
        float bossItemDuration =
            stageSettings_->GetItemCoolDownDuration(Obstacle::Type::BossItem);
        stageSettings_->SetItemCoolDownTimer(Obstacle::Type::BossItem,
                                             bossItemDuration);
      }
    } else {
      // カメラ遷移が終わってからボスを出現させる
      boss_->Spawn(-6.0f, 3.0f, -2.0f);
    }
  }

  // easeInOut（スムーズな動きのため）
  float easeT = t * t * (3.0f - 2.0f * t);

  cameraTransform_.translate = Lerp(startCameraTransform_.translate,
                                    targetCameraTransform_.translate, easeT);

  // 弧を描くためのオフセット (Y軸方向に膨らむ)
  float arcHeight = 10.0f;
  cameraTransform_.translate.y += std::sin(easeT * 3.14159265f) * arcHeight;

  cameraTransform_.rotate =
      Lerp(startCameraTransform_.rotate, targetCameraTransform_.rotate, easeT);
  cameraTransform_.scale =
      Lerp(startCameraTransform_.scale, targetCameraTransform_.scale, easeT);

  camera_->SetTransform(cameraTransform_);
  gameCamera_->SetTransform(cameraTransform_);
}

void GameScene::UpdateFirstPersonCamera() {
  if (!player_) {
    return;
  }

  Vector3 playerPos = player_->GetTransform().translate;
  Vector3 playerRot = player_->GetTransform().rotate;

  Transform fpTransform;
  fpTransform.scale = {1.0f, 1.0f, 1.0f};
  fpTransform.rotate = {
      playerRot.x + firstPersonRotate_.x,
      playerRot.y + firstPersonRotate_.y,
      playerRot.z + firstPersonRotate_.z,
  };
  fpTransform.translate = {
      playerPos.x + firstPersonOffset_.x,
      playerPos.y + firstPersonOffset_.y,
      playerPos.z + firstPersonOffset_.z,
  };

  camera_->SetTransform(fpTransform);
  gameCamera_->SetTransform(fpTransform);

  player_->SetVisible(drawPlayerInFirstPerson_);
}

void GameScene::UpdateTitleCamera() {
  if (!player_) {
    return;
  }

  float timeScale = EditorManager::GetPlaySpeed();
  float dt = (1.0f / 60.0f) * timeScale;

  // プレイヤーが中心（中央レーン・地面）にいるときの座標を基準・注視点とする
  // （左右移動やジャンプ・スライディングによるカメラの急激なブレやカクつきを防止）
  Vector3 centerPos = {0.0f, player_->GetBaseHeight(),
                       player_->GetTransform().translate.z};
  Vector3 targetPos = {centerPos.x, centerPos.y + titleTargetOffsetY_,
                       centerPos.z};

  Transform defaultCamTransform;
  defaultCamTransform.scale = {1.0f, 1.0f, 1.0f};
  defaultCamTransform.rotate = {0.3f, 0.0f, 0.0f};
  defaultCamTransform.translate = {0.0f, 8.0f, -15.0f};

  if (isTitleExiting_) {
    // ゲーム開始演出中：旋回位置から通常プレイ用カメラ位置へと滑らかにイージング復帰
    float progress =
        (kTitleExitDuration_ > 0.0f) ? (titleExitTimer_ / kTitleExitDuration_) : 1.0f;
    if (progress > 1.0f) {
      progress = 1.0f;
    }

    // easeInOutQuad
    float easeT = (progress < 0.5f)
                      ? (2.0f * progress * progress)
                      : (1.0f - std::pow(-2.0f * progress + 2.0f, 2.0f) * 0.5f);

    // 平行移動・スケールの補間
    cameraTransform_.translate =
        Lerp(titleExitStartCamTransform_.translate, defaultCamTransform.translate, easeT);
    cameraTransform_.scale =
        Lerp(titleExitStartCamTransform_.scale, defaultCamTransform.scale, easeT);

    // 回転（Y軸の最短角度差補間）
    float startY = titleExitStartCamTransform_.rotate.y;
    float targetY = defaultCamTransform.rotate.y;
    float diffY = targetY - startY;
    while (diffY > 3.14159265f) diffY -= 6.2831853f;
    while (diffY < -3.14159265f) diffY += 6.2831853f;

    cameraTransform_.rotate.x =
        Lerp(titleExitStartCamTransform_.rotate.x, defaultCamTransform.rotate.x, easeT);
    cameraTransform_.rotate.y = startY + diffY * easeT;
    cameraTransform_.rotate.z =
        Lerp(titleExitStartCamTransform_.rotate.z, defaultCamTransform.rotate.z, easeT);
  } else {
    // タイトル通常待機中：プレイヤーを中心に回転
    titleCameraAngle_ += titleCameraSpeed_ * dt;
    if (titleCameraAngle_ >= 6.2831853f) {
      titleCameraAngle_ -= 6.2831853f;
    } else if (titleCameraAngle_ < 0.0f) {
      titleCameraAngle_ += 6.2831853f;
    }

    // カメラ位置の算出（プレイヤー中心の円周軌道）
    Vector3 camPos;
    camPos.x = targetPos.x + std::sin(titleCameraAngle_) * titleCameraRadius_;
    camPos.y = targetPos.y + titleCameraHeight_;
    camPos.z = targetPos.z - std::cos(titleCameraAngle_) * titleCameraRadius_;

    // カメラの向き（プレイヤー注視）の算出
    Vector3 dir = {targetPos.x - camPos.x, targetPos.y - camPos.y,
                   targetPos.z - camPos.z};
    float distXZ = std::sqrt(dir.x * dir.x + dir.z * dir.z);

    cameraTransform_.scale = {1.0f, 1.0f, 1.0f};
    cameraTransform_.translate = camPos;
    cameraTransform_.rotate.x = std::atan2(-dir.y, distXZ);
    cameraTransform_.rotate.y = std::atan2(dir.x, dir.z);
    cameraTransform_.rotate.z = 0.0f;
  }

  camera_->SetTransform(cameraTransform_);
  gameCamera_->SetTransform(cameraTransform_);
}

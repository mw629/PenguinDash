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
  currentReflectLane_ = -1;
}

void BossBattleController::Update(float timeScale, Boss *boss,
                                  StageSettings *stageSettings,
                                  GameSceneEffect *effectManager) {
  if (!boss || !stageSettings) return;

  // ボス戦中はボスアイテムのクールタイムを満タンに維持
  float bossItemDuration =
      stageSettings->GetItemCoolDownDuration(Obstacle::Type::BossItem);
  stageSettings->SetItemCoolDownTimer(Obstacle::Type::BossItem,
                                       bossItemDuration);

  currentReflectLane_ = -1;

  if (boss->GetIsActive() && boss->GetState() == BossState::Battle) {
    bossAttackTimer_ += timeScale;

    // 攻撃の生成（約2秒に1回 = 120フレーム）
    if (bossAttackTimer_ >= 120.0f) {
      bossAttackTimer_ -= 120.0f;

      // 3レーンのうち、1つを安全地帯、1つを白(通常)、1つを緑(跳ね返し)にする
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
        for (int j = 0; j < stageSettings->GetMaxObstacles(); j++) {
          Obstacle *obs = stageSettings->GetObstacle(j);
          if (!obs->GetIsActive()) {
            obs->SetType(type);
            obs->SetDropHeight(bossAttackDropHeight_);
            obs->SetFallDuration(bossAttackFallDuration_);
            obs->SetReflectArcHeight(bossReflectArcHeight_);
            obs->SetReflectDuration(bossReflectDuration_);
            float x = (i - 1) * stageSettings->GetLaneWidth();
            obs->Spawn(x, 2.0f + obs->GetCollisionHeight() * 0.5f,
                       bossAttackSpawnZ_);
            break;
          }
        }
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
        Collision::MakeAABB(boss->GetTransform(), 5.0f, 5.0f, 5.0f);

    for (int j = 0; j < stageSettings->GetMaxObstacles(); j++) {
      Obstacle *obs = stageSettings->GetObstacle(j);
      if (!obs || !obs->GetIsActive())
        continue;

      // 着地時の砂煙エフェクト
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
        if (obsX > laneW / 2.0f)
          lane = 0;
        else if (obsX < -laneW / 2.0f)
          lane = 2;

        float z = obs->GetTransform().translate.z;
        if (z > bossAttackReflectMinZ_ && z < 15.0f) {
          currentReflectLane_ = lane;

          if ((lane == 0 && push1) || (lane == 1 && push2) ||
              (lane == 2 && push3)) {
            obs->SetReflectArcHeight(bossReflectArcHeight_);
            obs->SetReflectDuration(bossReflectDuration_);
            obs->SetReflected(true);
            obs->SetReflectedTarget(boss->GetTransform().translate);
            SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossReflect);
            GamePadInput::Rumble(0.18f, 32000, 32000);
          }
        }
      } else if (obs->GetIsReflected()) {
        // ボスの最新位置を追従
        obs->SetReflectedTarget(boss->GetTransform().translate);

        // 跳ね返った障害物とボスとの当たり判定
        AABB obsAABB = Collision::MakeAABB(
            obs->GetTransform(), obs->GetCollisionWidth(),
            obs->GetCollisionHeight(), obs->GetCollisionDepth());

        if ((obs->GetReflectProgress() >= 0.7f &&
             Collision::CheckAABB(bossAABB, obsAABB)) ||
            obs->GetReflectProgress() >= 1.0f) {
          obs->Deactivate();
          boss->OnDamage();
          SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossHit);

          if (boss->GetState() == BossState::Defeat) {
            SoundManager::GetInstance()->PlaySE(SoundManager::SE::BossDefeat);
            GamePadInput::Rumble(0.5f, 65535, 65535);
            // 画面内のボス攻撃をすべて消す
            for (int k = 0; k < stageSettings->GetMaxObstacles(); k++) {
              Obstacle *o = stageSettings->GetObstacle(k);
              if (o && o->GetIsActive() &&
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

void BossBattleController::ImGui(Boss *boss,
                                 GameCameraController *cameraController,
                                 bool isPlayingBoss,
                                 bool isCameraTransitioning) {
#ifdef _USE_IMGUI
  if (boss && boss->GetIsActive()) {
    if (ImGui::TreeNode("Boss Info")) {
      ImGui::Text("HP: %d / %d", boss->GetHP(), boss->GetMaxHP());
      ImGui::DragFloat3("Target Pos", &boss->GetTargetPosRef().x, 0.1f);
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
        if (isPlayingBoss && !isCameraTransitioning && cameraController) {
          Transform ct = cameraController->GetTransform();
          ct.translate = bossCameraTranslate_;
          cameraController->SetTransform(ct);
        }
      }
      if (ImGui::DragFloat3("Boss Cam Rot", &bossCameraRotate_.x, 0.01f)) {
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

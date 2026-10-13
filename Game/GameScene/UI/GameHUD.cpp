#include "GameHUD.h"
#include "../../Direction/Fade.h"
#include "Graphics/Font/TextRenderer.h"
#include "Graphics/Render/Draw.h"
#include "Input/GamePadInput.h"
#include <Core/WindowConfig.h>
#include <Entity/Boss.h>
#include <GameObjects/Object/2d/Sprite.h>
#include <Stage/StageSettings.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include "Network/Firebase/FirebaseRankingManager.h"

// GameState values matching GameScene
enum GameStateEnum {
  State_Title = 0,
  State_Playing = 1,
  State_Paused = 2,
  State_PlayerHit = 3,
  State_GameClear = 4,
  State_GameOver = 5,
  State_Editor = 6
};

// PlayingState values matching GameScene
enum PlayingStateEnum {
  PlayState_ThreeLane = 0,
  PlayState_OneLane = 1,
  PlayState_Boss = 2
};

void GameHUD::PreloadFontStrings(Draw &draw) {
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
        "ろ戦指令障害物魚押飛進残移動来固定再開初終体視替人称接続"
        "行圏外以成通最新取得設定送信功失敗未判");
  }
}

void GameHUD::DrawHUD(Draw &draw, const GameHUDContext &ctx) {
  PreloadFontStrings(draw);

  if (ctx.gameState == State_Title) {
    DrawTitleHUD(draw, ctx);
  } else if (ctx.gameState == State_GameOver) {
    DrawGameOverHUD(draw, ctx);
  } else if (ctx.gameState == State_Paused) {
    DrawPlayingHUD(draw, ctx);
    DrawPauseHUD(draw, ctx);
  } else if (ctx.gameState == State_PlayerHit) {
    DrawPlayingHUD(draw, ctx);
    if (ctx.crashSprite) {
      draw.DrawSprite(ctx.crashSprite);
    }
  } else if (ctx.gameState == State_Playing) {
    DrawPlayingHUD(draw, ctx);
    if (ctx.playingState == PlayState_Boss && ctx.boss && ctx.boss->GetIsActive()) {
      DrawBossHUD(draw, ctx);
    }
    DrawControlsGuide(draw, ctx);
    if (ctx.isTitleExiting) {
      DrawTitleHUD(draw, ctx);
    }
  } else if (ctx.gameState == State_Editor) {
    DrawEditorHUD(draw, ctx);
  }

  // 一人称視点バッジ
  if (ctx.isFirstPersonView && ctx.gameState != State_Editor) {
    DrawBadgeFPV(draw);
  }

  // フェード描画
  if (ctx.fade) {
    ctx.fade->Draw(draw);
  }
}

void GameHUD::DrawPlayingHUD(Draw &draw, const GameHUDContext &ctx) {
  // 左上: メインステータスパネル
  const float panelX = kStatusPanelPos.x;
  const float panelY = kStatusPanelPos.y;
  const float panelW = kStatusPanelSize.x;
  const float panelH = kStatusPanelSize.y;

  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, panelH),
                    kPanelBgColor);
  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, kPanelBorderHeight),
                    kPanelBorderColor);

  const float textX = panelX + kPanelTextOffsetX;

  char distBuf[64];
  snprintf(distBuf, sizeof(distBuf), "距離: %.1f m", ctx.currentDistance);
  draw.DrawMSDFString(distBuf, Vector2(textX, panelY + kDistTextOffsetY), kDistFontSize,
                      Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                      Vector4(0.05f, 0.15f, 0.25f, 1.0f), 0.08f);

  char scoreBuf[96];
  if (ctx.bonusEnemyHitCount > 0) {
    snprintf(scoreBuf, sizeof(scoreBuf), "スコア: %.0f pt  (+ボーナスx%d)",
             ctx.currentScore, ctx.bonusEnemyHitCount);
  } else {
    snprintf(scoreBuf, sizeof(scoreBuf), "スコア: %.0f pt", ctx.currentScore);
  }
  draw.DrawMSDFString(scoreBuf, Vector2(textX, panelY + kScoreTextOffsetY), kScoreFontSize,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.15f, 0.10f, 0.0f, 1.0f), 0.08f);

  // スピードメーター
  float currentSpeed = ctx.scrollSpeed;
  float baseSpeed = ctx.baseScrollSpeed;
  float maxSpeed = ctx.maxScrollSpeed;
  float speedKm = currentSpeed * kSpeedDisplayMultiplier;
  float speedRatio = (maxSpeed > baseSpeed)
                         ? ((currentSpeed - baseSpeed) / (maxSpeed - baseSpeed))
                         : 0.0f;
  if (speedRatio < 0.0f) speedRatio = 0.0f;
  if (speedRatio > 1.0f) speedRatio = 1.0f;

  std::string speedBar = "[";
  int totalSegments = kSpeedBarSegments;
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
  draw.DrawMSDFString(speedText, Vector2(textX, panelY + kSpeedTextOffsetY), kSpeedFontSize,
                      speedColor, true, Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  // ベスト記録
  char bestText[96];
  float topDist = ctx.topRankings ? ctx.topRankings[0] : 0.0f;
  float topScore = ctx.topScoreRankings ? ctx.topScoreRankings[0] : 0.0f;
  snprintf(bestText, sizeof(bestText), "BEST: %.1f m  /  %.0f pt",
           topDist, topScore);
  draw.DrawMSDFString(bestText, Vector2(textX, panelY + kBestTextOffsetY), kBestFontSize,
                      Vector4(0.8f, 0.85f, 0.9f, 0.85f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.06f);

  // 右上: 1レーン時のみ残り距離を表示
  if (ctx.playingState == PlayState_OneLane) {
    float remainDist = (kOneLaneTotalDistance - ctx.rightSideDistance > 0.0f)
                           ? (kOneLaneTotalDistance - ctx.rightSideDistance)
                           : 0.0f;
    char oneLaneBuf[64];
    snprintf(oneLaneBuf, sizeof(oneLaneBuf), "[ 1-LANE DASH: 残り %.0f m ]",
             remainDist);

    const float rightW = kOneLanePanelSize.x;
    const float rightH = kOneLanePanelSize.y;
    const float rightX = 1280.0f - rightW - kOneLaneMarginRight;
    const float rightY = kOneLanePanelY;

    draw.DrawFillRect(Vector2(rightX, rightY), Vector2(rightW, rightH),
                      Vector4(0.04f, 0.07f, 0.12f, 0.82f));
    draw.DrawFillRect(Vector2(rightX, rightY), Vector2(rightW, kPanelBorderHeight),
                      Vector4(1.0f, 0.45f, 0.9f, 0.9f));
    draw.DrawMSDFString(oneLaneBuf, Vector2(rightX + kPanelTextOffsetX, rightY + 10.0f),
                        22.0f, Vector4(1.0f, 0.45f, 0.9f, 1.0f), true,
                        Vector4(0.35f, 0.0f, 0.35f, 1.0f), 0.08f);
  }
}

void GameHUD::DrawBossHUD(Draw &draw, const GameHUDContext &ctx) {
  if (!ctx.boss) return;

  const float panelX = kBossPanelPos.x;
  const float panelY = kBossPanelPos.y;
  const float panelW = kBossPanelSize.x;
  const float panelH = kBossPanelSize.y;

  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, panelH),
                    Vector4(0.10f, 0.03f, 0.03f, 0.85f));
  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, kPanelBorderHeight),
                    Vector4(0.9f, 0.2f, 0.2f, 0.9f));

  auto *tr = draw.GetTextRenderer();

  // ボスヘッダー
  const char *titleStr = "=== BOSS: ICE KING ===";
  const float titleSize = kBossTitleFontSize;
  float titleW = tr ? tr->MeasureString(titleStr, titleSize).x : 307.0f;
  float titleX = panelX + (panelW - titleW) * 0.5f;
  draw.DrawMSDFString(titleStr, Vector2(titleX, kBossTitleY), titleSize,
                      Vector4(0.8f, 0.95f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.2f, 0.5f, 1.0f), 0.08f);

  // ボスHPゲージ
  int maxHp = ctx.boss->GetMaxHP();
  int hp = ctx.boss->GetHP();
  if (hp < 0) hp = 0;
  if (hp > maxHp) hp = maxHp;

  std::string hpGauge = "[";
  for (int i = 0; i < maxHp; i++) {
    if (i < hp)
      hpGauge += "■";
    else
      hpGauge += "░";
  }
  hpGauge += "]";

  char hpText[128];
  snprintf(hpText, sizeof(hpText), "HP %s  %d / %d", hpGauge.c_str(), hp, maxHp);
  const float hpSize = kBossHpFontSize;
  float hpW = tr ? tr->MeasureString(hpText, hpSize).x : 567.0f;
  float hpX = panelX + (panelW - hpW) * 0.5f;
  draw.DrawMSDFString(hpText, Vector2(hpX, kBossHpY), hpSize,
                      Vector4(1.0f, 0.45f, 0.45f, 1.0f), true,
                      Vector4(0.2f, 0.0f, 0.0f, 1.0f), 0.07f);

  // 反撃ガイド
  bool isPad = GamePadInput::IsConnected();
  const char *guideStr = isPad
      ? "[X] 左レーン  |  [Y] 中央レーン  |  [B] 右レーン (魚を押して敵へ飛ばす)"
      : "[1] 左レーン  |  [2] 中央レーン  |  [3] 右レーン (魚を押して敵へ飛ばす)";
  const float guideSize = kBossGuideFontSize;
  float subGuideW = tr ? tr->MeasureString(guideStr, guideSize).x : 680.0f;
  float subGuideX = panelX + (panelW - subGuideW) * 0.5f;
  draw.DrawMSDFString(guideStr, Vector2(subGuideX, kBossGuideY), guideSize,
                      Vector4(0.8f, 0.95f, 0.5f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  // 反撃チャンスのアラート点滅表示
  if (ctx.reflectLane != -1) {
    const char *keyName = "";
    if (isPad) {
      keyName = (ctx.reflectLane == 0)   ? "Xボタン"
                : (ctx.reflectLane == 1) ? "Yボタン"
                                         : "Bボタン";
    } else {
      keyName = (ctx.reflectLane == 0)   ? "1キー"
                : (ctx.reflectLane == 1) ? "2キー"
                                         : "3キー";
    }
    const char *laneName = (ctx.reflectLane == 0)   ? "左レーン"
                           : (ctx.reflectLane == 1) ? "中央レーン"
                                                    : "右レーン";
    char alertBuf[96];
    snprintf(alertBuf, sizeof(alertBuf),
             ">>> 反撃チャンス！ [%s] で%sの魚を敵に飛ばせ！ <<<", keyName,
             laneName);

    float pulseScale = kBossAlertPulseBase + kBossAlertPulseAmp * std::sin(ctx.uiTimer * kBossAlertPulseFreq);
    const float alertW = kBossAlertWidth;
    const float screenCenterX = static_cast<float>(WindowConfig::kDefaultClientWidth) * 0.5f;
    const float alertX = screenCenterX - alertW * 0.5f;
    draw.DrawFillRect(Vector2(alertX, kBossAlertY), Vector2(alertW, kBossAlertHeight),
                      Vector4(0.2f, 0.1f, 0.0f, 0.85f));
    float alertTextW = tr ? tr->MeasureString(alertBuf, kBossAlertFontSize).x : 670.0f;
    float alertTextX = alertX + (alertW - alertTextW) * 0.5f;
    draw.DrawMSDFString(alertBuf, Vector2(alertTextX, kBossAlertTextY), kBossAlertFontSize,
                        Vector4(1.0f, 0.95f, 0.15f, pulseScale), true,
                        Vector4(0.5f, 0.1f, 0.0f, 1.0f), 0.09f);
  }

  // 右下: ボス倒し方（攻略ガイド）パネル
  {
    const float guideX = kBossHowToPanelPos.x;
    const float guideY = kBossHowToPanelPos.y;
    const float guideW = kBossHowToPanelSize.x;
    const float guideH = kBossHowToPanelSize.y;

    draw.DrawFillRect(Vector2(guideX, guideY), Vector2(guideW, guideH),
                      Vector4(0.08f, 0.03f, 0.04f, 0.88f));
    draw.DrawFillRect(Vector2(guideX, guideY), Vector2(guideW, kPanelBorderHeight),
                      Vector4(1.0f, 0.45f, 0.2f, 0.95f));

    draw.DrawMSDFString("【 ボスの倒し方 】",
                        Vector2(guideX + 16.0f, guideY + 12.0f), 22.0f,
                        Vector4(1.0f, 0.88f, 0.25f, 1.0f), true,
                        Vector4(0.3f, 0.05f, 0.0f, 1.0f), 0.12f, 0.08f);
    draw.DrawMSDFString("HOW TO DEFEAT",
                        Vector2(guideX + 225.0f, guideY + 16.0f), 15.0f,
                        Vector4(0.85f, 0.65f, 0.5f, 0.85f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.08f, 0.05f);

    draw.DrawFillRect(Vector2(guideX + 14.0f, guideY + 42.0f),
                      Vector2(guideW - 28.0f, 1.0f),
                      Vector4(0.5f, 0.25f, 0.2f, 0.65f));

    draw.DrawMSDFString("① 障害物をよけて進め！",
                        Vector2(guideX + 16.0f, guideY + 52.0f), 19.0f,
                        Vector4(1.0f, 0.5f, 0.5f, 1.0f), true,
                        Vector4(0.2f, 0.0f, 0.0f, 1.0f), 0.10f, 0.06f);
    draw.DrawMSDFString("   移動で回避",
                        Vector2(guideX + 16.0f, guideY + 77.0f), 16.0f,
                        Vector4(0.9f, 0.9f, 0.9f, 0.9f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.08f, 0.05f);

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

    draw.DrawMSDFString("③ 魚を敵に飛ばして撃破！",
                        Vector2(guideX + 16.0f, guideY + 156.0f), 19.0f,
                        Vector4(1.0f, 0.8f, 0.25f, 1.0f), true,
                        Vector4(0.2f, 0.1f, 0.0f, 1.0f), 0.10f, 0.06f);
    draw.DrawMSDFString("   飛ばした魚を当ててHPを削り切れ！",
                        Vector2(guideX + 16.0f, guideY + 181.0f), 16.0f,
                        Vector4(0.95f, 0.95f, 0.95f, 0.9f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.08f, 0.05f);
  }

  // ボス登場時作戦指令バナー
  if (ctx.boss->GetState() == BossState::Appearance) {
    float alpha = std::clamp(ctx.boss->GetStateTimer() * 2.0f, 0.0f, 1.0f);
    draw.DrawFillRect(kBossMissionBannerPos, kBossMissionBannerSize,
                      Vector4(0.06f, 0.02f, 0.02f, 0.94f * alpha));
    draw.DrawFillRect(kBossMissionBannerPos, Vector2(kBossMissionBannerSize.x, kPauseAccentHeight),
                      Vector4(1.0f, 0.8f, 0.2f, 0.95f * alpha));
    draw.DrawFillRect(Vector2(kBossMissionBannerPos.x, kBossMissionBannerBorderY), Vector2(kBossMissionBannerSize.x, kPauseAccentHeight),
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

  // ボス撃破バナー
  if (ctx.boss->GetState() == BossState::Defeat) {
    draw.DrawFillRect(kBossDefeatPanelPos, kBossDefeatPanelSize,
                      Vector4(0.05f, 0.12f, 0.05f, 0.9f));
    draw.DrawFillRect(kBossDefeatPanelPos, Vector2(kBossDefeatPanelSize.x, kPanelBorderHeight),
                      Vector4(1.0f, 0.8f, 0.2f, 0.95f));
    draw.DrawMSDFString("★ BOSS DEFEATED! ★", Vector2(430.0f, 185.0f), kBossDefeatTitleFontSize,
                        Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.10f);
    draw.DrawMSDFString("撃破ボーナス獲得！", Vector2(510.0f, 230.0f), kBossDefeatBonusFontSize,
                        Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  }
}

void GameHUD::DrawControlsGuide(Draw &draw, const GameHUDContext &ctx, float alpha) {
  if (alpha <= 0.01f) return;

  bool isPad = GamePadInput::IsConnected();

  draw.DrawFillRect(kControlsBarPos, kControlsBarSize,
                    Vector4(0.03f, 0.06f, 0.11f, 0.88f * alpha));
  draw.DrawFillRect(kControlsBarPos, Vector2(kControlsBarSize.x, kControlsBarBorderH),
                    Vector4(0.3f, 0.6f, 0.9f, 0.85f * alpha));

  const char *badgeText = isPad ? "PAD" : "KEYBOARD";
  float badgeW = isPad ? kControlsBadgeWidthPad : kControlsBadgeWidthKey;
  draw.DrawFillRect(kControlsBadgePos, Vector2(badgeW + 2.0f, kControlsBadgeHeight),
                    Vector4(0.35f, 0.40f, 0.48f, 0.75f * alpha));
  draw.DrawFillRect(Vector2(kControlsBadgePos.x + 1.0f, kControlsBadgePos.y + 1.0f), Vector2(badgeW, kControlsBadgeInnerH),
                    Vector4(0.07f, 0.09f, 0.13f, 0.95f * alpha));

  Vector4 badgeColor = isPad ? Vector4(0.35f, 1.0f, 0.75f, alpha)
                             : Vector4(1.0f, 0.88f, 0.35f, alpha);
  float badgeTextX = isPad ? kControlsBadgePadTextX : kControlsBadgeKeyTextX;
  draw.DrawMSDFString(badgeText, Vector2(badgeTextX, kControlsBadgeTextY), kControlsBadgeFontSize,
                      badgeColor, true,
                      Vector4(0.0f, 0.0f, 0.0f, alpha), 0.08f);

  const char *guideText = "";
  if (isPad) {
    if (ctx.playingState == PlayState_ThreeLane || ctx.gameState == State_Title) {
      guideText = "移動: [Lスティック / 十字]    ジャンプ: [Aボタン]    スライド: [Bボタン]    ポーズ: [START]";
    } else if (ctx.playingState == PlayState_OneLane) {
      guideText = "ジャンプ: [Aボタン]    スライド: [Bボタン]    ポーズ: [START]    (※1レーン固定中)";
    } else if (ctx.playingState == PlayState_Boss) {
      guideText = "【ボス反撃】 魚跳ね返し: [X]左  [Y]中  [B]右    |    移動: [Lスティック]    |    ポーズ: [START]";
    }
  } else {
    if (ctx.playingState == PlayState_ThreeLane || ctx.gameState == State_Title) {
      guideText = "移動: [A/D / ← →]    ジャンプ: [SPACE / W]    スライド: [S / ↓]    ポーズ: [ESC]";
    } else if (ctx.playingState == PlayState_OneLane) {
      guideText = "ジャンプ: [SPACE / W]    スライド: [S / ↓]    ポーズ: [ESC]    (※1レーン固定中)";
    } else if (ctx.playingState == PlayState_Boss) {
      guideText = "【ボス反撃】 魚跳ね返し: [1]左  [2]中  [3]右    |    移動: [A/D]    |    ポーズ: [ESC]";
    }
  }

  float guideTextX = kControlsBadgePos.x + 1.0f + badgeW + 14.0f;
  draw.DrawMSDFString(guideText, Vector2(guideTextX, kControlsBadgeTextY), kControlsGuideFontSize,
                      Vector4(0.92f, 0.96f, 1.0f, 0.95f * alpha), true,
                      Vector4(0.0f, 0.0f, 0.0f, alpha), 0.06f);
}

void GameHUD::DrawPauseHUD(Draw &draw, const GameHUDContext &ctx) {
  const Vector2 fullScreenSize(static_cast<float>(WindowConfig::kDefaultClientWidth),
                               static_cast<float>(WindowConfig::kDefaultClientHeight));
  draw.DrawFillRect(Vector2(0.0f, 0.0f), fullScreenSize,
                    Vector4(0.0f, 0.0f, 0.0f, kPauseDimAlpha));

  draw.DrawFillRect(kPausePanelPos, kPausePanelSize,
                    Vector4(0.06f, 0.08f, 0.12f, 0.94f));
  draw.DrawFillRect(kPausePanelPos, Vector2(kPausePanelSize.x, kPauseAccentHeight),
                    Vector4(1.0f, 0.8f, 0.2f, 0.95f));

  draw.DrawMSDFString("=== PAUSE ===", kPauseTitlePos, kPauseTitleFontSize,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.10f);

  draw.DrawMSDFString("ゲーム一時停止中", kPauseSubTitlePos, kPauseSubTitleFontSize,
                      Vector4(0.85f, 0.85f, 0.85f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  draw.DrawFillRect(kPauseDividerPos1, kPauseDividerSize,
                    Vector4(0.3f, 0.4f, 0.5f, 0.7f));

  char pDistBuf[64];
  snprintf(pDistBuf, sizeof(pDistBuf), "現在の到達距離:  %.1f m", ctx.currentDistance);
  draw.DrawMSDFString(pDistBuf, kPauseDistPos, kPauseStatFontSize,
                      Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  char pScoreBuf[64];
  snprintf(pScoreBuf, sizeof(pScoreBuf), "現在のスコア:    %.0f pt", ctx.currentScore);
  draw.DrawMSDFString(pScoreBuf, kPauseScorePos, kPauseStatFontSize,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  draw.DrawFillRect(kPauseDividerPos2, kPauseDividerSize,
                    Vector4(0.3f, 0.4f, 0.5f, 0.7f));

  bool isPausePad = GamePadInput::IsConnected();
  if (isPausePad) {
    draw.DrawMSDFString("[ START ] ゲームを再開 (RESUME)", kPauseBtnResumePos,
                        kPauseBtnFontSize, Vector4(0.3f, 0.9f, 1.0f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
    draw.DrawMSDFString("[ Aボタン ] 最初からリスタート (RESTART)",
                        kPauseBtnRestartPos, kPauseBtnFontSize,
                        Vector4(0.9f, 0.9f, 0.9f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
    draw.DrawMSDFString("[ Bボタン ] タイトルへ戻る (TITLE)", kPauseBtnTitlePos,
                        kPauseBtnFontSize, Vector4(0.9f, 0.9f, 0.9f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  } else {
    draw.DrawMSDFString("[ ESC ] ゲームを再開 (RESUME)", kPauseBtnResumePos,
                        kPauseBtnFontSize, Vector4(0.3f, 0.9f, 1.0f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
    draw.DrawMSDFString("[  1  ] 最初からリスタート (RESTART)",
                        kPauseBtnRestartPos, kPauseBtnFontSize,
                        Vector4(0.9f, 0.9f, 0.9f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
    draw.DrawMSDFString("[  2  ] タイトルへ戻る (TITLE)", kPauseBtnTitlePos,
                        kPauseBtnFontSize, Vector4(0.9f, 0.9f, 0.9f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  }
}

void GameHUD::DrawGameOverHUD(Draw &draw, const GameHUDContext &ctx) {
  const Vector2 fullScreenSize(static_cast<float>(WindowConfig::kDefaultClientWidth),
                               static_cast<float>(WindowConfig::kDefaultClientHeight));
  draw.DrawFillRect(Vector2(0.0f, 0.0f), fullScreenSize,
                    Vector4(0.0f, 0.0f, 0.0f, kGameOverDimAlpha));

  draw.DrawFillRect(kGameOverPanelPos, kGameOverPanelSize,
                    Vector4(0.06f, 0.08f, 0.12f, 0.95f));
  draw.DrawFillRect(kGameOverPanelPos, Vector2(kGameOverPanelSize.x, kGameOverAccentHeight),
                    Vector4(0.9f, 0.2f, 0.2f, 0.95f));

  draw.DrawMSDFString("GAME OVER", kGameOverTitlePos, kGameOverTitleFontSize,
                      Vector4(1.0f, 0.25f, 0.25f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.10f);

  float topDist0 = ctx.topRankings ? ctx.topRankings[0] : 0.0f;
  float topScore0 = ctx.topScoreRankings ? ctx.topScoreRankings[0] : 0.0f;
  bool isNewDistRecord = (topDist0 > 0.0f && ctx.currentDistance >= topDist0);
  bool isNewScoreRecord = (topScore0 > 0.0f && ctx.currentScore >= topScore0);

  char distBuf[96];
  snprintf(distBuf, sizeof(distBuf), "到達距離: %.1f m  %s", ctx.currentDistance,
           isNewDistRecord ? "★ NEW RECORD! ★" : "");
  draw.DrawMSDFString(distBuf, kGameOverDistPos, kGameOverDistFontSize,
                      isNewDistRecord ? Vector4(1.0f, 0.9f, 0.2f, 1.0f)
                                      : Vector4(1.0f, 1.0f, 1.0f, 1.0f),
                      true, Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  char scoreBuf[96];
  snprintf(scoreBuf, sizeof(scoreBuf), "最終スコア: %.0f pt  %s", ctx.currentScore,
           isNewScoreRecord ? "★ NEW RECORD! ★" : "");
  draw.DrawMSDFString(scoreBuf, kGameOverScorePos, kGameOverScoreFontSize,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  char bonusBuf[64];
  snprintf(bonusBuf, sizeof(bonusBuf), "ボーナス敵撃破: %d 体",
           ctx.bonusEnemyHitCount);
  draw.DrawMSDFString(bonusBuf, kGameOverBonusPos, kGameOverBonusFontSize,
                      Vector4(0.6f, 0.9f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.06f);

  draw.DrawFillRect(kGameOverDividerPos1, kGameOverDividerSize,
                    Vector4(0.3f, 0.4f, 0.5f, 0.7f));

  // ==========================================
  // ランキング表示アイテム構造体 & リスト構築
  // ==========================================
  struct DisplayRankItem {
    int rank = 0;
    std::string playerName;
    float value = 0.0f;
    bool isCurrentPlay = false;
  };

  // 走行距離リストの統合構築
  auto buildDistanceList = [&]() {
    std::vector<DisplayRankItem> list;
    bool alreadyInList = false;

    if (ctx.onlineDistanceRankings && !ctx.onlineDistanceRankings->empty()) {
      for (const auto& e : *ctx.onlineDistanceRankings) {
        DisplayRankItem item;
        item.playerName = e.playerName;
        item.value = e.distance;
        bool isMe = (!ctx.lastSubmittedDocId.empty() && e.documentId == ctx.lastSubmittedDocId) ||
                    (e.playerName == ctx.playerName && std::abs(e.distance - ctx.currentDistance) < 0.05f);
        if (isMe && !alreadyInList) {
          item.isCurrentPlay = true;
          alreadyInList = true;
        }
        list.push_back(item);
      }
    } else if (ctx.topRankings) {
      for (int i = 0; i < 3; i++) {
        float d = ctx.topRankings[i];
        if (d > 0.0f) {
          DisplayRankItem item;
          item.playerName = ctx.playerName;
          item.value = d;
          if (!alreadyInList && (i + 1 == ctx.localDistanceRank) && std::abs(d - ctx.currentDistance) < 0.05f) {
            item.isCurrentPlay = true;
            alreadyInList = true;
          }
          list.push_back(item);
        }
      }
    }

    // クラウド反映待ちや未反映の場合、今回のプレイ記録を統合して即時反映
    if (!alreadyInList && ctx.currentDistance > 0.0f) {
      DisplayRankItem myItem;
      myItem.playerName = ctx.playerName.empty() ? "Player" : ctx.playerName;
      myItem.value = ctx.currentDistance;
      myItem.isCurrentPlay = true;
      list.push_back(myItem);
    }

    // 降順ソート
    std::stable_sort(list.begin(), list.end(), [](const DisplayRankItem& a, const DisplayRankItem& b) {
      return a.value > b.value;
    });

    for (size_t i = 0; i < list.size(); i++) {
      list[i].rank = static_cast<int>(i + 1);
    }
    return list;
  };

  // スコアリストの統合構築
  auto buildScoreList = [&]() {
    std::vector<DisplayRankItem> list;
    bool alreadyInList = false;

    if (ctx.onlineScoreRankings && !ctx.onlineScoreRankings->empty()) {
      for (const auto& e : *ctx.onlineScoreRankings) {
        DisplayRankItem item;
        item.playerName = e.playerName;
        item.value = e.score;
        bool isMe = (!ctx.lastSubmittedDocId.empty() && e.documentId == ctx.lastSubmittedDocId) ||
                    (e.playerName == ctx.playerName && std::abs(e.score - ctx.currentScore) < 0.5f);
        if (isMe && !alreadyInList) {
          item.isCurrentPlay = true;
          alreadyInList = true;
        }
        list.push_back(item);
      }
    } else if (ctx.topScoreRankings) {
      for (int i = 0; i < 3; i++) {
        float s = ctx.topScoreRankings[i];
        if (s > 0.0f) {
          DisplayRankItem item;
          item.playerName = ctx.playerName;
          item.value = s;
          if (!alreadyInList && (i + 1 == ctx.localScoreRank) && std::abs(s - ctx.currentScore) < 0.5f) {
            item.isCurrentPlay = true;
            alreadyInList = true;
          }
          list.push_back(item);
        }
      }
    }

    // クラウド反映待ちや未反映の場合、今回のプレイ記録を統合して即時反映
    if (!alreadyInList && ctx.currentScore > 0.0f) {
      DisplayRankItem myItem;
      myItem.playerName = ctx.playerName.empty() ? "Player" : ctx.playerName;
      myItem.value = ctx.currentScore;
      myItem.isCurrentPlay = true;
      list.push_back(myItem);
    }

    // 降順ソート
    std::stable_sort(list.begin(), list.end(), [](const DisplayRankItem& a, const DisplayRankItem& b) {
      return a.value > b.value;
    });

    for (size_t i = 0; i < list.size(); i++) {
      list[i].rank = static_cast<int>(i + 1);
    }
    return list;
  };

  // ==========================================
  // 左カラム: 走行距離ランキング TOP 3
  // ==========================================
  bool isDistLoading = ctx.isOnlineRankingConfigured && (ctx.isOnlineRankingSending || ctx.isOnlineRankingFetching);
  char distHeader[96];
  if (isDistLoading) {
    snprintf(distHeader, sizeof(distHeader), "【 走行距離ランキング (通信中...) 】");
  } else {
    snprintf(distHeader, sizeof(distHeader), "【 走行距離ランキング TOP 3 】");
  }
  draw.DrawMSDFString(distHeader, kGameOverRankHeaderDistPos,
                      kGameOverRankHeaderFontSize, Vector4(0.4f, 0.85f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  auto distList = buildDistanceList();
  int distMyRank = -1;
  bool isDistRankIn = false;
  for (const auto& item : distList) {
    if (item.isCurrentPlay) {
      distMyRank = item.rank;
      if (distMyRank <= 3) {
        isDistRankIn = true;
      }
      break;
    }
  }

  // 走行距離 TOP 3 リスト描画
  int distDisplayCount = std::min(3, static_cast<int>(distList.size()));
  for (int i = 0; i < distDisplayCount; i++) {
    const auto& item = distList[i];
    char rBuf[96];
    if (item.isCurrentPlay) {
      snprintf(rBuf, sizeof(rBuf), "★%d位: %-8s  %.1f m", item.rank, item.playerName.c_str(), item.value);
    } else {
      snprintf(rBuf, sizeof(rBuf), " %d位: %-8s  %.1f m", item.rank, item.playerName.c_str(), item.value);
    }

    Vector2 itemPos(kGameOverRankItemDistX, kGameOverRankItemBaseY + i * kGameOverRankItemStepY);
    if (item.isCurrentPlay) {
      draw.DrawFillRect(Vector2(itemPos.x - 6.0f, itemPos.y - 2.0f), Vector2(365.0f, 30.0f),
                        Vector4(0.2f, 0.45f, 0.8f, 0.28f));
      draw.DrawMSDFString(rBuf, itemPos, kGameOverRankItemFontSize + 1.5f,
                          Vector4(1.0f, 0.95f, 0.25f, 1.0f), true,
                          Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.16f, 0.14f);
    } else {
      Vector4 rankColor = (i == 0)   ? Vector4(1.0f, 0.9f, 0.2f, 1.0f)
                          : (i == 1) ? Vector4(0.85f, 0.85f, 0.9f, 1.0f)
                                     : Vector4(0.85f, 0.65f, 0.45f, 1.0f);
      draw.DrawMSDFString(rBuf, itemPos, kGameOverRankItemFontSize,
                          rankColor, true, Vector4(0.0f, 0.0f, 0.0f, 1.0f),
                          0.06f, 0.0f);
    }
  }
  for (int i = distDisplayCount; i < 3; i++) {
    char rBuf[64];
    snprintf(rBuf, sizeof(rBuf), " %d位: ---", i + 1);
    draw.DrawMSDFString(rBuf, Vector2(kGameOverRankItemDistX, kGameOverRankItemBaseY + i * kGameOverRankItemStepY),
                        kGameOverRankItemFontSize,
                        Vector4(0.6f, 0.6f, 0.6f, 1.0f), true, Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.06f, 0.0f);
  }

  // 走行距離ランキング下の表示
  {
    Vector2 boxPos(kGameOverRankItemDistX - 6.0f, kGameOverMyRankBoxY);
    draw.DrawFillRect(boxPos, kGameOverMyRankBoxSize, Vector4(0.04f, 0.07f, 0.13f, 0.85f));
    draw.DrawFillRect(boxPos, Vector2(3.0f, kGameOverMyRankBoxSize.y), Vector4(0.3f, 0.7f, 1.0f, 0.9f));

    char myDistBuf[96];
    if (isDistRankIn) {
      snprintf(myDistBuf, sizeof(myDistBuf), "★ 走行距離 %d位 ランクイン達成！", distMyRank);
      draw.DrawMSDFString(myDistBuf, Vector2(boxPos.x + 12.0f, boxPos.y + kGameOverMyRankTextOffsetY),
                          kGameOverMyRankFontSize, Vector4(1.0f, 0.92f, 0.3f, 1.0f), true,
                          Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.10f, 0.10f);
    } else {
      if (distMyRank > 3) {
        snprintf(myDistBuf, sizeof(myDistBuf), "あなたの順位:  %d位  (%.1f m)", distMyRank, ctx.currentDistance);
      } else {
        snprintf(myDistBuf, sizeof(myDistBuf), "あなたの順位:  圏外  (%.1f m)", ctx.currentDistance);
      }
      draw.DrawMSDFString(myDistBuf, Vector2(boxPos.x + 12.0f, boxPos.y + kGameOverMyRankTextOffsetY),
                          kGameOverMyRankFontSize, Vector4(0.75f, 0.88f, 1.0f, 1.0f), true,
                          Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f, 0.04f);
    }
  }

  // ==========================================
  // 右カラム: スコアランキング TOP 3
  // ==========================================
  bool isScoreLoading = ctx.isOnlineRankingConfigured && (ctx.isOnlineRankingSending || ctx.isOnlineRankingFetching);
  char scoreHeader[96];
  if (isScoreLoading) {
    snprintf(scoreHeader, sizeof(scoreHeader), "【 スコアランキング (通信中...) 】");
  } else {
    snprintf(scoreHeader, sizeof(scoreHeader), "【 スコアランキング TOP 3 】");
  }
  draw.DrawMSDFString(scoreHeader, kGameOverRankHeaderScorePos,
                      kGameOverRankHeaderFontSize, Vector4(1.0f, 0.85f, 0.3f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  auto scoreList = buildScoreList();
  int scoreMyRank = -1;
  bool isScoreRankIn = false;
  for (const auto& item : scoreList) {
    if (item.isCurrentPlay) {
      scoreMyRank = item.rank;
      if (scoreMyRank <= 3) {
        isScoreRankIn = true;
      }
      break;
    }
  }

  // スコア TOP 3 リスト描画
  int scoreDisplayCount = std::min(3, static_cast<int>(scoreList.size()));
  for (int i = 0; i < scoreDisplayCount; i++) {
    const auto& item = scoreList[i];
    char rBuf[96];
    if (item.isCurrentPlay) {
      snprintf(rBuf, sizeof(rBuf), "★%d位: %-8s  %.0f pt", item.rank, item.playerName.c_str(), item.value);
    } else {
      snprintf(rBuf, sizeof(rBuf), " %d位: %-8s  %.0f pt", item.rank, item.playerName.c_str(), item.value);
    }

    Vector2 itemPos(kGameOverRankItemScoreX, kGameOverRankItemBaseY + i * kGameOverRankItemStepY);
    if (item.isCurrentPlay) {
      draw.DrawFillRect(Vector2(itemPos.x - 6.0f, itemPos.y - 2.0f), Vector2(365.0f, 30.0f),
                        Vector4(0.8f, 0.45f, 0.1f, 0.28f));
      draw.DrawMSDFString(rBuf, itemPos, kGameOverRankItemFontSize + 1.5f,
                          Vector4(1.0f, 0.95f, 0.25f, 1.0f), true,
                          Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.16f, 0.14f);
    } else {
      Vector4 rankColor = (i == 0)   ? Vector4(1.0f, 0.9f, 0.2f, 1.0f)
                          : (i == 1) ? Vector4(0.85f, 0.85f, 0.9f, 1.0f)
                                     : Vector4(0.85f, 0.65f, 0.45f, 1.0f);
      draw.DrawMSDFString(rBuf, itemPos, kGameOverRankItemFontSize,
                          rankColor, true, Vector4(0.0f, 0.0f, 0.0f, 1.0f),
                          0.06f, 0.0f);
    }
  }
  for (int i = scoreDisplayCount; i < 3; i++) {
    char rBuf[64];
    snprintf(rBuf, sizeof(rBuf), " %d位: ---", i + 1);
    draw.DrawMSDFString(rBuf, Vector2(kGameOverRankItemScoreX, kGameOverRankItemBaseY + i * kGameOverRankItemStepY),
                        kGameOverRankItemFontSize,
                        Vector4(0.6f, 0.6f, 0.6f, 1.0f), true, Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.06f, 0.0f);
  }

  // スコアランキング下の表示
  {
    Vector2 boxPos(kGameOverRankItemScoreX - 6.0f, kGameOverMyRankBoxY);
    draw.DrawFillRect(boxPos, kGameOverMyRankBoxSize, Vector4(0.04f, 0.07f, 0.13f, 0.85f));
    draw.DrawFillRect(boxPos, Vector2(3.0f, kGameOverMyRankBoxSize.y), Vector4(1.0f, 0.75f, 0.2f, 0.9f));

    char myScoreBuf[96];
    if (isScoreRankIn) {
      snprintf(myScoreBuf, sizeof(myScoreBuf), "★ スコア %d位 ランクイン達成！", scoreMyRank);
      draw.DrawMSDFString(myScoreBuf, Vector2(boxPos.x + 12.0f, boxPos.y + kGameOverMyRankTextOffsetY),
                          kGameOverMyRankFontSize, Vector4(1.0f, 0.92f, 0.3f, 1.0f), true,
                          Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.10f, 0.10f);
    } else {
      if (scoreMyRank > 3) {
        snprintf(myScoreBuf, sizeof(myScoreBuf), "あなたの順位:  %d位  (%.0f pt)", scoreMyRank, ctx.currentScore);
      } else {
        snprintf(myScoreBuf, sizeof(myScoreBuf), "あなたの順位:  圏外  (%.0f pt)", ctx.currentScore);
      }
      draw.DrawMSDFString(myScoreBuf, Vector2(boxPos.x + 12.0f, boxPos.y + kGameOverMyRankTextOffsetY),
                          kGameOverMyRankFontSize, Vector4(1.0f, 0.92f, 0.75f, 1.0f), true,
                          Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f, 0.04f);
    }
  }

  draw.DrawFillRect(kGameOverDividerPos2, kGameOverDividerSize,
                    Vector4(0.3f, 0.4f, 0.5f, 0.7f));

  bool isOverPad = GamePadInput::IsConnected();
  if (isOverPad) {
    draw.DrawMSDFString("[ Aボタン ] もう一度プレイ (RESTART)   |   [ Bボタン ] "
                        "タイトルへ戻る (TITLE)",
                        kGameOverBottomBtnPos, kGameOverBottomBtnFontSize,
                        Vector4(0.9f, 0.95f, 1.0f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  } else {
    draw.DrawMSDFString("[ 1キー ] もう一度プレイ (RESTART)   |   [ 2キー ] "
                        "タイトルへ戻る (TITLE)",
                        kGameOverBottomBtnPos, kGameOverBottomBtnFontSize,
                        Vector4(0.9f, 0.95f, 1.0f, 1.0f), true,
                        Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  }
}

void GameHUD::DrawTitleHUD(Draw &draw, const GameHUDContext &ctx) {
  const std::string titleText = "ペンギンダッシュ";
  const std::string subTitleText = "― PENGUIN DASH ―";
  bool isTitlePad = GamePadInput::IsConnected();
  const std::string startText = isTitlePad ? "PRESS [A] BUTTON TO START" : "PRESS SPACE TO START";

  const float fontSize = kTitleMainFontSize;
  const float subFontSize = kTitleSubFontSize;
  const float startFontSize = kTitleStartFontSize;

  float titleWidth = 0.0f;
  float subWidth = 0.0f;
  float startWidth = 0.0f;
  if (draw.GetTextRenderer()) {
    titleWidth = draw.GetTextRenderer()->MeasureString(titleText, fontSize).x;
    subWidth = draw.GetTextRenderer()->MeasureString(subTitleText, subFontSize).x;
    startWidth = draw.GetTextRenderer()->MeasureString(startText, startFontSize).x;
  }
  if (titleWidth <= 0.0f) titleWidth = kTitleDefaultWidth;
  if (subWidth <= 0.0f) subWidth = kTitleDefaultSubWidth;
  if (startWidth <= 0.0f) startWidth = kTitleDefaultStartWidth;

  const float screenWidth = static_cast<float>(WindowConfig::kDefaultClientWidth);
  float titleX = (screenWidth - titleWidth) * 0.5f;
  float subX = (screenWidth - subWidth) * 0.5f;
  float startX = (screenWidth - startWidth) * 0.5f;

  const float baseTitleY = kTitleBaseY;
  const float baseSubY = baseTitleY + kTitleSubOffsetY;
  const float baseStartY = kTitleStartBaseY;

  float currentTitleY = baseTitleY;
  float currentSubY = baseSubY;
  float currentStartY = baseStartY;

  float mainAlpha = 1.0f;
  float subAlpha = 0.95f;
  float startAlpha = 1.0f;

  if (ctx.isTitleExiting) {
    float progress =
        std::clamp(ctx.titleExitTimer / ctx.titleExitDuration, 0.0f, 1.0f);
    float ease = 1.0f - std::pow(1.0f - progress, kTitleFlyEaseExponent);
    float flyOffset = ease * kTitleFlyOffset;

    currentTitleY = baseTitleY - flyOffset;
    currentSubY = baseSubY - flyOffset;
    currentStartY = baseStartY - ease * kTitleStartFlyOffset;

    mainAlpha = (progress < kTitleMainFadeStart) ? 1.0f : std::clamp(1.0f - (progress - kTitleMainFadeStart) / kTitleMainFadeSpan, 0.0f, 1.0f);
    subAlpha = (progress < kTitleSubFadeStart) ? 0.95f : std::clamp(0.95f - (progress - kTitleSubFadeStart) / kTitleSubFadeSpan, 0.0f, 1.0f);
    startAlpha = std::clamp(1.0f - progress * kTitleStartFadeRate, 0.0f, 1.0f);
  } else {
    float hover = sinf(ctx.uiTimer * kTitleHoverFreq) * kTitleHoverAmp;
    currentTitleY = baseTitleY + hover;
    currentSubY = baseSubY + hover;

    float blink = sinf(ctx.uiTimer * kTitleBlinkFreq);
    startAlpha = (blink > kTitleBlinkThreshold) ? 1.0f : 0.0f;
  }

  if (mainAlpha > 0.01f) {
    Vector4 shadowColor = Vector4(0.01f, 0.04f, 0.12f, 0.8f * mainAlpha);
    draw.DrawMSDFString(titleText, Vector2(titleX + kTitleShadowOffset.x, currentTitleY + kTitleShadowOffset.y),
                        fontSize, shadowColor, false, {0.0f, 0.0f, 0.0f, 0.0f},
                        0.0f, 0.18f);

    Vector4 textColor = Vector4(0.95f, 0.98f, 1.0f, mainAlpha);
    Vector4 outlineColor = Vector4(0.06f, 0.16f, 0.35f, mainAlpha);
    draw.DrawMSDFString(titleText, Vector2(titleX, currentTitleY), fontSize,
                        textColor, true, outlineColor, 0.22f, 0.15f);
  }

  if (subAlpha > 0.01f) {
    Vector4 subColor = Vector4(0.65f, 0.88f, 1.0f, subAlpha);
    Vector4 subOutline = Vector4(0.04f, 0.1f, 0.24f, subAlpha);
    draw.DrawMSDFString(subTitleText, Vector2(subX, currentSubY), subFontSize,
                        subColor, true, subOutline, 0.18f, 0.08f);
  }

  if (startAlpha > 0.02f) {
    draw.DrawMSDFString(startText, Vector2(startX, currentStartY),
                        startFontSize, Vector4(1.0f, 1.0f, 1.0f, startAlpha),
                        true, Vector4(0.0f, 0.0f, 0.0f, startAlpha), 0.15f,
                        0.08f);
  }

  float guideAlpha = ctx.isTitleExiting
                         ? std::clamp(1.0f - (ctx.titleExitTimer / kTitleExitGuideDuration), 0.0f, 1.0f)
                         : mainAlpha;
  DrawControlsGuide(draw, ctx, guideAlpha);
}

void GameHUD::DrawEditorHUD(Draw &draw, const GameHUDContext &ctx) {
  draw.DrawFillRect(kEditorPanelPos, kEditorPanelSize,
                    Vector4(0.04f, 0.06f, 0.1f, 0.8f));
  char distBuf[64];
  snprintf(distBuf, sizeof(distBuf), "[EDITOR] 距離: %.1f m", ctx.currentDistance);
  draw.DrawMSDFString(distBuf, kEditorDistPos, kEditorDistFontSize,
                      Vector4(0.9f, 0.9f, 0.9f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  char scoreBuf[64];
  snprintf(scoreBuf, sizeof(scoreBuf), "スコア: %.0f pt", ctx.currentScore);
  draw.DrawMSDFString(scoreBuf, kEditorScorePos, kEditorScoreFontSize,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
}

void GameHUD::DrawBadgeFPV(Draw &draw) {
  draw.DrawFillRect(kFpvBadgePos, kFpvBadgeSize,
                    Vector4(0.04f, 0.10f, 0.18f, 0.85f));
  draw.DrawFillRect(kFpvBadgePos, Vector2(kFpvBorderWidth, kFpvBadgeSize.y),
                    Vector4(0.3f, 0.85f, 1.0f, 1.0f));
  draw.DrawMSDFString("[F2] 1ST PERSON", Vector2(kFpvBadgePos.x + kFpvTextOffset.x, kFpvBadgePos.y + kFpvTextOffset.y), kFpvFontSize,
                      Vector4(0.4f, 0.9f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.08f);
}

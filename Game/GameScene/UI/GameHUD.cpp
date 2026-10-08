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
        "ろ戦指令障害物魚押飛進残移動来固定再開初終体視替人称接続");
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
  const float panelX = 20.0f;
  const float panelY = 15.0f;
  const float panelW = 265.0f;
  const float panelH = 145.0f;

  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, panelH),
                    Vector4(0.04f, 0.07f, 0.12f, 0.82f));
  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, 3.0f),
                    Vector4(0.2f, 0.6f, 0.9f, 0.9f));

  const float textX = panelX + 12.0f;

  char distBuf[64];
  snprintf(distBuf, sizeof(distBuf), "距離: %.1f m", ctx.currentDistance);
  draw.DrawMSDFString(distBuf, Vector2(textX, panelY + 9.0f), 30.0f,
                      Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                      Vector4(0.05f, 0.15f, 0.25f, 1.0f), 0.08f);

  char scoreBuf[96];
  if (ctx.bonusEnemyHitCount > 0) {
    snprintf(scoreBuf, sizeof(scoreBuf), "スコア: %.0f pt  (+ボーナスx%d)",
             ctx.currentScore, ctx.bonusEnemyHitCount);
  } else {
    snprintf(scoreBuf, sizeof(scoreBuf), "スコア: %.0f pt", ctx.currentScore);
  }
  draw.DrawMSDFString(scoreBuf, Vector2(textX, panelY + 47.0f), 24.0f,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.15f, 0.10f, 0.0f, 1.0f), 0.08f);

  // スピードメーター
  float currentSpeed = ctx.scrollSpeed;
  float baseSpeed = ctx.baseScrollSpeed;
  float maxSpeed = ctx.maxScrollSpeed;
  float speedKm = currentSpeed * 300.0f;
  float speedRatio = (maxSpeed > baseSpeed)
                         ? ((currentSpeed - baseSpeed) / (maxSpeed - baseSpeed))
                         : 0.0f;
  if (speedRatio < 0.0f) speedRatio = 0.0f;
  if (speedRatio > 1.0f) speedRatio = 1.0f;

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

  // ベスト記録
  char bestText[96];
  float topDist = ctx.topRankings ? ctx.topRankings[0] : 0.0f;
  float topScore = ctx.topScoreRankings ? ctx.topScoreRankings[0] : 0.0f;
  snprintf(bestText, sizeof(bestText), "BEST: %.1f m  /  %.0f pt",
           topDist, topScore);
  draw.DrawMSDFString(bestText, Vector2(textX, panelY + 111.0f), 17.0f,
                      Vector4(0.8f, 0.85f, 0.9f, 0.85f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.06f);

  // 右上: 1レーン時のみ残り距離を表示
  if (ctx.playingState == PlayState_OneLane) {
    float remainDist = (200.0f - ctx.rightSideDistance > 0.0f)
                           ? (200.0f - ctx.rightSideDistance)
                           : 0.0f;
    char oneLaneBuf[64];
    snprintf(oneLaneBuf, sizeof(oneLaneBuf), "[ 1-LANE DASH: 残り %.0f m ]",
             remainDist);

    const float rightW = 275.0f;
    const float rightH = 46.0f;
    const float rightX = 1260.0f - rightW;
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

void GameHUD::DrawBossHUD(Draw &draw, const GameHUDContext &ctx) {
  if (!ctx.boss) return;

  const float panelX = 320.0f;
  const float panelY = 15.0f;
  const float panelW = 640.0f;
  const float panelH = 115.0f;

  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, panelH),
                    Vector4(0.10f, 0.03f, 0.03f, 0.85f));
  draw.DrawFillRect(Vector2(panelX, panelY), Vector2(panelW, 3.0f),
                    Vector4(0.9f, 0.2f, 0.2f, 0.9f));

  auto *tr = draw.GetTextRenderer();

  // ボスヘッダー
  const char *titleStr = "=== BOSS: ICE KING ===";
  const float titleSize = 28.0f;
  float titleW = tr ? tr->MeasureString(titleStr, titleSize).x : 307.0f;
  float titleX = panelX + (panelW - titleW) * 0.5f;
  draw.DrawMSDFString(titleStr, Vector2(titleX, 24.0f), titleSize,
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
  const float hpSize = 22.0f;
  float hpW = tr ? tr->MeasureString(hpText, hpSize).x : 567.0f;
  float hpX = panelX + (panelW - hpW) * 0.5f;
  draw.DrawMSDFString(hpText, Vector2(hpX, 58.0f), hpSize,
                      Vector4(1.0f, 0.45f, 0.45f, 1.0f), true,
                      Vector4(0.2f, 0.0f, 0.0f, 1.0f), 0.07f);

  // 反撃ガイド
  bool isPad = GamePadInput::IsConnected();
  const char *guideStr = isPad
      ? "[X] 左レーン  |  [Y] 中央レーン  |  [B] 右レーン (魚を押して敵へ飛ばす)"
      : "[1] 左レーン  |  [2] 中央レーン  |  [3] 右レーン (魚を押して敵へ飛ばす)";
  const float guideSize = 20.0f;
  float subGuideW = tr ? tr->MeasureString(guideStr, guideSize).x : 680.0f;
  float subGuideX = panelX + (panelW - subGuideW) * 0.5f;
  draw.DrawMSDFString(guideStr, Vector2(subGuideX, 92.0f), guideSize,
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

    float pulseScale = 0.8f + 0.2f * std::sin(ctx.uiTimer * 12.0f);
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

  // 右下: ボス倒し方（攻略ガイド）パネル
  {
    const float guideX = 880.0f;
    const float guideY = 455.0f;
    const float guideW = 380.0f;
    const float guideH = 205.0f;

    draw.DrawFillRect(Vector2(guideX, guideY), Vector2(guideW, guideH),
                      Vector4(0.08f, 0.03f, 0.04f, 0.88f));
    draw.DrawFillRect(Vector2(guideX, guideY), Vector2(guideW, 3.0f),
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

  // ボス撃破バナー
  if (ctx.boss->GetState() == BossState::Defeat) {
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

void GameHUD::DrawControlsGuide(Draw &draw, const GameHUDContext &ctx, float alpha) {
  if (alpha <= 0.01f) return;

  bool isPad = GamePadInput::IsConnected();

  draw.DrawFillRect(Vector2(20.0f, 672.0f), Vector2(1240.0f, 36.0f),
                    Vector4(0.03f, 0.06f, 0.11f, 0.88f * alpha));
  draw.DrawFillRect(Vector2(20.0f, 672.0f), Vector2(1240.0f, 2.0f),
                    Vector4(0.3f, 0.6f, 0.9f, 0.85f * alpha));

  const char *badgeText = isPad ? "PAD" : "KEYBOARD";
  float badgeW = isPad ? 72.0f : 110.0f;
  draw.DrawFillRect(Vector2(25.0f, 675.0f), Vector2(badgeW + 2.0f, 28.0f),
                    Vector4(0.35f, 0.40f, 0.48f, 0.75f * alpha));
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

  float guideTextX = 26.0f + badgeW + 14.0f;
  draw.DrawMSDFString(guideText, Vector2(guideTextX, 680.0f), 17.5f,
                      Vector4(0.92f, 0.96f, 1.0f, 0.95f * alpha), true,
                      Vector4(0.0f, 0.0f, 0.0f, alpha), 0.06f);
}

void GameHUD::DrawPauseHUD(Draw &draw, const GameHUDContext &ctx) {
  const Vector2 fullScreenSize(static_cast<float>(WindowConfig::kDefaultClientWidth),
                               static_cast<float>(WindowConfig::kDefaultClientHeight));
  draw.DrawFillRect(Vector2(0.0f, 0.0f), fullScreenSize,
                    Vector4(0.0f, 0.0f, 0.0f, 0.65f));

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
  snprintf(pDistBuf, sizeof(pDistBuf), "現在の到達距離:  %.1f m", ctx.currentDistance);
  draw.DrawMSDFString(pDistBuf, Vector2(430.0f, 315.0f), 26.0f,
                      Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  char pScoreBuf[64];
  snprintf(pScoreBuf, sizeof(pScoreBuf), "現在のスコア:    %.0f pt", ctx.currentScore);
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

void GameHUD::DrawGameOverHUD(Draw &draw, const GameHUDContext &ctx) {
  const Vector2 fullScreenSize(static_cast<float>(WindowConfig::kDefaultClientWidth),
                               static_cast<float>(WindowConfig::kDefaultClientHeight));
  draw.DrawFillRect(Vector2(0.0f, 0.0f), fullScreenSize,
                    Vector4(0.0f, 0.0f, 0.0f, 0.72f));

  draw.DrawFillRect(Vector2(200.0f, 60.0f), Vector2(880.0f, 570.0f),
                    Vector4(0.06f, 0.08f, 0.12f, 0.95f));
  draw.DrawFillRect(Vector2(200.0f, 60.0f), Vector2(880.0f, 4.0f),
                    Vector4(0.9f, 0.2f, 0.2f, 0.95f));

  draw.DrawMSDFString("GAME OVER", Vector2(480.0f, 85.0f), 56.0f,
                      Vector4(1.0f, 0.25f, 0.25f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.10f);

  float topDist0 = ctx.topRankings ? ctx.topRankings[0] : 0.0f;
  float topScore0 = ctx.topScoreRankings ? ctx.topScoreRankings[0] : 0.0f;
  bool isNewDistRecord = (topDist0 > 0.0f && ctx.currentDistance >= topDist0);
  bool isNewScoreRecord = (topScore0 > 0.0f && ctx.currentScore >= topScore0);

  char distBuf[96];
  snprintf(distBuf, sizeof(distBuf), "到達距離: %.1f m  %s", ctx.currentDistance,
           isNewDistRecord ? "★ NEW RECORD! ★" : "");
  draw.DrawMSDFString(distBuf, Vector2(260.0f, 160.0f), 26.0f,
                      isNewDistRecord ? Vector4(1.0f, 0.9f, 0.2f, 1.0f)
                                      : Vector4(1.0f, 1.0f, 1.0f, 1.0f),
                      true, Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  char scoreBuf[96];
  snprintf(scoreBuf, sizeof(scoreBuf), "最終スコア: %.0f pt  %s", ctx.currentScore,
           isNewScoreRecord ? "★ NEW RECORD! ★" : "");
  draw.DrawMSDFString(scoreBuf, Vector2(260.0f, 200.0f), 26.0f,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);

  char bonusBuf[64];
  snprintf(bonusBuf, sizeof(bonusBuf), "ボーナス敵撃破: %d 体",
           ctx.bonusEnemyHitCount);
  draw.DrawMSDFString(bonusBuf, Vector2(260.0f, 240.0f), 22.0f,
                      Vector4(0.6f, 0.9f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.06f);

  draw.DrawFillRect(Vector2(240.0f, 280.0f), Vector2(800.0f, 2.0f),
                    Vector4(0.3f, 0.4f, 0.5f, 0.7f));

  // 左カラム: 距離ランキング TOP 3
  draw.DrawMSDFString("【 距離ランキング TOP 3 】", Vector2(260.0f, 305.0f),
                      22.0f, Vector4(0.4f, 0.85f, 1.0f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  for (int i = 0; i < 3; i++) {
    char rBuf[64];
    float r = ctx.topRankings ? ctx.topRankings[i] : 0.0f;
    if (r > 0.0f) {
      snprintf(rBuf, sizeof(rBuf), " %d位: %.1f m", i + 1, r);
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
    float sr = ctx.topScoreRankings ? ctx.topScoreRankings[i] : 0.0f;
    if (sr > 0.0f) {
      snprintf(rBuf, sizeof(rBuf), " %d位: %.0f pt", i + 1, sr);
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

void GameHUD::DrawTitleHUD(Draw &draw, const GameHUDContext &ctx) {
  const std::string titleText = "ペンギンダッシュ";
  const std::string subTitleText = "― PENGUIN DASH ―";
  bool isTitlePad = GamePadInput::IsConnected();
  const std::string startText = isTitlePad ? "PRESS [A] BUTTON TO START" : "PRESS SPACE TO START";

  const float fontSize = 112.0f;
  const float subFontSize = 32.0f;
  const float startFontSize = 34.0f;

  float titleWidth = 0.0f;
  float subWidth = 0.0f;
  float startWidth = 0.0f;
  if (draw.GetTextRenderer()) {
    titleWidth = draw.GetTextRenderer()->MeasureString(titleText, fontSize).x;
    subWidth = draw.GetTextRenderer()->MeasureString(subTitleText, subFontSize).x;
    startWidth = draw.GetTextRenderer()->MeasureString(startText, startFontSize).x;
  }
  if (titleWidth <= 0.0f) titleWidth = 760.0f;
  if (subWidth <= 0.0f) subWidth = 350.0f;
  if (startWidth <= 0.0f) startWidth = 640.0f;

  const float screenWidth = static_cast<float>(WindowConfig::kDefaultClientWidth);
  float titleX = (screenWidth - titleWidth) * 0.5f;
  float subX = (screenWidth - subWidth) * 0.5f;
  float startX = (screenWidth - startWidth) * 0.5f;

  const float baseTitleY = 170.0f;
  const float baseSubY = baseTitleY + 125.0f;
  const float baseStartY = 570.0f;

  float currentTitleY = baseTitleY;
  float currentSubY = baseSubY;
  float currentStartY = baseStartY;

  float mainAlpha = 1.0f;
  float subAlpha = 0.95f;
  float startAlpha = 1.0f;

  if (ctx.isTitleExiting) {
    float progress =
        std::clamp(ctx.titleExitTimer / ctx.titleExitDuration, 0.0f, 1.0f);
    float ease = 1.0f - std::pow(1.0f - progress, 2.5f);
    float flyOffset = ease * 380.0f;

    currentTitleY = baseTitleY - flyOffset;
    currentSubY = baseSubY - flyOffset;
    currentStartY = baseStartY - ease * 250.0f;

    mainAlpha = (progress < 0.65f) ? 1.0f : std::clamp(1.0f - (progress - 0.65f) / 0.35f, 0.0f, 1.0f);
    subAlpha = (progress < 0.55f) ? 0.95f : std::clamp(0.95f - (progress - 0.55f) / 0.45f, 0.0f, 1.0f);
    startAlpha = std::clamp(1.0f - progress * 3.5f, 0.0f, 1.0f);
  } else {
    float hover = sinf(ctx.uiTimer * 2.2f) * 6.0f;
    currentTitleY = baseTitleY + hover;
    currentSubY = baseSubY + hover;

    float blink = sinf(ctx.uiTimer * 5.0f);
    startAlpha = (blink > -0.2f) ? 1.0f : 0.0f;
  }

  if (mainAlpha > 0.01f) {
    Vector4 shadowColor = Vector4(0.01f, 0.04f, 0.12f, 0.8f * mainAlpha);
    draw.DrawMSDFString(titleText, Vector2(titleX + 5.0f, currentTitleY + 6.0f),
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
                         ? std::clamp(1.0f - (ctx.titleExitTimer / 0.4f), 0.0f, 1.0f)
                         : mainAlpha;
  DrawControlsGuide(draw, ctx, guideAlpha);
}

void GameHUD::DrawEditorHUD(Draw &draw, const GameHUDContext &ctx) {
  draw.DrawFillRect(Vector2(20.0f, 20.0f), Vector2(300.0f, 75.0f),
                    Vector4(0.04f, 0.06f, 0.1f, 0.8f));
  char distBuf[64];
  snprintf(distBuf, sizeof(distBuf), "[EDITOR] 距離: %.1f m", ctx.currentDistance);
  draw.DrawMSDFString(distBuf, Vector2(30.0f, 28.0f), 22.0f,
                      Vector4(0.9f, 0.9f, 0.9f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
  char scoreBuf[64];
  snprintf(scoreBuf, sizeof(scoreBuf), "スコア: %.0f pt", ctx.currentScore);
  draw.DrawMSDFString(scoreBuf, Vector2(30.0f, 58.0f), 20.0f,
                      Vector4(1.0f, 0.9f, 0.2f, 1.0f), true,
                      Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.07f);
}

void GameHUD::DrawBadgeFPV(Draw &draw) {
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

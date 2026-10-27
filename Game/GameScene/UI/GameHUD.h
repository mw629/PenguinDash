#pragma once
#include <Engine.h>
#include <Math/Calculation.h>
#include <memory>

#include <vector>
#include <string>

class Draw;
class Boss;
class StageSettings;
class Sprite;
class Texture;
struct RankingEntry;

struct GameHUDContext {
  int gameState = 0;
  int playingState = 0;
  float currentDistance = 0.0f;
  float currentScore = 0.0f;
  int bonusEnemyHitCount = 0;
  float scrollSpeed = 0.0f;
  float baseScrollSpeed = 0.0f;
  float maxScrollSpeed = 0.0f;
  const float *topRankings = nullptr;
  const float *topScoreRankings = nullptr;
  bool isOneLaneMode = false;
  float rightSideDistance = 0.0f;
  Boss *boss = nullptr;
  StageSettings *stageSettings = nullptr;
  int reflectLane = -1;
  float uiTimer = 0.0f;
  float resultTimer = 0.0f;
  bool isTitleExiting = false;
  float titleExitTimer = 0.0f;
  static constexpr float kDefaultTitleExitDuration = 3.0f;
  float titleExitDuration = kDefaultTitleExitDuration;
  bool isFirstPersonView = false;
  Sprite *crashSprite = nullptr;

  // ランキング関連（毎フレームの文字列コピー・ヒープ確保を避けるためポインタ参照）
  const std::vector<RankingEntry>* onlineScoreRankings = nullptr;
  const std::vector<RankingEntry>* onlineDistanceRankings = nullptr;
  const std::string* lastSubmittedDocId = nullptr;
  const std::string* playerName = nullptr;
  int localDistanceRank = -1;
  int localScoreRank = -1;
  bool isOnlineRankingSending = false;
  bool isOnlineRankingFetching = false;
  bool isOnlineRankingConfigured = false;
  // タイトル画面用メニュー・名前登録・サブビュー
  int titleMenuIndex = 1;
  int titleSubView = 0;
  const std::string* titlePlayerName = nullptr;
  bool isEditingTitlePlayerName = false;
};

class GameHUD {
public:
  GameHUD() = default;
  ~GameHUD() = default;

  /// <summary>
  /// HUDで使用する各種スプライト・テクスチャリソースの初期化
  /// </summary>
  /// <param name="texture">テクスチャマネージャ</param>
  void Initialize(Texture *texture);

  /// <summary>
  /// ゲーム状態に応じたHUD（タイトル/プレイ/ボス/ポーズ/ゲームオーバー等）の描画
  /// </summary>
  /// <param name="draw">描画マネージャ</param>
  /// <param name="ctx">HUD描画用コンテキスト</param>
  void DrawHUD(Draw &draw, const GameHUDContext &ctx);

private:
  static constexpr float kSpeedDisplayMultiplier = 300.0f;
  static constexpr int kSpeedBarSegments = 10;
  static constexpr float kOneLaneTotalDistance = 200.0f;
  static constexpr float kDefaultGuideAlpha = 1.0f;

  // プレイ中ステータスパネル
  static constexpr Vector2 kStatusPanelPos = {20.0f, 54.0f};
  static constexpr Vector2 kStatusPanelSize = {265.0f, 145.0f};
  static constexpr float kPanelBorderHeight = 3.0f;
  static constexpr float kPanelTextOffsetX = 12.0f;
  static constexpr float kDistTextOffsetY = 9.0f;
  static constexpr float kScoreTextOffsetY = 47.0f;
  static constexpr float kSpeedTextOffsetY = 81.0f;
  static constexpr float kBestTextOffsetY = 111.0f;
  static constexpr float kDistFontSize = 30.0f;
  static constexpr float kScoreFontSize = 24.0f;
  static constexpr float kSpeedFontSize = 19.0f;
  static constexpr float kBestFontSize = 17.0f;
  static constexpr Vector4 kPanelBgColor = {0.04f, 0.07f, 0.12f, 0.82f};
  static constexpr Vector4 kPanelBorderColor = {0.2f, 0.6f, 0.9f, 0.9f};

  // 1レーンパネル
  static constexpr Vector2 kOneLanePanelSize = {275.0f, 46.0f};
  static constexpr float kOneLaneMarginRight = 20.0f;
  static constexpr float kOneLanePanelY = 15.0f;

  // ボスパネル & 氷HPバー
  static constexpr Vector2 kBossPanelPos = {310.0f, 10.0f};
  static constexpr Vector2 kBossPanelSize = {660.0f, 96.0f};
  static constexpr float kBossTitleFontSize = 24.0f;
  static constexpr float kBossTitleY = 18.0f;
  static constexpr Vector2 kBossHpBarPos = {340.0f, 52.0f};
  static constexpr Vector2 kBossHpBarSize = {600.0f, 32.0f};
  static constexpr float kBossHpTextFontSize = 20.0f;
  static constexpr float kBossAlertWidth = 740.0f;
  static constexpr float kBossAlertHeight = 40.0f;
  static constexpr float kBossAlertY = 116.0f;
  static constexpr float kBossAlertTextY = 121.0f;
  static constexpr float kBossAlertFontSize = 24.0f;
  static constexpr float kBossAlertPulseBase = 0.8f;
  static constexpr float kBossAlertPulseAmp = 0.2f;
  static constexpr float kBossAlertPulseFreq = 12.0f;
  static constexpr Vector2 kBossMissionBannerPos = {260.0f, 240.0f};
  static constexpr Vector2 kBossMissionBannerSize = {760.0f, 190.0f};
  static constexpr float kBossMissionBannerBorderY = 426.0f;
  static constexpr Vector2 kBossDefeatPanelPos = {360.0f, 170.0f};
  static constexpr Vector2 kBossDefeatPanelSize = {560.0f, 95.0f};
  static constexpr float kBossDefeatTitleFontSize = 38.0f;
  static constexpr float kBossDefeatBonusFontSize = 24.0f;

  // 操作ガイドバー
  static constexpr Vector2 kControlsBarPos = {20.0f, 672.0f};
  static constexpr Vector2 kControlsBarSize = {1240.0f, 36.0f};
  static constexpr float kControlsBarBorderH = 2.0f;
  static constexpr Vector2 kControlsBadgePos = {25.0f, 675.0f};
  static constexpr float kControlsBadgeWidthPad = 72.0f;
  static constexpr float kControlsBadgeWidthKey = 110.0f;
  static constexpr float kControlsBadgeHeight = 28.0f;
  static constexpr float kControlsBadgeInnerH = 26.0f;
  static constexpr float kControlsBadgePadTextX = 43.0f;
  static constexpr float kControlsBadgeKeyTextX = 32.0f;
  static constexpr float kControlsBadgeTextY = 680.0f;
  static constexpr float kControlsBadgeFontSize = 15.5f;
  static constexpr float kControlsGuideFontSize = 17.5f;

  // ポーズ画面
  static constexpr float kPauseDimAlpha = 0.65f;
  static constexpr Vector2 kPausePanelPos = {360.0f, 160.0f};
  static constexpr Vector2 kPausePanelSize = {560.0f, 410.0f};
  static constexpr Vector2 kPauseBossPanelPos = {300.0f, 95.0f};
  static constexpr Vector2 kPauseBossPanelSize = {680.0f, 495.0f};
  static constexpr float kPauseAccentHeight = 4.0f;
  static constexpr Vector2 kPauseTitlePos = {490.0f, 190.0f};
  static constexpr float kPauseTitleFontSize = 52.0f;
  static constexpr Vector2 kPauseSubTitlePos = {540.0f, 255.0f};
  static constexpr float kPauseSubTitleFontSize = 22.0f;
  static constexpr Vector2 kPauseDividerPos1 = {400.0f, 290.0f};
  static constexpr Vector2 kPauseDividerPos2 = {400.0f, 400.0f};
  static constexpr Vector2 kPauseDividerSize = {480.0f, 2.0f};
  static constexpr Vector2 kPauseDistPos = {430.0f, 315.0f};
  static constexpr Vector2 kPauseScorePos = {430.0f, 355.0f};
  static constexpr float kPauseStatFontSize = 26.0f;
  static constexpr Vector2 kPauseBtnResumePos = {410.0f, 425.0f};
  static constexpr Vector2 kPauseBtnRestartPos = {410.0f, 465.0f};
  static constexpr Vector2 kPauseBtnTitlePos = {410.0f, 505.0f};
  static constexpr float kPauseBtnFontSize = 24.0f;

  // リザルト画面
  static constexpr float kGameOverDimAlpha = 0.72f;
  static constexpr Vector2 kGameOverPanelPos = {200.0f, 50.0f};
  static constexpr Vector2 kGameOverPanelSize = {880.0f, 600.0f};
  static constexpr float kGameOverAccentHeight = 4.0f;
  static constexpr Vector2 kGameOverTitlePos = {540.0f, 75.0f};
  static constexpr float kGameOverTitleFontSize = 54.0f;
  static constexpr float kResultDropDuration = 0.60f;
  static constexpr float kResultDropStartY = -720.0f;
  static constexpr Vector2 kGameOverDistPos = {260.0f, 142.0f};
  static constexpr float kGameOverDistFontSize = 25.0f;
  static constexpr Vector2 kGameOverScorePos = {260.0f, 180.0f};
  static constexpr float kGameOverScoreFontSize = 25.0f;
  static constexpr Vector2 kGameOverBonusPos = {260.0f, 218.0f};
  static constexpr float kGameOverBonusFontSize = 21.0f;
  static constexpr Vector2 kGameOverDividerPos1 = {240.0f, 255.0f};
  static constexpr Vector2 kGameOverDividerPos2 = {240.0f, 505.0f};
  static constexpr Vector2 kGameOverDividerSize = {800.0f, 2.0f};
  static constexpr Vector2 kGameOverRankHeaderDistPos = {260.0f, 275.0f};
  static constexpr Vector2 kGameOverRankHeaderScorePos = {660.0f, 275.0f};
  static constexpr float kGameOverRankHeaderFontSize = 22.0f;
  static constexpr float kGameOverRankItemDistX = 265.0f;
  static constexpr float kGameOverRankItemScoreX = 665.0f;
  static constexpr float kGameOverRankItemBaseY = 315.0f;
  static constexpr float kGameOverRankItemStepY = 36.0f;
  static constexpr float kGameOverRankItemFontSize = 20.0f;
  static constexpr Vector2 kGameOverMyRankBoxSize = {375.0f, 44.0f};
  static constexpr float kGameOverMyRankBoxY = 435.0f;
  static constexpr float kGameOverMyRankTextOffsetY = 11.0f;
  static constexpr float kGameOverMyRankFontSize = 19.5f;
  static constexpr Vector2 kGameOverBottomBtnPos = {245.0f, 535.0f};
  static constexpr float kGameOverBottomBtnFontSize = 22.0f;

  // タイトル画面
  static constexpr float kTitleMainFontSize = 112.0f;
  static constexpr float kTitleSubFontSize = 32.0f;
  static constexpr float kTitleStartFontSize = 34.0f;
  static constexpr float kTitleDefaultWidth = 760.0f;
  static constexpr float kTitleDefaultSubWidth = 350.0f;
  static constexpr float kTitleDefaultStartWidth = 640.0f;
  static constexpr float kTitleBaseY = 170.0f;
  static constexpr float kTitleSubOffsetY = 125.0f;
  static constexpr float kTitleStartBaseY = 570.0f;
  static constexpr float kTitleFlyEaseExponent = 2.5f;
  static constexpr float kTitleFlyOffset = 380.0f;
  static constexpr float kTitleStartFlyOffset = 250.0f;
  static constexpr float kTitleMainFadeStart = 0.65f;
  static constexpr float kTitleMainFadeSpan = 0.35f;
  static constexpr float kTitleSubFadeStart = 0.55f;
  static constexpr float kTitleSubFadeSpan = 0.45f;
  static constexpr float kTitleStartFadeRate = 3.5f;
  static constexpr float kTitleHoverFreq = 2.2f;
  static constexpr float kTitleHoverAmp = 6.0f;
  static constexpr float kTitleBlinkFreq = 5.0f;
  static constexpr float kTitleBlinkThreshold = -0.2f;
  static constexpr float kTitleExitGuideDuration = 0.4f;
  static constexpr Vector2 kTitleShadowOffset = {5.0f, 6.0f};

  // エディタ HUD & FPV バッジ
  static constexpr Vector2 kEditorPanelPos = {20.0f, 20.0f};
  static constexpr Vector2 kEditorPanelSize = {300.0f, 75.0f};
  static constexpr Vector2 kEditorDistPos = {30.0f, 28.0f};
  static constexpr float kEditorDistFontSize = 22.0f;
  static constexpr Vector2 kEditorScorePos = {30.0f, 58.0f};
  static constexpr float kEditorScoreFontSize = 20.0f;
  static constexpr Vector2 kFpvBadgePos = {20.0f, 675.0f};
  static constexpr Vector2 kFpvBadgeSize = {175.0f, 30.0f};
  static constexpr float kFpvBorderWidth = 3.0f;
  static constexpr Vector2 kFpvTextOffset = {10.0f, 6.0f};
  static constexpr float kFpvFontSize = 16.0f;

  /// <summary>
  /// フォントアトラスの事前キャッシュ処理
  /// </summary>
  void PreloadFontStrings(Draw &draw);

  /// <summary>
  /// 通常プレイ中HUDの描画（距離、スコア、速度メーター等）
  /// </summary>
  void DrawPlayingHUD(Draw &draw, const GameHUDContext &ctx);

  /// <summary>
  /// ボス戦中HUDの描画（ボス氷HPバー、警告表示等）
  /// </summary>
  void DrawBossHUD(Draw &draw, const GameHUDContext &ctx);

  /// <summary>
  /// 操作ガイドUIの描画（フェードイン/アウト対応）
  /// </summary>
  void DrawControlsGuide(Draw &draw, const GameHUDContext &ctx, float alpha);

  /// <summary>
  /// 操作ガイドUIの描画（通常表示）
  /// </summary>
  void DrawControlsGuide(Draw &draw, const GameHUDContext &ctx) { DrawControlsGuide(draw, ctx, kDefaultGuideAlpha); }

  /// <summary>
  /// 通常ポーズ画面HUDの描画
  /// </summary>
  void DrawPauseHUD(Draw &draw, const GameHUDContext &ctx);

  /// <summary>
  /// ボス戦ポーズ画面HUDの描画
  /// </summary>
  void DrawBossPauseHUD(Draw &draw, const GameHUDContext &ctx);

  /// <summary>
  /// リザルト画面HUDの描画（スコア・ランキング結果、落下イン演出対応）
  /// </summary>
  void DrawResultHUD(Draw &draw, const GameHUDContext &ctx);
  void DrawGameOverHUD(Draw &draw, const GameHUDContext &ctx) { DrawResultHUD(draw, ctx); }

  /// <summary>
  /// タイトル画面HUDの描画（タイトルロゴ、メニュー、名前入力）
  /// </summary>
  void DrawTitleHUD(Draw &draw, const GameHUDContext &ctx);

  /// <summary>
  /// タイトル画面ランキングモーダルの描画
  /// </summary>
  void DrawTitleRankingModal(Draw &draw, const GameHUDContext &ctx);

  /// <summary>
  /// タイトル画面操作説明モーダルの描画
  /// </summary>
  void DrawTitleHowToPlayModal(Draw &draw, const GameHUDContext &ctx);

  /// <summary>
  /// エディターモード中HUDの描画
  /// </summary>
  void DrawEditorHUD(Draw &draw, const GameHUDContext &ctx);

  /// <summary>
  /// 一人称視点（FPV）表示バッジの描画
  /// </summary>
  void DrawBadgeFPV(Draw &draw);

  /// <summary>
  /// 左上デモ告知バッジ（「演出は二つあります」）の描画
  /// </summary>
  void DrawDemoNotice(Draw &draw);

  // 氷HPバー用スプライトリソース
  int hpBarFrameTexture_ = -1;
  int hpBarFillTexture_ = -1;
  std::unique_ptr<Sprite> hpBarFrameSprite_;
  std::unique_ptr<Sprite> hpBarFillSprite_;
  bool isSpritesInitialized_ = false;

  // ボスHPバーのアニメーション演出用
  int lastKnownBossHp_ = -1;
  float bossDamageLagHp_ = 5.0f;
  float bossDamageFlashTimer_ = 0.0f;
  float bossDamageShakeTimer_ = 0.0f;
  float lastUiTimer_ = 0.0f;

  // リザルト着地音再生フラグ
  bool resultSePlayed_ = false;
};

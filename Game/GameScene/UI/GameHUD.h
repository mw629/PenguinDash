#pragma once
#include <Engine.h>
#include <Math/Calculation.h>
#include <memory>

class Draw;
class Boss;
class StageSettings;
class Sprite;
class Fade;

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
  bool isTitleExiting = false;
  float titleExitTimer = 0.0f;
  float titleExitDuration = 3.0f;
  bool isFirstPersonView = false;
  Sprite *crashSprite = nullptr;
  Fade *fade = nullptr;
};

class GameHUD {
public:
  GameHUD() = default;
  ~GameHUD() = default;

  void DrawHUD(Draw &draw, const GameHUDContext &ctx);

private:
  void PreloadFontStrings(Draw &draw);
  void DrawPlayingHUD(Draw &draw, const GameHUDContext &ctx);
  void DrawBossHUD(Draw &draw, const GameHUDContext &ctx);
  void DrawControlsGuide(Draw &draw, const GameHUDContext &ctx, float alpha = 1.0f);
  void DrawPauseHUD(Draw &draw, const GameHUDContext &ctx);
  void DrawGameOverHUD(Draw &draw, const GameHUDContext &ctx);
  void DrawTitleHUD(Draw &draw, const GameHUDContext &ctx);
  void DrawEditorHUD(Draw &draw, const GameHUDContext &ctx);
  void DrawBadgeFPV(Draw &draw);
};

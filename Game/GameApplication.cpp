#include "GameApplication.h"

GameApplication::GameApplication(int32_t kClientWidth, int32_t kClientHeight) {
  engine = std::make_unique<Engine>(kClientWidth, kClientHeight);
  engine.get()->Setting();
  sceneManager = std::make_unique<SceneManager>();
  editorManager = std::make_unique<EditorManager>();

  // 初期化時に読み込まれたリソースの転送をGPUへ確定・実行し、
  // 不要になった中間アップロードバッファを全破棄してVRAMをクリーンにする
  engine->FlushGpu();
}

void GameApplication::Run() {

  // ウィンドウのxが押されるまでループ
  while (true) {
    // windowにメッセージが来てたら最優先で処理させる
    if (WindowConfig::ProcessMessage()) {
      break;
    }

    engine.get()->NewFrame();

    sceneManager.get()->PreUpdate();

    // 1. まずシーンの更新を実行（入力、オブジェクト・パーティクルの更新、emitフラグ確定）
    sceneManager.get()->Update();

#ifdef _USE_IMGUI
    // 2. エディタ全体のImGuiおよびGameView描画（メインメニューバー、DockSpace、各種エディタウィンドウ）
    // ※DockSpaceを先に宣言しないと、シーン側ウィンドウ(Hierarchy/Inspector等)が毎フレームDockSpaceから切断される
    editorManager->Update(engine.get());

    // 3. シーン固有のImGui（Hierarchy, Inspector, GameScene等）
    sceneManager.get()->ImGui();

    // 4. メインシーン描画
    sceneManager.get()->Draw(*engine->draw);
#else
    // Release/Developmentでは直接シーンを描画
    sceneManager.get()->Draw(*engine->draw);
#endif

    engine.get()->EndFrame(
        [&]() { sceneManager.get()->DrawUI(*engine->draw); });
  }
  engine.get()->End();
}

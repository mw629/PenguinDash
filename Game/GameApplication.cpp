#include "GameApplication.h"
#include "Network/Firebase/FirebaseRankingManager.h"

// ゲームアプリケーションの初期化
GameApplication::GameApplication(int32_t kClientWidth, int32_t kClientHeight) {
  // Firebase オンラインランキングの初期化 (My.env から設定読み込み)
  FirebaseRankingManager::GetInstance()->Initialize("My.env");

  engine = std::make_unique<Engine>(kClientWidth, kClientHeight);
  engine.get()->Setting();
  sceneManager = std::make_unique<SceneManager>();
  editorManager = std::make_unique<EditorManager>();

  // 初期化時に読み込まれたリソースの転送をGPUへ確定・実行し、
  // 不要になった中間アップロードバッファを全破棄してVRAMをクリーンにする
  engine->FlushGpu();
}

// メインループの実行（メッセージループ、フレーム更新、描画、ImGuiの統括）
void GameApplication::Run() {

  // ウィンドウのxが押されるまでループ
  while (true) {
    // windowにメッセージが来てたら最優先で処理させる
    if (WindowConfig::ProcessMessage()) {
      break;
    }

    engine.get()->NewFrame();

    // Firebase 通信コールバックのメインスレッドディスパッチ
    FirebaseRankingManager::GetInstance()->Update();

    sceneManager.get()->PreUpdate();

    // 1. まずシーンの更新を実行（入力、オブジェクト・パーティクルの更新、emitフラグ確定）
    sceneManager.get()->Update();

#ifdef _USE_IMGUI
    // 2. エディタ全体のImGuiおよびGameView描画（メインメニューバー、DockSpace、各種エディタウィンドウ）
    // ※DockSpaceを先に宣言しないと、シーン側ウィンドウ(Hierarchy/Inspector等)が毎フレームDockSpaceから切断される
    editorManager->Update(engine.get());

    // 3. シーン固有のImGui（Hierarchy, Inspector, GameScene等）
    sceneManager.get()->ImGui();

    // 4. Engine固有のImGui（ネットワーク接続等）
    engine.get()->ImGui();

    // 5. Firebase オンラインランキング用 ImGui デバッグウィンドウ
    FirebaseRankingManager::GetInstance()->ImGui();

    // 6. メインシーン描画
    sceneManager.get()->Draw(*engine->GetDraw());
#else
    // Release/Developmentでは直接シーンを描画
    sceneManager.get()->Draw(*engine->GetDraw());
#endif

    engine.get()->EndFrame(
        [&]() { sceneManager.get()->DrawUI(*engine->GetDraw()); });
  }
  engine.get()->End();
}

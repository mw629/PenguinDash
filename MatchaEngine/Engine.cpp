#include "Engine.h"

#include <iostream>
#include <psapi.h>
#include <windows.h>


#ifdef _USE_IMGUI
#include "../externals/imgui/imgui.h"
#include "ImGuiManager.h"
#endif // _USE_IMGUI

#include <thread>

#pragma comment(lib, "winmm.lib")

bool Engine::isEnd_ = false;

Engine::~Engine() {}

Engine::Engine(int32_t kClientWidth, int32_t kClientHeight) {
  kClientWidth_ = kClientWidth;
  kClientHeight_ = kClientHeight;

  // 時間の初期化
  reference_ = std::chrono::steady_clock::now();

  InitializeErrorHandlers();
  logStream = CurrentTimestamp();
  SetGlobalLogStream(&logStream);
  LOG_INFO("MatchaEngine Initializing...");

  // DebugLayer//
#ifdef _DEBUG
  Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    // デバッグレイヤーを有効化する
    debugController->EnableDebugLayer();
    // GPU-Based Validation はシェーダーコンパイルとPSO生成を極端に遅くするため、GPUクラッシュ検証時以外は無効化
    // debugController->SetEnableGPUBasedValidation(TRUE);
  }
#endif

  graphics = std::make_unique<GraphicsDevice>(logStream);
  command = std::make_unique<CommandContext>(graphics->GetDevice());
  swapChain = std::make_unique<SwapChain>();
  descriptorHeap = std::make_unique<DescriptorHeap>(graphics->GetDevice());
  renderTargetView = std::make_unique<RenderTargetView>();
  viewportScissor =
      std::make_unique<ViewportScissor>(kClientWidth, kClientHeight);
  // 入力
  input = std::make_unique<Input>();
  gamePadInput = std::make_unique<GamePadInput>();
  // 描画
  debugCamera = std::make_unique<DebugCamera>();
  depthStencil = std::make_unique<DepthStencil>();
  renderTextures[0] = std::make_unique<RenderTexture>();
  renderTextures[1] = std::make_unique<RenderTexture>();
  draw = std::make_unique<Draw>();
  textureLoader = std::make_unique<TextureLoader>();
  // バリア
  resourceBarrierHelper = std::make_unique<ResourceBarrierHelper>();
  imGuiManager = std::make_unique<ImGuiManager>();

  graphicsPipelineState = std::make_unique<GraphicsPipelineState>();
}

void Engine::Setting() {

  window.DrawWindow(kClientWidth_, kClientHeight_);
  timeBeginPeriod(1);
  input->Initialize(window.GetWc(), window.GetHwnd());

  debugCamera->Initialize();

  // エラー・警告、即ちに停止//
#ifdef _DEBUG
  Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
  if (SUCCEEDED(
          graphics->GetDevice()->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
    // やばいエラー時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    // エラー時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    // 警告時に止まる
    // infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

    // エラーと警告の抑制//
    // 抑制するメッセージのID
    D3D12_MESSAGE_ID denyIds[] = {
        // Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
        // https://stackoverflow.com/question/69805245/directx-12-application-is-crashing-in-windows--11
        D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};
    // 抑制レベル
    D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
    D3D12_INFO_QUEUE_FILTER filter{};
    filter.DenyList.NumIDs = _countof(denyIds);
    filter.DenyList.pIDList = denyIds;
    filter.DenyList.NumSeverities = _countof(severities);
    filter.DenyList.pSeverityList = severities;
    // 指定したメッセージの表示を抑制
    infoQueue->PushStorageFilter(&filter);
  }
#endif

  swapChain->CreateSwapChain(graphics->GetDxgiFactory(),
                             command->GetCommandQueue(), window.GetHwnd(),
                             kClientWidth_, kClientHeight_);
  descriptorHeap->CreateHeap(graphics->GetDevice());
  renderTargetView->CreateRenderTargetView(
      graphics->GetDevice(), swapChain->GetSwapChainResources(),
      descriptorHeap->GetRtvDescriptorHeap());

  gpuSyncManager.Initialize(graphics.get()->GetDevice());

  gpuProfiler = std::make_unique<GpuProfiler>();
  gpuProfiler->Initialize(graphics->GetDevice(), command->GetCommandQueue());

  depthStencil->CreateDepthStencil(graphics->GetDevice(), kClientWidth_,
                                   kClientHeight_,
                                   descriptorHeap->GetSrvDescriptorHeap(),
                                   descriptorHeap->GetDescriptorSizeSRV());

  renderTextures[0]->Initialize(graphics->GetDevice(), kClientWidth_,
                                kClientHeight_,
                                descriptorHeap->GetSrvDescriptorHeap(),
                                descriptorHeap->GetDescriptorSizeSRV());
  renderTextures[1]->Initialize(graphics->GetDevice(), kClientWidth_,
                                kClientHeight_,
                                descriptorHeap->GetSrvDescriptorHeap(),
                                descriptorHeap->GetDescriptorSizeSRV());

  graphicsPipelineState.get()->ALLPSOCreate(logStream,
                                            graphics.get()->GetDevice());

  // ポストエフェクトシェーダーフォルダをスキャンしてImGuiリストを自動構築
  PostEffect::ScanPostEffectShaders();

  lightManager = std::make_unique<LightManager>();
  lightManager->Initialize();

  // ビューポート
  viewportScissor->CreateViewPort();
  // シーザー矩形
  viewportScissor->CreateSxissor();

  descriptorHeaps[0] = {descriptorHeap->GetSrvDescriptorHeap()};

#ifdef _USE_IMGUI
  imGuiManager->Initialize(window.GetHwnd(), graphics->GetDevice(),
                           swapChain->GetSwapChainDesc().BufferCount,
                           renderTargetView->GetRtvDesc().Format,
                           descriptorHeap->GetSrvDescriptorHeap(),
                           descriptorHeap->GetDescriptorSizeSRV(),
                           command->GetCommandQueue());

  // 初回表示位置・サイズを厳密に指定（ImGuiCond_Once または ImGuiCond_Always
  // に変更可能）
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(500.0f, 100.0f), ImGuiCond_Once);

#endif // _USE_IMGUI

  Audio::Initialize();

  lineRenderer = std::make_unique<LineRenderer>();
  lineRenderer->Initialize();

  draw->Initialize(command.get()->GetCommandList(), graphicsPipelineState.get(),
                   lightManager.get(), lineRenderer.get());
  Texture::Initialize(graphics->GetDevice(), command->GetCommandList(),
                      descriptorHeap.get(), textureLoader.get());

  textRenderer = std::make_unique<MatchaEngine::TextRenderer>();
  textRenderer->Initialize(graphics->GetDevice(), descriptorHeap.get(),
                           graphicsPipelineState.get(),
                           "Resources/Font/NotoSansJP-VariableFont_wght.ttf");
  textRenderer->SetScreenSize(static_cast<float>(kClientWidth_),
                              static_cast<float>(kClientHeight_));
  draw->SetTextRenderer(textRenderer.get());

  // ダミーテクスチャ（0番目）としてロードしておくことで、テクスチャ無しのオブジェクトが描画されたときのクラッシュを防ぐ
  std::unique_ptr<Texture> dummyTex = std::make_unique<Texture>();
  try {
    dummyTex->CreateTexture("Resources/DDS/SnowWorld.dds");
  } catch (...) {
    // もし無ければ無視する（フォールバックが機能しなくなるが、少なくとも起動はする）
  }

  CharacterAnimator::SetData(graphics.get()->GetDevice(), descriptorHeap.get());
  // TransformAnimation::SetData(graphics.get()->GetDevice(),
  // descriptorHeap.get());

  Vector2 Client = {(float)kClientWidth_, (float)kClientHeight_};
  ObjectBase::SetObjectResource(Client);
  Line::SetScreenSize(Client);
  Grid::SetScreenSize(Client);
  EffectDefinition::SetScreenSize(Client);
  Triangle::SetScreenSize(Client);
  Sprite::SetScreenSize(Client);
  Camera::SetScreenSize(Client);

  EffectDefinition::SetDescriptorHeap(descriptorHeap.get());
  EffectDefinition::SetDevice(graphics.get()->GetDevice());
  EffectDefinition::SetGpuProfiler(gpuProfiler.get());
  draw->SetGpuProfiler(gpuProfiler.get());

  // 1段目: アウトライン (OutLineShader)
  constexpr float kDefaultOutlineSensitivity = 1.0f;
  constexpr float kDefaultOutlineThickness = 1.0f;
  auto outlineEffect = std::make_unique<PostEffect>();
  outlineEffect->Initialize();
  outlineEffect->SetActivePostEffect("OutLineShader");
  outlineEffect->SetValue1(kDefaultOutlineSensitivity); // 感度・強さ
  outlineEffect->SetValue2(kDefaultOutlineThickness);   // 線の太さ
  outlineEffect->SetColor(0.0f, 0.0f, 0.0f); // 輪郭線の色（黒）
  postEffects_.push_back(std::move(outlineEffect));

  // 2段目: フォグ (FogShader)
  constexpr float kDefaultFogR = 0.82f;
  constexpr float kDefaultFogG = 0.88f;
  constexpr float kDefaultFogB = 0.95f;
  constexpr float kDefaultFogDensity = 7.5f;
  constexpr float kDefaultFogStartDistance = 12.0f;
  constexpr float kDefaultFogHeightRatio = 0.4f;

  auto fogEffect = std::make_unique<PostEffect>();
  fogEffect->Initialize();
  fogEffect->SetActivePostEffect("FogShader");
  fogEffect->SetColor(kDefaultFogR, kDefaultFogG, kDefaultFogB); // 冬の雪景色にマッチする淡い水色白
  fogEffect->SetValue1(kDefaultFogDensity);                     // 濃度
  fogEffect->SetValue2(kDefaultFogStartDistance);               // 開始距離 (プレイヤーの手前をクリアに)
  fogEffect->SetRatio(kDefaultFogHeightRatio);                  // 高さフォグの影響度
  postEffects_.push_back(std::move(fogEffect));
}

void Engine::PostDraw(const std::function<void()> &drawUI) {
  // Draw all lines using the active camera from Draw class
  draw->DrawAllLines(lineRenderer.get());

  // Scene描画が終わったRenderTextureをShaderResourceへ
  renderTextures[0]->TransitionToShaderResource(command->GetCommandList());
  depthStencil->TransitionToShaderResource(command->GetCommandList());

  int currentRT = 0;
  int nextRT = 1;

  for (auto &effect : postEffects_) {
    if (effect->IsNormalEffect())
      continue; // Normal (CopyShader) はスキップ

    renderTextures[nextRT]->TransitionToRenderTarget(command->GetCommandList());
    // renderTextures[nextRT]->Clear(command->GetCommandList()); // No need to
    // clear since we render full screen

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        renderTextures[nextRT]->GetRtvHandle();
    command->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false,
                                                  nullptr);

    command->GetCommandList()->RSSetViewports(1,
                                              viewportScissor->GetViewport());
    command->GetCommandList()->RSSetScissorRects(
        1, viewportScissor->GetScissorRect());

    draw->DrawPostEffect(renderTextures[currentRT]->GetSrvHandleGPU(),
                         effect->GetActiveShaderName(), effect.get(),
                         depthStencil->GetSrvHandleGPU());

    renderTextures[nextRT]->TransitionToShaderResource(
        command->GetCommandList());

    std::swap(currentRT, nextRT);
  }

  // ポストエフェクト適用後のテクスチャに対して UI (HUD/Sprite/Text) を描画
  if (drawUI) {
    renderTextures[currentRT]->TransitionToRenderTarget(
        command->GetCommandList());

    depthStencil->TransitionToDepthWrite(command->GetCommandList());
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        renderTextures[currentRT]->GetRtvHandle();
    depthStencil->SetDSV(command->GetCommandList(), &rtvHandle);

    command->GetCommandList()->RSSetViewports(1,
                                              viewportScissor->GetViewport());
    command->GetCommandList()->RSSetScissorRects(
        1, viewportScissor->GetScissorRect());

    ID3D12DescriptorHeap *descriptorHeaps[] = {
        descriptorHeap->GetSrvDescriptorHeap()};
    command->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);

    drawUI();

    renderTextures[currentRT]->TransitionToShaderResource(
        command->GetCommandList());
    depthStencil->TransitionToShaderResource(command->GetCommandList());
  }

  // SwapchainをRenderTargetへ
  resourceBarrierHelper->Transition(command->GetCommandList(), swapChain.get());
  // RenderTargetの設定とClear（DepthStencilは設定・クリアしない）
  renderTargetView->SetAndClear(
      command->GetCommandList(),
      swapChain->GetSwapChain()->GetCurrentBackBufferIndex());

  command->GetCommandList()->RSSetViewports(1, viewportScissor->GetViewport());
  command->GetCommandList()->RSSetScissorRects(
      1, viewportScissor->GetScissorRect());

#ifdef _USE_IMGUI
  // _USE_IMGUI有効時はSceneウィンドウ内のImGui::Imageで描画するため
  // Swapchainへの全画面PostEffect描画はスキップ
#else
  draw->DrawPostEffect(renderTextures[currentRT]->GetSrvHandleGPU(),
                       "CopyShader", nullptr, depthStencil->GetSrvHandleGPU());
#endif

#ifdef _USE_IMGUI
  imGuiManager->Render(command->GetCommandList());
#endif // _USE_IMGUI

  if (gpuProfiler) {
    gpuProfiler->EndFrame(command->GetCommandList());
  }

  // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
  // 今回RenderTargetからPresentにする
  resourceBarrierHelper->TransitionToPresent(command->GetCommandList());
}

RenderTexture *Engine::GetFinalRenderTexture() {
  int finalIndex = 0;
  for (auto &effect : postEffects_) {
    if (!effect->IsNormalEffect()) {
      finalIndex = 1 - finalIndex;
    }
  }
  return renderTextures[finalIndex].get();
}

void Engine::NewFrame() {

  RECT clientRect;
  GetClientRect(window.GetHwnd(), &clientRect);
  int width = clientRect.right - clientRect.left;
  int height = clientRect.bottom - clientRect.top;

  if (width > 0 && height > 0 &&
      (width != kClientWidth_ || height != kClientHeight_)) {
    kClientWidth_ = width;
    kClientHeight_ = height;

    gpuSyncManager.WaitForGpu();

    swapChain->ChangeScreen(width, height, !window.IsFullscreen());

    renderTargetView->RecreateRenderTargetViews(
        graphics->GetDevice(), swapChain->GetSwapChainResources(),
        descriptorHeap->GetRtvDescriptorHeap());

    depthStencil->CreateDepthStencil(graphics->GetDevice(), width, height,
                                     descriptorHeap->GetSrvDescriptorHeap(),
                                     descriptorHeap->GetDescriptorSizeSRV());

    renderTextures[0]->Initialize(graphics->GetDevice(), width, height,
                                  descriptorHeap->GetSrvDescriptorHeap(),
                                  descriptorHeap->GetDescriptorSizeSRV());
    renderTextures[1]->Initialize(graphics->GetDevice(), width, height,
                                  descriptorHeap->GetSrvDescriptorHeap(),
                                  descriptorHeap->GetDescriptorSizeSRV());

    viewportScissor = std::make_unique<ViewportScissor>(width, height);
    viewportScissor->CreateViewPort();
    viewportScissor->CreateSxissor();

    Vector2 Client = {(float)kClientWidth_, (float)kClientHeight_};
    ObjectBase::SetObjectResource(Client);
    Line::SetScreenSize(Client);
    Grid::SetScreenSize(Client);
    EffectDefinition::SetScreenSize(Client);
    Triangle::SetScreenSize(Client);
    Sprite::SetScreenSize(Client);
    Camera::SetScreenSize(Client);
    if (textRenderer) {
      textRenderer->SetScreenSize((float)kClientWidth_, (float)kClientHeight_);
    }
  }

#ifdef _USE_IMGUI
  imGuiManager->NewFrame();
#endif // _USE_IMGUI

  if (gpuProfiler) {
    gpuProfiler->BeginFrame(command->GetCommandList());
  }

  if (draw) {
    draw->ResetCullingStats();
  }

  if (textRenderer) {
    textRenderer->BeginFrame();
  }

  EffectDefinition::BeginFrame();

  // コマンドを積み込んで確定させる//
  renderTextures[0]->TransitionToRenderTarget(command->GetCommandList());
  renderTextures[0]->Clear(command->GetCommandList());

  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = renderTextures[0]->GetRtvHandle();
  // DSVを設定する
  depthStencil->TransitionToDepthWrite(command->GetCommandList());
  depthStencil->SetDSV(command->GetCommandList(), &rtvHandle);
  // コマンドを積む//

  ID3D12DescriptorHeap *descriptorHeaps[] = {
      descriptorHeap->GetSrvDescriptorHeap()};
  command->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);

  command->GetCommandList()->RSSetViewports(
      1, viewportScissor->GetViewport()); // Viewportを設定
  command->GetCommandList()->RSSetScissorRects(
      1, viewportScissor->GetScissorRect()); // Scissorを設定

  input.get()->Update();

  gamePadInput.get()->Update();

  Matrix4x4 projectionMatri = MakePerspectiveFovMatrix(
      Camera::kDefaultFovY, float(kClientWidth_) / float(kClientHeight_),
      Camera::kDefaultNearClip, Camera::kDefaultFarClip);
  Matrix4x4 projInverse = Inverse(projectionMatri);
  for (auto &effect : postEffects_) {
    effect->SetProjectionInverse(projInverse);
    effect->Update(kDefaultDeltaTime);
  }
}

void Engine::EndFrame(const std::function<void()> &drawUI) {

  PostDraw(drawUI);

  // コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
  hr_ = command->GetCommandList()->Close();
  if (FAILED(hr_)) {
    CheckHResult(hr_, "CommandList::Close failed", graphics->GetDevice());
    assert(SUCCEEDED(hr_));
  }

  // コマンドをキックする//

  // GPU2コマンドリストの実行を行わせる
  ID3D12CommandList *commandLists[] = {command->GetCommandList()};
  command->GetCommandQueue()->ExecuteCommandLists(1, commandLists);

  // GPUとOSに画面の交換を行うよう通知する
  HRESULT presentHr = swapChain->GetSwapChain()->Present(1, 0);
  if (FAILED(presentHr)) {
    CheckHResult(presentHr, "SwapChain::Present failed", graphics->GetDevice());
    assert(SUCCEEDED(presentHr));
  }

  // 初回フレームのPresent完了後にウィンドウを可視化（白い未描画画面の露出を防止）
  window.Show();

  gpuSyncManager.Signal(command.get()->GetCommandQueue());

  gpuSyncManager.WaitForGpu();

  // GPU転送完了後に中間アップロードバッファを解放してVRAM消費を半減させる
  if (textureLoader) {
    textureLoader->ReleaseIntermediateResources();
  }

  UpdateFixFPS();

  // 次のフレーム用のコマンドを準備
  hr_ = command->GetCommandAllocator()->Reset();
  if (FAILED(hr_)) {
    CheckHResult(hr_, "CommandAllocator::Reset failed", graphics->GetDevice());
    assert(SUCCEEDED(hr_));
  }
  hr_ =
      command->GetCommandList()->Reset(command->GetCommandAllocator(), nullptr);
  if (FAILED(hr_)) {
    CheckHResult(hr_, "CommandList::Reset failed", graphics->GetDevice());
    assert(SUCCEEDED(hr_));
  }
}

void Engine::FlushGpu() {
  // 蓄積されたコマンドリストを確定してキック
  hr_ = command->GetCommandList()->Close();
  if (FAILED(hr_)) {
    CheckHResult(hr_, "CommandList::Close failed in FlushGpu", graphics->GetDevice());
    assert(SUCCEEDED(hr_));
    return;
  }

  ID3D12CommandList *commandLists[] = {command->GetCommandList()};
  command->GetCommandQueue()->ExecuteCommandLists(1, commandLists);

  gpuSyncManager.Signal(command.get()->GetCommandQueue());
  gpuSyncManager.WaitForGpu();

  // 初期化等で生成された転送用中間バッファを全破棄してVRAMを即時解放
  if (textureLoader) {
    textureLoader->ReleaseIntermediateResources();
  }

  // 次の描画コマンドのためにコマンドリストをリセット
  hr_ = command->GetCommandAllocator()->Reset();
  if (FAILED(hr_)) {
    CheckHResult(hr_, "CommandAllocator::Reset failed in FlushGpu", graphics->GetDevice());
    assert(SUCCEEDED(hr_));
  }
  hr_ = command->GetCommandList()->Reset(command->GetCommandAllocator(), nullptr);
  if (FAILED(hr_)) {
    CheckHResult(hr_, "CommandList::Reset failed in FlushGpu", graphics->GetDevice());
    assert(SUCCEEDED(hr_));
  }
}

void Engine::End() {

  Audio::Finalize();

#ifdef _USE_IMGUI
  imGuiManager->Shutdown();
#endif // _USE_IMGUI

  window.Finalize();
}

void Engine::UpdateFixFPS() {
  constexpr float kMicrosecondsPerSecond = 1000000.0f;
  const std::chrono::microseconds kMinTime(uint64_t(kMicrosecondsPerSecond / kTargetFPS));
  const std::chrono::microseconds kMinCheckTime(uint64_t(kMicrosecondsPerSecond / kCheckFPS));

  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

  std::chrono::microseconds elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

  // 1/60秒経っていない場合
  if (elapsed < kMinCheckTime) {
    while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  }
  reference_ = std::chrono::steady_clock::now();
}

size_t Engine::GetProcessMemoryUsage() {
  PROCESS_MEMORY_COUNTERS_EX pmc{};
  if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc,
                           sizeof(pmc))) {
    return pmc.WorkingSetSize; // 現在の物理メモリ使用量(バイト)
  }
  return 0;
}
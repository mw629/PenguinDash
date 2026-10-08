#pragma once
#include <fstream>
#include <chrono>
#include <functional>

///自作エンジン///

//Common
#include "CommandContext.h"

//Core
#include "LogHandler.h"
#include "VariableTypes.h"
#include "WindowConfig.h"
#include "ImGuiManager.h"

//GameObjects

//Light
#include "LightManager.h"

//Camera

#include "Camera.h"
#include "DebugCamera.h"


#include "MaterialFactory.h"
#include "Model.h"
#include "CharacterAnimator.h"
#include "TransformAnimation.h"
#include "EffectDefinition.h"
#include "Sphere.h"
#include "Cylinder.h"
#include "Sprite.h"
#include "Triangle.h"
#include "Ring.h"
#include "Cube.h"
#include "Line.h"
#include "Grid.h"
#include "LineRenderer.h"


//Graphics

#include "DepthStencil.h"
#include "DescriptorHeap.h"
#include "Draw.h"
#include "GpuSyncManager.h"
#include "GraphicsDevice.h"
#include "RenderTargetView.h"
#include "ResourceBarrierHelper.h"
#include "SwapChain.h"
#include "ViewportScissor.h"
#include "RenderTexture.h"
#include "Graphics/GpuProfiler.h"
#include "Graphics/Font/TextRenderer.h"
//PSO
#include "PipelineState.h"
#include "ShaderCompiler.h"
#include "GraphicsPipelineState.h"
#include "RootSignature.h"
#include "Sampler.h"

//Input

#include "Input.h"
#include "GamePadInput.h"

//Math

#include "Calculation.h"

//Resource

#include "Audio.h"
#include "Load.h"
#include "Texture.h"
#include "TextureLoader.h"


#include "Graphics/Render/PostEffect.h"

class Engine
{
public:
	HRESULT hr_;
	std::chrono::steady_clock::time_point reference_;

	int32_t kClientWidth_;
	int32_t kClientHeight_;

	std::ofstream logStream;

	WindowConfig window;

	std::unique_ptr<GraphicsDevice> graphics;
	std::unique_ptr<CommandContext> command;
	std::unique_ptr<SwapChain> swapChain;
	std::unique_ptr<DescriptorHeap> descriptorHeap;
	std::unique_ptr<RenderTargetView> renderTargetView;
	std::unique_ptr<ViewportScissor> viewportScissor;

	std::unique_ptr<Input> input;
	std::unique_ptr<GamePadInput> gamePadInput;

	std::unique_ptr<DebugCamera> debugCamera;
	std::unique_ptr<DepthStencil> depthStencil;
	std::unique_ptr<RenderTexture> renderTextures[2];

	std::unique_ptr<Draw> draw;
	std::unique_ptr<TextureLoader> textureLoader;
	std::vector<std::unique_ptr<PostEffect>> postEffects_;
	std::unique_ptr<ImGuiManager> imGuiManager;

	GpuSyncManager gpuSyncManager;

	std::unique_ptr<ResourceBarrierHelper> resourceBarrierHelper;

	std::unique_ptr<GraphicsPipelineState> graphicsPipelineState;
	std::unique_ptr<LightManager> lightManager;
	std::unique_ptr<LineRenderer> lineRenderer;
	std::unique_ptr<GpuProfiler> gpuProfiler;
	std::unique_ptr<MatchaEngine::TextRenderer> textRenderer;

	ID3D12DescriptorHeap* descriptorHeaps[1];


	static bool isEnd_;

public:

	~Engine();
	Engine(int32_t kClientWidth, int32_t kClientHeight);

	void Setting();

	void PostDraw(const std::function<void()>& drawUI = nullptr);

	void NewFrame();

	void EndFrame(const std::function<void()>& drawUI = nullptr);

	void FlushGpu();

	void End();

	void UpdateFixFPS();

	size_t GetProcessMemoryUsage();

	void Debug(); // Editor分離後は非推奨・不要になりますが一旦空で残すか削除します

	Input* GetInput() { return input.get(); }
	const std::vector<std::unique_ptr<PostEffect>>& GetPostEffects() { return postEffects_; }
	LightManager* GetLightManager() { return lightManager.get(); }
	TextureLoader* GetTextureLoader() { return textureLoader.get(); }
	Draw* GetDraw() { return draw.get(); }
	RenderTexture* GetRenderTexture() { return renderTextures[0].get(); } // The main scene is always rendered to renderTextures[0]
	RenderTexture* GetFinalRenderTexture(); // Return the final texture after all post effects
	int32_t GetClientWidth() const { return kClientWidth_; }
	int32_t GetClientHeight() const { return kClientHeight_; }
	GpuProfiler* GetGpuProfiler() { return gpuProfiler.get(); }
	MatchaEngine::TextRenderer* GetTextRenderer() { return textRenderer.get(); }

	static void SetEnd(bool isEnd) { isEnd_ = isEnd; }
	static bool IsEnd() { return isEnd_; }

	void ShowWindow() { window.Show(); }
	WindowConfig& GetWindow() { return window; }
};





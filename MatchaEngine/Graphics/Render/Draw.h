#pragma once  
#include "../Core/VariableTypes.h"  
#include "Math/Calculation.h"  
#include "PSO/GraphicsPipelineState.h"
#include <d3d12.h>  
#include <cstdint>  

#include "ObjectBase.h"

#include "Model.h"
#include "EffectDefinition.h"
#include "Sprite.h"
#include "Cube.h"
#include "Sphere.h"
#include "Triangle.h"
#include "Line.h"
#include "Grid.h"
#include "LineRenderer.h"
#include "Camera.h"

#include "CharacterAnimator.h"

#include "LightManager.h"

namespace MatchaEngine {
	class TextRenderer;
	enum class TextScaleMode;
}

class Draw {
public:

	void Initialize(ID3D12GraphicsCommandList* commandList, GraphicsPipelineState* graphicsPipelineState, 
		LightManager* lightManager, LineRenderer* lineRenderer = nullptr);

	LightManager* GetLightManager() const { return lightManager_; }
	LineRenderer* GetLineRenderer() const { return lineRenderer_; }

	void SetCamera(Camera* setcamera);
	void SetEnvironmentTexture(int textureHandle);
	void SetGpuProfiler(class GpuProfiler* profiler) { gpuProfiler_ = profiler; }

	void preDraw(ShaderName shader, BlendMode blend, CullMode cull = kCullModeNone);

	void ResetCullingStats() { totalDrawCalls_ = 0; culledDrawCalls_ = 0; }
	uint32_t GetTotalDrawCalls() const { return totalDrawCalls_; }
	uint32_t GetCulledDrawCalls() const { return culledDrawCalls_; }
	void SetFrustumCullingEnabled(bool enable) { isFrustumCullingEnabled_ = enable; }
	bool IsFrustumCullingEnabled() const { return isFrustumCullingEnabled_; }
	void SetDebugDrawAABB(bool enable) { isDebugDrawAABB_ = enable; }
	bool IsDebugDrawAABB() const { return isDebugDrawAABB_; }
	void DrawWireframeAABB(const AABB& aabb, const Vector4& color = { 0.0f, 1.0f, 0.0f, 1.0f });

	void DrawObj(ObjectBase *obj);

	void DrawAnimation(CharacterAnimator* obj);

	void DrawModel(Model* model);

   void DrawParticle(EffectDefinition* particle);

	void DrawSprite(Sprite* sprite);

	void DrawSphere(Sphere* sphere);

	void DrawTriangle(Triangle* triangle);

	void DrawLine(Line* line);

	void DrawGrid(Grid* grid);

	void DrawAllLines(LineRenderer* lineRenderer, bool depthTest = true);


	void DrawPostEffect(D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, ShaderName shader = "CopyShader", class PostEffect* postEffect = nullptr, D3D12_GPU_DESCRIPTOR_HANDLE depthTextureHandle = {0});

	void SetTextRenderer(class MatchaEngine::TextRenderer* textRenderer) { textRenderer_ = textRenderer; }
	class MatchaEngine::TextRenderer* GetTextRenderer() const { return textRenderer_; }

	void SetTextReferenceResolution(float width, float height);
	void SetTextScaleMode(MatchaEngine::TextScaleMode mode);
	MatchaEngine::TextScaleMode GetTextScaleMode() const;

	void SetTextBaseBoldness(float boldness);
	float GetTextBaseBoldness() const;

	void DrawMSDFString(const std::string& text, const Vector2& pos, float fontSize = 32.0f,
		const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
		bool enableOutline = false, const Vector4& outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f },
		float outlineWidth = 0.15f,
		float boldness = 0.0f);

	void DrawMSDFString(const std::wstring& text, const Vector2& pos, float fontSize = 32.0f,
		const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
		bool enableOutline = false, const Vector4& outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f },
		float outlineWidth = 0.15f,
		float boldness = 0.0f);

	// 太字描画用の簡易メソッド (アウトラインなしで手軽に太字描画)
	void DrawMSDFStringBold(const std::string& text, const Vector2& pos, float fontSize = 32.0f,
		const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
		float boldness = 0.08f);

	void DrawMSDFStringBold(const std::wstring& text, const Vector2& pos, float fontSize = 32.0f,
		const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
		float boldness = 0.08f);

	void DrawFillRect(const Vector2& pos, const Vector2& size, const Vector4& color = { 0.0f, 0.0f, 0.0f, 0.7f });

private:
	void SetCBV(ShaderName shader, BlendMode blend, const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address);
	void SetSRV(ShaderName shader, BlendMode blend, const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address);
	void SetTable(ShaderName shader, BlendMode blend, const std::string& name, D3D12_GPU_DESCRIPTOR_HANDLE handle);

	void BindCommonSceneParameters(ShaderName shader, BlendMode blend);
	bool IsFrustumCulled(const AABB& worldAABB, bool cullingEnabled);

	ID3D12GraphicsCommandList* commandList_{};
	GraphicsPipelineState* graphicsPipelineState_{};
	LightManager* lightManager_{};
	LineRenderer* lineRenderer_{};
	Camera* camera_{};
	D3D12_GPU_DESCRIPTOR_HANDLE environmentTextureSrvHandleGPU_{};
	class GpuProfiler* gpuProfiler_{};
	class MatchaEngine::TextRenderer* textRenderer_{};

	bool isFrustumCullingEnabled_ = true;
	bool isDebugDrawAABB_ = false;
	uint32_t totalDrawCalls_ = 0;
	uint32_t culledDrawCalls_ = 0;
};

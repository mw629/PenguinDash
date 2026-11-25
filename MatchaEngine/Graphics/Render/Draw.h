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

	/// <summary>
	/// 描画マネージャの初期化
	/// </summary>
	/// <param name="commandList">D3D12グラフィックスコマンドリスト</param>
	/// <param name="graphicsPipelineState">パイプラインステート管理</param>
	/// <param name="lightManager">ライト管理</param>
	/// <param name="lineRenderer">ライン描画管理</param>
	void Initialize(ID3D12GraphicsCommandList* commandList, GraphicsPipelineState* graphicsPipelineState, 
		LightManager* lightManager, LineRenderer* lineRenderer = nullptr);

	LightManager* GetLightManager() const { return lightManager_; }
	LineRenderer* GetLineRenderer() const { return lineRenderer_; }

	/// <summary>
	/// 描画に使用するアクティブカメラの設定
	/// </summary>
	void SetCamera(Camera* setcamera);
	Camera* GetCamera() const { return camera_; }

	/// <summary>
	/// 環境マップ（スカイボックス）テクスチャの設定
	/// </summary>
	void SetEnvironmentTexture(int textureHandle);

	/// <summary>
	/// GPUプロファイラの設定
	/// </summary>
	void SetGpuProfiler(class GpuProfiler* profiler) { gpuProfiler_ = profiler; }

	/// <summary>
	/// 描画前のパイプラインステート・共通定数バッファのバインド
	/// </summary>
	void preDraw(ShaderName shader, BlendMode blend, CullMode cull = kCullModeNone);

	void ResetCullingStats() { totalDrawCalls_ = 0; culledDrawCalls_ = 0; }
	uint32_t GetTotalDrawCalls() const { return totalDrawCalls_; }
	uint32_t GetCulledDrawCalls() const { return culledDrawCalls_; }
	void SetFrustumCullingEnabled(bool enable) { isFrustumCullingEnabled_ = enable; }
	bool IsFrustumCullingEnabled() const { return isFrustumCullingEnabled_; }
	void SetDebugDrawAABB(bool enable) { isDebugDrawAABB_ = enable; }
	bool IsDebugDrawAABB() const { return isDebugDrawAABB_; }
	void DrawWireframeAABB(const AABB& aabb, const Vector4& color = { 0.0f, 1.0f, 0.0f, 1.0f });

	/// <summary>
	/// 汎用3Dオブジェクト（ObjectBase継承）の描画
	/// </summary>
	void DrawObj(ObjectBase *obj);

	/// <summary>
	/// スキニングアニメーションモデル（CharacterAnimator）の描画
	/// </summary>
	void DrawAnimation(CharacterAnimator* obj);

	/// <summary>
	/// 通常3Dモデル（Model）の描画
	/// </summary>
	void DrawModel(Model* model);

	/// <summary>
	/// GPUパーティクルエフェクトの描画
	/// </summary>
	void DrawParticle(EffectDefinition* particle);

	/// <summary>
	/// 2Dスプライトの描画
	/// </summary>
	void DrawSprite(Sprite* sprite);

	/// <summary>
	/// 3D球体の描画
	/// </summary>
	void DrawSphere(Sphere* sphere);

	/// <summary>
	/// 3D三角形メッシュの描画
	/// </summary>
	void DrawTriangle(Triangle* triangle);

	/// <summary>
	/// 3D単一ラインの描画
	/// </summary>
	void DrawLine(Line* line);

	/// <summary>
	/// 3Dグリッドの描画
	/// </summary>
	void DrawGrid(Grid* grid);

	/// <summary>
	/// 蓄積された全ラインの一括描画
	/// </summary>
	/// <param name="lineRenderer">対象ラインレンダラー</param>
	/// <param name="depthTest">深度テストを有効にするか</param>
	void DrawAllLines(LineRenderer* lineRenderer, bool depthTest = true);

	/// <summary>
	/// ポストエフェクトシェーダーによる全画面描画
	/// </summary>
	void DrawPostEffect(D3D12_GPU_DESCRIPTOR_HANDLE textureHandle, ShaderName shader = "CopyShader", class PostEffect* postEffect = nullptr, D3D12_GPU_DESCRIPTOR_HANDLE depthTextureHandle = {0}, BlendMode blend = BlendMode::kBlendModeNone);

	void SetTextRenderer(class MatchaEngine::TextRenderer* textRenderer) { textRenderer_ = textRenderer; }
	class MatchaEngine::TextRenderer* GetTextRenderer() const { return textRenderer_; }

	void SetTextReferenceResolution(float width, float height);
	void SetTextScaleMode(MatchaEngine::TextScaleMode mode);
	MatchaEngine::TextScaleMode GetTextScaleMode() const;

	void SetTextBaseBoldness(float boldness);
	float GetTextBaseBoldness() const;

	/// <summary>
	/// MSDFフォントによるUTF-8文字列の2D描画
	/// </summary>
	void DrawMSDFString(const std::string& text, const Vector2& pos, float fontSize = 32.0f,
		const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
		bool enableOutline = false, const Vector4& outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f },
		float outlineWidth = 0.15f,
		float boldness = 0.0f);

	/// <summary>
	/// MSDFフォントによるUTF-16文字列（日本語等）の2D描画
	/// </summary>
	void DrawMSDFString(const std::wstring& text, const Vector2& pos, float fontSize = 32.0f,
		const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
		bool enableOutline = false, const Vector4& outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f },
		float outlineWidth = 0.15f,
		float boldness = 0.0f);

	/// <summary>
	/// 太字でのMSDFテキスト描画（UTF-8）
	/// </summary>
	void DrawMSDFStringBold(const std::string& text, const Vector2& pos, float fontSize = 32.0f,
		const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
		float boldness = 0.08f);

	/// <summary>
	/// 太字でのMSDFテキスト描画（UTF-16）
	/// </summary>
	void DrawMSDFStringBold(const std::wstring& text, const Vector2& pos, float fontSize = 32.0f,
		const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
		float boldness = 0.08f);

	/// <summary>
	/// 2D矩形塗りつぶし描画（半透明背景パネル等）
	/// </summary>
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

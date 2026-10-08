#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include "Core/VariableTypes.h"
#include "MaterialFactory.h"
#include "PipelineState.h"
#include "../Object/GameObject.h"
#include "../Component/MaterialComponent.h"

/// <summary>
/// スプライトの画面変動時スケーリングモード
/// </summary>
enum class SpriteScaleMode {
	Fit,       // アスペクト比維持で画面内に収める (レターボックス/ピラーボックス)
	Fill,      // アスペクト比維持で画面全体を覆う (余白なし、端は見切れ)
	Stretch,   // 画面全体に合わせて引き伸ばす (アスペクト比無視)
	None       // スケーリングなし (実画面ピクセル座標系)
};

/// <summary>
/// スプライトのアンカー位置（画面変動時の基準点）
/// </summary>
enum class SpriteAnchor {
	None,         // 仮想解像度(基準解像度)の座標系にそのまま配置
	TopLeft,      // 画面左上
	TopCenter,    // 画面上中央
	TopRight,     // 画面右上
	MiddleLeft,   // 画面左中央
	Center,       // 画面中央
	MiddleRight,  // 画面右中央
	BottomLeft,   // 画面左下
	BottomCenter, // 画面下中央
	BottomRight   // 画面右下
};

struct SpriteData {
	Transform transform;
	Vector2 size;
	Vector2 textureArea[2];
	Vector2 pivot = { 0.5f, 0.5f };                    // 原点位置 (0,0: 左上, 0.5,0.5: 中央, 1,1: 右下)
	SpriteScaleMode scaleMode = SpriteScaleMode::Fit;   // スケーリングモード
	SpriteAnchor anchor = SpriteAnchor::None;          // アンカー位置
};

class Sprite : public GameObject
{
public:
	static constexpr float kDefaultReferenceWidth = 1280.0f;
	static constexpr float kDefaultReferenceHeight = 720.0f;
	static constexpr Vector2 kDefaultReferenceResolution = { kDefaultReferenceWidth, kDefaultReferenceHeight };

	struct ScreenTransformResult {
		Vector2 screenPosition;
		Vector2 scale;
	};

private:
	// 静的画面情報
	static float kClientWidth;
	static float kClientHeight;
	static Vector2 referenceResolution_;
	static std::vector<Sprite*> instances_;

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	VertexData* vertexData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	uint32_t* indexData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
	TransformationMatrix* wvpData_ = nullptr;

	Vector2 size_ = { 200.0f, 200.0f };
	Vector2 textureArea_[2] = { {0.0f, 0.0f}, {1.0f, 1.0f} };
	Vector2 pivot_ = { 0.5f, 0.5f };
	SpriteScaleMode scaleMode_ = SpriteScaleMode::Fit;
	SpriteAnchor anchor_ = SpriteAnchor::None;

	Vector2 leftTop_;
	Vector2 rigthBottom_;
	Vector2 textureLeftTop_;
	Vector2 textureRigthBottom_;

public:
	Sprite();
	~Sprite();

	// 画面サイズ・基準解像度の設定
	static void SetScreenSize(Vector2 screenSize);
	static Vector2 GetScreenSize();
	static void SetReferenceResolution(Vector2 refResolution);
	static Vector2 GetReferenceResolution();

	void Initialize(SpriteData spriteData, int textureHandle);

	void CreateVertexData();
	void CreateIndexResource();
	void CreateWVP();
	void CreateSprite();

	void SettingWvp();
	void UpdateVertexBuffer();
	void SetCustomTriangleVertices(const Vector2 positions[3], const Vector2 uvs[3]);

	void SetTransform(Transform transform);
	void SetSize(Vector2 size);
	void SetTextureArea(Vector2 textureArea[2]);
	void SetPivot(Vector2 pivot);
	void SetScaleMode(SpriteScaleMode scaleMode);
	void SetAnchor(SpriteAnchor anchor);

	Vector2 GetPivot() const { return pivot_; }
	SpriteScaleMode GetScaleMode() const { return scaleMode_; }
	SpriteAnchor GetAnchor() const { return anchor_; }
	Vector2 GetSize() const { return size_; }

	ScreenTransformResult CalculateScreenTransform() const;

	void Update(SpriteData spriteData);

	void SetMaterialLighting(bool isActive) { 
		auto matComp = GetComponent<MaterialComponent>();
		if(matComp) matComp->GetMaterialFactory()->SetMaterialLighting(isActive);
	}

	void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU) { textureSrvHandleGPU_ = textureSrvHandleGPU; }

	D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() { return &vertexBufferView_; }
	ID3D12Resource* GetVertexResource() { return wvpResource_.Get(); }
	D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() { return &indexBufferView_; }

	Transform& GetTransform() { return transform_; }
	
	MaterialFactory* GetMaterial() { 
		auto matComp = GetComponent<MaterialComponent>();
		return matComp ? matComp->GetMaterialFactory() : nullptr;
	}
	[[deprecated("Use GetMaterial instead")]]
	MaterialFactory* GetMartial() { return GetMaterial(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU() { return textureSrvHandleGPU_; }

private:
	ShaderName shader_ = "ObjectShader";
	BlendMode blend_ = BlendMode::kBlendModeNormal;
public:
	void SetBlend(BlendMode blend) { blend_ = blend; }
	ShaderName GetShader() { return shader_; }
	BlendMode GetBlend() { return blend_; }

};




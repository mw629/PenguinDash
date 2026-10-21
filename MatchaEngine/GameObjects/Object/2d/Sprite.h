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
	static constexpr Vector2 kDefaultPivot = { 0.5f, 0.5f };

	Transform transform;
	Vector2 size;
	Vector2 textureArea[2];
	Vector2 pivot = kDefaultPivot;                    // 原点位置 (0,0: 左上, 0.5,0.5: 中央, 1,1: 右下)
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
	// 内部定数
	static constexpr uint32_t kVertexCount = 4;
	static constexpr uint32_t kVertexBufferSize = 6;
	static constexpr uint32_t kIndexCount = 6;
	static constexpr Vector3 kDefaultNormal = { 0.0f, 0.0f, -1.0f };
	static constexpr Vector2 kDefaultSize = { 200.0f, 200.0f };
	static constexpr float kDefaultNearClip = 0.0f;
	static constexpr float kDefaultFarClip = 100.0f;

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

	Vector2 size_ = kDefaultSize;
	Vector2 textureArea_[2] = { {0.0f, 0.0f}, {1.0f, 1.0f} };
	Vector2 pivot_ = SpriteData::kDefaultPivot;
	SpriteScaleMode scaleMode_ = SpriteScaleMode::Fit;
	SpriteAnchor anchor_ = SpriteAnchor::None;

	Vector2 leftTop_;
	Vector2 rigthBottom_;
	Vector2 textureLeftTop_;
	Vector2 textureRigthBottom_;

public:
	Sprite();
	~Sprite();

	/// <summary>
	/// 画面サイズ（ピクセル）の設定
	/// </summary>
	static void SetScreenSize(Vector2 screenSize);
	static Vector2 GetScreenSize();

	/// <summary>
	/// 仮想解像度（基準解像度）の設定
	/// </summary>
	static void SetReferenceResolution(Vector2 refResolution);
	static Vector2 GetReferenceResolution();

	/// <summary>
	/// スプライトの初期化
	/// </summary>
	/// <param name="spriteData">初期スプライトデータ</param>
	/// <param name="textureHandle">テクスチャハンドル</param>
	void Initialize(SpriteData spriteData, int textureHandle);

	/// <summary>
	/// 頂点リソースの生成
	/// </summary>
	void CreateVertexData();

	/// <summary>
	/// インデックスリソースの生成
	/// </summary>
	void CreateIndexResource();

	/// <summary>
	/// WVP定数バッファの生成
	/// </summary>
	void CreateWVP();

	/// <summary>
	/// スプライトリソース一括生成
	/// </summary>
	void CreateSprite();

	/// <summary>
	/// WVP行列の計算および定数バッファ転送
	/// </summary>
	void SettingWvp();

	/// <summary>
	/// 頂点バッファデータの更新（位置・UV）
	/// </summary>
	void UpdateVertexBuffer();

	/// <summary>
	/// カスタム三角形頂点の設定
	/// </summary>
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

	/// <summary>
	/// アンカー・スケールモードを適用した画面上座標とスケールを計算
	/// </summary>
	ScreenTransformResult CalculateScreenTransform() const;

	/// <summary>
	/// スプライトデータの毎フレーム更新
	/// </summary>
	/// <param name="spriteData">更新後スプライトデータ</param>
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




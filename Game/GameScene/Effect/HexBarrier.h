#pragma once
#include "ObjectBase.h"
#include <vector>
#include <string>
#include <directxmath.h>

enum class HexBarrierShape {
	Single,    // 単一の六角形パネル
	Honeycomb, // 7枚の六角形が結合したハニカムシールド
};

class HexBarrier : public ObjectBase {
public:
	HexBarrier();
	~HexBarrier() override = default;

	/// <summary>
	/// バリアの初期化
	/// </summary>
	/// <param name="texturePath">使用するテクスチャパス</param>
	void Initialize(const std::string& texturePath = "Resources/Texture/white64x64.png");

	/// <summary>
	/// 頂点リソースの生成
	/// </summary>
	void CreateVertexData() override;

	/// <summary>
	/// インデックスリソースの生成
	/// </summary>
	void CreateIndexResource() override;

	/// <summary>
	/// メッシュの再構築（半径や形状に応じた頂点バッファの再生成）
	/// </summary>
	void RebuildMesh();

	/// <summary>
	/// 六角形の外接円半径を設定
	/// </summary>
	/// <param name="radius">半径</param>
	void SetRadius(float radius);

	/// <summary>
	/// 頂点の向き（尖り頭/平ら頭）を設定
	/// </summary>
	/// <param name="pointyTopped">true: 尖り頭, false: 平ら頭</param>
	void SetPointyTopped(bool pointyTopped);

	/// <summary>
	/// バリア形状（単一/ハニカム）を設定
	/// </summary>
	/// <param name="shape">形状タイプ</param>
	void SetShape(HexBarrierShape shape);

	float GetRadius() const { return radius_; }
	bool GetPointyTopped() const { return pointyTopped_; }
	HexBarrierShape GetShape() const { return shape_; }

	/// <summary>
	/// 六角形メッシュの頂点・インデックス生成ヘルパー
	/// </summary>
	/// <param name="radius">外接円半径</param>
	/// <param name="pointyTopped">尖り頭フラグ</param>
	/// <param name="outVertices">出力先頂点配列</param>
	/// <param name="outIndices">出力先インデックス配列</param>
	/// <param name="centerOffset">中心オフセット</param>
	/// <param name="uvScale">UVスケール</param>
	/// <param name="uvCenter">UV中心</param>
	static void GenerateHexBarrierMesh(
		float radius, bool pointyTopped,
		std::vector<VertexData>& outVertices,
		std::vector<uint32_t>& outIndices,
		DirectX::XMFLOAT3 centerOffset = { 0.0f, 0.0f, 0.0f },
		float uvScale = 1.0f,
		DirectX::XMFLOAT2 uvCenter = { 0.5f, 0.5f }
	);

private:
	static constexpr float kDefaultRadius = 0.5f;
	static constexpr Vector4 kDefaultBarrierColor = { 0.25f, 0.8f, 1.0f, 0.85f }; // エレクトリックシアン

	float radius_ = kDefaultRadius;
	bool pointyTopped_ = false; // false: 上辺が水平 / true: 上辺が頂点 (30度回転)
	HexBarrierShape shape_ = HexBarrierShape::Honeycomb;
};

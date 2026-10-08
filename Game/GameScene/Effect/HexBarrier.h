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

	void Initialize(const std::string& texturePath = "Resources/Texture/white64x64.png");

	void CreateVertexData() override;
	void CreateIndexResource() override;

	// メッシュの再構築（半径や形状変更時）
	void RebuildMesh();

	// パラメータ設定
	void SetRadius(float radius);
	void SetPointyTopped(bool pointyTopped);
	void SetShape(HexBarrierShape shape);

	float GetRadius() const { return radius_; }
	bool GetPointyTopped() const { return pointyTopped_; }
	HexBarrierShape GetShape() const { return shape_; }

	// 六角形メッシュ（ローカル座標系）の生成
	static void GenerateHexBarrierMesh(
		float radius, bool pointyTopped,
		std::vector<VertexData>& outVertices,
		std::vector<uint32_t>& outIndices,
		DirectX::XMFLOAT3 centerOffset = { 0.0f, 0.0f, 0.0f },
		float uvScale = 1.0f,
		DirectX::XMFLOAT2 uvCenter = { 0.5f, 0.5f }
	);

private:
	float radius_ = 0.5f;
	bool pointyTopped_ = false; // false: 上辺が水平 / true: 上辺が頂点 (30度回転)
	HexBarrierShape shape_ = HexBarrierShape::Honeycomb;
};

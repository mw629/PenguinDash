#include "HexBarrier.h"
#include "Graphics/GraphicsDevice.h"
#include <cmath>

HexBarrier::HexBarrier()
{
}

void HexBarrier::Initialize(const std::string& texturePath)
{
	if (!texturePath.empty()) {
		texture->CreateTexture(texturePath);
		textureSrvHandleGPU_ = texture->TextureData(texturePath);
	}

	auto matComp = AddComponent<MaterialComponent>();
	if (matComp) {
		matComp->SetShader("ObjectShader");
		matComp->SetBlend(BlendMode::kBlendModeAdd);
		auto factory = matComp->GetMaterialFactory();
		if (factory) {
			factory->SetMaterialLighting(false);
			factory->SetColor({ 0.25f, 0.8f, 1.0f, 0.85f }); // エレクトリックシアン
		}
	}

	CreateObject();
}

void HexBarrier::GenerateHexBarrierMesh(
	float radius, bool pointyTopped,
	std::vector<VertexData>& outVertices,
	std::vector<uint32_t>& outIndices,
	DirectX::XMFLOAT3 centerOffset,
	float uvScale,
	DirectX::XMFLOAT2 uvCenter)
{
	using namespace DirectX;

	uint32_t baseIndex = static_cast<uint32_t>(outVertices.size());

	// 法線はバリア正面（+Z方向）
	Vector3 normal = { 0.0f, 0.0f, 1.0f };

	// 0番: 中心点
	VertexData centerVertex;
	centerVertex.position = { centerOffset.x, centerOffset.y, centerOffset.z, 1.0f };
	centerVertex.normal = normal;
	centerVertex.texcoord = { uvCenter.x, uvCenter.y };
	outVertices.push_back(centerVertex);

	// 1~6番: 外周頂点
	float angleOffset = pointyTopped ? (DirectX::XM_PI / 6.0f) : 0.0f; // 30度 or 0度

	for (int i = 0; i < 6; ++i) {
		float angle = angleOffset + (i * DirectX::XM_PI / 3.0f); // 60度刻み
		float localX = radius * std::cos(angle);
		float localY = radius * std::sin(angle);

		// UV座標（0.0 ~ 1.0に正規化、uvCenterとuvScaleを考慮）
		float u = uvCenter.x + (localX / (2.0f * radius)) * uvScale;
		float v = uvCenter.y - (localY / (2.0f * radius)) * uvScale; // DirectX UV系（下が+V）

		VertexData vtx;
		vtx.position = { centerOffset.x + localX, centerOffset.y + localY, centerOffset.z, 1.0f };
		vtx.normal = normal;
		vtx.texcoord = { u, v };
		outVertices.push_back(vtx);
	}

	// インデックス定義
	// 両面描画（表面: 時計回り、裏面: 反時計回り）を登録し、カメラの手前/奥の両方から確実に視認できるようにする
	for (uint32_t i = 1; i <= 6; ++i) {
		uint32_t next = (i % 6) + 1;

		// 表面 (+Z向き)
		outIndices.push_back(baseIndex + 0);
		outIndices.push_back(baseIndex + i);
		outIndices.push_back(baseIndex + next);

		// 裏面 (-Z向き、カメラ手前から視認可能)
		outIndices.push_back(baseIndex + 0);
		outIndices.push_back(baseIndex + next);
		outIndices.push_back(baseIndex + i);
	}
}

void HexBarrier::CreateVertexData()
{
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;

	if (shape_ == HexBarrierShape::Single) {
		// 単一の六角形パネル
		GenerateHexBarrierMesh(radius_, pointyTopped_, vertices, indices, { 0.0f, 0.0f, 0.0f });
	}
	else {
		// 7枚の六角形が結合したハニカムシールド
		// 中心セル
		GenerateHexBarrierMesh(radius_, pointyTopped_, vertices, indices, { 0.0f, 0.0f, 0.0f }, 0.5f, { 0.5f, 0.5f });

		// 六角形の間隔（外接円半径 radius に対し、隣接中心間距離は sqrt(3) * radius）
		float dist = std::sqrt(3.0f) * radius_;

		// 外周6セルの中心位置
		// pointyTopped == true (頂点が上) の場合、辺の法線方向は 0度, 60度, 120度...
		// pointyTopped == false (水平辺が上) の場合、辺の法線方向は 30度, 90度, 150度...
		float dirOffset = pointyTopped_ ? 0.0f : (DirectX::XM_PI / 6.0f);

		for (int k = 0; k < 6; ++k) {
			float angle = dirOffset + (k * DirectX::XM_PI / 3.0f);
			float cx = dist * std::cos(angle);
			float cy = dist * std::sin(angle);
			// わずかに手前に湾曲させることでドーム状の立体シールド感を演出
			float cz = -0.06f;

			// UVオフセット
			float uvX = 0.5f + (cx / (dist * 2.5f));
			float uvY = 0.5f - (cy / (dist * 2.5f));

			GenerateHexBarrierMesh(radius_, pointyTopped_, vertices, indices, { cx, cy, cz }, 0.4f, { uvX, uvY });
		}
	}

	vertexSize_ = static_cast<int>(vertices.size());
	vertexResource_ = GraphicsDevice::CreateBufferResource(sizeof(VertexData) * vertexSize_);
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * vertexSize_;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	std::memcpy(vertexData_, vertices.data(), sizeof(VertexData) * vertexSize_);
}

void HexBarrier::CreateIndexResource()
{
	std::vector<VertexData> dummyVertices;
	std::vector<uint32_t> indices;

	if (shape_ == HexBarrierShape::Single) {
		GenerateHexBarrierMesh(radius_, pointyTopped_, dummyVertices, indices, { 0.0f, 0.0f, 0.0f });
	}
	else {
		// 中心
		GenerateHexBarrierMesh(radius_, pointyTopped_, dummyVertices, indices, { 0.0f, 0.0f, 0.0f });
		// 周囲6個
		float dist = std::sqrt(3.0f) * radius_;
		float dirOffset = pointyTopped_ ? 0.0f : (DirectX::XM_PI / 6.0f);
		for (int k = 0; k < 6; ++k) {
			float angle = dirOffset + (k * DirectX::XM_PI / 3.0f);
			float cx = dist * std::cos(angle);
			float cy = dist * std::sin(angle);
			GenerateHexBarrierMesh(radius_, pointyTopped_, dummyVertices, indices, { cx, cy, -0.06f });
		}
	}

	indexSize_ = static_cast<int>(indices.size());
	indexResource_ = GraphicsDevice::CreateBufferResource(sizeof(uint32_t) * indexSize_);
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * indexSize_;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
	std::memcpy(indexData_, indices.data(), sizeof(uint32_t) * indexSize_);
}

void HexBarrier::RebuildMesh()
{
	CreateVertexData();
	CreateIndexResource();
}

void HexBarrier::SetRadius(float radius)
{
	if (std::abs(radius_ - radius) > 0.001f) {
		radius_ = radius;
		RebuildMesh();
	}
}

void HexBarrier::SetPointyTopped(bool pointyTopped)
{
	if (pointyTopped_ != pointyTopped) {
		pointyTopped_ = pointyTopped;
		RebuildMesh();
	}
}

void HexBarrier::SetShape(HexBarrierShape shape)
{
	if (shape_ != shape) {
		shape_ = shape;
		RebuildMesh();
	}
}

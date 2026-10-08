#include "Ring.h"

Ring::~Ring()
{
}

void Ring::Initialize(int textureSrvHandle)
{

	radianPerDivide_ = 2.0f * 3.14f / float(kRingDivide_);

	textureSrvHandleGPU_ = texture.get()->TextureData(textureSrvHandle);

	AddComponent<MaterialComponent>();

	CreateObject();
}

void Ring::CreateVertexData()
{
	vertexSize_ = kRingDivide_ * 6;

	//Shpere用の頂点リソースを作る//

	vertexResource_ = GraphicsDevice::CreateBufferResource(sizeof(VertexData) * vertexSize_);

	//頂点バッファービューを作成する

	//リソースの先頭アドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * vertexSize_;
	//1頂点当たりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	//頂点データを設定する//

	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));


	for (uint32_t index = 0; index < kRingDivide_; ++index) {
		float sin = std::sin(radianPerDivide_ * index);
		float cos = std::cos(radianPerDivide_ * index);
		float sinNext = std::sin(radianPerDivide_ * (index + 1));
		float cosNext = std::cos(radianPerDivide_ * (index + 1));
		float u = float(index) / float(kRingDivide_);
		float uNext = float(index + 1) / float(kRingDivide_);

		uint32_t startIndex = index * 6;

		VertexData a;
		a.position = { -sin * outerRadius_, cos * outerRadius_, 0.0f, 1.0f };
		a.texcoord = { u, 0.0f };
		a.normal = { 0.0f, 0.0f, -1.0f };

		VertexData b;
		b.position = { -sinNext * outerRadius_, cosNext * outerRadius_, 0.0f, 1.0f };
		b.texcoord = { uNext, 0.0f };
		b.normal = { 0.0f, 0.0f, -1.0f };

		VertexData c;
		c.position = { -sin * innerRadius_, cos * innerRadius_, 0.0f, 1.0f };
		c.texcoord = { u, 1.0f };
		c.normal = { 0.0f, 0.0f, -1.0f };

		VertexData d;
		d.position = { -sinNext * innerRadius_, cosNext * innerRadius_, 0.0f, 1.0f };
		d.texcoord = { uNext, 1.0f };
		d.normal = { 0.0f, 0.0f, -1.0f };

		//頂点にデータを入力する。基準点a
		vertexData_[startIndex] = b;
		vertexData_[startIndex + 1] = c;
		vertexData_[startIndex + 2] = a;

		vertexData_[startIndex + 3] = d;
		vertexData_[startIndex + 4] = c;
		vertexData_[startIndex + 5] = b;
	}

}

void Ring::CreateIndexResource()
{
	// インデックス数は頂点数と同じ
	indexSize_ = vertexSize_;

	// インデックスバッファ作成
	indexResource_ = GraphicsDevice::CreateBufferResource(sizeof(uint32_t) * indexSize_);

	// インデックスバッファビュー設定
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * indexSize_;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// マップ
	uint32_t* indexData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	// 0,1,2,3,4,5... を順番に詰めるだけ
	for (int i = 0; i < indexSize_; ++i) {
		indexData[i] = i;
	}
}

#include "Cylinder.h"
#include <numbers>



Cylinder::~Cylinder()
{
}

void Cylinder::Initialize(int textureSrvHandle)
{
	divide_ = 32;
	topRadius_ = 1.0f;
	bottomRadius_ = 1.0f;
 height_ = 3.0f;
	radianPerDivide_ = 2.0f * std::numbers::pi_v<float> / float(divide_);

	textureSrvHandleGPU_ = texture.get()->TextureData(textureSrvHandle);

	AddComponent<MaterialComponent>();

	CreateObject();
}

void Cylinder::CreateVertexData()
{
	vertexSize_ = divide_ * 6;

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


	for (uint32_t index = 0; index < divide_; ++index) {
		float sin = std::sin(radianPerDivide_ * index);
		float cos = std::cos(radianPerDivide_ * index);
		float sinNext = std::sin(radianPerDivide_ * (index + 1));
		float cosNext = std::cos(radianPerDivide_ * (index + 1));
        float halfHeight = height_ * 0.5f;
		float u = float(index) / float(divide_);
		float uNext = float(index + 1) / float(divide_);

		uint32_t startIndex = index * 6;

       VertexData a;
		a.position = { -sin * topRadius_, halfHeight, cos * topRadius_, 1.0f };
		a.texcoord = { u, 0.0f };
		a.normal = { -sin, 0.0f, cos };

		VertexData b;
      b.position = { -sinNext * topRadius_, halfHeight, cosNext * topRadius_, 1.0f };
		b.texcoord = { uNext, 0.0f };
      b.normal = { -sinNext, 0.0f, cosNext };

		VertexData c;
     c.position = { -sin * bottomRadius_, -halfHeight, cos * bottomRadius_, 1.0f };
		c.texcoord = { u, 1.0f };
      c.normal = { -sin, 0.0f, cos };

		VertexData d;
     d.position = { -sinNext * bottomRadius_, -halfHeight, cosNext * bottomRadius_, 1.0f };
		d.texcoord = { uNext, 1.0f };
      d.normal = { -sinNext, 0.0f, cosNext };

		//頂点にデータを入力する。基準点a
		vertexData_[startIndex] = b;
		vertexData_[startIndex + 1] = c;
		vertexData_[startIndex + 2] = a;

		vertexData_[startIndex + 3] = d;
		vertexData_[startIndex + 4] = c;
		vertexData_[startIndex + 5] = b;
	}

}

void Cylinder::CreateIndexResource()
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
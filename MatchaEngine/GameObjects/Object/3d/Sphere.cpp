#include "Sphere.h"
#include "Graphics/GraphicsDevice.h"
#include "Core/VariableTypes.h"
#include "Math/Calculation.h"



Sphere::~Sphere()
{
}



void Sphere::Initialize(int textureSrvHandle)
{
	textureSrvHandleGPU_ = texture.get()->TextureData(textureSrvHandle);

	localAABB_ = { {-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f} };
	localSphere_ = { {0.0f, 0.0f, 0.0f}, 1.0f };

	AddComponent<MaterialComponent>();
	
	CreateObject(); // この行を追加
}

void Sphere::CreateVertexData()
{

	float pi = 3.14f;


	//Shpere用の頂点リソースを作る//

	vertexResource_ = GraphicsDevice::CreateBufferResource(sizeof(VertexData) * (kSubdivision_ * kSubdivision_) * 6);

	//頂点バッファービューを作成する

	//リソースの先頭アドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * (kSubdivision_ * kSubdivision_) * 6;
	//1頂点当たりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	//頂点データを設定する//

	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	//経度分割1つ分の角度φd
	const float kLonEvery = pi * 2.0f / float(kSubdivision_);
	//緯度分割1つ分の角度Θd
	const float kLatEvery = pi / float(kSubdivision_);
	//緯度の方向に分割	
	for (uint32_t latIndex = 0; latIndex < kSubdivision_; ++latIndex) {
		float lat = -pi / 2.0f + kLatEvery * latIndex;//Θ
		//経度方向に分割しながら線を描く
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision_; ++lonIndex) {
			uint32_t startIndex = (latIndex * kSubdivision_ + lonIndex) * 6;
			float lon = lonIndex * kLonEvery;

			//一つ目の三角形

			VertexData a;
			VertexData b;
			VertexData c;
			VertexData d;

			//position

			a.position = {
				cos(lat) * cos(lon),
				sin(lat),
				cos(lat) * sin(lon),
				1.0f };

			b.position = {
				cos(lat + kLatEvery) * cos(lon),
				sin(lat + kLatEvery),
				cos(lat + kLatEvery) * sin(lon) ,
				1.0f };

			c.position = {
				cos(lat) * cos(lon + kLonEvery),
				sin(lat),
				cos(lat) * sin(lon + kLonEvery),
				1.0f };

			d.position = {
				cos(lat + kLatEvery) * cos(lon + kLonEvery),
				sin(lat + kLatEvery),
				cos(lat + kLatEvery) * sin(lon + kLonEvery),
				1.0f };

			//texcoord
			a.texcoord = {
			float(lonIndex) / float(kSubdivision_),
			1.0f - float(latIndex) / float(kSubdivision_) };

			b.texcoord = {
				float(lonIndex) / float(kSubdivision_),
				1.0f - float(latIndex + 1) / float(kSubdivision_) };

			c.texcoord = {
				float(lonIndex + 1) / float(kSubdivision_),
				1.0f - float(latIndex) / float(kSubdivision_) };

			d.texcoord = {
				float(lonIndex + 1) / float(kSubdivision_),
				1.0f - float(latIndex + 1) / float(kSubdivision_) };

			//法線ベクトルを計算する

			a.normal = Vector3(a.position.x, a.position.y, a.position.z);
			b.normal = Vector3(b.position.x, b.position.y, b.position.z);
			c.normal = Vector3(c.position.x, c.position.y, c.position.z);
			d.normal = Vector3(d.position.x, d.position.y, d.position.z);


			//頂点にデータを入力する。基準点a
			vertexData_[startIndex] = b;
			vertexData_[startIndex + 1] = c;
			vertexData_[startIndex + 2] = a;
			//二つ目の三角形
			vertexData_[startIndex + 3] = d;
			vertexData_[startIndex + 4] = c;
			vertexData_[startIndex + 5] = b;


		}
	}
	vertexSize_ = kSubdivision_ * kSubdivision_ * 6;

}

void Sphere::CreateIndexResource()
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



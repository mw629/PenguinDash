#include "Grid.h"
#include "Graphics/GraphicsDevice.h" 
#include "Math/Calculation.h"       
#include <cassert>                  

namespace {
	float kClientWidth;
	float kClientHeight;
}


void Grid::SetScreenSize(Vector2 screenSize)
{
	kClientWidth = screenSize.x;
	kClientHeight = screenSize.y;
}

Grid::~Grid() {
	// MapしたポインタをUnmapする
	if (vertexData_) {
		vertexResource_->Unmap(0, nullptr);
	}
	if (wvpData_) {
		wvpResource_->Unmap(0, nullptr);
	}
	// ComPtrは自動的に解放される
	vertexResource_.Reset();
	wvpResource_.Reset();
}





void Grid::CreateVertexData()
{
	//VertexResourceを生成する//
	vertexResource_ = GraphicsDevice::CreateBufferResource(sizeof(LineVertexData) * kSubdivision_ * 4);
	//頂点バッファビューを作成する
	//リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点2つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(LineVertexData) * kSubdivision_ * 4;
	//1頂点当たりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(LineVertexData);
	//Resourceにデータを書き込む//

	//書き込むためのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));


	for (int i = 0; i < kSubdivision_; i++) {
		index = i * 2;
		Vector4 color = kGridColorZNormal;
		if (i % kMajorGridInterval == 0) {
			color = kGridColorZMajor;
		}
		vertexData_[index].position = Vector3{ static_cast<float>(0 - kSubdivision_ / 2 + i),0,static_cast<float>(-kSubdivision_ / 2) };
		vertexData_[index].color = color;

		vertexData_[index + 1].position = Vector3{ static_cast<float>(0 - kSubdivision_ / 2 + i),0,static_cast<float>(kSubdivision_ / 2) };
		vertexData_[index + 1].color = color;
	}
	for (int i = 0; i < kSubdivision_; i++) {
		index = kSubdivision_ * 2 + i * 2;
		Vector4 color = kGridColorXNormal;
		if (i % kMajorGridInterval == 0) {
			color = kGridColorXMajor;
		}

		vertexData_[index].position = Vector3{ static_cast<float>(-kSubdivision_ / 2),0,static_cast<float>(0 - kSubdivision_ / 2 + i) };
		vertexData_[index].color = color;

		vertexData_[index + 1].position = Vector3{ static_cast<float>(kSubdivision_ / 2),0,static_cast<float>(0 - kSubdivision_ / 2 + i) };
		vertexData_[index + 1].color = color;
	}
	vertexData_[kSubdivision_].color = kGridColorZAxis;
	vertexData_[kSubdivision_ + 1].color = kGridColorZAxis;
	vertexData_[kSubdivision_ * 2 + kSubdivision_].color = kGridColorXAxis;
	vertexData_[kSubdivision_ * 2 + kSubdivision_ + 1].color = kGridColorXAxis;
}

void Grid::CreateWVP() {
	//WVP用のリソースを作る
	wvpResource_ = GraphicsDevice::CreateBufferResource(sizeof(LineTransformationMatrix));
	//
	wvpData_ = nullptr;
	//
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
	//
	wvpData_->WVP = IdentityMatrix();
}



void Grid::CreateGrid()
{
	CreateVertexData();
	CreateWVP();
}

#include "Camera.h"

void Grid::SettingWvp(Matrix4x4 viewMatrix, const Matrix4x4* customProjection)
{
	float aspect = (kClientHeight > 0.0f) ? (kClientWidth / kClientHeight) : Camera::kDefaultAspectRatio;
	Matrix4x4 projectionMatri = customProjection
		? *customProjection
		: MakePerspectiveFovMatrix(Camera::kDefaultFovY, aspect, Camera::kDefaultNearClip, Camera::kDefaultFarClip);
	Matrix4x4 worldMatrix = MakeAffineMatrix(Vector3(0, 0, 0), Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f));
	Matrix4x4 worldViewProjectionMatrix = MultiplyMatrix4x4(worldMatrix, MultiplyMatrix4x4(viewMatrix, projectionMatri));
	*wvpData_ = { worldViewProjectionMatrix };
}




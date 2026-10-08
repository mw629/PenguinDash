#include "Line.h"
#include "Graphics/GraphicsDevice.h"
#include "Math/Calculation.h"       
#include <cassert>                  

namespace {
	float kClientWidth;
	float kClientHeight;
}


void Line::SetScreenSize(Vector2 screenSize)
{
	kClientWidth = screenSize.x;
	kClientHeight = screenSize.y;
}


Line::~Line(){
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





void Line::CreateVertexData()
{
	//VertexResourceを生成する//
	vertexResource_ = GraphicsDevice::CreateBufferResource(sizeof(LineVertexData) * 2);
	//頂点バッファビューを作成する
	//リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点2つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(LineVertexData) * 2;
	//1頂点当たりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(LineVertexData);
	//Resouceにデータを書き込む//

	//書き込むためのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	//左下
	vertexData_[0] = vertex_[0];
	//上
	vertexData_[1] = vertex_[1];
}

void Line::CreateWVP() {
	//WVP用のリソースを作る
	wvpResource_ = GraphicsDevice::CreateBufferResource(sizeof(LineTransformationMatrix));
	//
	wvpData_ = nullptr;
	//
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
	//
	wvpData_->WVP = IdentityMatrix();
}



void Line::CreateLine()
{
	CreateVertexData();
	CreateWVP();
}

void Line::SettingWvp(Matrix4x4 viewMatrix)
{
	Matrix4x4 projectionMatri = MakePerspectiveFovMatrix(0.45f, float(1280) / float(720), 0.1f, 10000.0f);
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.translate, transform_.scale, transform_.rotate);
	Matrix4x4 worldViewProjectionMatrix = MultiplyMatrix4x4(worldMatrix, MultiplyMatrix4x4(viewMatrix, projectionMatri));
	*wvpData_ = { worldViewProjectionMatrix };

}




void Line::SetTransform(Transform transform)
{
	transform_.translate = transform.translate;
	transform_.scale = transform.scale;
	transform_.rotate = transform.rotate;

}

void Line::SetVertex(LineVertexData vertex[2])
{
	for (int i = 0; i < 2; i++) {
		vertexData_[i] = vertex[i];
	}

}

#include "Cube.h"
#include "Graphics/GraphicsDevice.h"
#include "Core/VariableTypes.h"
#include "Math/Calculation.h"

Cube::~Cube()
{
}

void Cube::Initialize(int textureSrvHandle)
{
    textureSrvHandleGPU_ = texture.get()->TextureData(textureSrvHandle);

    AddComponent<MaterialComponent>();

    CreateObject();
}

void Cube::CreateVertexData()
{
    // 頂点の角は8個なので8頂点に削減
    vertexSize_ = 8;
    vertexResource_ = GraphicsDevice::CreateBufferResource(sizeof(VertexData) * vertexSize_);
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * vertexSize_;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    // ローカル座標の8つの角を定義
    // SkyBox用であれば法線やUVはそのまま使わない・共有で問題ないため一括で処理できます
    vertexData_[0].position = { -kDefaultHalfSize, -kDefaultHalfSize, -kDefaultHalfSize, 1.0f }; // 0: 左下奥
    vertexData_[1].position = { -kDefaultHalfSize,  kDefaultHalfSize, -kDefaultHalfSize, 1.0f }; // 1: 左上奥
    vertexData_[2].position = {  kDefaultHalfSize, -kDefaultHalfSize, -kDefaultHalfSize, 1.0f }; // 2: 右下奥
    vertexData_[3].position = {  kDefaultHalfSize,  kDefaultHalfSize, -kDefaultHalfSize, 1.0f }; // 3: 右上奥
    vertexData_[4].position = { -kDefaultHalfSize, -kDefaultHalfSize,  kDefaultHalfSize, 1.0f }; // 4: 左下手前
    vertexData_[5].position = { -kDefaultHalfSize,  kDefaultHalfSize,  kDefaultHalfSize, 1.0f }; // 5: 左上手前
    vertexData_[6].position = {  kDefaultHalfSize, -kDefaultHalfSize,  kDefaultHalfSize, 1.0f }; // 6: 右下手前
    vertexData_[7].position = {  kDefaultHalfSize,  kDefaultHalfSize,  kDefaultHalfSize, 1.0f }; // 7: 右上手前

    // 便宜上のデータを埋めておく
    for (int i = 0; i < 8; ++i) {
        vertexData_[i].texcoord = { 0.0f, 0.0f };
        
        // 0,0,0だとShaderでNaNが発生してCubemapのSampleでGPUがクラッシュするので、
        // positionを元に正規化した法線を設定する
        Vector3 pos = { vertexData_[i].position.x, vertexData_[i].position.y, vertexData_[i].position.z };
        float length = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
        if (length > 0.0f) {
            vertexData_[i].normal = { pos.x / length, pos.y / length, pos.z / length };
        } else {
            vertexData_[i].normal = { 0.0f, 1.0f, 0.0f };
        }
    }
}

void Cube::CreateIndexResource()
{
    indexSize_ = 36;
    indexResource_ = GraphicsDevice::CreateBufferResource(sizeof(uint32_t) * indexSize_);
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * indexSize_;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

    // 8頂点を使い回して6面(36インデックス)を構成 (時計回り)
    // Front (-Z)
    indexData_[0]  = 0; indexData_[1]  = 1; indexData_[2]  = 2;
    indexData_[3]  = 2; indexData_[4]  = 1; indexData_[5]  = 3;
    // Back (+Z)
    indexData_[6]  = 6; indexData_[7]  = 7; indexData_[8]  = 4;
    indexData_[9]  = 4; indexData_[10] = 7; indexData_[11] = 5;
    // Top (+Y)
    indexData_[12] = 1; indexData_[13] = 5; indexData_[14] = 3;
    indexData_[15] = 3; indexData_[16] = 5; indexData_[17] = 7;
    // Bottom (-Y)
    indexData_[18] = 4; indexData_[19] = 0; indexData_[20] = 6;
    indexData_[21] = 6; indexData_[22] = 0; indexData_[23] = 2;
    // Right (+X)
    indexData_[24] = 2; indexData_[25] = 3; indexData_[26] = 6;
    indexData_[27] = 6; indexData_[28] = 3; indexData_[29] = 7;
    // Left (-X)
    indexData_[30] = 4; indexData_[31] = 5; indexData_[32] = 0;
    indexData_[33] = 0; indexData_[34] = 5; indexData_[35] = 1;
}

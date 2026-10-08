#include "LineRenderer.h"
#include "../../Graphics/GraphicsDevice.h"
#include "../../Math/Calculation.h"

LineRenderer::LineRenderer() {
    vertices_.reserve(kMaxVertexCount);
}

LineRenderer::~LineRenderer() {
    if (vertexData_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (wvpData_) {
        wvpResource_->Unmap(0, nullptr);
    }
    vertexResource_.Reset();
    wvpResource_.Reset();
}

void LineRenderer::Initialize() {
    // 頂点バッファの作成
    vertexResource_ = GraphicsDevice::CreateBufferResource(sizeof(LineVertexData) * kMaxVertexCount);
    
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(LineVertexData) * kMaxVertexCount;
    vertexBufferView_.StrideInBytes = sizeof(LineVertexData);

    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    // WVPバッファの作成
    wvpResource_ = GraphicsDevice::CreateBufferResource(sizeof(LineTransformationMatrix));
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = IdentityMatrix();
}

void LineRenderer::AddLine(const Vector3& start, const Vector3& end, const Vector4& color) {
    if (vertices_.size() + 2 <= kMaxVertexCount) {
        vertices_.push_back({ start, color });
        vertices_.push_back({ end, color });
    }
}

void LineRenderer::DrawAll(ID3D12GraphicsCommandList* commandList, Camera* camera) {
    if (vertices_.empty() || !camera) {
        return;
    }

    // カメラの行列を更新
    wvpData_->WVP = camera->GetViewProjectionMatrix();

    // 頂点データを更新
    size_t drawCount = vertices_.size();
    memcpy(vertexData_, vertices_.data(), sizeof(LineVertexData) * drawCount);

    // 描画
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    
    // 定数バッファ (gTransform) の設定は呼び出し元 (Draw クラス) で行うか、ここで固定スロットにセットするかですが、
    // Draw::DrawLines 側で PipelineState や CBV のバインドを想定するため、CBVのアドレスだけ取得可能にしておくか、
    // ここでバインドさせないといけません。
    // ※今回は呼び出し元の Draw 側で SetCBV してもらうために必要な GPU アドレスは DrawAll 呼び出し前に行う前提とします。
    // (ここでは描画コールだけ)
    commandList->DrawInstanced(static_cast<UINT>(drawCount), 1, 0, 0);

    // リセット
    vertices_.clear();
}

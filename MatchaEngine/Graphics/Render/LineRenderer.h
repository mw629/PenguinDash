#pragma once
#include "../../Core/VariableTypes.h"
#include "../../Graphics/GraphicsDevice.h"
#include "../../GameObjects/Camera/Camera.h"
#include <wrl.h>
#include <vector>

class LineRenderer {
public:
    static const uint32_t kMaxLineCount = 2048;
    static const uint32_t kMaxVertexCount = kMaxLineCount * 2;

    LineRenderer();
    ~LineRenderer();

    void Initialize();
    
    // 描画する線を追加する。毎フレーム追加し、描画後にリセットされます。
    void AddLine(const Vector3& start, const Vector3& end, const Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f});

    void DrawAll(ID3D12GraphicsCommandList* commandList, Camera* camera);

    ID3D12Resource* GetWVPResource() const { return wvpResource_.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    LineVertexData* vertexData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
    LineTransformationMatrix* wvpData_ = nullptr;

    std::vector<LineVertexData> vertices_;
};

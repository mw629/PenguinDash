#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include "MsdfGenerator.h"
#include "../../Math/Calculation.h"
#include "../DescriptorHeap.h"

namespace MatchaEngine {

struct FontGlyph {
    uint32_t codepoint = 0;
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    float width = 0.0f;        // グリフのピクセル幅
    float height = 0.0f;       // グリフのピクセル高さ
    float offsetX = 0.0f;      // ベースラインからのオフセットX
    float offsetY = 0.0f;      // ベースラインからのオフセットY (上端)
    float advanceWidth = 0.0f; // 次の文字までの進み幅
};

class DynamicFontAtlas {
public:
    DynamicFontAtlas();
    ~DynamicFontAtlas();

    // アトラスの初期化
    // device: D3D12デバイス
    // descriptorHeap: SRV用デスクリプタヒープ
    // fontPath: TTF/OTFフォントファイルのパス
    // atlasWidth, atlasHeight: アトラス解像度 (通常 2048x2048)
    // glyphBaseSize: MSDFグリフ生成の基準ピクセルサイズ (通常 36.0f)
    // pxRange: MSDFの符号付き距離範囲 (通常 4.0f)
    bool Initialize(ID3D12Device* device,
                    DescriptorHeap* descriptorHeap,
                    const std::string& fontPath,
                    int atlasWidth = 2048,
                    int atlasHeight = 2048,
                    float glyphBaseSize = 36.0f,
                    float pxRange = 4.0f);

    // 指定したUnicodeコードポイントのグリフ情報を取得 (未登録なら自動で動的生成)
    const FontGlyph* GetOrAddGlyph(uint32_t codepoint);

    // 文字列に含まれる全文字を事前に登録 (未登録のものがあれば生成)
    void PreloadString(const std::string& utf8Text);
    void PreloadString(const std::wstring& wideText);

    // 新規追加されたグリフをGPUテクスチャへアップロード (フレーム描画前に呼ぶ)
    void UpdateGpu(ID3D12GraphicsCommandList* commandList);

    // フレーム開始通知 (ダブルバッファリング管理)
    void BeginFrame(size_t frameIndex);

    // 単色矩形描画用の白色ピクセルUV
    Vector2 GetWhitePixelUV() const { return whitePixelUV_; }

    // SRVハンドルを取得 (シェーダーでサンプリングする用)
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU() const { return srvHandleGPU_; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSrvHandleCPU() const { return srvHandleCPU_; }

    // フォントの基本メトリクス
    float GetGlyphBaseSize() const { return glyphBaseSize_; }
    float GetPxRange() const { return pxRange_; }
    Vector2 GetAtlasSize() const { return Vector2(static_cast<float>(atlasWidth_), static_cast<float>(atlasHeight_)); }
    void GetVerticalMetrics(float& ascent, float& descent, float& lineGap) {
        msdfGen_.GetVerticalMetrics(glyphBaseSize_, ascent, descent, lineGap);
    }

private:
    struct PendingRegion {
        int dstX = 0;
        int dstY = 0;
        int width = 0;
        int height = 0;
        std::vector<uint8_t> pixels; // RGBA8
    };

    ID3D12Device* device_ = nullptr;
    DescriptorHeap* descriptorHeap_ = nullptr;

    MsdfGenerator msdfGen_;
    int atlasWidth_ = 2048;
    int atlasHeight_ = 2048;
    float glyphBaseSize_ = 36.0f;
    float pxRange_ = 4.0f;

    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadResource_;
    uint8_t* uploadMappedPtr_ = nullptr;
    UINT64 uploadBufferSize_ = 0;
    UINT64 uploadBufferOffset_ = 0;
    UINT64 uploadBufferFrameEnd_ = 0;

    Vector2 whitePixelUV_{ 0.0f, 0.0f };

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_{};

    // グリフキャッシュ
    std::unordered_map<uint32_t, FontGlyph> glyphCache_;
    std::vector<PendingRegion> pendingUploads_;

    // パッカー用内部データ
    struct PackerContext;
    std::unique_ptr<PackerContext> packer_;
};

} // namespace MatchaEngine

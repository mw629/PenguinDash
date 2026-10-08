#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <memory>
#include "DynamicFontAtlas.h"
#include "../../Math/Calculation.h"
#include "../PSO/GraphicsPipelineState.h"

namespace MatchaEngine {

struct TextVertex {
    Vector4 position; // x, y, z, w (スクリーン座標)
    Vector2 texcoord; // u, v (アトラスUV)
    Vector4 color;    // r, g, b, a
};

struct TextParamsConstantBuffer {
    Matrix4x4 wvp;              // 画面サイズ用正射影行列 (64 bytes)
    Vector4 outlineColor;       // アウトライン色 (16 bytes)
    float outlineWidth = 0.0f;  // 0.0fでアウトラインなし (4 bytes)
    float boldness = 0.0f;      // 0.0fで標準太さ、正で太く、負で細く (4 bytes)
    Vector2 texSize = { 2048.0f, 2048.0f }; // アトラス解像度 (8 bytes)
    float pxRange = 4.0f;       // MSDFピクセル範囲 (4 bytes)
    float pad[3] = { 0.0f, 0.0f, 0.0f };   // 16バイト境界パディング (12 bytes)
};
static_assert(sizeof(TextParamsConstantBuffer) == 112, "TextParamsConstantBuffer size mismatch with HLSL TextParams");

/// <summary>
/// 2D・テキスト描画の画面変動時スケーリングモード
/// </summary>
enum class TextScaleMode {
    Fit,     // アスペクト比維持で画面内に収める (Letterbox/Pillarbox、中央配置)
    Fill,    // アスペクト比維持で画面全体を覆う
    Stretch, // 画面全体に合わせて引き伸ばす (アスペクト比無視)
    None     // スケーリングなし (実画面ピクセル直接配置)
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // 初期化
    // device: D3D12デバイス
    // descriptorHeap: SRV用デスクリプタヒープ
    // pipelineState: GraphicsPipelineState
    // fontPath: フォントファイルへのパス (NotoSansJP等)
    bool Initialize(ID3D12Device* device,
                    DescriptorHeap* descriptorHeap,
                    GraphicsPipelineState* pipelineState,
                    const std::string& fontPath,
                    int atlasWidth = 2048,
                    int atlasHeight = 2048);

    // 画面サイズの設定 (ウィンドウリサイズ時に呼び出し)
    void SetScreenSize(float screenWidth, float screenHeight);

    // 基準解像度の設定 (デフォルト: 1280x720)
    void SetReferenceResolution(float refWidth, float refHeight) {
        referenceWidth_ = refWidth;
        referenceHeight_ = refHeight;
    }
    Vector2 GetReferenceResolution() const { return { referenceWidth_, referenceHeight_ }; }
    Vector2 GetScreenSize() const { return { screenWidth_, screenHeight_ }; }

    // スケーリングモードの設定
    void SetScaleMode(TextScaleMode mode) { scaleMode_ = mode; }
    TextScaleMode GetScaleMode() const { return scaleMode_; }

    // 2D投影行列の計算
    Matrix4x4 Calculate2DProjectionMatrix() const;

    // 文字列の描画 (UTF-8)
    // text: 表示する文字列
    // pos: 描画開始座標 (左上ベース)
    // fontSize: 表示フォントサイズ (ピクセル単位、例: 32.0f)
    // color: 文字色 (RGBA)
    // enableOutline: 縁取りを有効にするか
    // outlineColor: 縁取り色
    // outlineWidth: 縁取り幅 (0.0f〜0.5f、通常 0.1f〜0.2f)
    // boldness: 太さオフセット (0.0fで通常、正の値で太く、負の値で細く。通常 0.02f〜0.15f)
    void DrawString(ID3D12GraphicsCommandList* commandList,
                    const std::string& text,
                    const Vector2& pos,
                    float fontSize = 32.0f,
                    const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
                    bool enableOutline = false,
                    const Vector4& outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f },
                    float outlineWidth = 0.15f,
                    float boldness = 0.0f);

    // 文字列の描画 (wide string)
    void DrawString(ID3D12GraphicsCommandList* commandList,
                    const std::wstring& text,
                    const Vector2& pos,
                    float fontSize = 32.0f,
                    const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
                    bool enableOutline = false,
                    const Vector4& outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f },
                    float outlineWidth = 0.15f,
                    float boldness = 0.0f);

    // 太字描画の簡易メソッド (アウトラインなしで手軽に太字描画)
    void DrawStringBold(ID3D12GraphicsCommandList* commandList,
                        const std::string& text,
                        const Vector2& pos,
                        float fontSize = 32.0f,
                        const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
                        float boldness = 0.08f) {
        DrawString(commandList, text, pos, fontSize, color, false, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.15f, boldness);
    }

    void DrawStringBold(ID3D12GraphicsCommandList* commandList,
                        const std::wstring& text,
                        const Vector2& pos,
                        float fontSize = 32.0f,
                        const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
                        float boldness = 0.08f) {
        DrawString(commandList, text, pos, fontSize, color, false, { 0.0f, 0.0f, 0.0f, 1.0f }, 0.15f, boldness);
    }

    // 単色塗りつぶし矩形（背景パネルやゲージ用）の描画
    void DrawFillRect(ID3D12GraphicsCommandList* commandList,
                      const Vector2& pos,
                      const Vector2& size,
                      const Vector4& color);

    // 文字列の描画サイズ計測 (幅, 高さ)
    Vector2 MeasureString(const std::string& text, float fontSize = 32.0f);
    Vector2 MeasureString(const std::wstring& text, float fontSize = 32.0f);

    // フレーム開始時のバッファリセット (毎フレーム呼び出し)
    void BeginFrame();

    // 基本の太さオフセット設定（全体の文字を太くするベース値）
    void SetBaseBoldness(float baseBoldness) { baseBoldness_ = baseBoldness; }
    float GetBaseBoldness() const { return baseBoldness_; }

    DynamicFontAtlas* GetAtlas() { return atlas_.get(); }

private:
    static constexpr size_t kFrameCount = 2;
    static constexpr size_t kMaxVerticesPerFrame = 65536;
    static constexpr size_t kMaxIndicesPerFrame = 98304;
    static constexpr size_t kMaxDrawCallsPerFrame = 256;
    static constexpr size_t kConstantBufferAlignment = 256;

    ID3D12Device* device_ = nullptr;
    GraphicsPipelineState* pipelineState_ = nullptr;
    std::unique_ptr<DynamicFontAtlas> atlas_;

    float screenWidth_ = 1280.0f;
    float screenHeight_ = 720.0f;
    float referenceWidth_ = 1280.0f;
    float referenceHeight_ = 720.0f;
    TextScaleMode scaleMode_ = TextScaleMode::Fit;
    float baseBoldness_ = 0.07f; // 全体的なデフォルト太さオフセット (0.0fが標準、0.07fでしっかり太字)

    size_t currentFrameIndex_ = 0;

    // ダブルバッファリング対応GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffers_[kFrameCount];
    D3D12_VERTEX_BUFFER_VIEW vertexBufferViews_[kFrameCount]{};
    TextVertex* mappedVertices_[kFrameCount]{};
    size_t currentVertexOffset_[kFrameCount]{};

    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffers_[kFrameCount];
    D3D12_INDEX_BUFFER_VIEW indexBufferViews_[kFrameCount]{};
    uint32_t* mappedIndices_[kFrameCount]{};
    size_t currentIndexOffset_[kFrameCount]{};

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffers_[kFrameCount];
    uint8_t* mappedConstantBuffers_[kFrameCount]{};
    size_t currentDrawCall_[kFrameCount]{};
};

} // namespace MatchaEngine

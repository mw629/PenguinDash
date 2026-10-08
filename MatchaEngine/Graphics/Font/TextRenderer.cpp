#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "TextRenderer.h"
#include <d3dx12.h>
#include <cassert>
#include <algorithm>
#include "../GraphicsDevice.h"
#include "Core/LogHandler.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace MatchaEngine {

namespace {

uint32_t DecodeUtf8(const char*& ptr, const char* end) {
    if (ptr >= end) return 0;
    unsigned char c = static_cast<unsigned char>(*ptr++);
    if (c < 0x80) {
        return c;
    } else if ((c & 0xE0) == 0xC0) {
        if (ptr >= end) return 0;
        uint32_t cp = (c & 0x1F) << 6;
        cp |= (static_cast<unsigned char>(*ptr++) & 0x3F);
        return cp;
    } else if ((c & 0xF0) == 0xE0) {
        if (ptr + 1 >= end) return 0;
        uint32_t cp = (c & 0x0F) << 12;
        cp |= (static_cast<unsigned char>(*ptr++) & 0x3F) << 6;
        cp |= (static_cast<unsigned char>(*ptr++) & 0x3F);
        return cp;
    } else if ((c & 0xF8) == 0xF0) {
        if (ptr + 2 >= end) return 0;
        uint32_t cp = (c & 0x07) << 18;
        cp |= (static_cast<unsigned char>(*ptr++) & 0x3F) << 12;
        cp |= (static_cast<unsigned char>(*ptr++) & 0x3F) << 6;
        cp |= (static_cast<unsigned char>(*ptr++) & 0x3F);
        return cp;
    }
    return 0;
}

} // namespace

TextRenderer::TextRenderer() = default;

TextRenderer::~TextRenderer() {
    for (size_t f = 0; f < kFrameCount; ++f) {
        if (mappedVertices_[f] && vertexBuffers_[f]) {
            vertexBuffers_[f]->Unmap(0, nullptr);
            mappedVertices_[f] = nullptr;
        }
        if (mappedIndices_[f] && indexBuffers_[f]) {
            indexBuffers_[f]->Unmap(0, nullptr);
            mappedIndices_[f] = nullptr;
        }
        if (mappedConstantBuffers_[f] && constantBuffers_[f]) {
            constantBuffers_[f]->Unmap(0, nullptr);
            mappedConstantBuffers_[f] = nullptr;
        }
    }
}

bool TextRenderer::Initialize(ID3D12Device* device,
                              DescriptorHeap* descriptorHeap,
                              GraphicsPipelineState* pipelineState,
                              const std::string& fontPath,
                              int atlasWidth,
                              int atlasHeight) {
    device_ = device;
    pipelineState_ = pipelineState;

    atlas_ = std::make_unique<DynamicFontAtlas>();
    if (!atlas_->Initialize(device_, descriptorHeap, fontPath, atlasWidth, atlasHeight, 36.0f, 4.0f)) {
        LOG_ERROR("TextRenderer: Failed to initialize DynamicFontAtlas!");
        return false;
    }

    // ダブルバッファリング対応のGPUリソースを確保
    for (size_t f = 0; f < kFrameCount; ++f) {
        // 頂点バッファ
        size_t vbSize = sizeof(TextVertex) * kMaxVerticesPerFrame;
        vertexBuffers_[f] = GraphicsDevice::CreateBufferResource(vbSize);
        HRESULT hr = vertexBuffers_[f]->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices_[f]));
        if (FAILED(hr)) {
            LOG_ERROR("TextRenderer: Failed to map vertex buffer!");
            return false;
        }
        vertexBufferViews_[f].BufferLocation = vertexBuffers_[f]->GetGPUVirtualAddress();
        vertexBufferViews_[f].SizeInBytes = static_cast<UINT>(vbSize);
        vertexBufferViews_[f].StrideInBytes = sizeof(TextVertex);

        // インデックスバッファ
        size_t ibSize = sizeof(uint32_t) * kMaxIndicesPerFrame;
        indexBuffers_[f] = GraphicsDevice::CreateBufferResource(ibSize);
        hr = indexBuffers_[f]->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndices_[f]));
        if (FAILED(hr)) {
            LOG_ERROR("TextRenderer: Failed to map index buffer!");
            return false;
        }
        indexBufferViews_[f].BufferLocation = indexBuffers_[f]->GetGPUVirtualAddress();
        indexBufferViews_[f].SizeInBytes = static_cast<UINT>(ibSize);
        indexBufferViews_[f].Format = DXGI_FORMAT_R32_UINT;

        // 定数バッファ (256バイトアライメント x 最大ドローコール数)
        size_t cbSize = kConstantBufferAlignment * kMaxDrawCallsPerFrame;
        constantBuffers_[f] = GraphicsDevice::CreateBufferResource(cbSize);
        hr = constantBuffers_[f]->Map(0, nullptr, reinterpret_cast<void**>(&mappedConstantBuffers_[f]));
        if (FAILED(hr)) {
            LOG_ERROR("TextRenderer: Failed to map constant buffer!");
            return false;
        }

        currentVertexOffset_[f] = 0;
        currentIndexOffset_[f] = 0;
        currentDrawCall_[f] = 0;
    }

    currentFrameIndex_ = 0;

    LOG_INFO("TextRenderer initialized successfully with multi-draw support.");
    return true;
}

void TextRenderer::SetScreenSize(float screenWidth, float screenHeight) {
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
}

Matrix4x4 TextRenderer::Calculate2DProjectionMatrix() const {
    float sw = screenWidth_ > 0.0f ? screenWidth_ : referenceWidth_;
    float sh = screenHeight_ > 0.0f ? screenHeight_ : referenceHeight_;
    float rw = referenceWidth_ > 0.0f ? referenceWidth_ : 1280.0f;
    float rh = referenceHeight_ > 0.0f ? referenceHeight_ : 720.0f;

    if (scaleMode_ == TextScaleMode::None) {
        return MakeOrthographicMatrix(0.0f, sw, 0.0f, sh, 0.0f, 100.0f);
    }

    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    switch (scaleMode_) {
    case TextScaleMode::Fit: {
        float scale = (std::min)(sw / rw, sh / rh);
        scaleX = scale;
        scaleY = scale;
        offsetX = (sw - rw * scale) * 0.5f;
        offsetY = (sh - rh * scale) * 0.5f;
        break;
    }
    case TextScaleMode::Fill: {
        float scale = (std::max)(sw / rw, sh / rh);
        scaleX = scale;
        scaleY = scale;
        offsetX = (sw - rw * scale) * 0.5f;
        offsetY = (sh - rh * scale) * 0.5f;
        break;
    }
    case TextScaleMode::Stretch: {
        scaleX = sw / rw;
        scaleY = sh / rh;
        offsetX = 0.0f;
        offsetY = 0.0f;
        break;
    }
    default:
        break;
    }

    Matrix4x4 view2D = IdentityMatrix();
    view2D.m[0][0] = scaleX;
    view2D.m[1][1] = scaleY;
    view2D.m[3][0] = offsetX;
    view2D.m[3][1] = offsetY;

    Matrix4x4 ortho = MakeOrthographicMatrix(0.0f, sw, 0.0f, sh, 0.0f, 100.0f);
    return MultiplyMatrix4x4(view2D, ortho);
}

void TextRenderer::BeginFrame() {
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
    currentVertexOffset_[currentFrameIndex_] = 0;
    currentIndexOffset_[currentFrameIndex_] = 0;
    currentDrawCall_[currentFrameIndex_] = 0;
    if (atlas_) {
        atlas_->BeginFrame(currentFrameIndex_);
    }
}

Vector2 TextRenderer::MeasureString(const std::string& text, float fontSize) {
    if (!atlas_) return { 0.0f, 0.0f };

    float scale = fontSize / atlas_->GetGlyphBaseSize();
    float cursorX = 0.0f;
    float maxX = 0.0f;
    float lineHeight = fontSize * 1.2f;
    int lineCount = 1;

    const char* ptr = text.data();
    const char* end = ptr + text.size();
    while (ptr < end) {
        uint32_t cp = DecodeUtf8(ptr, end);
        if (cp == '\n') {
            maxX = (std::max)(maxX, cursorX);
            cursorX = 0.0f;
            lineCount++;
            continue;
        }
        const FontGlyph* glyph = atlas_->GetOrAddGlyph(cp);
        if (glyph) {
            cursorX += glyph->advanceWidth * scale;
        }
    }
    maxX = (std::max)(maxX, cursorX);
    return { maxX, lineCount * lineHeight };
}

Vector2 TextRenderer::MeasureString(const std::wstring& text, float fontSize) {
    if (!atlas_) return { 0.0f, 0.0f };

    float scale = fontSize / atlas_->GetGlyphBaseSize();
    float cursorX = 0.0f;
    float maxX = 0.0f;
    float lineHeight = fontSize * 1.2f;
    int lineCount = 1;

    for (wchar_t wc : text) {
        uint32_t cp = static_cast<uint32_t>(wc);
        if (cp == '\n') {
            maxX = (std::max)(maxX, cursorX);
            cursorX = 0.0f;
            lineCount++;
            continue;
        }
        const FontGlyph* glyph = atlas_->GetOrAddGlyph(cp);
        if (glyph) {
            cursorX += glyph->advanceWidth * scale;
        }
    }
    maxX = (std::max)(maxX, cursorX);
    return { maxX, lineCount * lineHeight };
}

void TextRenderer::DrawString(ID3D12GraphicsCommandList* commandList,
                              const std::string& text,
                              const Vector2& pos,
                              float fontSize,
                              const Vector4& color,
                              bool enableOutline,
                              const Vector4& outlineColor,
                              float outlineWidth,
                              float boldness) {
    if (text.empty() || !atlas_ || !commandList) return;

    // 1. 文字列の解析と頂点・インデックスの構築
    float scale = fontSize / atlas_->GetGlyphBaseSize();
    float lineHeight = fontSize * 1.2f;

    float ascent, descent, lineGap;
    atlas_->GetVerticalMetrics(ascent, descent, lineGap);
    float baselineOffset = ascent * scale;

    float cursorX = pos.x;
    float cursorY = pos.y + baselineOffset;

    std::vector<TextVertex> vertices;
    std::vector<uint32_t> indices;

    const char* ptr = text.data();
    const char* end = ptr + text.size();
    while (ptr < end) {
        uint32_t cp = DecodeUtf8(ptr, end);
        if (cp == '\n') {
            cursorX = pos.x;
            cursorY += lineHeight;
            continue;
        }

        const FontGlyph* glyph = atlas_->GetOrAddGlyph(cp);
        if (!glyph) continue;

        if (glyph->width > 0.0f && glyph->height > 0.0f) {
            float x0 = cursorX + glyph->offsetX * scale;
            float x1 = x0 + glyph->width * scale;
            float y0 = cursorY - glyph->offsetY * scale;
            float y1 = y0 + glyph->height * scale;

            uint32_t quadBase = static_cast<uint32_t>(vertices.size());

            // 4頂点 (左上、右上、左下、右下)
            TextVertex v0{ { x0, y0, 0.0f, 1.0f }, { glyph->u0, glyph->v0 }, color };
            TextVertex v1{ { x1, y0, 0.0f, 1.0f }, { glyph->u1, glyph->v0 }, color };
            TextVertex v2{ { x0, y1, 0.0f, 1.0f }, { glyph->u0, glyph->v1 }, color };
            TextVertex v3{ { x1, y1, 0.0f, 1.0f }, { glyph->u1, glyph->v1 }, color };

            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v3);

            // 2つの三角形 (0-1-2, 1-3-2)
            indices.push_back(quadBase + 0);
            indices.push_back(quadBase + 1);
            indices.push_back(quadBase + 2);
            indices.push_back(quadBase + 1);
            indices.push_back(quadBase + 3);
            indices.push_back(quadBase + 2);
        }

        cursorX += glyph->advanceWidth * scale;
    }

    if (vertices.empty()) return;

    // 2. 新規グリフのGPUアップロード (今回登録された文字を即座にテクスチャへ転送)
    atlas_->UpdateGpu(commandList);

    // 3. 現在フレームのスロットとオフセットの取得
    size_t f = currentFrameIndex_;
    if (currentVertexOffset_[f] + vertices.size() > kMaxVerticesPerFrame ||
        currentIndexOffset_[f] + indices.size() > kMaxIndicesPerFrame ||
        currentDrawCall_[f] >= kMaxDrawCallsPerFrame) {
        // スロット満杯時は安全のためオフセットをリセット
        currentVertexOffset_[f] = 0;
        currentIndexOffset_[f] = 0;
        currentDrawCall_[f] = 0;
    }

    size_t vOffset = currentVertexOffset_[f];
    size_t iOffset = currentIndexOffset_[f];
    size_t drawIdx = currentDrawCall_[f];

    // 4. 頂点・インデックスを現在フレームの領域にコピー
    memcpy(mappedVertices_[f] + vOffset, vertices.data(), sizeof(TextVertex) * vertices.size());
    memcpy(mappedIndices_[f] + iOffset, indices.data(), sizeof(uint32_t) * indices.size());

    // 5. 定数バッファの書き込み (256バイトオフセット単位)
    size_t cbOffset = drawIdx * kConstantBufferAlignment;
    auto* cbPtr = reinterpret_cast<TextParamsConstantBuffer*>(mappedConstantBuffers_[f] + cbOffset);
    cbPtr->wvp = Calculate2DProjectionMatrix();
    cbPtr->outlineColor = outlineColor;
    cbPtr->outlineWidth = enableOutline ? outlineWidth : 0.0f;
    cbPtr->boldness = baseBoldness_ + boldness;
    cbPtr->texSize = atlas_->GetAtlasSize();
    cbPtr->pxRange = atlas_->GetPxRange();

    // 6. パイプライン状態とルートシグネチャの設定
    ShaderName shader = MSDFShader;
    BlendMode blend = BlendMode::kBlendModeNormal;
    CullMode cull = CullMode::kCullModeNone;

    auto* pso = pipelineState_->GetGraphicsPipelineState(shader, blend, cull);
    auto* rootSig = pipelineState_->GetRootSignature(shader, blend);
    if (!pso || !rootSig) {
        LOG_ERROR("TextRenderer: PSO or RootSignature not found for MSDFShader!");
        return;
    }

    commandList->SetPipelineState(pso);
    commandList->SetGraphicsRootSignature(rootSig->GetRootSignature());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferViews_[f]);
    commandList->IASetIndexBuffer(&indexBufferViews_[f]);

    // ルートパラメータの設定
    UINT cbParamIndex = pipelineState_->GetRootParameterIndex(shader, blend, "gTextParams");
    if (cbParamIndex != static_cast<UINT>(-1)) {
        commandList->SetGraphicsRootConstantBufferView(cbParamIndex, constantBuffers_[f]->GetGPUVirtualAddress() + cbOffset);
    }

    UINT texParamIndex = pipelineState_->GetRootParameterIndex(shader, blend, "gMSDFTexture");
    if (texParamIndex != static_cast<UINT>(-1)) {
        commandList->SetGraphicsRootDescriptorTable(texParamIndex, atlas_->GetSrvHandleGPU());
    }

    // ドローコール (BaseVertexLocation に vOffset、StartIndexLocation に iOffset を指定)
    commandList->DrawIndexedInstanced(
        static_cast<UINT>(indices.size()),
        1,
        static_cast<UINT>(iOffset),
        static_cast<INT>(vOffset),
        0);

    // 次のドローコール用にオフセットを進める
    currentVertexOffset_[f] += vertices.size();
    currentIndexOffset_[f] += indices.size();
    currentDrawCall_[f]++;
}

void TextRenderer::DrawString(ID3D12GraphicsCommandList* commandList,
                              const std::wstring& text,
                              const Vector2& pos,
                              float fontSize,
                              const Vector4& color,
                              bool enableOutline,
                              const Vector4& outlineColor,
                              float outlineWidth,
                              float boldness) {
    if (text.empty()) return;

    // ワイド文字列を UTF-8 に変換して描画
    std::string utf8;
    for (wchar_t wc : text) {
        uint32_t cp = static_cast<uint32_t>(wc);
        if (cp < 0x80) {
            utf8.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            utf8.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            utf8.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            utf8.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            utf8.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            utf8.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    DrawString(commandList, utf8, pos, fontSize, color, enableOutline, outlineColor, outlineWidth, boldness);
}

void TextRenderer::DrawFillRect(ID3D12GraphicsCommandList* commandList,
                                const Vector2& pos,
                                const Vector2& size,
                                const Vector4& color) {
    if (!atlas_ || !commandList || size.x <= 0.0f || size.y <= 0.0f) return;

    atlas_->UpdateGpu(commandList);

    size_t f = currentFrameIndex_;
    if (currentVertexOffset_[f] + 4 > kMaxVerticesPerFrame ||
        currentIndexOffset_[f] + 6 > kMaxIndicesPerFrame ||
        currentDrawCall_[f] >= kMaxDrawCallsPerFrame) {
        currentVertexOffset_[f] = 0;
        currentIndexOffset_[f] = 0;
        currentDrawCall_[f] = 0;
    }

    Vector2 uv = atlas_->GetWhitePixelUV();
    float x0 = pos.x;
    float y0 = pos.y;
    float x1 = pos.x + size.x;
    float y1 = pos.y + size.y;

    TextVertex v0{ { x0, y0, 0.0f, 1.0f }, uv, color };
    TextVertex v1{ { x1, y0, 0.0f, 1.0f }, uv, color };
    TextVertex v2{ { x0, y1, 0.0f, 1.0f }, uv, color };
    TextVertex v3{ { x1, y1, 0.0f, 1.0f }, uv, color };

    size_t vOffset = currentVertexOffset_[f];
    size_t iOffset = currentIndexOffset_[f];
    size_t drawIdx = currentDrawCall_[f];

    TextVertex* vDest = mappedVertices_[f] + vOffset;
    vDest[0] = v0; vDest[1] = v1; vDest[2] = v2; vDest[3] = v3;

    uint32_t* iDest = mappedIndices_[f] + iOffset;
    iDest[0] = 0; iDest[1] = 1; iDest[2] = 2;
    iDest[3] = 1; iDest[4] = 3; iDest[5] = 2;

    size_t cbOffset = drawIdx * kConstantBufferAlignment;
    auto* cbPtr = reinterpret_cast<TextParamsConstantBuffer*>(mappedConstantBuffers_[f] + cbOffset);
    cbPtr->wvp = Calculate2DProjectionMatrix();
    cbPtr->outlineColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    cbPtr->outlineWidth = 0.0f;
    cbPtr->boldness = 0.0f;
    cbPtr->texSize = atlas_->GetAtlasSize();
    cbPtr->pxRange = atlas_->GetPxRange();

    ShaderName shader = MSDFShader;
    BlendMode blend = BlendMode::kBlendModeNormal;
    CullMode cull = CullMode::kCullModeNone;

    auto* pso = pipelineState_->GetGraphicsPipelineState(shader, blend, cull);
    auto* rootSig = pipelineState_->GetRootSignature(shader, blend);
    if (!pso || !rootSig) return;

    commandList->SetPipelineState(pso);
    commandList->SetGraphicsRootSignature(rootSig->GetRootSignature());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferViews_[f]);
    commandList->IASetIndexBuffer(&indexBufferViews_[f]);

    UINT cbParamIndex = pipelineState_->GetRootParameterIndex(shader, blend, "gTextParams");
    if (cbParamIndex != static_cast<UINT>(-1)) {
        commandList->SetGraphicsRootConstantBufferView(cbParamIndex, constantBuffers_[f]->GetGPUVirtualAddress() + cbOffset);
    }
    UINT texParamIndex = pipelineState_->GetRootParameterIndex(shader, blend, "gMSDFTexture");
    if (texParamIndex != static_cast<UINT>(-1)) {
        commandList->SetGraphicsRootDescriptorTable(texParamIndex, atlas_->GetSrvHandleGPU());
    }

    commandList->DrawIndexedInstanced(6, 1, static_cast<UINT>(iOffset), static_cast<INT>(vOffset), 0);

    currentVertexOffset_[f] += 4;
    currentIndexOffset_[f] += 6;
    currentDrawCall_[f]++;
}

} // namespace MatchaEngine


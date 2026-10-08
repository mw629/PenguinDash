#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "DynamicFontAtlas.h"
#include <d3dx12.h>
#include <cassert>
#include <iostream>
#include <algorithm>
#include "Core/LogHandler.h"
#include "Resource/Load.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include "../../../externals/imgui/imstb_rectpack.h"

namespace MatchaEngine {

namespace {

// UTF-8 文字列から次の Unicode コードポイントを抽出するヘルパー関数
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

// 256バイト境界にアライメント
inline UINT AlignTo256(UINT size) {
    return (size + 255) & ~255;
}

} // namespace

struct DynamicFontAtlas::PackerContext {
    stbrp_context context{};
    std::vector<stbrp_node> nodes;

    void Init(int width, int height) {
        nodes.resize(width);
        stbrp_init_target(&context, width, height, nodes.data(), static_cast<int>(nodes.size()));
    }
};

DynamicFontAtlas::DynamicFontAtlas() : packer_(std::make_unique<PackerContext>()) {}

DynamicFontAtlas::~DynamicFontAtlas() {
    if (uploadMappedPtr_ && uploadResource_) {
        uploadResource_->Unmap(0, nullptr);
        uploadMappedPtr_ = nullptr;
    }
}

bool DynamicFontAtlas::Initialize(ID3D12Device* device,
                                  DescriptorHeap* descriptorHeap,
                                  const std::string& fontPath,
                                  int atlasWidth,
                                  int atlasHeight,
                                  float glyphBaseSize,
                                  float pxRange) {
    device_ = device;
    descriptorHeap_ = descriptorHeap;
    atlasWidth_ = atlasWidth;
    atlasHeight_ = atlasHeight;
    glyphBaseSize_ = glyphBaseSize;
    pxRange_ = pxRange;

    // 1. フォントファイルの読み込み
    if (!msdfGen_.LoadFontFile(fontPath)) {
        LOG_ERROR(std::format("DynamicFontAtlas: Failed to load font file '{}'", fontPath));
        return false;
    }

    // 2. パッカーの初期化
    packer_->Init(atlasWidth_, atlasHeight_);

    // 2.1 単色矩形描画用の白色8x8ブロックをアトラスに予約
    stbrp_rect whiteRect{};
    whiteRect.id = -1;
    whiteRect.w = 8;
    whiteRect.h = 8;
    stbrp_pack_rects(&packer_->context, &whiteRect, 1);

    whitePixelUV_ = Vector2(
        (static_cast<float>(whiteRect.x) + 4.0f) / static_cast<float>(atlasWidth_),
        (static_cast<float>(whiteRect.y) + 4.0f) / static_cast<float>(atlasHeight_)
    );

    PendingRegion whiteRegion{};
    whiteRegion.dstX = whiteRect.x;
    whiteRegion.dstY = whiteRect.y;
    whiteRegion.width = 8;
    whiteRegion.height = 8;
    whiteRegion.pixels.assign(8 * 8 * 4, 255); // 全ピクセル RGBA(255, 255, 255, 255)
    pendingUploads_.push_back(std::move(whiteRegion));

    // 3. DirectX 12 テクスチャリソースの作成
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = static_cast<UINT64>(atlasWidth_);
    texDesc.Height = static_cast<UINT>(atlasHeight_);
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    HRESULT hr = device_->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(&textureResource_));
    if (FAILED(hr)) {
        LOG_ERROR("DynamicFontAtlas: Failed to create texture resource!");
        return false;
    }

    // 4. アップロード用バッファの作成 (一括で十分な容量を確保: 8MB程度)
    uploadBufferSize_ = 8 * 1024 * 1024;
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize_);
    hr = device_->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadResource_));
    if (FAILED(hr)) {
        LOG_ERROR("DynamicFontAtlas: Failed to create upload buffer!");
        return false;
    }

    hr = uploadResource_->Map(0, nullptr, reinterpret_cast<void**>(&uploadMappedPtr_));
    if (FAILED(hr)) {
        LOG_ERROR("DynamicFontAtlas: Failed to map upload buffer!");
        return false;
    }

    // 5. SRV (Shader Resource View) の作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    srvHandleCPU_ = GetCPUDescriptorHandle(descriptorHeap_->GetSrvDescriptorHeap(), descriptorHeap_->GetDescriptorSizeSRV());
    srvHandleGPU_ = GetGPUDescriptorHandle(descriptorHeap_->GetSrvDescriptorHeap(), descriptorHeap_->GetDescriptorSizeSRV());

    device_->CreateShaderResourceView(textureResource_.Get(), &srvDesc, srvHandleCPU_);

    // 6. 基本ASCII文字（半角英数記号）をあらかじめ登録しておく
    for (uint32_t c = 32; c <= 126; ++c) {
        GetOrAddGlyph(c);
    }

    LOG_INFO(std::format("DynamicFontAtlas initialized: {}x{}, font='{}'", atlasWidth_, atlasHeight_, fontPath));
    return true;
}

const FontGlyph* DynamicFontAtlas::GetOrAddGlyph(uint32_t codepoint) {
    auto it = glyphCache_.find(codepoint);
    if (it != glyphCache_.end()) {
        return &it->second;
    }

    // 未登録グリフのMSDF生成
    MsdfGlyphBitmap glyphBmp = msdfGen_.GenerateGlyph(codepoint, glyphBaseSize_, pxRange_);

    // アトラスへのパッキング
    stbrp_rect rect{};
    rect.id = static_cast<int>(codepoint);
    rect.w = static_cast<stbrp_coord>(glyphBmp.width + 1); // 境界のにじみ防止のため+1ピクセルマージン
    rect.h = static_cast<stbrp_coord>(glyphBmp.height + 1);

    if (!stbrp_pack_rects(&packer_->context, &rect, 1) || !rect.was_packed) {
        LOG_WARN(std::format("DynamicFontAtlas: Atlas full! Cannot pack codepoint U+{:04X}", codepoint));
        // スペース等の既存グリフにフォールバック
        auto spaceIt = glyphCache_.find(' ');
        return spaceIt != glyphCache_.end() ? &spaceIt->second : nullptr;
    }

    FontGlyph glyph{};
    glyph.codepoint = codepoint;
    glyph.width = static_cast<float>(glyphBmp.width);
    glyph.height = static_cast<float>(glyphBmp.height);
    glyph.offsetX = glyphBmp.offsetX;
    glyph.offsetY = glyphBmp.offsetY;
    glyph.advanceWidth = glyphBmp.advanceWidth;

    // UV座標の計算
    glyph.u0 = static_cast<float>(rect.x) / static_cast<float>(atlasWidth_);
    glyph.v0 = static_cast<float>(rect.y) / static_cast<float>(atlasHeight_);
    glyph.u1 = static_cast<float>(rect.x + glyphBmp.width) / static_cast<float>(atlasWidth_);
    glyph.v1 = static_cast<float>(rect.y + glyphBmp.height) / static_cast<float>(atlasHeight_);

    glyphCache_[codepoint] = glyph;

    // GPUへのアップロードキューに積む
    PendingRegion region{};
    region.dstX = rect.x;
    region.dstY = rect.y;
    region.width = glyphBmp.width;
    region.height = glyphBmp.height;
    region.pixels = std::move(glyphBmp.pixels);
    pendingUploads_.push_back(std::move(region));

    return &glyphCache_[codepoint];
}

void DynamicFontAtlas::PreloadString(const std::string& utf8Text) {
    const char* ptr = utf8Text.data();
    const char* end = ptr + utf8Text.size();
    while (ptr < end) {
        uint32_t cp = DecodeUtf8(ptr, end);
        if (cp != 0) {
            GetOrAddGlyph(cp);
        }
    }
}

void DynamicFontAtlas::PreloadString(const std::wstring& wideText) {
    for (wchar_t wc : wideText) {
        GetOrAddGlyph(static_cast<uint32_t>(wc));
    }
}

void DynamicFontAtlas::BeginFrame(size_t frameIndex) {
    if (uploadBufferSize_ > 0) {
        UINT64 half = uploadBufferSize_ / 2;
        uploadBufferOffset_ = (frameIndex % 2) * half;
        uploadBufferFrameEnd_ = uploadBufferOffset_ + half;
    }
}

void DynamicFontAtlas::UpdateGpu(ID3D12GraphicsCommandList* commandList) {
    if (pendingUploads_.empty() || !uploadMappedPtr_) return;

    // テクスチャを COPY_DEST へ遷移
    CD3DX12_RESOURCE_BARRIER toCopy = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->ResourceBarrier(1, &toCopy);

    if (uploadBufferFrameEnd_ == 0) {
        uploadBufferFrameEnd_ = uploadBufferSize_;
    }

    for (const auto& region : pendingUploads_) {
        UINT rowPitch = AlignTo256(region.width * 4);
        UINT slicePitch = rowPitch * region.height;

        if (uploadBufferOffset_ + slicePitch > uploadBufferFrameEnd_) {
            LOG_WARN("DynamicFontAtlas: Upload buffer full for current frame!");
            break;
        }

        // アップロードバッファへコピー (各行ごとに rowPitch に合わせて詰める)
        uint8_t* dstRow = uploadMappedPtr_ + uploadBufferOffset_;
        const uint8_t* srcRow = region.pixels.data();
        for (int y = 0; y < region.height; ++y) {
            memcpy(dstRow, srcRow, region.width * 4);
            dstRow += rowPitch;
            srcRow += region.width * 4;
        }

        // CopyTextureRegion の設定
        D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
        dstLoc.pResource = textureResource_.Get();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
        srcLoc.pResource = uploadResource_.Get();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint.Offset = uploadBufferOffset_;
        srcLoc.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srcLoc.PlacedFootprint.Footprint.Width = region.width;
        srcLoc.PlacedFootprint.Footprint.Height = region.height;
        srcLoc.PlacedFootprint.Footprint.Depth = 1;
        srcLoc.PlacedFootprint.Footprint.RowPitch = rowPitch;

        commandList->CopyTextureRegion(&dstLoc, region.dstX, region.dstY, 0, &srcLoc, nullptr);

        // D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT (512 bytes) にアライメントして次へ
        uploadBufferOffset_ = (uploadBufferOffset_ + slicePitch + 511) & ~511;
    }

    pendingUploads_.clear();

    // テクスチャを PIXEL_SHADER_RESOURCE へ戻す
    CD3DX12_RESOURCE_BARRIER toShader = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource_.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &toShader);
}

} // namespace MatchaEngine

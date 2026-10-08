#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include "../../Math/Calculation.h"

namespace MatchaEngine {

struct MsdfGlyphBitmap {
    int width = 0;
    int height = 0;
    float offsetX = 0.0f;      // グリフ原点(ベースライン)からのオフセットX (ピクセル単位)
    float offsetY = 0.0f;      // グリフ原点(ベースライン)からのオフセットY (ピクセル単位)
    float advanceWidth = 0.0f; // 次の文字までの進み幅 (ピクセル単位)
    std::vector<uint8_t> pixels; // RGBA8形式 (幅 * 高さ * 4)
};

class MsdfGenerator {
public:
    MsdfGenerator();
    ~MsdfGenerator();

    // TTF/OTFフォントバイナリを初期化
    bool Initialize(const uint8_t* fontData, size_t dataSize);

    // フォントファイルから初期化
    bool LoadFontFile(const std::string& filePath);

    // 指定したUnicodeコードポイントのMSDFグリフを生成
    // glyphSize: グリフの高さピクセルサイズ (例: 32.0f, 48.0f)
    // pxRange: MSDFの符号付き距離の広がりピクセル数 (通常 4.0f)
    MsdfGlyphBitmap GenerateGlyph(uint32_t codepoint, float glyphSize = 36.0f, float pxRange = 4.0f);

    // フォントの行メトリクスを取得 (ベースライン、アセント、ディセント、行間)
    void GetVerticalMetrics(float glyphSize, float& ascent, float& descent, float& lineGap);

    bool IsLoaded() const { return isLoaded_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool isLoaded_ = false;
    std::vector<uint8_t> fontBuffer_;
};

} // namespace MatchaEngine

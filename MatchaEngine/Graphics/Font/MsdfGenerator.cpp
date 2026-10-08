#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "MsdfGenerator.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <limits>
#include <cassert>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// stb_truetype をこのコンパイル単位専用の実装としてインクルード
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../../externals/imgui/imstb_truetype.h"

namespace MatchaEngine {

namespace {

struct Point2D {
    float x = 0.0f;
    float y = 0.0f;

    Point2D() = default;
    Point2D(float inX, float inY) : x(inX), y(inY) {}

    Point2D operator+(const Point2D& o) const { return Point2D(x + o.x, y + o.y); }
    Point2D operator-(const Point2D& o) const { return Point2D(x - o.x, y - o.y); }
    Point2D operator*(float s) const { return Point2D(x * s, y * s); }
    Point2D operator/(float s) const { return Point2D(x / s, y / s); }

    float Dot(const Point2D& o) const { return x * o.x + y * o.y; }
    float Cross(const Point2D& o) const { return x * o.y - y * o.x; }
    float LengthSq() const { return x * x + y * y; }
    float Length() const { return std::sqrt(LengthSq()); }

    Point2D Normalized() const {
        float len = Length();
        if (len > 0.000001f) return Point2D(x / len, y / len);
        return Point2D(0.0f, 0.0f);
    }
};

enum class EdgeColor : uint8_t {
    Black   = 0,
    Red     = 1,
    Green   = 2,
    Yellow  = 3, // Red + Green
    Blue    = 4,
    Magenta = 5, // Red + Blue
    Cyan    = 6, // Green + Blue
    White   = 7  // Red + Green + Blue
};

inline bool ColorHasChannel(EdgeColor color, int channel) {
    // channel: 0=Red, 1=Green, 2=Blue
    return ((static_cast<uint8_t>(color) >> channel) & 1) != 0;
}

struct Segment {
    enum class Type { Line, QuadraticBezier } type = Type::Line;
    EdgeColor color = EdgeColor::White;
    Point2D p0;
    Point2D p1; // 制御点 (Bezier) または終点 (Line)
    Point2D p2; // 終点 (Bezier)

    Point2D PointAt(float t) const {
        if (type == Type::Line) {
            return p0 + (p1 - p0) * t;
        } else {
            float it = 1.0f - t;
            return p0 * (it * it) + p1 * (2.0f * it * t) + p2 * (t * t);
        }
    }

    Point2D DirectionAt(float t) const {
        if (type == Type::Line) {
            return p1 - p0;
        } else {
            return (p1 - p0) * (2.0f * (1.0f - t)) + (p2 - p1) * (2.0f * t);
        }
    }

    float DistanceSq(const Point2D& p, float& outT) const {
        if (type == Type::Line) {
            Point2D v = p1 - p0;
            float lenSq = v.LengthSq();
            if (lenSq < 0.000001f) {
                outT = 0.0f;
                return (p - p0).LengthSq();
            }
            float t = std::clamp((p - p0).Dot(v) / lenSq, 0.0f, 1.0f);
            outT = t;
            Point2D nearest = p0 + v * t;
            return (p - nearest).LengthSq();
        } else {
            // 2次ベジェ曲線に対する最短点探索 (多分割 + 局所ニュートン法/補間)
            const int kSamples = 8;
            float bestT = 0.0f;
            float bestDistSq = (std::numeric_limits<float>::max)();
            for (int i = 0; i <= kSamples; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(kSamples);
                Point2D pt = PointAt(t);
                float distSq = (p - pt).LengthSq();
                if (distSq < bestDistSq) {
                    bestDistSq = distSq;
                    bestT = t;
                }
            }
            // 局所探索 (二分法/黄金分割)
            float step = 1.0f / (kSamples * 2.0f);
            for (int iter = 0; iter < 4; ++iter) {
                float tMinus = std::clamp(bestT - step, 0.0f, 1.0f);
                float tPlus  = std::clamp(bestT + step, 0.0f, 1.0f);
                float dMinus = (p - PointAt(tMinus)).LengthSq();
                float dPlus  = (p - PointAt(tPlus)).LengthSq();
                if (dMinus < bestDistSq) {
                    bestDistSq = dMinus;
                    bestT = tMinus;
                } else if (dPlus < bestDistSq) {
                    bestDistSq = dPlus;
                    bestT = tPlus;
                }
                step *= 0.5f;
            }
            outT = bestT;
            return bestDistSq;
        }
    }
};

struct Contour {
    std::vector<Segment> segments;
};

// 輪郭のエッジにRGBを割り振る (Edge Coloring)
void ColorContourEdges(Contour& contour) {
    if (contour.segments.empty()) return;

    // 1つのセグメントしかない、または全て直線で角がない場合
    if (contour.segments.size() == 1) {
        contour.segments[0].color = EdgeColor::White;
        return;
    }

    // 各頂点（セグメント間の接続部）で「角（コーナー）」を検出
    // 連続する接線のなす角が大きく変化しているか
    std::vector<int> cornerIndices;
    size_t n = contour.segments.size();
    for (size_t i = 0; i < n; ++i) {
        size_t prevIdx = (i + n - 1) % n;
        Point2D dirPrev = contour.segments[prevIdx].DirectionAt(1.0f).Normalized();
        Point2D dirCurr = contour.segments[i].DirectionAt(0.0f).Normalized();
        float dot = dirPrev.Dot(dirCurr);
        // 内角が急激に変化している場合 (cos < cos(3.0 rad) ~= -0.99 または 鋭い角)
        // ここでは外角が 35度以上(dot < 0.82) ならコーナーと判定
        if (dot < 0.82f) {
            cornerIndices.push_back(static_cast<int>(i));
        }
    }

    if (cornerIndices.empty()) {
        // コーナーがなければ全て White
        for (auto& seg : contour.segments) {
            seg.color = EdgeColor::White;
        }
        return;
    }

    // コーナーで区切られた各エッジシーケンスに色を順番に割り当てる
    // 3色の循環: Yellow (R+G) -> Cyan (G+B) -> Magenta (R+B)
    const EdgeColor kColors[3] = { EdgeColor::Yellow, EdgeColor::Cyan, EdgeColor::Magenta };
    int colorIdx = 0;

    size_t cornerCount = cornerIndices.size();
    for (size_t c = 0; c < cornerCount; ++c) {
        int start = cornerIndices[c];
        int end = cornerIndices[(c + 1) % cornerCount];
        EdgeColor currentColor = kColors[colorIdx % 3];
        colorIdx++;

        // 最後の区間で最初のエッジと同じ色になるのを防ぐ
        if (c + 1 == cornerCount && cornerCount > 1 && currentColor == contour.segments[cornerIndices[0]].color) {
            currentColor = kColors[colorIdx % 3];
        }

        int curr = start;
        while (curr != end) {
            contour.segments[curr].color = currentColor;
            curr = (curr + 1) % static_cast<int>(n);
        }
    }
}

// 点Pが輪郭の内側にあるかどうかの内外判定 (Winding number法)
int ComputeContourWinding(const Contour& contour, const Point2D& p) {
    int winding = 0;
    for (const auto& seg : contour.segments) {
        // 線分近似で交差を判定
        const int kSteps = (seg.type == Segment::Type::QuadraticBezier) ? 4 : 1;
        Point2D prevPt = seg.PointAt(0.0f);
        for (int i = 1; i <= kSteps; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(kSteps);
            Point2D currPt = seg.PointAt(t);

            if (prevPt.y <= p.y) {
                if (currPt.y > p.y && (currPt - prevPt).Cross(p - prevPt) > 0.0f) {
                    winding++;
                }
            } else {
                if (currPt.y <= p.y && (currPt - prevPt).Cross(p - prevPt) < 0.0f) {
                    winding--;
                }
            }
            prevPt = currPt;
        }
    }
    return winding;
}

} // namespace

struct MsdfGenerator::Impl {
    stbtt_fontinfo fontInfo{};
};

MsdfGenerator::MsdfGenerator() : impl_(std::make_unique<Impl>()) {}
MsdfGenerator::~MsdfGenerator() = default;

bool MsdfGenerator::Initialize(const uint8_t* fontData, size_t dataSize) {
    if (!fontData || dataSize == 0) return false;

    fontBuffer_.assign(fontData, fontData + dataSize);
    if (!stbtt_InitFont(&impl_->fontInfo, fontBuffer_.data(), stbtt_GetFontOffsetForIndex(fontBuffer_.data(), 0))) {
        isLoaded_ = false;
        return false;
    }

    isLoaded_ = true;
    return true;
}

bool MsdfGenerator::LoadFontFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return false;
    }

    return Initialize(buffer.data(), buffer.size());
}

void MsdfGenerator::GetVerticalMetrics(float glyphSize, float& ascent, float& descent, float& lineGap) {
    if (!isLoaded_) {
        ascent = glyphSize;
        descent = 0.0f;
        lineGap = 0.0f;
        return;
    }

    int fontAscent, fontDescent, fontLineGap;
    stbtt_GetFontVMetrics(&impl_->fontInfo, &fontAscent, &fontDescent, &fontLineGap);
    float scale = stbtt_ScaleForPixelHeight(&impl_->fontInfo, glyphSize);
    ascent = fontAscent * scale;
    descent = fontDescent * scale;
    lineGap = fontLineGap * scale;
}

MsdfGlyphBitmap MsdfGenerator::GenerateGlyph(uint32_t codepoint, float glyphSize, float pxRange) {
    MsdfGlyphBitmap result;
    if (!isLoaded_) return result;

    int glyphIndex = stbtt_FindGlyphIndex(&impl_->fontInfo, static_cast<int>(codepoint));
    if (glyphIndex == 0 && codepoint != ' ') {
        // グリフが見つからない場合はデフォルトの置換文字または0
    }

    int advanceWidthInt, leftSideBearingInt;
    stbtt_GetGlyphHMetrics(&impl_->fontInfo, glyphIndex, &advanceWidthInt, &leftSideBearingInt);
    float scale = stbtt_ScaleForPixelHeight(&impl_->fontInfo, glyphSize);
    result.advanceWidth = advanceWidthInt * scale;

    stbtt_vertex* stbVertices = nullptr;
    int numVertices = stbtt_GetGlyphShape(&impl_->fontInfo, glyphIndex, &stbVertices);

    if (numVertices <= 0 || !stbVertices) {
        // 空白文字など輪郭がない文字
        result.width = 1;
        result.height = 1;
        result.offsetX = 0.0f;
        result.offsetY = 0.0f;
        result.pixels.resize(4, 0); // 完全に透明
        if (stbVertices) stbtt_FreeShape(&impl_->fontInfo, stbVertices);
        return result;
    }

    // 1. 輪郭の抽出
    std::vector<Contour> contours;
    Contour currentContour;
    Point2D contourStart(0.0f, 0.0f);
    Point2D currentPos(0.0f, 0.0f);

    for (int i = 0; i < numVertices; ++i) {
        const stbtt_vertex& v = stbVertices[i];
        Point2D target(v.x * scale, v.y * scale);

        if (v.type == STBTT_vmove) {
            if (!currentContour.segments.empty()) {
                // 閉じていなければ閉じる
                if ((currentPos - contourStart).LengthSq() > 0.0001f) {
                    Segment closeSeg;
                    closeSeg.type = Segment::Type::Line;
                    closeSeg.p0 = currentPos;
                    closeSeg.p1 = contourStart;
                    currentContour.segments.push_back(closeSeg);
                }
                contours.push_back(currentContour);
                currentContour.segments.clear();
            }
            contourStart = target;
            currentPos = target;
        } else if (v.type == STBTT_vline) {
            Segment seg;
            seg.type = Segment::Type::Line;
            seg.p0 = currentPos;
            seg.p1 = target;
            currentContour.segments.push_back(seg);
            currentPos = target;
        } else if (v.type == STBTT_vcurve) {
            Point2D ctrl(v.cx * scale, v.cy * scale);
            Segment seg;
            seg.type = Segment::Type::QuadraticBezier;
            seg.p0 = currentPos;
            seg.p1 = ctrl;
            seg.p2 = target;
            currentContour.segments.push_back(seg);
            currentPos = target;
        }
    }

    if (!currentContour.segments.empty()) {
        if ((currentPos - contourStart).LengthSq() > 0.0001f) {
            Segment closeSeg;
            closeSeg.type = Segment::Type::Line;
            closeSeg.p0 = currentPos;
            closeSeg.p1 = contourStart;
            currentContour.segments.push_back(closeSeg);
        }
        contours.push_back(currentContour);
    }

    stbtt_FreeShape(&impl_->fontInfo, stbVertices);

    if (contours.empty()) {
        result.width = 1;
        result.height = 1;
        result.pixels.resize(4, 0);
        return result;
    }

    // 2. バウンディングボックスの計算
    int x0, y0, x1, y1;
    stbtt_GetGlyphBox(&impl_->fontInfo, glyphIndex, &x0, &y0, &x1, &y1);

    float minX = x0 * scale;
    float minY = y0 * scale;
    float maxX = x1 * scale;
    float maxY = y1 * scale;

    const float padding = pxRange + 1.0f; // 距離場をカバーするためのパディング
    int width = static_cast<int>(std::ceil(maxX - minX + padding * 2.0f));
    int height = static_cast<int>(std::ceil(maxY - minY + padding * 2.0f));
    if (width < 1) width = 1;
    if (height < 1) height = 1;

    result.width = width;
    result.height = height;
    // Y軸は上が正 (TrueType座標系) から画面座標系(下が正)への変換を考慮
    result.offsetX = minX - padding;
    result.offsetY = maxY + padding; // ベースラインからの上方向高さ

    // 3. エッジカラーリング
    for (auto& contour : contours) {
        ColorContourEdges(contour);
    }

    // 4. 各ピクセルについてMSDF符号付き距離を計算
    result.pixels.resize(width * height * 4);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // ピクセル中心のワールド(フォント)座標
            // フォント座標系ではY軸上が正なので、y=0 がmaxY(上端)側になるようにマッピング
            float fontX = (minX - padding) + x + 0.5f;
            float fontY = (maxY + padding) - y - 0.5f;
            Point2D pt(fontX, fontY);

            // 内外判定 (全輪郭のWinding numberの合計)
            int totalWinding = 0;
            for (const auto& contour : contours) {
                totalWinding += ComputeContourWinding(contour, pt);
            }
            float sign = (totalWinding != 0) ? 1.0f : -1.0f;

            // R, G, B 各チャンネルごとの最短距離を計算
            float minDistR = (std::numeric_limits<float>::max)();
            float minDistG = (std::numeric_limits<float>::max)();
            float minDistB = (std::numeric_limits<float>::max)();
            float minDistAll = (std::numeric_limits<float>::max)();

            for (const auto& contour : contours) {
                for (const auto& seg : contour.segments) {
                    float t = 0.0f;
                    float dSq = seg.DistanceSq(pt, t);
                    if (dSq < minDistAll) minDistAll = dSq;

                    if (ColorHasChannel(seg.color, 0)) { // Red
                        if (dSq < minDistR) minDistR = dSq;
                    }
                    if (ColorHasChannel(seg.color, 1)) { // Green
                        if (dSq < minDistG) minDistG = dSq;
                    }
                    if (ColorHasChannel(seg.color, 2)) { // Blue
                        if (dSq < minDistB) minDistB = dSq;
                    }
                }
            }

            // セグメントに特定チャンネルの色が含まれない場合、全セグメント最短距離にフォールバック
            if (minDistR == (std::numeric_limits<float>::max)()) minDistR = minDistAll;
            if (minDistG == (std::numeric_limits<float>::max)()) minDistG = minDistAll;
            if (minDistB == (std::numeric_limits<float>::max)()) minDistB = minDistAll;

            float distR = std::sqrt(minDistR);
            float distG = std::sqrt(minDistG);
            float distB = std::sqrt(minDistB);

            // 符号付き距離 (ピクセル単位)
            float sdR = sign * distR;
            float sdG = sign * distG;
            float sdB = sign * distB;

            // [0, 1] に正規化 (0.5が輪郭境界)
            float normR = std::clamp(0.5f + sdR / pxRange, 0.0f, 1.0f);
            float normG = std::clamp(0.5f + sdG / pxRange, 0.0f, 1.0f);
            float normB = std::clamp(0.5f + sdB / pxRange, 0.0f, 1.0f);

            // 通常のSDF (3チャンネルの中央値)
            float medianVal = (std::max)((std::min)(normR, normG), (std::min)((std::max)(normR, normG), normB));

            int pixelIdx = (y * width + x) * 4;
            result.pixels[pixelIdx + 0] = static_cast<uint8_t>(normR * 255.0f + 0.5f);
            result.pixels[pixelIdx + 1] = static_cast<uint8_t>(normG * 255.0f + 0.5f);
            result.pixels[pixelIdx + 2] = static_cast<uint8_t>(normB * 255.0f + 0.5f);
            result.pixels[pixelIdx + 3] = static_cast<uint8_t>(medianVal * 255.0f + 0.5f);
        }
    }

    return result;
}

} // namespace MatchaEngine

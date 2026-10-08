#pragma once

struct VertexShaderInput
{
    float32_t4 position : POSITION0; // スクリーン座標 (x, y, 0, 1)
    float32_t2 texcoord : TEXCOORD0; // アトラスUV
    float32_t4 color    : COLOR0;    // 頂点カラー (文字色)
};

struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    float32_t4 color    : COLOR0;
};

struct TextParams
{
    float32_t4x4 wvp;          // 正射影行列 (Orthographic Projection)
    float32_t4   outlineColor; // アウトラインカラー
    float32_t    outlineWidth; // アウトライン幅 (0.0ならアウトライン無効)
    float32_t    boldness;     // 太さオフセット (0.0で標準、正で太く、負で細く)
    float32_t2   texSize;      // アトラス解像度 (例: 2048, 2048)
    float32_t    pxRange;      // MSDFのピクセル範囲 (通常 4.0f)
    float32_t3   pad;          // 16バイトアライメント用パディング
};

#include "MSDF.hlsli"

Texture2D<float32_t4> gMSDFTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<TextParams> gTextParams : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

float32_t median(float32_t r, float32_t g, float32_t b)
{
    return max(min(r, g), min(max(r, g), b));
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // MSDFテクスチャからRGB(符号付き距離)をサンプリング
    float32_t4 msd = gMSDFTexture.Sample(gSampler, input.texcoord);
    float32_t sd = median(msd.r, msd.g, msd.b);

    // スクリーンピクセル単位の距離変化率を計算（fwidthを用いた解像度適応型アンチエイリアス）
    float32_t2 unitRange = float32_t2(gTextParams.pxRange, gTextParams.pxRange) / gTextParams.texSize;
    float32_t2 screenTexSize = float32_t2(1.0f, 1.0f) / max(fwidth(input.texcoord), float32_t2(0.000001f, 0.000001f));
    float32_t screenPxRange = max(0.5f * dot(unitRange, screenTexSize), 1.0f);

    // 0.5が文字輪郭の境界 (boldnessを加算して太さを調整)
    float32_t screenDist = screenPxRange * (sd - 0.5f + gTextParams.boldness);
    float32_t opacity = saturate(screenDist + 0.5f);

    // アウトラインの計算
    if (gTextParams.outlineWidth > 0.001f)
    {
        float32_t outlineDist = screenPxRange * (sd - 0.5f + gTextParams.boldness + gTextParams.outlineWidth);
        float32_t outlineOpacity = saturate(outlineDist + 0.5f);

        float32_t4 finalColor = lerp(gTextParams.outlineColor, input.color, opacity);
        finalColor.a = max(gTextParams.outlineColor.a * outlineOpacity, input.color.a * opacity);
        output.color = finalColor;
    }
    else
    {
        output.color = input.color;
        output.color.a *= opacity;
    }

    // 完全に透明なピクセルは破棄してフィルレートを最適化
    if (output.color.a <= 0.001f)
    {
        discard;
    }

    return output;
}

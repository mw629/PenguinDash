#include "PostEffect.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gMaskTexture : register(t1);
sampler gSampler : register(s0);

ConstantBuffer<PostEffectData> gPostEffect : register(b0);

cbuffer PixelationParams : register(b1)
{
    float32_t blockCountX;    // 横方向のブロック数 (例: 32)
    float32_t blockCountY;    // 縦方向のブロック数 (例: 24)
    float32_t usePixelation;  // 1.0で有効、0.0で無効
    float32_t pixelPadding;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float32_t2 uv = input.texcoord;

    // --- ピクセレート処理 ---
    if (usePixelation > 0.5f)
    {
        // 1. UV座標をブロック数でカクカクさせる
        // 例: uv(0.123, 0.456) * 32 -> floor(3.936, 14.592) -> (3, 14) / 32 -> finalUV(0.09375, 0.4375)
        uv.x = floor(uv.x * blockCountX) / blockCountX;
        uv.y = floor(uv.y * blockCountY) / blockCountY;
    }

    // --- 1. 量子化されたUVを使ってサンプリング (Modern HLSL) ---
    float32_t4 color = gTexture.Sample(gSampler, uv);

    // --- 2. 輝度を計算（加重平均） ---
    float32_t luminance = dot(color.rgb, float32_t3(0.299, 0.587, 0.114));

    // --- 3. RGBを輝度にし、Alphaは元の値を保持 ---
    float32_t3 monoColor = float32_t3(luminance, luminance, luminance);

    // --- オプション: マスク画像（gMaskTexture）を使う場合 ---
    float32_t mask = gMaskTexture.Sample(gSampler, input.texcoord).r;
    output.color.rgb = lerp(color.rgb, monoColor, mask);
    output.color.a = color.a;

    // もしマスクを使わない場合はシンプルに：
    // output.color = float32_t4(monoColor, color.a);

    return output;
    
}

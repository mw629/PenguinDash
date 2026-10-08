#include "PostEffect.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t>  gMaskTexture : register(t1);
sampler gSampler : register(s0);

ConstantBuffer<PostEffectData> gPostEffect : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // --- パラメータ取得 ---
    // value1 : ドットのグリッドサイズ (ピクセル単位, 例: 8.0)
    // value2 : ドットエッジのソフトネス (0.0=ハード, 1.0=ソフト, 例: 0.5)
    // ratio  : エフェクト強度 (0.0=オリジナル, 1.0=フルエフェクト)
    
    float gridSize = max(gPostEffect.value1, 2.0);
    float softness = clamp(gPostEffect.value2, 0.0, 2.0);
    float strength = clamp(gPostEffect.ratio, 0.0, 1.0);

    // --- スクリーンサイズをテクスチャから推定 ---
    uint screenW, screenH;
    gTexture.GetDimensions(screenW, screenH);
    float2 screenSize = float2((float)screenW, (float)screenH);

    // --- グリッドセルの中心を計算 ---
    float2 pixelPos   = input.texcoord * screenSize;
    float2 cellIndex  = floor(pixelPos / gridSize);
    float2 cellCenter = (cellIndex + 0.5) * gridSize;
    float2 cellCenterUV = cellCenter / screenSize;

    // --- セル中心の輝度を取得してドット半径を決定 ---
    float4 centerColor = gTexture.Sample(gSampler, cellCenterUV);
    float lum = dot(centerColor.rgb, float3(0.299, 0.587, 0.114));

    // ドット半径: 輝度が高い(明るい)ほどドットが大きい
    float maxRadius = gridSize * 0.5;
    float dotRadius = lum * maxRadius;

    // --- 現在ピクセルとセル中心の距離 ---
    float dist = length(pixelPos - cellCenter);

    // --- ドットマスクを計算 ---
    // softness が 0 ならハードエッジ, 大きいほど滑らか
    float edgeSoft = max(softness * 0.5, 0.001);
    float dotMask = 1.0 - smoothstep(dotRadius - edgeSoft, dotRadius + edgeSoft, dist);

    // --- ドット色を元の色から取得 ---
    float4 originalColor = gTexture.Sample(gSampler, input.texcoord);
    float4 dotColor      = float4(originalColor.rgb * dotMask, originalColor.a);

    // --- ratio でオリジナルとブレンド ---
    output.color = lerp(originalColor, dotColor, strength);

    return output;
}

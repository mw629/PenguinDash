#include "PostEffect.hlsli"

Texture2D<float32_t4> gTexture : register(t0);        // 氷表面テクスチャ (ice_surface.png)
Texture2D<float32_t4> gFrostTexture : register(t1);    // 霜テクスチャ (ice_frost.png)
sampler gSampler : register(s0);

ConstantBuffer<PostEffectData> gPostEffect : register(b0);

// 擬似乱数ハッシュ
float hash21(float2 p) {
    p = frac(p * float2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return frac(p.x * p.y);
}

// 2Dバリューノイズ
float noise(float2 p) {
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(hash21(i), hash21(i + float2(1, 0)), f.x),
                lerp(hash21(i + float2(0, 1)), hash21(i + float2(1, 1)), f.x), f.y);
}

// フラクタル・ブラウン運動 (FBM) による氷晶の有機的成長
float fbm(float2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; ++i) {
        v += a * noise(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float2 uv = input.texcoord;
    
    // -------------------------------------------------------------
    // 外側度 (Outer Score): 外側 (画面端・四隅) ほど 1.0, 中心 (0.5, 0.5) ほど 0.0
    // -------------------------------------------------------------
    // 16:9アスペクト比を考慮した中心からの楕円距離
    float2 toCenter = float2((uv.x - 0.5) * (16.0 / 9.0), uv.y - 0.5);
    float centerDist = length(toCenter); // 中心で0.0, 左右端で0.88, 四隅で1.02
    
    // 画面四辺からの距離 (0.0: フチ 〜 1.0: 中心)
    float edgeX = saturate(min(uv.x, 1.0 - uv.x) * 2.0);
    float edgeY = saturate(min(uv.y, 1.0 - uv.y) * 2.0);
    float edgeDist = min(edgeX, edgeY);
    
    // 楕円距離と矩形フチ距離をブレンドし、四辺・四隅の双方から美しく包み込む
    float outerScore = saturate(lerp(1.0 - edgeDist, centerDist / 0.95, 0.45));
    
    // テクスチャサンプリング
    float4 frostTex = gFrostTexture.Sample(gSampler, uv);
    float4 surfTex  = gTexture.Sample(gSampler, uv * 1.4);
    
    // 霜・氷の結晶パターン (テクスチャ + FBMノイズ)
    float frostPattern = frostTex.r * 0.6 + surfTex.r * 0.4;
    float organicNoise = fbm(uv * 12.0 + float2(gPostEffect.time * 0.04, 0.0));
    
    // 結晶のトゲによる乱れ (結晶の針が外側から中心へ向かって鋭く突き出す)
    float crystalSpike = (frostPattern * 0.65 + organicNoise * 0.35 - 0.45) * 0.35;
    
    // ピクセルごとの凍結優先度 (外側ほど高く、内側へ結晶のトゲが伸びる)
    float freezePriority = outerScore + crystalSpike;
    
    // 凍結進行度 (0.0: 未凍結 〜 1.0: 完全凍結)
    float progress = saturate(gPostEffect.ratio);
    
    // progressに応じて、外側 (優先度大) から中心 (優先度小) へ前進する侵食閾値
    // progress=0.0 -> 1.18 (どこも凍らない)
    // progress=1.0 -> -0.18 (画面全体・中心の穴まで完全に凍結)
    float frontierThreshold = lerp(1.18, -0.18, progress);
    
    // 氷の不透明度マスク (外側から中心へ向かって迫る侵食)
    float iceMask = smoothstep(frontierThreshold - 0.05, frontierThreshold + 0.05, freezePriority);
    iceMask = saturate(iceMask);
    
    // 凍結前線 (外側から迫る氷の先端ライン) の白青い潜熱発光
    float edgeDistance = abs(freezePriority - frontierThreshold);
    float edgeGlow = exp(-pow(edgeDistance * 16.0, 2.0)) * saturate(progress * 1.6);
    
    // 氷のカラーリング (クリスタルシアン〜ペールアイスブルー〜純白)
    float3 baseIceCol = gPostEffect.color;
    float iceSpecular = surfTex.g * 0.5 + frostTex.g * 0.5;
    float3 iceColor = lerp(baseIceCol * 0.88, float3(1.0, 1.0, 1.0), iceSpecular);
    
    // 外周の分厚い冷気グラデーション
    float coldVignette = outerScore * outerScore * progress * 0.45;
    
    // 最終カラー合成
    float3 finalRgb = iceColor 
                    + float3(0.7, 0.9, 1.0) * edgeGlow * 2.0 
                    + float3(0.12, 0.32, 0.52) * coldVignette;
    
    // 最終アルファ値 (未凍結の中心は0、凍った外側は1.0、エッジは発光)
    float finalAlpha = saturate(iceMask * 0.98 + edgeGlow * 0.7 + coldVignette * iceMask);
    
    output.color = float32_t4(finalRgb, finalAlpha);
    return output;
}

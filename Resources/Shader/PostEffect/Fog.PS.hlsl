#include "PostEffect.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gDepthTexture : register(t1);
sampler gSampler : register(s0);

ConstantBuffer<PostEffectData> gPostEffect : register(b0);

// --- 簡易的な2Dノイズ関数 ---
float hash(float2 p) {
    p = frac(p * 0.3183099 + 0.1);
    p *= 17.0;
    return frac(p.x * p.y * (p.x + p.y));
}

float noise(float2 x) {
    float2 i = floor(x);
    float2 f = frac(x);
    f = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(hash(i + float2(0.0, 0.0)), hash(i + float2(1.0, 0.0)), f.x),
                lerp(hash(i + float2(0.0, 1.0)), hash(i + float2(1.0, 1.0)), f.x), f.y);
}

// フラクタルノイズ（FBM）で自然なムラを作る
float fbm(float2 p) {
    float f = 0.0;
    f += 0.5000 * noise(p); p = p * 2.02;
    f += 0.2500 * noise(p); p = p * 2.03;
    f += 0.1250 * noise(p); p = p * 2.01;
    f += 0.0625 * noise(p);
    return f;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
  // 1. 元画像のカラーと深度値（Depth）を取得
    float32_t4 sceneColor = gTexture.Sample(gSampler, input.texcoord);
    float32_t depth = gDepthTexture.Sample(gSampler, input.texcoord);

    // 2. デプス値をカメラからの実際の距離 (Linear Distance) に変換
    // UV座標からNDC(正規化デバイス座標)を求める [-1, 1] の範囲
    float32_t ndcX = input.texcoord.x * 2.0f - 1.0f;
    float32_t ndcY = 1.0f - input.texcoord.y * 2.0f;
    
    // ProjectionInverseを使ってView座標系に逆変換し、View空間での座標を求める
    float32_t4 viewSpacePos = mul(float32_t4(ndcX, ndcY, depth, 1.0f), gPostEffect.ProjectionInverse);
    viewSpacePos /= viewSpacePos.w; // 座標を正規化（wで除算）
    float32_t linearDistance = viewSpacePos.z;

    // --- 【距離によるフォグの開始位置調整 (Distance Offset)】 ---
    // gPostEffect.value2 を「フォグが発生し始める距離」として使用（デフォルト: 12.0m）
    float32_t startDistance = (gPostEffect.value2 <= 0.0f) ? 12.0f : gPostEffect.value2;
    float32_t fogDistance = max(0.0f, linearDistance - startDistance);

    // --- 【揺らぎの計算 (Swaying Fog)】 ---
    // 空間座標と時間を組み合わせてサイン波による揺らぎを作る
    float32_t sway = sin(viewSpacePos.x * 0.5f + gPostEffect.time * 2.0f) * 0.5f 
                   + cos(viewSpacePos.z * 0.5f + gPostEffect.time * 1.5f) * 0.5f;

    // --- 【高さによるフォグの調整 (Height Fog)】 ---
    // viewSpacePos.y はカメラより下がマイナス、上がプラスになります。
    // sway を加算することで、フォグの高さがゆらゆらと動くようになります。
    // ratio を使って「高さによるフォグの消え具合」をコントロールします。
    float32_t heightFog = saturate((viewSpacePos.y + sway) * -0.3f + 1.0f);
    heightFog = lerp(1.0f, heightFog, gPostEffect.ratio);

    // --- 【まばらなフォグのムラ (Patchiness)】 ---
    // 奥に向かって流れるようにZ座標に時間を足す（引く）
    float2 noisePos = float2(viewSpacePos.x, viewSpacePos.z);
    noisePos.y -= gPostEffect.time * 8.0f; // Z方向（奥）へ流れるスピード
    noisePos.x += gPostEffect.time * 1.5f; // X方向（横）へ流れるスピード

    // FBMノイズを使って、周期的ではない自然でランダムなムラを作る
    float32_t patchiness = fbm(noisePos * 0.08f); 
    
    // patchiness を使って局所的なムラを作りつつ、扱いやすい濃度(density)を計算
    // value1が0またはデフォルト値でも綺麗に表示されるスケール
    float32_t baseDensity = (gPostEffect.value1 <= 0.0f) ? 0.015f : (gPostEffect.value1 * 0.002f);
    float32_t density = baseDensity * (patchiness * 1.5f + 0.5f);

    // 3. 指数関数モデルでフォグ係数を計算 (0.0 ～ 1.0)
    float32_t fogFactor = exp(-fogDistance * density * heightFog);
    fogFactor = saturate(fogFactor);

    // スカイボックス（背景）の特別処理: 空や遠山のグラデーションを美しく残す
    if (depth >= 0.9999f)
    {
        // 遠景の空は地平線付近（下部）ほど霞み、上空は青空が抜けるようにブレンド
        float32_t skyFogRatio = saturate(0.65f + 0.25f * (1.0f - input.texcoord.y));
        fogFactor = max(fogFactor, 1.0f - skyFogRatio);
    }

    // 4. 元の色とフォグカラーを線形補間
    float32_t3 fogColor = gPostEffect.color;
    // デフォルト色または未設定の場合のフォールバック (淡い雪景色のアイスブルーホワイト)
    if (dot(fogColor, float32_t3(1.0f, 1.0f, 1.0f)) < 0.01f)
    {
        fogColor = float32_t3(0.82f, 0.88f, 0.95f);
    }
    
    float32_t3 finalColor = lerp(fogColor, sceneColor.rgb, fogFactor);

    output.color = float32_t4(finalColor, sceneColor.a);
    
    return output;
}

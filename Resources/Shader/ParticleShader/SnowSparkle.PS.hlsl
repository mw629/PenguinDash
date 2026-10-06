#include "Particle.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// =================================================================
// 【雪のキラキラ（Snow Sparkle）調整用パラメーター】
// =================================================================

// --- [きらめき（Twinkle / 点滅）設定] ---
// ① きらめく速さ・周期 (小さいとゆっくり、大きいと頻繁にキラキラ: 10.0〜35.0)
static const float kTwinkleSpeed = 20.0f;

// ② きらめきの鋭さ (大きいほど「一瞬だけキランッ」と鋭く光ります: 4.0〜10.0)
static const float kTwinkleSharpness = 6.0f;

// ③ 普段（光っていない瞬間）の明るさ比率 (0.1〜0.5)
static const float kBaseBrightness = 0.25f;

// ④ キランッと光った瞬間の最大輝度倍率 (2.0〜4.5)
static const float kFlashBoost = 3.2f;

// --- [光条・発光設定] ---
// ⑤ 十字の主光条の太さと長さ (kSpikeSharpnessが小さいほど太くクッキリ: 6.0〜15.0)
static const float kSpikeSharpness = 8.5f;
static const float kSpikeLength = 1.9f;       // 光条の伸びる長さ (1.2〜2.5)
static const float kCrossIntensity = 1.6f;    // 主光条の輝度

// ⑥ 斜め45度のサブ光条（8芒星効果）の強度比率 (0.0: 十字のみ 〜 1.0: 均等8芒星)
static const float kDiagSpikeIntensity = 0.75f;

// ⑦ 中心のコア（超高輝度発光）の集光度と強さ
static const float kCoreGlowPower = 8.0f;     // 中心光の広がり
static const float kCoreIntensity = 3.0f;     // コアの輝度

// ⑧ 光の散乱・周囲のグローオーラ強度
static const float kGlowHaloIntensity = 0.8f;

// ⑨ 結晶感を生むダイヤモンド（ひし形）ハイライトの強さ
static const float kDiamondIntensity = 1.0f;

// ⑩ ブルーム（発光）ブースト倍率
static const float kBloomBoost = 4.0f;

// ⑪ 雪・氷の澄んだアイシーカラー
static const float kIceColorBlend = 0.45f;
static const float3 kIceColor = float3(0.70f, 0.90f, 1.0f); // 澄んだ氷雪シアンブルー
static const float3 kPrismEdge = float3(0.60f, 0.75f, 1.0f); // 光芒のフチのプリズム光

// =================================================================

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // UVから中心へのベクトルと距離 (中心を0.0とする -0.5〜+0.5 のローカル座標系)
    float2 p = input.texcoord - float2(0.5f, 0.5f);
    float dist = length(p);

    // 外側の四角いポリゴンエッジが出ないよう、円形外側で滑らかにフェードアウト
    float outerFade = smoothstep(0.5f, 0.30f, dist);

    // -------------------------------------------------------------
    // 0. 「時々きらめく（Twinkle Flash）」アニメーション計算
    // -------------------------------------------------------------
    // パーティクルの寿命進行度 (1.0 -> 0.0) を時間の基準として使用
    float progress = 1.0f - saturate(input.color.a);

    // 個体ごとのランダム位相シード（全パーティクルが同時に光らず、バラバラに光る）
    float seed = frac(sin(dot(floor(input.position.xy * 0.02f), float2(12.9898f, 78.233f)) + input.color.a * 19.17f) * 43758.5453f);

    // 複数周波数の波を重ねて非周期的な自然なゆらぎを生成
    float t = progress * kTwinkleSpeed + seed * 6.283185f;
    float wave = sin(t) * 0.5f + sin(t * 2.3f + 1.2f) * 0.3f + sin(t * 4.7f + 2.4f) * 0.2f;

    // べき乗で引き絞り、「普段は静かで時々キランッ」と鋭く閃光する係数 (0.0〜1.0)
    float flash = pow(saturate(wave * 0.6f + 0.45f), kTwinkleSharpness);

    // キラメキの動的スケール：フラッシュ時に光条が外側にグワッと伸びる
    float flashScale = lerp(0.55f, 1.25f, flash);
    float dynSpikeLength = kSpikeLength * flashScale;
    float dynCrossIntensity = kCrossIntensity * lerp(0.35f, 1.4f, flash);
    float dynCoreIntensity = kCoreIntensity * lerp(0.4f, 1.35f, flash);

    // フラッシュの瞬間にわずかに光条が回転してキラッと感を高める
    float rotAngle = flash * 0.15f;
    float sRot = sin(rotAngle);
    float cRot = cos(rotAngle);
    float2 twistedP = float2(p.x * cRot - p.y * sRot, p.x * sRot + p.y * cRot);

    // -------------------------------------------------------------
    // 1. 十字の主光条 (Cross Spikes)
    // -------------------------------------------------------------
    float rayX = saturate(1.0f - abs(twistedP.y) * kSpikeSharpness) * saturate(1.0f - abs(twistedP.x) * dynSpikeLength);
    float rayY = saturate(1.0f - abs(twistedP.x) * kSpikeSharpness) * saturate(1.0f - abs(twistedP.y) * dynSpikeLength);
    float crossSpikes = (pow(rayX, 2.0f) + pow(rayY, 2.0f)) * dynCrossIntensity;

    // 中心に向かって集光する光学グレア
    float flareX = (0.025f / (abs(twistedP.y) * 12.0f + 0.03f)) * saturate(1.0f - abs(twistedP.x) * dynSpikeLength);
    float flareY = (0.025f / (abs(twistedP.x) * 12.0f + 0.03f)) * saturate(1.0f - abs(twistedP.y) * dynSpikeLength);
    float flare = (flareX + flareY) * (0.7f * lerp(0.4f, 1.2f, flash));

    // -------------------------------------------------------------
    // 2. 斜め45度のサブ光条 (Diagonal Spikes -> 8芒星)
    // -------------------------------------------------------------
    float2 rotP = float2(
        (twistedP.x - twistedP.y) * 0.70710678f,
        (twistedP.x + twistedP.y) * 0.70710678f
    );
    float diagRayX = saturate(1.0f - abs(rotP.y) * (kSpikeSharpness * 1.2f)) * saturate(1.0f - abs(rotP.x) * (dynSpikeLength * 1.2f));
    float diagRayY = saturate(1.0f - abs(rotP.x) * (kSpikeSharpness * 1.2f)) * saturate(1.0f - abs(rotP.y) * (dynSpikeLength * 1.2f));
    float diagSpikes = (pow(diagRayX, 2.0f) + pow(diagRayY, 2.0f)) * (kDiagSpikeIntensity * lerp(0.3f, 1.3f, flash));

    // -------------------------------------------------------------
    // 3. 結晶ダイヤモンド（ひし形）ハイライト
    // -------------------------------------------------------------
    float diamondDist = (abs(p.x) + abs(p.y)) * 2.2f;
    float diamond = pow(saturate(1.0f - diamondDist), 2.0f) * (kDiamondIntensity * lerp(0.5f, 1.2f, flash));

    // -------------------------------------------------------------
    // 4. 中心の高輝度コア光 & 柔らかなグロー（散乱光）
    // -------------------------------------------------------------
    float core = exp(-dist * kCoreGlowPower) * dynCoreIntensity;
    float halo = exp(-dist * 4.5f) * kGlowHaloIntensity;

    // -------------------------------------------------------------
    // 5. テクスチャのサンプリング
    // -------------------------------------------------------------
    float32_t4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 texColor = gTexture.Sample(gSampler, transformedUV.xy);

    float texBrightness = max(texColor.a, max(texColor.r, max(texColor.g, texColor.b)));
    float centerDisc = texBrightness * smoothstep(0.35f, 0.0f, dist) * 0.5f;

    // きらめき倍率（ベース輝度 〜 フラッシュ輝度）
    float twinkleIntensity = lerp(kBaseBrightness, kFlashBoost, flash);

    // キラキラ全体の光強度
    float lightSum = (core + halo + crossSpikes + flare + diagSpikes + diamond + centerDisc) * outerFade * twinkleIntensity;

    // -------------------------------------------------------------
    // 6. カラーリング & 眩い発光（ハイライト・Bloom）
    // -------------------------------------------------------------
    // 基本カラー：マテリアル色 × 頂点色
    float3 baseColor = gMaterial.color.rgb * input.color.rgb;
    baseColor *= texColor.rgb;

    // 光芒の外周部には澄んだアイシーブルーとプリズム光をブレンド
    float3 icyTint = lerp(baseColor, kIceColor * length(baseColor), kIceColorBlend);
    float3 rimPrism = lerp(icyTint, kPrismEdge, saturate((dist * 2.0f) - 0.2f) * 0.5f);

    // キランッと光った瞬間は中心がまばゆい純白へ強くシフト
    float whiteShift = saturate((lightSum - 0.4f) * 1.5f);
    float3 emissionColor = lerp(rimPrism * lightSum, float3(1.4f, 1.45f, 1.55f) * lightSum, whiteShift);

    // Bloom（発光エフェクト）ブースト
    float bloomFactor = saturate(lightSum * 0.7f);
    emissionColor *= (1.0f + bloomFactor * (kBloomBoost - 1.0f));

    output.color.rgb = emissionColor;

    // アルファ値（透明度）：普段も存在感は保ちつつ、光った瞬間にくっきり浮かび上がる
    float alphaMask = saturate(lightSum * 1.1f + 0.15f * outerFade) * outerFade;
    output.color.a = alphaMask * gMaterial.color.a * input.color.a;

    // 完全に透明なピクセルは破棄
    if (output.color.a <= 0.001f)
    {
        discard;
    }

    return output;
}
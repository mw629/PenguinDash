#include "Particle.hlsli" 

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float32_t4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // =================================================================
    // 【トゥーン調整用パラメーター】（エフェクトの見た目を見ながら微調整してください）
    // =================================================================
    
    // ① 輪郭を切り抜く基準の濃さ (0.1 〜 0.9)
    // ※ 大きくするほどパーティクルが一回り「細く・小さく」なります
    float baseThreshold = 0.4f;
    
    // ② 輪郭の滑らかさ・アンチエイリアス幅 (0.01 〜 0.1)
    // ※ 0にするとドット絵のようにジャギー(ギザギザ)が出ます。0.03前後がベストです
    float smoothness = 0.03f;

    // ③【超重要】アニメ作画風の消え方（イロージョン）を使うか？ (0 = 使わない, 1 = 使う)
#define USE_ANIME_DISSOLVE 1 

    // =================================================================

    float threshold = baseThreshold;

#if USE_ANIME_DISSOLVE
    // パーティクルの寿命(input.color.a)が減るにつれて、切り抜き閾値を「上にスライド」させる。
    // これにより、全体が幽霊のように薄くなって消えるのではなく「外側から内側へ削られて消える」
    threshold = baseThreshold + (1.0f - input.color.a);
#endif

    // テクスチャのぼやけたグラデーションを、バキッとしたアニメ調の「2値マスク」に変換
    float shapeMask = smoothstep(threshold - smoothness, threshold + smoothness, textureColor.r);

    // 【RGB（色）の出力】
    // テクスチャの暗さを一切掛け算せず、マテリアル色 × 頂点色を100%そのまま出力する（発色MAX）
    output.color.rgb = gMaterial.color.rgb * input.color.rgb;

    // 【Alpha（透明度）の出力】
#if USE_ANIME_DISSOLVE
    // 寿命による消滅は上の threshold スライドで完全に制御できているため、input.color.a は掛けない
    output.color.a = gMaterial.color.a * shapeMask;
#else
    // スイッチが0の時は、従来の「全体が徐々に透けて消える」アニメーションになる
    output.color.a = gMaterial.color.a * input.color.a * shapeMask;
#endif

    // 完全に透明なピクセルは描画処理そのものを破棄する（半透明の重なりによる黒ずみ防止）
    if (output.color.a <= 0.001f)
    {
        discard;
    }
   
    return output;
}
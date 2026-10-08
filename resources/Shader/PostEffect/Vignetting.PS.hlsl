#include "PostEffect.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
sampler gSampler : register(s0);


ConstantBuffer<PostEffectData> gPostEffect : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
   
    //周囲を0に、中心になるほど暗くなるように計算する
    float32_t2 correct = input.texcoord * (1.0f - input.texcoord.yx);
    //correctだけで計算すると中心の最大値が0.0625で暗すぎるのでScaleで調整
    float vignette = correct.x * correct.y * 16.0f;
    //時間経過と強さ(ratio)を反映して累乗値を調整
    float vignettePower = (0.8f + 0.2f * sin(gPostEffect.time * 3.0f)) * gPostEffect.ratio;
    vignette = saturate(pow(vignette, vignettePower));
    //係数として乗算
    output.color.rgb *= vignette;
        
    return output;
}

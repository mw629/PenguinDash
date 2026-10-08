#include "PostEffect.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gDepthTexture : register(t1);
sampler gSampler : register(s0);

ConstantBuffer<PostEffectData> gPostEffect : register(b0);

static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f,  0.0f }, { 0.0f,  0.0f }, { 1.0f,  0.0f } },
    { { -1.0f,  1.0f }, { 0.0f,  1.0f }, { 1.0f,  1.0f } }
};

static const float32_t kPrewittHorizontalKernel[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};

static const float32_t kPrewittVerticalKernel[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    {  0.0f,         0.0f,         0.0f },
    {  1.0f / 6.0f,  1.0f / 6.0f,  1.0f / 6.0f },
};

float32_t GetLinearDepth(float32_t ndcDepth)
{
    float32_t4 viewSpacePos = mul(float32_t4(0.0f, 0.0f, ndcDepth, 1.0f), gPostEffect.ProjectionInverse);
    return max(0.1f, viewSpacePos.z / viewSpacePos.w);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 sceneColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = sceneColor;
    
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    
    // 線の太さ（value2: 1.0f〜3.0f、デフォルト 1.0f）
    float32_t lineWidth = (gPostEffect.value2 <= 0.0f) ? 1.0f : clamp(gPostEffect.value2, 0.5f, 3.0f);
    float32_t2 uvStepSize = float32_t2(rcp((float)width), rcp((float)height)) * lineWidth;
    
    // 中心ピクセルのデプスと距離
    float32_t centerDepth = gDepthTexture.Sample(gSampler, input.texcoord);
    float32_t centerZ = GetLinearDepth(centerDepth);
    
    float32_t2 difference = float32_t2(0.0f, 0.0f);
    for (int32_t y = 0; y < 3; ++y)
    {
        for (int32_t x = 0; x < 3; ++x)
        {
            float32_t2 texcoord = input.texcoord + kIndex3x3[y][x] * uvStepSize;
            float32_t sampleDepth = gDepthTexture.Sample(gSampler, texcoord);
            float32_t sampleZ = GetLinearDepth(sampleDepth);
            
            difference.x += sampleZ * kPrewittHorizontalKernel[y][x];
            difference.y += sampleZ * kPrewittVerticalKernel[y][x];
        }
    }
    
    // 距離で正規化した相対深度差（奥の平坦面が黒ずむのを防ぎ、明確な輪郭・段差のみを抽出）
    float32_t relativeDiff = length(difference) / centerZ;
    
    // 感度調整（value1: デフォルト 1.0f）
    float32_t sensitivity = (gPostEffect.value1 <= 0.0f) ? 1.0f : (gPostEffect.value1 * 0.2f);
    
    // しきい値（微小な傾斜はカットし、オブジェクトの輪郭を抽出）
    float32_t threshold = 0.025f;
    float32_t weight = saturate((relativeDiff - threshold) * 45.0f * sensitivity);
    
    // アウトラインカラー（デフォルトは黒）
    float32_t3 outlineColor = gPostEffect.color;
    
    output.color.rgb = lerp(sceneColor.rgb, outlineColor, weight);
    output.color.a = sceneColor.a;
    
    return output;
}
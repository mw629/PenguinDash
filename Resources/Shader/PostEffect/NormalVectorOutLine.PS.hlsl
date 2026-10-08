#include "PostEffect.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gDepthTexture : register(t1);
sampler gSampler : register(s0);

ConstantBuffer<PostEffectData> gPostEffect : register(b0);

static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } }
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
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
};

float32_t3 ReconstructViewPosition(float32_t2 uv, float32_t depth)
{
    float32_t x = uv.x * 2.0f - 1.0f;
    float32_t y = 1.0f - uv.y * 2.0f;
    float32_t4 ndc = float32_t4(x, y, depth, 1.0f);
    float32_t4 viewPos = mul(ndc, gPostEffect.ProjectionInverse);
    return viewPos.xyz / viewPos.w;
}

float32_t3 ComputeNormalFromDepth(float32_t2 uv, float32_t2 texelSize)
{
    float32_t depth0 = gDepthTexture.Sample(gSampler, uv);
    float32_t depth1 = gDepthTexture.Sample(gSampler, uv + float32_t2(texelSize.x, 0.0f));
    float32_t depth2 = gDepthTexture.Sample(gSampler, uv + float32_t2(0.0f, texelSize.y));
    
    float32_t3 p0 = ReconstructViewPosition(uv, depth0);
    float32_t3 p1 = ReconstructViewPosition(uv + float32_t2(texelSize.x, 0.0f), depth1);
    float32_t3 p2 = ReconstructViewPosition(uv + float32_t2(0.0f, texelSize.y), depth2);
    
    float32_t3 v1 = p1 - p0;
    float32_t3 v2 = p2 - p0;
    
    return normalize(cross(v1, v2));
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));
    
    float32_t3 differenceHorizontal = float32_t3(0.0f, 0.0f, 0.0f);
    float32_t3 differenceVertical = float32_t3(0.0f, 0.0f, 0.0f);
    
    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            float32_t2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            float32_t3 normal = ComputeNormalFromDepth(texcoord, uvStepSize);
            
            differenceHorizontal += normal * kPrewittHorizontalKernel[x][y];
            differenceVertical += normal * kPrewittVerticalKernel[x][y];
        }
    }
    
    // 変化の長さをウェイトとして合成
    float32_t weight = length(differenceHorizontal) + length(differenceVertical);
    
    // 緩やかな角度（カーブなど）では線が出ないように、しきい値(threshold)を設ける
    // ImGui の Value1 (0〜100) でしきい値を調整可能に（0の場合はデフォルトの0.2を使用）
    float32_t threshold = gPostEffect.value1 == 0.0f ? 0.2f : (gPostEffect.value1 / 100.0f);
    
    // しきい値以下の変化は 0 になり、超えた部分のみが線として抽出される
    weight = saturate((weight - threshold) * 10.0f);
    
    output.color.rgb = (1.0f - weight) * gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.0f;
    
    return output;
}

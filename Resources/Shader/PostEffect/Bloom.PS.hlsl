#include "PostEffect.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gMaskTexture : register(t1);
sampler gSampler : register(s0);

ConstantBuffer<PostEffectData> gPostEffect : register(b0);

static const float32_t PI = 3.14159265f;

float gauss(float32_t x, float32_t y, float32_t sigma)
{
    float exponent = -(x * x + y * y) * rcp(2.0f * sigma * sigma);
    float denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}

float32_t Luminance(float32_t3 v)
{
    return dot(v, float32_t3(0.2125f, 0.7154f, 0.0721f));
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 originalColor = gTexture.Sample(gSampler, input.texcoord);
    
    float32_t3 bloomColor = float32_t3(0.0f, 0.0f, 0.0f);
    float32_t weight = 0.0f;
    
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp((float)width), rcp((float)height)) * gPostEffect.blurStrength;
    
    float32_t threshold = gPostEffect.value1;
    if (threshold == 0.0f) threshold = 0.8f; // Default luminance threshold
    
    if (gPostEffect.kernelSize == 3)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            for (int32_t x = 0; x < 3; ++x)
            {
                float32_t w = gauss(gPostEffect.index3x3[y][x].x, gPostEffect.index3x3[y][x].y, 2.0f);
                weight += w;
                float32_t2 texcood = input.texcoord + gPostEffect.index3x3[y][x] * uvStepSize;
                float32_t3 fetchColor = gTexture.Sample(gSampler, texcood).rgb;
                
                float32_t lum = Luminance(fetchColor);
                bloomColor += fetchColor * w * step(threshold, lum);
            }
        }
    }
    else
    {
        for (int32_t y = 0; y < 5; ++y)
        {
            for (int32_t x = 0; x < 5; ++x)
            {
                float32_t w = gauss(gPostEffect.index5x5[y][x].x, gPostEffect.index5x5[y][x].y, 2.0f);
                weight += w;
                float32_t2 texcood = input.texcoord + gPostEffect.index5x5[y][x] * uvStepSize;
                float32_t3 fetchColor = gTexture.Sample(gSampler, texcood).rgb;
                
                float32_t lum = Luminance(fetchColor);
                bloomColor += fetchColor * w * step(threshold, lum);
            }
        }
    }
    
    bloomColor /= weight;
    
    float32_t intensity = gPostEffect.value2;
    if (intensity == 0.0f) intensity = 1.0f; // Default bloom intensity
    
    output.color.rgb = originalColor.rgb + bloomColor * intensity;
    output.color.a = originalColor.a;
    
    return output;
}

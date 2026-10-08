#include "Object3d.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
TextureCube<float32_t4> gEnvironmentTexture : register(t1);
SamplerState gSampler : register(s0);

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<Camera> gCamera : register(b1);

struct DirectionalLightGroup { int32_t numLights; float32_t3 padding; DirectionalLight lights[64]; };
struct PointLightGroup       { int32_t numLights; float32_t3 padding; PointLight lights[64]; };
struct SpotLightGroup        { int32_t numLights; float32_t3 padding; SpotLight lights[64]; };

ConstantBuffer<DirectionalLightGroup> gDirectionalLightGroup : register(b2);
ConstantBuffer<PointLightGroup>       gPointLightGroup       : register(b3);
ConstantBuffer<SpotLightGroup>        gSpotLightGroup        : register(b4);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    float3 N = normalize(input.normal);
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    
    // フレネル反射率の計算 (Schlickの近似: 浅い角度ほど反射が強くなる)
    float NdotV = saturate(dot(N, toEye));
    float fresnel = 0.04f + (1.0f - 0.04f) * pow(1.0f - NdotV, 5.0f);
    
    // 水面の色 (深海色と浅瀬色/エメラルドグリーン)
    float3 deepWaterColor = float3(0.01f, 0.08f, 0.22f);
    float3 shallowWaterColor = float3(0.05f, 0.35f, 0.55f);
    
    // マテリアルカラーが設定されている場合はその色味をブレンド
    if (gMaterial.color.a > 0.0f)
    {
        deepWaterColor *= gMaterial.color.rgb * 1.5f;
        shallowWaterColor *= gMaterial.color.rgb * 1.8f;
    }
    
    // 傾きや視線に応じた基本水面色
    float3 waterBaseColor = lerp(deepWaterColor, shallowWaterColor, pow(1.0f - NdotV, 2.0f));
    
    // 環境マッピング (空の映り込み)
    float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
    float3 reflectedVector = reflect(cameraToPosition, N);
    float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);
    
    // ライティング (太陽光・ポイントライト等のスペキュラハイライト)
    float3 diffuse = float3(0.0f, 0.0f, 0.0f);
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    
    if (gMaterial.enableLighting != 0)
    {
        // Directional Lights
        for (int i = 0; i < gDirectionalLightGroup.numLights; ++i)
        {
            if (gDirectionalLightGroup.lights[i].active != 0)
            {
                DirectionalLight dirLight = gDirectionalLightGroup.lights[i];
                float3 halfVector = normalize(-dirLight.direction + toEye);
                float NDotH = dot(N, halfVector);
                // 水面のシャープな太陽光ハイライト
                float specularPow = pow(saturate(NDotH), max(gMaterial.shininess, 64.0f));
                float NdirL = dot(N, -dirLight.direction);
                float cosTerm = saturate(NdirL);
                
                diffuse += dirLight.color.rgb * cosTerm * dirLight.intensity * 0.3f;
                specular += dirLight.color.rgb * dirLight.intensity * specularPow * float3(1.2f, 1.2f, 1.2f);
            }
        }
        
        // Point Lights
        for (int j = 0; j < gPointLightGroup.numLights; ++j)
        {
            if (gPointLightGroup.lights[j].active != 0)
            {
                PointLight ptLight = gPointLightGroup.lights[j];
                float3 ptDir = normalize(input.worldPosition - ptLight.position);
                float3 ptHalf = normalize(-ptDir + toEye);
                float ptDotH = dot(N, ptHalf);
                float ptSpecPow = pow(saturate(ptDotH), max(gMaterial.shininess, 64.0f));
                float distance = length(ptLight.position - input.worldPosition);
                float factor = pow(saturate(-distance / ptLight.radius + 1.0f), ptLight.decay);
                
                specular += ptLight.color.rgb * ptLight.intensity * ptSpecPow * factor;
            }
        }
    }
    
    // 水面色 + 環境反射 (フレネルブレンド) + スペキュラハイライト
    float3 reflection = environmentColor.rgb * gMaterial.environmentCoefficient;
    float3 waterSurfaceColor = lerp(waterBaseColor + diffuse, reflection, fresnel);
    
    output.color.rgb = (waterSurfaceColor * textureColor.rgb) + specular;
    output.color.a = gMaterial.color.a * textureColor.a;
    
    return output;
}

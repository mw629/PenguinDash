#include "Object3d.hlsli" 

Texture2D<float32_t4> gTexture : register(t0);
TextureCube<float32_t4> gEnvironmentTexture : register(t1);
SamplerState gSampler : register(s0);
ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<Camera> gCamera : register(b1);
struct DirectionalLightGroup
{
    int32_t numLights;
    float32_t3 padding;
    DirectionalLight lights[64];
};
struct PointLightGroup
{
    int32_t numLights;
    float32_t3 padding;
    PointLight lights[64];
};
struct SpotLightGroup
{
    int32_t numLights;
    float32_t3 padding;
    SpotLight lights[64];
};

ConstantBuffer<DirectionalLightGroup> gDirectionalLightGroup : register(b2);
ConstantBuffer<PointLightGroup> gPointLightGroup : register(b3);
ConstantBuffer<SpotLightGroup> gSpotLightGroup : register(b4);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
   
   
     //Phong
    //float32_t3 reflectLight = reflect(gDirectionalLight.direction, normalize(input.normal));
    //float RdotE = dot(reflectLight, toEye);
    //float specularPow = pow(saturate(RdotE), gMaterial.shininess);
    
    float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    
    //環境マッピング
    float32_t3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
    float32_t3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
    float32_t4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);
    
    if (gMaterial.enableLighting != 0)
    {
        float32_t3 diffuse = float32_t3(0.0f, 0.0f, 0.0f);
        float32_t3 specular = float32_t3(0.0f, 0.0f, 0.0f);
        
        // Directional Lights
        for (int i = 0; i < gDirectionalLightGroup.numLights; ++i)
        {
            if (gDirectionalLightGroup.lights[i].active != 0)
            {
                DirectionalLight dirLight = gDirectionalLightGroup.lights[i];
                float32_t3 halfVector = normalize(-dirLight.direction + toEye);
                float32_t NDotH = dot(normalize(input.normal), halfVector);
                float32_t specularPow = pow(saturate(NDotH), gMaterial.shininess);
                float32_t NdirL = dot(normalize(input.normal), -dirLight.direction);
                float32_t cos = pow(NdirL * 0.5f + 0.5f, 2.0f);
                
                diffuse += gMaterial.color.rgb * textureColor.rgb * dirLight.color.rgb * cos * dirLight.intensity;
                specular += dirLight.color.rgb * dirLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
            }
        }
        
        // Point Lights
        for (int j = 0; j < gPointLightGroup.numLights; ++j)
        {
            if (gPointLightGroup.lights[j].active != 0)
            {
                PointLight ptLight = gPointLightGroup.lights[j];
                float32_t3 pointLightDirection = normalize(input.worldPosition - ptLight.position);
                float32_t3 pointHalfVector = normalize(-pointLightDirection + toEye);
                float32_t pointDotH = dot(normalize(input.normal), pointHalfVector);
                float32_t pointSpecularPow = pow(saturate(pointDotH), gMaterial.shininess);
                float32_t pointNdirL = dot(normalize(input.normal), -pointLightDirection);
                float32_t pointCos = pow(pointNdirL * 0.5f + 0.5f, 2.0f);
                float32_t distance = length(ptLight.position - input.worldPosition);
                float32_t factor = pow(saturate(-distance / ptLight.radius + 1.0f), ptLight.decay);
                
                diffuse += gMaterial.color.rgb * textureColor.rgb * ptLight.color.rgb * pointCos * ptLight.intensity * factor;
                specular += ptLight.color.rgb * ptLight.intensity * pointSpecularPow * float32_t3(1.0f, 1.0f, 1.0f) * factor;
            }
        }
        
        // Spot Lights
        for (int k = 0; k < gSpotLightGroup.numLights; ++k)
        {
            if (gSpotLightGroup.lights[k].active != 0)
            {
                SpotLight spLight = gSpotLightGroup.lights[k];
                float32_t3 spotLightDirectionOnSurface = normalize(input.worldPosition - spLight.position);
                float32_t3 spotHalfVector = normalize(-spotLightDirectionOnSurface + toEye);
                float32_t spotDotH = dot(normalize(input.normal), spotHalfVector);
                float32_t spotSpecularPow = pow(saturate(spotDotH), gMaterial.shininess);
                float32_t spotNdirL = dot(normalize(input.normal), -spotLightDirectionOnSurface);
                float32_t spotCos = pow(spotNdirL * 0.5f + 0.5f, 2.0f);
                float32_t conAngle = dot(spotLightDirectionOnSurface, spLight.direction);
                float32_t fallffFactor = saturate((conAngle - spLight.cosAngle) / (spLight.cosFalloffStart));
                float32_t spotDistance = length(spLight.position - input.worldPosition);
                float32_t attenuationFactor = pow(saturate(-spotDistance / spLight.distance + 1.0f), spLight.decay);
                
                diffuse += gMaterial.color.rgb * textureColor.rgb * spLight.color.rgb * conAngle * spLight.intensity * fallffFactor * attenuationFactor;
                specular += spLight.color.rgb * spLight.intensity * spotSpecularPow * float32_t3(1.0f, 1.0f, 1.0f) * fallffFactor * attenuationFactor;
            }
        }
        
        output.color.rgb = diffuse + specular;
       
        float32_t upDot = dot(normalize(input.normal), float32_t3(0.0f, 1.0f, 0.0f));
        float32_t snowFactor = smoothstep(0.5f, 0.8f, saturate(upDot));
        float32_t3 snowColor = float32_t3(0.9f, 0.9f, 0.95f);
        output.color.rgb = lerp(output.color.rgb, snowColor, snowFactor);
        
        //αは今まで通り
        output.color.a = gMaterial.color.a * textureColor.a;
  
    
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    //環境マッピングの色を加算
    output.color.rgb += environmentColor.rgb * gMaterial.environmentCoefficient;
    
    if (output.color.a <= 0.5f)
    {
        discard;
    }
    
    return output;
}
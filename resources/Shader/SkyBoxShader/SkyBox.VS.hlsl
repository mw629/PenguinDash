#include "SkyBox.hlsli"

StructuredBuffer<TransformationMatrix> gTransformationMatrix : register(t1);

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix[instanceID].WVP).xyww;
    output.texcoord = input.position.xyz;
    return output;
}
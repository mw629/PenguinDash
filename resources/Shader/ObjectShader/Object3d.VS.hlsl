#include "Object3d.hlsli" 

StructuredBuffer<TransformationMatrix> gTransformationMatrix : register(t1);

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceID : SV_InstanceID) {
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix[instanceID].WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float32_t3x3) gTransformationMatrix[instanceID].WorldInverseTranspose));
    output.worldPosition = mul(input.position, gTransformationMatrix[instanceID].World).xyz;
    return output;
}
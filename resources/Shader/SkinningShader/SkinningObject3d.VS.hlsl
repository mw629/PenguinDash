#include "SkinningObject3d.hlsli"

StructuredBuffer<TransformationMatrix> gTransformationMatrix : register(t1);
StructuredBuffer<Well> gMatrixPalette : register(t0);

Skinned Skinning(VertexShaderInput input, uint32_t boneOffset)
{
    Skinned skinned;
    // 位置の変換
    skinned.position = mul(input.position,  gMatrixPalette[boneOffset + input.index.x].skeletonSpaceMatrix * input.weight.x);
    skinned.position += mul(input.position, gMatrixPalette[boneOffset + input.index.y].skeletonSpaceMatrix * input.weight.y);
    skinned.position += mul(input.position, gMatrixPalette[boneOffset + input.index.z].skeletonSpaceMatrix * input.weight.z);
    skinned.position += mul(input.position, gMatrixPalette[boneOffset + input.index.w].skeletonSpaceMatrix * input.weight.w);
    skinned.position.w = 1.0f;

    // 法線の変換
    skinned.normal = mul(input.normal,  (float32_t3x3)gMatrixPalette[boneOffset + input.index.x].skeletonSpaceInverseTransposeMatrix * input.weight.x);
    skinned.normal += mul(input.normal, (float32_t3x3)gMatrixPalette[boneOffset + input.index.y].skeletonSpaceInverseTransposeMatrix * input.weight.y);
    skinned.normal += mul(input.normal, (float32_t3x3)gMatrixPalette[boneOffset + input.index.z].skeletonSpaceInverseTransposeMatrix * input.weight.z);
    skinned.normal += mul(input.normal, (float32_t3x3)gMatrixPalette[boneOffset + input.index.w].skeletonSpaceInverseTransposeMatrix * input.weight.w);
    skinned.normal = normalize(skinned.normal);

    return skinned;
}


VertexShaderOutput main(VertexShaderInput input, uint32_t instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    uint32_t boneOffset = instanceID * gTransformationMatrix[instanceID].numBones;
    Skinned skinned = Skinning(input, boneOffset);
    
    output.position = mul(skinned.position, gTransformationMatrix[instanceID].WVP);
    output.worldPosition = mul(skinned.position, gTransformationMatrix[instanceID].World).xyz;
    output.normal = normalize(mul(skinned.normal, (float32_t3x3) gTransformationMatrix[instanceID].WorldInverseTranspose));
    output.texcoord = input.texcoord;
    
    return output;
}
#include "MSDF.hlsli"

ConstantBuffer<TextParams> gTextParams : register(b0);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTextParams.wvp);
    output.texcoord = input.texcoord;
    output.color = input.color;
    return output;
}

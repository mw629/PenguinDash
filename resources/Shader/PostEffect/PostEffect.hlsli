
struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET;
};

struct PostEffectData
{
    float32_t time;
    float32_t ratio;
    float32_t value1;
    float32_t value2;
    float32_t blurStrength;
    int32_t kernelSize;
    float32_t2 padding;
    float32_t3 color;
    float32_t paddingColor;
    float32_t kernel3x3[3][3];
    float32_t kernel5x5[5][5];
    float32_t2 index3x3[3][3];
    float32_t2 index5x5[5][5];
    float32_t4x4 ProjectionInverse;
    float32_t4x4 ViewInverse;
};
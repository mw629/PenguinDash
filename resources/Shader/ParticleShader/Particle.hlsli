struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    float32_t4 color : COLOR0;
    
};

static const uint32_t kMaxParticles = 10000;

struct Particle
{
    float32_t3 translate;
    float32_t3 scale;
    float32_t3 rotate;
    float32_t3 velocity;
    float32_t lifetime;
    float32_t currentTime;
    float32_t4 color;
};

struct PerView
{
    float32_t4x4 viewProjection;
    float32_t4x4 billboardMatrix;
};

struct EmitterSphere
{
    float32_t3 translate;       // エミッタの位置
    float32_t radius;           // エミッタの半径
    float32_t count;            // エミッタの生成数
    float32_t frequency;        // エミッタの生成頻度
    float32_t frequencyTime;    // エミッタの生成頻度の時間計測用
    uint32_t emit;              // 射出許可

    float32_t3 baseScale;       // パーティクルの初期スケール
    float32_t lifetime;         // パーティクルの寿命
    float32_t3 sizeVariance;    // スケールのランダム幅
    float32_t padding0;
    float32_t3 baseVelocity;    // 初期速度
    float32_t padding1;
    float32_t3 velocityVariance; // 速度のランダム幅
    float32_t padding2;
    float32_t4 color;           // 初期カラー
    float32_t3 baseRotate;      // 初期回転
    float32_t padding3;
};

struct EmitterBox
{
    float32_t3 translate;       // エミッタの中心位置
    float32_t count;            // エミッタの生成数
    float32_t3 size;            // エミッタのサイズ (X, Y, Z の幅)
    float32_t frequency;        // エミッタの生成頻度
    float32_t frequencyTime;    // エミッタの生成頻度の時間計測用
    uint32_t emit;              // 射出許可
    float32_t lifetime;         // 寿命
    float32_t padding0;

    float32_t3 baseScale;       // 初期スケール
    float32_t padding1;
    float32_t3 sizeVariance;    // スケールばらつき
    float32_t padding2;
    float32_t3 baseVelocity;    // 初期速度
    float32_t padding3;
    float32_t3 velocityVariance; // 速度ばらつき
    float32_t padding4;
    float32_t4 color;           // 初期カラー
    float32_t3 baseRotate;      // 初期回転
    float32_t padding5;
};

struct PerFrame
{
    float32_t deltaTime;
    float32_t time;
    float32_t2 padding0;
    float32_t3 acceleration;
    float32_t padding1;
    float32_t3 sizeDelta;
    float32_t padding2;
    
    // Field Settings
    uint32_t fieldType;
    float32_t fieldStrength;
    float32_t2 padding3;
    float32_t3 fieldPosition;
    float32_t padding4;
};


struct ParticleForGPU
{
    float32_t4x4 WVP;
    float32_t4x4 World;
    float32_t4 color;
};

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

struct DirectionalLight
{
    float32_t4 color; //ライトの色
    float32_t3 direction; //ライトの向き
    float32_t intensity; //輝度
    int32_t active;
    float32_t3 padding;
};

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
};

// 3D/1D Pseudo-Random Noise functions
float32_t3 rand3dTo3d(float32_t3 value)
{
    float32_t3 p = float32_t3(
        dot(value, float32_t3(127.1f, 311.7f, 747.2f)),
        dot(value, float32_t3(269.5f, 183.3f, 246.1f)),
        dot(value, float32_t3(113.5f, 271.9f, 124.6f))
    );
    return frac(sin(p) * 43758.5453123f);
}

float32_t rand3dTo1d(float32_t3 value)
{
    float32_t p = dot(value, float32_t3(127.1f, 311.7f, 747.2f));
    return frac(sin(p) * 43758.5453123f);
}

class RandomGenerator
{
    float32_t3 seed;
    float32_t3 Generate3d()
    {
        seed = rand3dTo3d(seed);
        return seed;
    }
    float32_t Generate1d()
    {
        float32_t result = rand3dTo1d(seed);
        seed.x = result;
        return result;
    }
};
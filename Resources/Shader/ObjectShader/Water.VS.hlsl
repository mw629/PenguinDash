#include "Object3d.hlsli"

StructuredBuffer<TransformationMatrix> gTransformationMatrix : register(t1);
ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<Camera> gCamera : register(b1);

// 単一波のパラメータ構造体
struct Wave
{
    float2 direction; // 進行方向
    float amplitude;  // 振幅 (波の高さ)
    float wavelength; // 波長
    float speed;      // 伝播速度
    float steepness;  // 尖り度 (Gerstner波の強さ: 0.0〜1.0)
};

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    
    // 時間の取得 (Camera定数バッファのtime、フォールバックとしてMaterialのuvTransformを利用)
    float time = gCamera.time;
    if (time <= 0.0f)
    {
        time = gMaterial.uvTransform[3][0] + gMaterial.uvTransform[3][1];
    }
    
    // オブジェクトの基準ワールド位置を取得
    float4 initialWorldPos = mul(input.position, gTransformationMatrix[instanceID].World);
    float2 worldXZ = initialWorldPos.xz;
    
    // 4つの異なる波を定義 (向き、振幅、波長、速度、尖り度)
    Wave waves[4];
    // 1. 大きなうねり (主波)
    waves[0].direction = float2(1.0f, 0.25f);
    waves[0].amplitude = 0.6f;
    waves[0].wavelength = 20.0f;
    waves[0].speed = 1.2f;
    waves[0].steepness = 0.35f;
    
    // 2. 斜めからの交差波
    waves[1].direction = float2(-0.35f, 0.93f);
    waves[1].amplitude = 0.35f;
    waves[1].wavelength = 11.0f;
    waves[1].speed = 1.4f;
    waves[1].steepness = 0.3f;
    
    // 3. 中波 (ざわめき)
    waves[2].direction = float2(0.8f, -0.6f);
    waves[2].amplitude = 0.18f;
    waves[2].wavelength = 5.5f;
    waves[2].speed = 1.8f;
    waves[2].steepness = 0.25f;
    
    // 4. さざ波 (ディテール)
    waves[3].direction = float2(-0.5f, -0.86f);
    waves[3].amplitude = 0.08f;
    waves[3].wavelength = 2.2f;
    waves[3].speed = 2.4f;
    waves[3].steepness = 0.2f;
    
    // メッシュの法線向きから平面の軸を自動判定
    float3 heightAxis;
    float3 tangentAxis;
    float3 binormalAxis;
    
    if (abs(input.normal.y) > 0.5f)
    {
        // XZ平面 (法線 +Y / -Y)
        heightAxis = float3(0.0f, input.normal.y > 0 ? 1.0f : -1.0f, 0.0f);
        tangentAxis = float3(1.0f, 0.0f, 0.0f);
        binormalAxis = float3(0.0f, 0.0f, 1.0f);
    }
    else
    {
        // XY平面 (法線 +Z / -Z, plane.obj互換)
        heightAxis = float3(0.0f, 0.0f, input.normal.z > 0 ? 1.0f : -1.0f);
        tangentAxis = float3(1.0f, 0.0f, 0.0f);
        binormalAxis = float3(0.0f, 1.0f, 0.0f);
    }
    
    float3 localDisplacement = float3(0.0f, 0.0f, 0.0f);
    
    // 法線計算用の接線・従接線ベクトル
    float3 tangent = tangentAxis;
    float3 binormal = binormalAxis;
    
    const float kPI = 3.14159265f;
    
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        Wave w = waves[i];
        float k = 2.0f * kPI / w.wavelength; // 波数
        float c = sqrt(9.8f / k) * w.speed;  // 位相速度
        float2 d = normalize(w.direction);
        
        // ワールドXZ座標と時間に基づく位相
        float phase = k * (dot(d, worldXZ) - c * time);
        float cosP = cos(phase);
        float sinP = sin(phase);
        
        // スティープネス係数
        float q = w.steepness / (k * w.amplitude * 4.0f + 0.0001f);
        
        // ローカル法線方向(高さ)へのサイン波変位
        localDisplacement += heightAxis * (w.amplitude * sinP);
        
        // ※メッシュのスケーリング(100倍)によって水平変位が数十m肥大化して波と流氷の位置がズレるのを防ぐため、
        //   変位は純粋な高さ(垂直)変位とし、CPU計算と100%完全一致させます
        // localDisplacement += tangentAxis * (q * w.amplitude * d.x * cosP);
        // localDisplacement += binormalAxis * (q * w.amplitude * d.y * cosP);
        
        // 偏微分による接線・従接線の傾き蓄積
        float wa = k * w.amplitude;
        tangent += heightAxis * (d.x * wa * cosP);
        binormal += heightAxis * (d.y * wa * cosP);
    }
    
    // 変位後のローカル頂点位置
    float4 displacedPos = float4(input.position.xyz + localDisplacement, 1.0f);
    
    // 接線と従接線の外積から法線を再計算
    float3 localNormal = normalize(cross(binormal, tangent));
    // 法線の向きが反転しないように確認
    if (dot(localNormal, heightAxis) < 0.0f)
    {
        localNormal = -localNormal;
    }
    
    // 出力データの構築
    output.position = mul(displacedPos, gTransformationMatrix[instanceID].WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(localNormal, (float32_t3x3)gTransformationMatrix[instanceID].WorldInverseTranspose));
    output.worldPosition = mul(displacedPos, gTransformationMatrix[instanceID].World).xyz;
    
    return output;
}

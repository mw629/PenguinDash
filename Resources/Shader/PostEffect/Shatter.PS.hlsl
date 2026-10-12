#include "PostEffect.hlsli"

Texture2D<float32_t4> gTexture : register(t0);        // 氷表面テクスチャ (ice_surface.png)
Texture2D<float32_t4> gCrackTexture : register(t1);    // 亀裂テクスチャ (ice_crack.png)
sampler gSampler : register(s0);

ConstantBuffer<PostEffectData> gPostEffect : register(b0);

// 2Dハッシュ関数
float2 hash22(float2 p) {
    float3 p3 = frac(float3(p.xyx) * float3(443.897, 441.423, 437.195));
    p3 += dot(p3, p3.yzx + 19.19);
    return frac((p3.xx + p3.yz) * p3.zy);
}

float hash12(float2 p) {
    float3 p3 = frac(float3(p.xyx) * 443.897);
    p3 += dot(p3, p3.yzx + 19.19);
    return frac((p3.x + p3.y) * p3.z);
}

// 2D回転
float2 rotate(float2 p, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return float2(c * p.x - s * p.y, s * p.x + c * p.y);
}

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float2 uv = input.texcoord;
    
    // パラメータ取得
    float shatterProgress = saturate(gPostEffect.ratio);  // 0.0: 未破砕 〜 1.0: 完全に四散
    float crackProgress   = saturate(gPostEffect.value1); // 0.0: 亀裂なし 〜 1.0: 亀裂全開
    float time            = gPostEffect.time;
    
    float2 center = float2(0.5, 0.5);
    float2 toCenter = uv - center;
    float distToCenter = length(float2(toCenter.x * (16.0 / 9.0), toCenter.y));
    
    // -------------------------------------------------------------
    // 1. ひび割れ (Crack) 計算 (Cracking期および破砕直後)
    // -------------------------------------------------------------
    float4 crackTex = gCrackTexture.Sample(gSampler, uv);
    float crackMask = crackTex.r;
    // 中心から外側へ走る蜘蛛の巣亀裂の先端フロンティア
    float crackFrontier = smoothstep(crackProgress * 1.3, crackProgress * 1.3 - 0.25, distToCenter);
    float activeCrack = crackMask * crackFrontier;
    
    // -------------------------------------------------------------
    // 2. ボロノイ破片シミュレーション (Shattering)
    // -------------------------------------------------------------
    float2 gridSize = float2(13.0, 8.0);
    float2 gridUV = uv * gridSize;
    float2 cellId = floor(gridUV);
    float2 cellFract = frac(gridUV);
    
    // 周囲9セルを探索して最近傍種点 (破片の中心) を特定
    float minDistance = 100.0;
    float secondMinDistance = 100.0;
    float2 bestCellId = cellId;
    float2 bestCellPoint = float2(0.0, 0.0);
    
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float2 neighbor = float2(float(x), float(y));
            float2 curCell = cellId + neighbor;
            float2 randPoint = hash22(curCell) * 0.72 + 0.14;
            float2 p = neighbor + randPoint - cellFract;
            float d = length(p);
            
            if (d < minDistance) {
                secondMinDistance = minDistance;
                minDistance = d;
                bestCellId = curCell;
                bestCellPoint = (curCell + randPoint) / gridSize;
            } else if (d < secondMinDistance) {
                secondMinDistance = d;
            }
        }
    }
    
    // ボロノイ境界線 (破片と破片の境界)
    float borderLine = secondMinDistance - minDistance;
    
    // 各破片の固有物理パラメータ
    float2 shardRand = hash22(bestCellId);
    float shardMass = hash12(bestCellId * 3.7) * 0.8 + 0.55;
    
    // 中心からの放射状飛散方向
    float2 shardOffsetFromCenter = bestCellPoint - center;
    float dist = length(float2(shardOffsetFromCenter.x * (16.0 / 9.0), shardOffsetFromCenter.y));
    float2 explodeDir = normalize(shardOffsetFromCenter + (shardRand - 0.5) * 0.45);
    
    // 爆発初速 (中心付近ほど大爆発、質量が小さいほど超高速)
    float explodeSpeed = (1.3 + (1.0 - saturate(dist)) * 0.9) / shardMass;
    
    // 破片の現在位置への移動量 (爆発飛散 + 重力加速度落下)
    float t = shatterProgress;
    float2 shardVelocity = explodeDir * explodeSpeed * 0.95;
    float2 shardPosOffset = shardVelocity * t;
    shardPosOffset.y += 0.85 * t * t; // 重力落下
    
    // 破片の3Dタンブリング回転角
    float rotSpeed = (shardRand.x - 0.5) * 16.0 / shardMass;
    float rotAngle = rotSpeed * t;
    
    // 現在のUVから、破片の移動と回転を逆変換
    float2 localPos = uv - (bestCellPoint + shardPosOffset);
    float2 rotatedLocalPos = rotate(localPos, -rotAngle);
    
    // 破片のスケール変化 (手前に飛び出してから奥へ小さくなる)
    float shardScale = 1.0 - t * 0.65;
    float2 origUV = bestCellPoint + rotatedLocalPos / shardScale;
    
    // 破片の内外判定 (破片が飛び散ると隙間が生じる)
    float gapFactor = t * 1.6;
    bool isInsideShard = (borderLine > (0.035 + gapFactor * 0.16)) && (shardScale > 0.05);
    
    // -------------------------------------------------------------
    // 3. ピクセルカラーとアルファの決定
    // -------------------------------------------------------------
    float4 surfaceTex = gTexture.Sample(gSampler, origUV * 1.4);
    
    // 破片の基本色 (クリスタルアイス)
    float3 shardColor = gPostEffect.color * (0.86 + surfaceTex.r * 0.28);
    
    // 破片の3D回転によるスペキュラー閃光 (Specular Glint)
    float glint = abs(cos(rotAngle * 2.2 + shardRand.y * 6.28));
    glint = pow(glint, 7.0) * (1.0 - t);
    shardColor += float3(1.0, 1.0, 1.0) * glint * 0.85;
    
    // 破片のエッジ断面ハイライト (ガラスの屈折断面光)
    float edgeGlow = (1.0 - smoothstep(0.0, 0.14, borderLine)) * (1.0 - t * 0.65);
    shardColor += float3(0.82, 0.95, 1.0) * edgeGlow * 1.25;
    
    // ひび割れの発光加算
    if (crackProgress > 0.0) {
        shardColor += float3(1.0, 1.0, 1.0) * activeCrack * 2.6;
    }
    
    // 破片のアルファ値
    float shardAlpha = 0.0;
    if (shatterProgress == 0.0) {
        // まだ割れていない (Cracking期) -> 画面全体を覆う氷の板
        shardAlpha = 0.96;
    } else {
        if (isInsideShard) {
            // 破片の中身: 後半からフェードアウト
            float fade = saturate(1.0 - (t - 0.25) / 0.75);
            shardAlpha = fade * 0.95;
        } else {
            // 破片の隙間: アルファ0 (奥の新シーンが露出！)
            shardAlpha = 0.0;
        }
    }
    
    // 破砕瞬間のホワイトフラッシュ (パリーン！の瞬間)
    float flashAlpha = 0.0;
    if (shatterProgress > 0.0 && shatterProgress < 0.14) {
        flashAlpha = (1.0 - shatterProgress / 0.14) * 0.45;
    }
    shardColor += float3(1.0, 1.0, 1.0) * flashAlpha;
    shardAlpha = max(shardAlpha, flashAlpha);
    
    output.color = float32_t4(shardColor, shardAlpha);
    return output;
}

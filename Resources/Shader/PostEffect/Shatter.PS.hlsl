#include "PostEffect.hlsli"

Texture2D<float32_t4> gTexture : register(t0);        // 氷・ガラス表面テクスチャ (ice_surface.png)
Texture2D<float32_t4> gCrackTexture : register(t1);    // 亀裂テクスチャ (ice_crack.png)
sampler gSampler : register(s0);

ConstantBuffer<PostEffectData> gPostEffect : register(b0);

// -------------------------------------------------------------
// 高品質ハッシュ関数
// -------------------------------------------------------------
float2 hash22(float2 p) {
    float3 p3 = frac(float3(p.xyx) * float3(443.897, 441.423, 437.195));
    p3 += dot(p3, p3.yzx + 19.19);
    return frac((p3.xx + p3.yz) * p3.zy);
}

float4 hash42(float2 p) {
    float4 p4 = frac(float4(p.xyxy) * float4(443.897, 441.423, 437.195, 449.873));
    p4 += dot(p4, p4.wzxy + 19.19);
    return frac((p4.xxyz + p4.yzzw) * p4.zywx);
}

// 2D回転行列の適用
float2 rotate(float2 p, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return float2(c * p.x - s * p.y, s * p.x + c * p.y);
}

// -------------------------------------------------------------
// グリッドセル c の初期破片中心 (種点)
// -------------------------------------------------------------
float2 GetCellSeed(float2 c, float2 gridSize) {
    float2 h = hash22(c);
    return (c + 0.5 + (h - 0.5) * 0.72) / gridSize;
}

// -------------------------------------------------------------
// メインピクセルシェーダー
// -------------------------------------------------------------
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float2 uv = input.texcoord;
    
    // パラメータ取得
    float shatterProgress = saturate(gPostEffect.ratio);  // 0.0: 未破砕 〜 1.0: 完全に四散
    float crackProgress   = saturate(gPostEffect.value1); // 0.0: 亀裂なし 〜 1.0: 亀裂全開
    float time            = gPostEffect.time;
    
    const float aspect = 16.0 / 9.0;
    const float2 center = float2(0.5, 0.5);
    const float2 gridSize = float2(16.0, 9.0); // 16:9比率に合わせた、より細かなガラス破片分割 (144個の破片)
    
    float2 toCenter = uv - center;
    float distToCenter = length(float2(toCenter.x * aspect, toCenter.y));

    // =============================================================
    // フェーズ A: まだ割れていない状態 (Cracking期: 激しく亀裂が走るガラス板)
    // =============================================================
    if (shatterProgress <= 0.0001) {
        // 1. 蜘蛛の巣状クラックテクスチャのサンプリング
        float4 crackTex = gCrackTexture.Sample(gSampler, uv);
        float crackVal = max(crackTex.r, crackTex.a);
        
        // 中心から外側へピキピキッと広がる先端フロンティア
        float frontierRadius = crackProgress * 1.5; // 中心から外周へ
        float crackReach = saturate((frontierRadius - distToCenter) / 0.14);
        float activeCrack = crackVal * crackReach;
        
        // 亀裂の先端が走る瞬間の鋭い閃光 (フロンティア発光)
        float frontierGlow = exp(-pow((distToCenter - frontierRadius) * 20.0, 2.0)) * saturate(crackProgress * 2.5);
        
        // 2. ボロノイ破片境界のクラック (これから割れる破片の幾何学ラインが浮き出る)
        float2 baseCell = floor(uv * gridSize);
        float minD1 = 100.0;
        float minD2 = 100.0;
        [unroll]
        for (int vy = -1; vy <= 1; ++vy) {
            [unroll]
            for (int vx = -1; vx <= 1; ++vx) {
                float2 c = baseCell + float2(float(vx), float(vy));
                float2 s = GetCellSeed(c, gridSize);
                float d = length((uv - s) * float2(aspect, 1.0));
                if (d < minD1) {
                    minD2 = minD1;
                    minD1 = d;
                } else if (d < minD2) {
                    minD2 = d;
                }
            }
        }
        float voronoiEdgeDist = minD2 - minD1; // 破片境界までの距離
        // 亀裂進行の後半から、ボロノイの割れ目ラインが鋭く白く走り出す
        float voronoiCrackLine = exp(-voronoiEdgeDist * 90.0);
        float voronoiCrackReach = saturate((crackProgress * 1.4 - distToCenter) / 0.18);
        float activeVoronoiCrack = voronoiCrackLine * voronoiCrackReach * saturate((crackProgress - 0.2) / 0.4);

        // 複合クラック強度
        float totalCrack = max(activeCrack, activeVoronoiCrack);
        
        // 3. 亀裂による屈折ズレ (ガラスにヒビが入ったことで奥の像がわずかにズレる)
        float2 crackRefrOffset = (totalCrack > 0.05) ? float2(0.005, -0.003) * totalCrack : float2(0.0, 0.0);
        float4 surfaceTex = gTexture.Sample(gSampler, (uv + crackRefrOffset) * 1.3);
        
        // 4. ガラス基本色
        float3 baseCol = gPostEffect.color * (0.88 + surfaceTex.r * 0.22);
        
        // 亀裂ラインの強烈な白光・シアン発光
        float3 crackGlowCol = float3(1.0, 1.0, 1.0) * totalCrack * 4.2;
        crackGlowCol += float3(0.65, 0.92, 1.0) * frontierGlow * crackVal * 4.0;
        baseCol += crackGlowCol;
        
        // 割れる直前の微細なフラッシュ（パリーン直前のタメ）
        if (crackProgress > 0.85) {
            float prepGlow = (crackProgress - 0.85) / 0.15;
            baseCol += float3(0.7, 0.9, 1.0) * prepGlow * 0.35;
        }
        
        // ガラス板の微細なスペキュラー
        float spec = pow(surfaceTex.g, 3.0) * 0.45;
        baseCol += float3(1.0, 1.0, 1.0) * spec;
        
        output.color = float32_t4(baseCol, 0.98);
        return output;
    }

    // =============================================================
    // フェーズ B: ガラス粉砕・崩落シミュレーション (Shattering期)
    // 外側へ爆発するのではなく、細かく割れた無数のガラス片が
    // ガラガラガラッと重力で崩れ落ちて散っていく (砕け散る表現)
    // =============================================================
    float t = shatterProgress;
    
    // 現在ピクセル uv に落ちてきた破片の初期位置を逆算推定
    // 主に上から下へ落下 (Y+) してくるため、元位置は上側 (Y-) にあった
    float avgFall = 2.8 * pow(t, 1.7);
    float2 normToCenter = (distToCenter > 0.001) ? (toCenter / distToCenter) : float2(0.0, -1.0);
    float2 estDisp = float2(normToCenter.x * 0.18 * t, avgFall);
    float2 estSrcUV = uv - estDisp;
    
    float2 baseCell = floor(estSrcUV * gridSize);
    
    // 最近傍・最前面の破片を探索
    bool foundShard = false;
    float bestDepth = 1000.0;
    float bestEdgeDist = 0.0;
    float2 bestSrcPos = uv;
    float bestRotAngle = 0.0;
    float2 bestTilt = float2(0.0, 0.0);
    float bestGlint = 0.0;
    float bestMass = 1.0;
    float2 bestCellId = float2(0.0, 0.0);
    float bestLocalT = 0.0;
    
    // 上から降ってくる破片を中心に周囲セルを探索
    [unroll]
    for (int dy = -2; dy <= 2; ++dy) {
        [unroll]
        for (int dx = -2; dx <= 2; ++dx) {
            float2 cell = baseCell + float2(float(dx), float(dy));
            float2 seed = GetCellSeed(cell, gridSize);
            
            // 各破片の固有物理パラメータ
            float4 r1 = hash42(cell);
            float4 r2 = hash42(cell + float2(37.2, 83.1));
            
            float mass = 0.6 + r1.x * 0.7; // 破片の質量
            
            // 中心からの距離
            float2 shardCenterOffset = seed - center;
            float shardDistCenter = length(float2(shardCenterOffset.x * aspect, shardCenterOffset.y));
            float2 radDir = (shardDistCenter > 0.001) ? normalize(shardCenterOffset) : float2(0.0, -1.0);
            
            // 崩落の時間差 (中心から外周へ向かって連鎖的にガラガラッと崩落が伝播)
            float crumbleDelay = saturate(shardDistCenter * 0.22 + (r1.w - 0.5) * 0.10);
            float localT = saturate((t - crumbleDelay) / max(1.0 - crumbleDelay, 0.001));
            
            // 割れ始めのカタカタとした微小ジッター (爆発ではなくわずかな押し出し)
            float horizShift = (r1.y - 0.5) * 0.28 * localT + radDir.x * 0.16 * pow(localT, 0.6);
            
            // 重力による急速な崩壊・落下加速度 (ガラガラと下へ崩れ落ちる)
            float fallDist = (2.6 + r1.z * 1.2) * pow(localT, 1.7) / mass;
            
            float2 disp;
            disp.x = horizShift;
            disp.y = fallDist + radDir.y * 0.08 * localT;
            
            // 3Dタンブリング回転 (前後にヒラヒラ・カタカタと傾きながら崩落)
            float rotAngle = (r2.x - 0.5) * 14.0 * localT;
            float2 tilt = float2((r2.y - 0.5) * 20.0 * localT, (r2.z - 0.5) * 18.0 * localT);
            
            // 現在の破片中心
            float2 curCenter = seed + disp;
            
            // uvから破片中心へのベクトル
            float2 dPos = uv - curCenter;
            
            // 破片の3Dチルトによる見かけのスケール変化
            float cosTiltX = max(abs(cos(tilt.x)), 0.35);
            float cosTiltY = max(abs(cos(tilt.y)), 0.35);
            
            // 逆回転・逆スケールで元の破片空間座標を求める
            float2 unrotPos = rotate(dPos, -rotAngle);
            unrotPos.x /= cosTiltX;
            unrotPos.y /= cosTiltY;
            
            float2 srcPos = seed + unrotPos;
            
            // -------------------------------------------------------------
            // ボロノイ境界判定: srcPosがセルcellの領域内にあるか？
            // -------------------------------------------------------------
            float dCurrent = length((srcPos - seed) * float2(aspect, 1.0));
            
            // 周囲の隣接セルの種点との距離を比較
            float minNeighborDist = 100.0;
            [unroll]
            for (int ny = -1; ny <= 1; ++ny) {
                [unroll]
                for (int nx = -1; nx <= 1; ++nx) {
                    if (nx == 0 && ny == 0) continue;
                    float2 nCell = cell + float2(float(nx), float(ny));
                    float2 nSeed = GetCellSeed(nCell, gridSize);
                    float nd = length((srcPos - nSeed) * float2(aspect, 1.0));
                    minNeighborDist = min(minNeighborDist, nd);
                }
            }
            
            // ボロノイエッジまでの距離 (破片の境界線までの距離)
            float edgeDist = minNeighborDist - dCurrent;
            
            // edgeDist > 0 なら、このピクセルは破片の内部！
            if (edgeDist > 0.0) {
                // 破片の擬似深度 (手前にある破片を優先描画)
                float depth = mass + r2.w * 0.4 + (r1.w - 0.5) * localT;
                if (depth < bestDepth) {
                    bestDepth = depth;
                    foundShard = true;
                    bestEdgeDist = edgeDist;
                    bestSrcPos = srcPos;
                    bestRotAngle = rotAngle;
                    bestTilt = tilt;
                    bestMass = mass;
                    bestCellId = cell;
                    bestLocalT = localT;
                    
                    // 3D法線とスペキュラー閃光 (崩落に伴うキラキラした光の反射)
                    float3 N = normalize(float3(sin(tilt.x), sin(tilt.y), cos(tilt.x) * cos(tilt.y)));
                    float3 lightDir = normalize(float3(0.35, -0.65, 0.68));
                    float3 viewDir = float3(0.0, 0.0, 1.0);
                    float3 halfVec = normalize(lightDir + viewDir);
                    float spec = pow(saturate(dot(N, halfVec)), 24.0);
                    bestGlint = spec * 2.2;
                }
            }
        }
    }

    // =============================================================
    // 破片のない隙間: パリーンと割れた空間から奥の新シーンが完全露出！
    // =============================================================
    if (!foundShard) {
        // 割れた瞬間のホワイトフラッシュのみわずかに残す
        float flash = 0.0;
        if (t < 0.12) {
            flash = (1.0 - t / 0.12) * 0.4;
        }
        output.color = float32_t4(1.0, 1.0, 1.0, flash);
        return output;
    }

    // =============================================================
    // ガラス破片の光学マテリアルレンダリング
    // =============================================================
    // 1. プリズム色収差 (ガラス断面屈折によるRGB微小ズレ)
    float2 refrDir = (bestEdgeDist < 0.02) ? normalize(bestSrcPos - GetCellSeed(bestCellId, gridSize) + 0.0001) : float2(0.0, 0.0);
    float2 refrOffset = refrDir * 0.006 * (1.0 - saturate(bestEdgeDist * 50.0));
    
    float texR = gTexture.Sample(gSampler, bestSrcPos * 1.3 + refrOffset).r;
    float texG = gTexture.Sample(gSampler, bestSrcPos * 1.3).r;
    float texB = gTexture.Sample(gSampler, bestSrcPos * 1.3 - refrOffset).r;
    float3 glassTex = float3(texR, texG, texB);
    
    // 2. ガラス破片のベース色 (透明感あるクリスタルクリア)
    float3 shardColor = gPostEffect.color * (0.85 + glassTex * 0.3);
    
    // 3. カミソリのように鋭利なガラス断面エッジハイライト (Razor Edge Highlight)
    // 境界線付近で急激に純白〜シアンに強く発光
    float edgeFactor = exp(-bestEdgeDist * 75.0);
    float3 edgeHighlight = float3(0.92, 0.97, 1.0) * edgeFactor * 3.5;
    shardColor += edgeHighlight;
    
    // 4. 3D回転によるスペキュラー閃光 (キラーン！と光る反射)
    shardColor += float3(1.0, 1.0, 1.0) * bestGlint;
    
    // 5. 割れた瞬間の爆発ホワイトフラッシュ (衝撃波閃光)
    float flash = 0.0;
    if (t < 0.14) {
        flash = (1.0 - t / 0.14) * 0.45;
    }
    shardColor += float3(1.0, 1.0, 1.0) * flash;
    
    // 6. アルファ値の決定 (落ちていくとき徐々に透明にする)
    // 割れた瞬間はくっきりと存在し、下へ落ちていく最中にスーッと滑らかに透明化
    float shardFallFade = saturate(1.0 - pow(bestLocalT, 1.25) * 1.15);
    float globalFallFade = saturate(1.0 - pow(t, 1.35) * 1.05);
    float fallFade = min(shardFallFade, globalFallFade);
    
    // ベースアルファに落下フェードを乗算
    float baseAlpha = lerp(0.92, 1.0, edgeFactor);
    float shardAlpha = baseAlpha * fallFade;
    
    shardAlpha = max(shardAlpha, flash);
    
    output.color = float32_t4(shardColor, shardAlpha);
    return output;
}

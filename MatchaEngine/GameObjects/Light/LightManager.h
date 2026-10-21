#pragma once
#include <vector>
#include <memory>
#include <wrl.h>
#include <d3dx12.h>
#include "Core/VariableTypes.h"
#include <numbers>

class GameObject;

class LightManager
{
private:
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLightGroupData* directionalLightData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    PointLightGroupData* pointLightData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
    SpotLightGroupData* spotLightData_ = nullptr;

public:
    LightManager();

    /// <summary>
    /// ライト管理用の定数バッファリソース等を初期化します。
    /// </summary>
    void Initialize();

    /// <summary>
    /// ライト調整用のImGuiデバッグUIを描画します。
    /// </summary>
    void ImGui();

    /// <summary>
    /// CPU側のライトパラメータをGPU定数バッファへ転送します。
    /// </summary>
    void TransferData();

    /// <summary>
    /// シーン内のGameObject群からライトコンポーネントを収集して更新します。
    /// </summary>
    /// <param name="objects">シーン内のGameObject配列</param>
    void UpdateLights(const std::vector<std::shared_ptr<GameObject>>& objects);

    /// <summary>
    /// フレームごとのライト収集を開始（カウンタをリセット）します。
    /// </summary>
    void BeginUpdate();

    /// <summary>
    /// フレームごとのライト収集を完了し、バッファへ反映します。
    /// </summary>
    void EndUpdate();

    /// <summary>
    /// 平行光源（Directional Light）を追加します。
    /// </summary>
    /// <param name="color">ライトカラー・輝度</param>
    /// <param name="direction">光の照射方向</param>
    /// <param name="intensity">光の強度</param>
    void AddDirectionalLight(const Vector4& color, const Vector3& direction, float intensity);

    /// <summary>
    /// 点光源（Point Light）を追加します。
    /// </summary>
    /// <param name="color">ライトカラー</param>
    /// <param name="position">光源のワールド座標</param>
    /// <param name="intensity">光の強度</param>
    /// <param name="radius">影響半径</param>
    /// <param name="decay">減衰率</param>
    void AddPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay);

    /// <summary>
    /// スポットライト（Spot Light）を追加します。
    /// </summary>
    /// <param name="color">ライトカラー</param>
    /// <param name="position">光源のワールド座標</param>
    /// <param name="direction">照射方向ベクトル</param>
    /// <param name="intensity">光の強度</param>
    /// <param name="distance">最大照射距離</param>
    /// <param name="decay">距離減衰率</param>
    /// <param name="cosAngle">照射コーン外側の余弦値</param>
    /// <param name="cosFalloffStart">フォールオフ開始角度の余弦値</param>
    void AddSpotLight(const Vector4& color, const Vector3& position, const Vector3& direction, float intensity, float distance, float decay, float cosAngle, float cosFalloffStart);

    /// <summary>
    /// ディレクショナルライトの定数バッファリソースを取得します。
    /// </summary>
    ID3D12Resource* GetDirectionalLightResource() { return directionalLightResource_.Get(); }

    /// <summary>
    /// ポイントライトの定数バッファリソースを取得します。
    /// </summary>
    ID3D12Resource* GetPointLightResource() { return pointLightResource_.Get(); }

    /// <summary>
    /// スポットライトの定数バッファリソースを取得します。
    /// </summary>
    ID3D12Resource* GetSpotLightResource() { return spotLightResource_.Get(); }

private:
    int dirCount_ = 0;
    int pointCount_ = 0;
    int spotCount_ = 0;
};

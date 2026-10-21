#pragma once
#include <vector>
#include <memory>
#include "../GameObjects/Object/GameObject.h"

class GameObjectManager
{
private:
    std::vector<std::shared_ptr<GameObject>> objects_;

public:
    /// <summary>
    /// 管理対象にゲームオブジェクトを追加します。
    /// </summary>
    /// <param name="obj">追加するGameObjectの共有ポインタ</param>
    void AddObject(std::shared_ptr<GameObject> obj);

    /// <summary>
    /// 管理対象から指定のゲームオブジェクトを削除します。
    /// </summary>
    /// <param name="obj">削除するGameObjectの共有ポインタ</param>
    void RemoveObject(std::shared_ptr<GameObject> obj);

    /// <summary>
    /// 指定オブジェクトの複製（Clone）を生成して管理対象に追加します。
    /// </summary>
    /// <param name="obj">複製元のGameObject</param>
    void CopyObject(std::shared_ptr<GameObject> obj);

    /// <summary>
    /// 管理している全オブジェクトを破棄・クリアします。
    /// </summary>
    void Clear();

    /// <summary>
    /// 管理下の全オブジェクトを更新します。
    /// </summary>
    /// <param name="view">カメラのビュー行列</param>
    /// <param name="speedMultiplier">更新速度倍率（タイムスケール）</param>
    void UpdateAll(Matrix4x4 view, float speedMultiplier = 1.0f);

    /// <summary>
    /// 管理下の全オブジェクトを描画します。
    /// </summary>
    /// <param name="draw">描画コマンド発行用のDrawインスタンス</param>
    void DrawAll(class Draw& draw);

    /// <summary>
    /// 管理下の全オブジェクトのImGuiデバッグUIを描画します。
    /// </summary>
    void ImGuiAll();

    /// <summary>
    /// シーン内の全オブジェクト情報をファイルへ保存します。
    /// </summary>
    /// <param name="filepath">保存先ファイルパス</param>
    void SaveScene(const std::string& filepath);

    /// <summary>
    /// ファイルからシーン内のオブジェクト情報を読み込み再構築します。
    /// </summary>
    /// <param name="filepath">読み込み元ファイルパス</param>
    void LoadScene(const std::string& filepath);

    /// <summary>
    /// 現在管理している全オブジェクトのリストを取得します。
    /// </summary>
    /// <returns>オブジェクトのshared_ptr配列への参照</returns>
    const std::vector<std::shared_ptr<GameObject>>& GetObjects() const { return objects_; }
};

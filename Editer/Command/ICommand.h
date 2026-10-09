#pragma once
#include <string>

/// <summary>
/// Command パターン: コマンドの抽象基底クラス
/// エディタの各操作（Transform変更、オブジェクト作成/削除等）をカプセル化し、
/// Undo（元に戻す） / Redo（やり直す）を可能にします。
/// </summary>
class ICommand {
public:
    virtual ~ICommand() = default;

    /// <summary>
    /// コマンドの実行（初回またはRedo時）
    /// </summary>
    virtual void Execute() = 0;

    /// <summary>
    /// コマンドの取り消し（元に戻す）
    /// </summary>
    virtual void Undo() = 0;

    /// <summary>
    /// やり直し（デフォルトでは再度Executeを呼び出す）
    /// </summary>
    virtual void Redo() {
        Execute();
    }

    /// <summary>
    /// コマンド名（デバッグやUIログ用）
    /// </summary>
    virtual std::string GetName() const = 0;
};

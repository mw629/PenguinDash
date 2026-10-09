#pragma once
#include "ICommand.h"
#include <vector>
#include <memory>
#include <string>

/// <summary>
/// Command パターン: コマンドの履歴管理と Undo/Redo を担うマネージャー
/// </summary>
class CommandManager {
public:
    static CommandManager& GetInstance() {
        static CommandManager instance;
        return instance;
    }

    /// <summary>
    /// 新規コマンドを実行し、Undo スタックにプッシュします。
    /// （Redo スタックはクリアされます）
    /// </summary>
    void ExecuteCommand(std::unique_ptr<ICommand> command);

    /// <summary>
    /// 既に実行された操作（ギズモドラッグ完了時など）を履歴に登録します。
    /// </summary>
    void PushExecutedCommand(std::unique_ptr<ICommand> command);

    /// <summary>
    /// 直前のコマンドを取り消す（元に戻す）
    /// </summary>
    /// <returns>取り消しに成功した場合 true</returns>
    bool Undo();

    /// <summary>
    /// 取り消したコマンドをやり直す
    /// </summary>
    /// <returns>やり直しに成功した場合 true</returns>
    bool Redo();

    bool CanUndo() const { return !undoStack_.empty(); }
    bool CanRedo() const { return !redoStack_.empty(); }

    /// <summary>
    /// 直近の Undo 可能なコマンド名を取得
    /// </summary>
    std::string GetUndoCommandName() const;

    /// <summary>
    /// 直近の Redo 可能なコマンド名を取得
    /// </summary>
    std::string GetRedoCommandName() const;

    /// <summary>
    /// 全ての履歴を消去（シーンのロード時など）
    /// </summary>
    void Clear();

    /// <summary>
    /// 最大履歴数の設定
    /// </summary>
    void SetMaxHistory(size_t maxHistory) { maxHistory_ = maxHistory; }

    size_t GetUndoCount() const { return undoStack_.size(); }
    size_t GetRedoCount() const { return redoStack_.size(); }

private:
    CommandManager() = default;
    ~CommandManager() = default;
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;

    std::vector<std::unique_ptr<ICommand>> undoStack_;
    std::vector<std::unique_ptr<ICommand>> redoStack_;

    static constexpr size_t kDefaultMaxHistory = 50; // メモリ肥大化を防ぐ最大履歴数
    size_t maxHistory_ = kDefaultMaxHistory;
};

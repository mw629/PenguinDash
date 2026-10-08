#include "CommandManager.h"
#include <iostream>

void CommandManager::ExecuteCommand(std::unique_ptr<ICommand> command) {
    if (!command) return;

    command->Execute();
    PushExecutedCommand(std::move(command));
}

void CommandManager::PushExecutedCommand(std::unique_ptr<ICommand> command) {
    if (!command) return;

    // 新規操作が入ったのでRedo履歴は無効化
    redoStack_.clear();

    undoStack_.push_back(std::move(command));

    // 最大履歴数を超えたら最も古い履歴を削除
    if (undoStack_.size() > maxHistory_) {
        undoStack_.erase(undoStack_.begin());
    }
}

bool CommandManager::Undo() {
    if (undoStack_.empty()) return false;

    // 直前のコマンドを取得
    auto cmd = std::move(undoStack_.back());
    undoStack_.pop_back();

    // 元に戻す処理を実行
    cmd->Undo();

    // Redo スタックへ退避
    redoStack_.push_back(std::move(cmd));
    return true;
}

bool CommandManager::Redo() {
    if (redoStack_.empty()) return false;

    // Redo コマンドを取得
    auto cmd = std::move(redoStack_.back());
    redoStack_.pop_back();

    // やり直しを実行
    cmd->Redo();

    // Undo スタックへ戻す
    undoStack_.push_back(std::move(cmd));
    return true;
}

std::string CommandManager::GetUndoCommandName() const {
    if (undoStack_.empty()) return "";
    return undoStack_.back()->GetName();
}

std::string CommandManager::GetRedoCommandName() const {
    if (redoStack_.empty()) return "";
    return redoStack_.back()->GetName();
}

void CommandManager::Clear() {
    undoStack_.clear();
    redoStack_.clear();
}

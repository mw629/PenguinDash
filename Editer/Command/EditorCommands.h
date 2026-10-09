#pragma once
#include "ICommand.h"
#include "../System/GameObjectManager.h"
#include "../../MatchaEngine/GameObjects/Object/GameObject.h"
#include <memory>
#include <string>

/// <summary>
/// Transform（位置・回転・スケール）の変更コマンド
/// </summary>
class TransformCommand : public ICommand {
public:
    TransformCommand(std::shared_ptr<GameObject> target, const Transform& oldT, const Transform& newT)
        : target_(target), oldTransform_(oldT), newTransform_(newT) {}

    void Execute() override {
        if (auto obj = target_.lock()) {
            obj->SetTransform(newTransform_);
        }
    }

    void Undo() override {
        if (auto obj = target_.lock()) {
            obj->SetTransform(oldTransform_);
        }
    }

    std::string GetName() const override {
        if (auto obj = target_.lock()) {
            return "Transform: " + obj->GetName();
        }
        return "Transform";
    }

private:
    std::weak_ptr<GameObject> target_;
    Transform oldTransform_;
    Transform newTransform_;
};

/// <summary>
/// GameObject の生成コマンド
/// </summary>
class CreateObjectCommand : public ICommand {
public:
    CreateObjectCommand(GameObjectManager* mgr, std::shared_ptr<GameObject> obj)
        : mgr_(mgr), object_(obj) {}

    void Execute() override {
        if (mgr_ && object_) {
            mgr_->AddObject(object_);
        }
    }

    void Undo() override {
        if (mgr_ && object_) {
            mgr_->RemoveObject(object_);
        }
    }

    std::string GetName() const override {
        return object_ ? ("Create: " + object_->GetName()) : "Create Object";
    }

private:
    GameObjectManager* mgr_;
    std::shared_ptr<GameObject> object_;
};

/// <summary>
/// GameObject の削除コマンド
/// </summary>
class DeleteObjectCommand : public ICommand {
public:
    DeleteObjectCommand(GameObjectManager* mgr, std::shared_ptr<GameObject> obj)
        : mgr_(mgr), object_(obj) {}

    void Execute() override {
        if (mgr_ && object_) {
            mgr_->RemoveObject(object_);
        }
    }

    void Undo() override {
        if (mgr_ && object_) {
            mgr_->AddObject(object_);
        }
    }

    std::string GetName() const override {
        return object_ ? ("Delete: " + object_->GetName()) : "Delete Object";
    }

private:
    GameObjectManager* mgr_;
    std::shared_ptr<GameObject> object_;
};

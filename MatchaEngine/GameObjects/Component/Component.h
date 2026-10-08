#pragma once
#include <string>
#include "../../Core/VariableTypes.h"

class GameObject;
class Draw;

class Component
{
protected:
    GameObject* gameObject_ = nullptr;

public:
    Component() = default;
    virtual ~Component() = default;

    virtual void Initialize() {}
    virtual void Update(Matrix4x4 /*view*/, float /*speedMultiplier*/ = 1.0f) {}
    virtual void Draw(Draw& /*draw*/) {}
    virtual void ImGui() {}

    void SetGameObject(GameObject* gameObject) { gameObject_ = gameObject; }
    GameObject* GetGameObject() const { return gameObject_; }
};

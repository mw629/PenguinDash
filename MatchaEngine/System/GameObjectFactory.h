#pragma once
#include <string>
#include <memory>
#include "../GameObjects/Object/GameObject.h"
#include "../GameObjects/Object/RenderObject.h"
#include "../GameObjects/Light/DirectionalLight.h"
#include "../GameObjects/Light/PointLight.h"
#include "../GameObjects/Light/SpotLight.h"

class GameObjectFactory {
public:
    static std::shared_ptr<GameObject> Create(const std::string& type) {
        if (type == "RenderObject") {
            return std::make_shared<RenderObject>();
        } else if (type == "DirectionalLight") {
            return std::make_shared<DirectionalLight>();
        } else if (type == "PointLight") {
            return std::make_shared<PointLight>();
        } else if (type == "SpotLight") {
            return std::make_shared<SpotLight>();
        } else {
            return std::make_shared<GameObject>();
        }
    }
};

#pragma once
#include <Engine.h>
#include "Camera.h"
#include "../IScene.h"
#include "Emitter.h"
#include <memory>
#include "../../MatchaEngine/System/GameObjectManager.h"
#include "../../Editer/EditorUI.h"
#include "../../MatchaEngine/GameObjects/Object/RenderObject.h"

class TestScene :public IScene
{
private:
	std::unique_ptr<GameObjectManager> gameObjectManager_ = std::make_unique<GameObjectManager>();
	std::unique_ptr<EditorUI> editorUI_ = std::make_unique<EditorUI>();

	std::unique_ptr<Texture> texture_ = std::make_unique<Texture>();

	std::unique_ptr<Camera>camera_ = std::make_unique<Camera>();
	Transform cameraTransform_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f,},{0.0f,2.0f,-5.0f} };

    std::vector<std::unique_ptr<Emitter>> particle_;

	std::unique_ptr<Sprite> sprite_ = std::make_unique<Sprite>();
	SpriteData spriteData_{
		{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {100.0f, 100.0f, 0.0f} }, // transform
		{ 300, 300 }, // size
		{ {0.0f, 0.0f}, {0.1f, 0.1f} } // texxtureArea
	};

	std::shared_ptr<Model> model_ = std::make_shared<Model>();
	Transform modelTransform_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f,},{0.0f,2.0f,1.5f} };

	std::shared_ptr<Sphere> sphere_ = std::make_shared<Sphere>();
	Transform Transform_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f,},{0.0f,2.0f,1.5f} };

	std::shared_ptr<Ring> ring_ = std::make_shared<Ring>();
	Transform ringTransform_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f,},{0.0f,2.0f,1.5f} };

	std::shared_ptr<Cylinder> cylinder_ = std::make_shared<Cylinder>();
	Transform cylinderTransform_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f,},{2.0f,2.0f,1.5f} };

	std::shared_ptr<Model> axe_ = std::make_shared<Model>();
	Transform axeOffset_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f,},{0.0f,0.0f,0.0f} };

	std::shared_ptr<Cube> skyBox_ = std::make_shared<Cube>();
	int skyBoxTexture_;
	Transform skyBoxTransform_{ {500.0f,500.0f,500.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	std::shared_ptr<Model> floor = std::make_shared<Model>();
	Transform floorT{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f,},{0.0f,0.0f,0.0f} };

	std::shared_ptr<CharacterAnimator> animation_ = std::make_shared<CharacterAnimator>();
	std::shared_ptr<TransformAnimation> nodeAnimation_ = std::make_shared<TransformAnimation>();

	bool bill = true;
	bool isSneaking_ = true;

public:
	~TestScene()override;

	void ImGui()override;

	void Initialize()override;

	void Update()override;

	void Draw(class Draw& draw)override;

};


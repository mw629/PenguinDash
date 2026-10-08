#include "TestScene.h"
#include "Graphics/Render/Draw.h"
#include "../../Editer/EditorManager.h"
#include <imgui.h>
#include <memory>
#include <cmath>

TestScene::~TestScene()
{
}

void TestScene::ImGui()
{
#ifdef _USE_IMGUI
	ImGui::Begin("TestScene");

	camera_.get()->ImGui();
	
	// スライダーで座標を操作（表示・編集ともに小数第1桁）
	// 範囲は整数部4桁を許容する -9999.9 〜 9999.9
	if (ImGui::CollapsingHeader("Sprite Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Spacing();
		ImGui::Text("SpritePos");
		ImGui::SliderFloat2("##sprite_pos_slider", &spriteData_.transform.translate.x, -9999.9f, 9999.9f, "%.1f");
		ImGui::Text("Pos: %4.1f, %4.1f", spriteData_.transform.translate.x, spriteData_.transform.translate.y);

		ImGui::Spacing();
		ImGui::SliderFloat2("SpriteSize", &spriteData_.size.x, 1.0f, 2000.0f, "%.1f");
		ImGui::SliderFloat2("Pivot", &spriteData_.pivot.x, 0.0f, 1.0f, "%.2f");

		const char* scaleModeNames[] = { "Fit", "Fill", "Stretch", "None" };
		int currentScaleMode = static_cast<int>(spriteData_.scaleMode);
		if (ImGui::Combo("ScaleMode", &currentScaleMode, scaleModeNames, IM_ARRAYSIZE(scaleModeNames))) {
			spriteData_.scaleMode = static_cast<SpriteScaleMode>(currentScaleMode);
		}

		const char* anchorNames[] = {
			"None (Virtual Res)", "TopLeft", "TopCenter", "TopRight",
			"MiddleLeft", "Center", "MiddleRight",
			"BottomLeft", "BottomCenter", "BottomRight"
		};
		int currentAnchor = static_cast<int>(spriteData_.anchor);
		if (ImGui::Combo("Anchor", &currentAnchor, anchorNames, IM_ARRAYSIZE(anchorNames))) {
			spriteData_.anchor = static_cast<SpriteAnchor>(currentAnchor);
		}

		Vector2 currentScreen = Sprite::GetScreenSize();
		Vector2 refRes = Sprite::GetReferenceResolution();
		ImGui::Text("Screen: %.0f x %.0f  |  Ref: %.0f x %.0f", currentScreen.x, currentScreen.y, refRes.x, refRes.y);
	}

	for (int i = 0, n = static_cast<int>(particle_.size()); i < n; ++i) {
		particle_[i].get()->ImGui();
	}

	if (ImGui::CollapsingHeader("Animation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		bool isVisibleBones = animation_.get()->GetVisibleBones();
		if (ImGui::Checkbox("Show Bones", &isVisibleBones)) {
			animation_.get()->SetVisibleBones(isVisibleBones);
		}
	}

	if (ImGui::CollapsingHeader("Axe Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat3("Offset Pos", &axeOffset_.translate.x, 0.01f);
		ImGui::DragFloat3("Offset Rot", &axeOffset_.rotate.x, 0.01f);
		ImGui::DragFloat3("Offset Scale", &axeOffset_.scale.x, 0.01f);
	}

	ImGui::End();

	if (editorUI_ && gameObjectManager_) {
		editorUI_->Draw(gameObjectManager_.get(), camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
	}

	EditorManager::SetSceneOverlayCallback([this]() {
		if (editorUI_) {
			editorUI_->DrawGizmoInScene(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
		}
	});

#endif // _USE_IMGUI
}

void TestScene::Initialize() {

	sceneID_ = SceneID::Test;

	camera_.get()->SetTransform(cameraTransform_);
	camera_.get()->Update();

	ModelData modelData = AssimpLoadObjFile("Resources/Model/obj", "sphere.obj");

	//Modelの初期化
	model_.get()->Initialize(modelData);
	model_.get()->SetTransform(modelTransform_);
	model_.get()->name_ = "Sphere Model";


	//Animationの初期化
	ModelData animModel = AssimpLoadObjFile("Resources/gltf/human", "sneakWalk.gltf");
	animation_.get()->Initialize(animModel, "Resources/gltf/human", "sneakWalk.gltf");
	animation_->LoadAdditionalAnimation("Resources/gltf/human", "sneakWalk.gltf", "sneakWalk");
	animation_->LoadAdditionalAnimation("Resources/gltf/human", "walk.gltf", "walk");
	animation_->SetAnimation("sneakWalk");
	animation_.get()->name_ = "Animation Model";
	animation_.get()->SetVisibleBones(true); // ボーンを表示

	// 手のボーンを登録
	animation_->SetBoneMapping(BoneType::RightHand, "mixamorig:RightHand");
	animation_->SetBoneMapping(BoneType::LeftHand, "mixamorig:LeftHand");

	// Axeの初期化
	ModelData axeData = AssimpLoadObjFile("Resources/Model/Axe", "Axe.obj");
	axe_.get()->Initialize(axeData);
	axe_.get()->name_ = "Axe Model";
	axeOffset_.scale = { 100.0f, 100.0f, 100.0f }; // キャラクター(gltf)のスケールが0.01の場合があるので、武器は100倍にして表示サイズを合わせる

	//NoodeAnimationの初期化
	ModelData cubeModel = AssimpLoadObjFile("Resources/AnimatedCube", "AnimatedCube.gltf");
	nodeAnimation_.get()->Initialize(cubeModel, "Resources/AnimatedCube", "AnimatedCube.gltf");
	nodeAnimation_.get()->name_ = "Node Animation Cube";

	
	//Sphereの初期化
	int texture1 = texture_.get()->CreateTexture("Resources/Texture/monsterBall.png");
	sphere_.get()->Initialize(texture1);
	sphere_.get()->SetTransform(modelTransform_);
	sphere_.get()->name_ = "MonsterBall Sphere";

	//SkyBoxの初期化
	skyBoxTexture_ = texture_.get()->CreateTexture("Resources/DDS/rostock_laage_airport_4k.dds");
	skyBox_.get()->Initialize(skyBoxTexture_);
	skyBox_.get()->SetShader("SkyBoxShader");
	skyBox_.get()->SetCullMode(kCullModeFront);
	skyBox_.get()->SetFrustumCullingEnabled(false);
	skyBox_.get()->SetLighting(false);
	skyBox_.get()->SetTransform(skyBoxTransform_);
	skyBox_.get()->name_ = "SkyBox";

	modelData = AssimpLoadObjFile("Resources/Model/Ground", "Ground.obj");
	floor.get()->Initialize(modelData);
	floor.get()->SetTransform(floorT);
	floor.get()->name_ = "Floor";

    EmitterData emitter;
	emitter.transform = modelTransform_;
	emitter.transform.scale = { 0.1f,0.1f,0.1f };
	emitter.count = 5;
	EffectDefinitionData data;
	data.transform.scale = { 0.05f,1.0f,1.0f };
	data.color = { 1.0f,1.0f,1.0f,1.0f };
	data.lifeTime = 1.0f;
	// Plane particle (White) - Snow/Sparkles
	std::unique_ptr<Emitter> particlePlane = std::make_unique<Emitter>();
	EmitterData emitterPlane = emitter;
	emitterPlane.transform.translate.x -= 2.0f;
	emitterPlane.count = 2; // Spawn fewer per frame but steadily
	emitterPlane.frequency = 0.1f;
	EffectDefinitionData dataPlane = data;
	dataPlane.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // White
	dataPlane.lifeTime = 2.0f;
	dataPlane.transform.scale = { 0.1f, 0.1f, 0.1f };
	particlePlane.get()->Initialize(emitterPlane, dataPlane, EffectShape::Plane);
	particlePlane.get()->name_ = "Particle (Snow/Sparkles)";
	particlePlane->generatorBehavior = [](EffectDefinitionData& p) {
		float randX = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
		float randY = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
		float randZ = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
		p.transform.translate.x += randX;
		p.transform.translate.y += randY;
		p.transform.translate.z += randZ;
		p.velocity = { randX * 0.01f, -0.02f, randZ * 0.01f }; // Falling down and drifting
		p.transform.rotate.z = ((float)rand() / RAND_MAX) * 3.14159f;
	};
	particle_.push_back(std::move(particlePlane));

	// Cylinder particle (Red) - Upward Fire Pillar / Laser
	std::unique_ptr<Emitter> particleCylinder = std::make_unique<Emitter>();
	EmitterData emitterCylinder = emitter;
	emitterCylinder.count = 3;
	emitterCylinder.frequency = 0.05f;
	EffectDefinitionData dataCylinder = data;
	dataCylinder.color = { 1.0f, 0.0f, 0.0f, 1.0f }; // Red
	dataCylinder.lifeTime = 0.8f;
	dataCylinder.transform.scale = { 0.15f, 1.5f, 0.15f }; // Long vertical shape
	particleCylinder.get()->Initialize(emitterCylinder, dataCylinder, EffectShape::Cylinder);
	particleCylinder.get()->name_ = "Particle (Upward Fire)";
	particleCylinder->generatorBehavior = [](EffectDefinitionData& p) {
		float randX = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
		float randZ = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
		p.transform.translate.x += randX;
		p.transform.translate.z += randZ;
		p.velocity = { 0.0f, 0.08f + ((float)rand() / RAND_MAX) * 0.05f, 0.0f }; // Shoot straight up fast
	};
	particle_.push_back(std::move(particleCylinder));

	// Ring particle (Blue) - Expanding Shockwave
	std::unique_ptr<Emitter> particleRing = std::make_unique<Emitter>();
	EmitterData emitterRing = emitter;
	emitterRing.transform.translate.x += 2.0f;
	emitterRing.count = 1;
	emitterRing.frequency = 0.5f; // Infrequent spawn
	EffectDefinitionData dataRing = data;
	dataRing.color = { 0.0f, 0.5f, 1.0f, 1.0f }; // Cyan-ish Blue
	dataRing.lifeTime = 1.0f;
	dataRing.transform.scale = { 0.1f, 0.1f, 0.1f }; // Start small
	particleRing.get()->Initialize(emitterRing, dataRing, EffectShape::Ring);
	particleRing.get()->name_ = "Particle (Shockwave)";
	particleRing->generatorBehavior = [](EffectDefinitionData& p) {
		p.velocity = { 0.0f, 0.0f, 0.0f }; // Stay in place
	};
	particle_.push_back(std::move(particleRing));

	int texture = texture_.get()->CreateTexture("Resources/Texture/uvChecker.png");
	sprite_.get()->Initialize(spriteData_, texture);

	ring_.get()->Initialize(texture);
	cylinder_.get()->Initialize(texture);
	cylinder_.get()->SetTransform(cylinderTransform_);
    
	if (gameObjectManager_) {
		gameObjectManager_->AddObject(std::make_shared<RenderObject>(model_));
		gameObjectManager_->AddObject(std::make_shared<RenderObject>(animation_));
		gameObjectManager_->AddObject(std::make_shared<RenderObject>(nodeAnimation_));
		gameObjectManager_->AddObject(std::make_shared<RenderObject>(sphere_));
		gameObjectManager_->AddObject(std::make_shared<RenderObject>(skyBox_));
		gameObjectManager_->AddObject(std::make_shared<RenderObject>(floor));
		gameObjectManager_->AddObject(std::make_shared<RenderObject>(ring_));
		gameObjectManager_->AddObject(std::make_shared<RenderObject>(cylinder_));
		gameObjectManager_->AddObject(std::make_shared<RenderObject>(axe_));
	}
}

void TestScene::Update() {

	camera_.get()->Update();
	Matrix4x4 view = camera_.get()->GetViewMatrix();

	sprite_.get()->Update(spriteData_);
	ring_.get()->SettingWvp(view);
	cylinder_.get()->SettingWvp(view);

	model_->SettingWvp(view);
	floor->SettingWvp(view);

	sphere_.get()->SettingWvp(view);
	skyBox_.get()->SettingWvp(view);

	for (int i = 0; i < particle_.size(); ++i) {
		if (i == 2) { // Ring particle
			particle_[i].get()->Update(view, [](const EffectDefinitionData& p) {
				EffectDefinitionData next = p;
				// Expand scale continuously to create a shockwave
				next.transform.scale.x += 0.04f;
				next.transform.scale.y += 0.04f;
				next.transform.scale.z += 0.04f;
				return next;
			});
		} else {
			particle_[i].get()->Update(view);
		}
	}

	Transform animationTransform = animation_->GetTransform();

	Vector3 moveInput = GamePadInput::GetLeftStick();
	Vector3 moveDirection = { moveInput.x, 0.0f, moveInput.y };

	if (Input::PushKey(DIK_SPACE)||GamePadInput::PushButton(XINPUT_GAMEPAD_A)) {
		isSneaking_ = !isSneaking_;
		if (isSneaking_) {
			animation_->SetAnimation("sneakWalk", 1.0f);
		} else {
			animation_->SetAnimation("walk", 1.0f);
		}
	}

	if (Input::PressKey(DIK_D) || Input::PressKey(DIK_RIGHT)) moveDirection.x += 1.0f;
	if (Input::PressKey(DIK_A) || Input::PressKey(DIK_LEFT))  moveDirection.x -= 1.0f;
	if (Input::PressKey(DIK_W) || Input::PressKey(DIK_UP))    moveDirection.z += 1.0f;
	if (Input::PressKey(DIK_S) || Input::PressKey(DIK_DOWN))  moveDirection.z -= 1.0f;

	float lengthSq = moveDirection.x * moveDirection.x + moveDirection.z * moveDirection.z;
	if (lengthSq > 0.0001f) {
		float length = std::sqrt(lengthSq);
		moveDirection.x /= length;
		moveDirection.z /= length;
		if (length > 1.0f) length = 1.0f;

		float speed = 0.05f;
		float moveDist = length * speed;
		animationTransform.translate.x += moveDirection.x * moveDist;
		animationTransform.translate.z += moveDirection.z * moveDist;

		animationTransform.rotate.y = std::atan2(moveDirection.x, moveDirection.z);
		animation_->SetTransform(animationTransform);

		float animSpeedScale = 1.0f;
		animation_.get()->UpdateWithDelta(view, moveDist * animSpeedScale);
	}
	else {
		animation_.get()->SetAnimationTime(0.0f);
		animation_.get()->UpdateWithDelta(view, 0.0f);
	}
	nodeAnimation_.get()->Update(view);

	// Axeのアタッチ処理
	Transform handTransform = animation_->GetBoneTransform(BoneType::RightHand);
	Matrix4x4 boneMatrix = MakeAffineMatrix(handTransform.translate, handTransform.scale, handTransform.rotate);
	Matrix4x4 offsetMatrix = MakeAffineMatrix(axeOffset_.translate, axeOffset_.scale, axeOffset_.rotate);
	Matrix4x4 finalMatrix = MultiplyMatrix4x4(offsetMatrix, boneMatrix);
	axe_->SetTransform(DecomposeMatrix(finalMatrix));
	axe_->SettingWvp(view);
}

void TestScene::Draw(class Draw& draw) {

	draw.SetCamera(camera_.get());
	// Set the SkyBox texture as environment map
	draw.SetEnvironmentTexture(skyBoxTexture_);

	//draw.DrawObj(ring_.get());
	//draw.DrawObj(cylinder_.get());

	draw.DrawObj(skyBox_.get());
	//draw.DrawObj(model_.get());
	//draw.DrawObj(floor.get());
	//draw.DrawObj(nodeAnimation_.get());
	draw.DrawAnimation(animation_.get());
	draw.DrawModel(axe_.get());

	//draw.DrawObj(sphere_.get());
	for (int i = 0, n = static_cast<int>(particle_.size()); i < n; ++i) {
	//	particle_[i].get()->Draw(draw);
	}
	//draw.DrawSprite(sprite_.get());
}
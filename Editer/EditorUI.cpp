#include "EditorUI.h"
#include "../MatchaEngine/Input/Input.h"

#ifdef _USE_IMGUI
#include <imgui.h>
#include "ImGuizmo.h"
#endif

#include "LanguageManager.h"
#include "EditorManager.h"
#include "../MatchaEngine/GameObjects/Object/Animation/CharacterAnimator.h"
#include "../MatchaEngine/GameObjects/Object/RenderObject.h"
#include "../MatchaEngine/GameObjects/Object/3d/Sphere.h"
#include "../MatchaEngine/GameObjects/Light/DirectionalLight.h"
#include "../MatchaEngine/GameObjects/Light/PointLight.h"
#include "../MatchaEngine/GameObjects/Light/SpotLight.h"

void EditorUI::ProcessMousePicking(GameObjectManager* gameObjectManager, const Matrix4x4& view, const Matrix4x4& projection) {
#ifdef _USE_IMGUI
    if (!gameObjectManager) return;

    // Sceneウィンドウの矩形とホバー状態を取得
    ImVec2 vMin, vMax, windowPos;
    bool isSceneHovered = false;
    ImGui::Begin(LanguageManager::Tr("Scene"));
    vMin = ImGui::GetWindowContentRegionMin();
    vMax = ImGui::GetWindowContentRegionMax();
    windowPos = ImGui::GetWindowPos();
    isSceneHovered = ImGui::IsWindowHovered();
    ImGui::End();

    // Sceneウィンドウにホバー中でない・ギズモ操作中は無視
    if (!isSceneHovered) return;
    if (ImGuizmo::IsOver()) return;

    // 左クリックの瞬間だけ処理
    if (!Input::PushMouse(0)) return;

    // ImGuiからマウス座標を取得
    ImVec2 mousePos = ImGui::GetMousePos();
    float mx = mousePos.x;
    float my = mousePos.y;

    // Sceneウィンドウの実際の描画領域（レターボックス等を考慮した絶対座標）
    ImVec2 scenePos = EditorManager::s_sceneImagePos;
    ImVec2 sceneSize = EditorManager::s_sceneImageSize;

    // NDC変換 (Sceneウィンドウ内を-1〜1にマッピング)
    float ndcX = (2.0f * (mx - scenePos.x)) / sceneSize.x - 1.0f;
    float ndcY = 1.0f - (2.0f * (my - scenePos.y)) / sceneSize.y;

    // Sceneウィンドウ外をクリックした場合は無視
    if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f) return;

    // ニア・ファークリップ坐標でレイを生成
    Vector3 nearPoint = { ndcX, ndcY, 0.0f };
    Vector3 farPoint  = { ndcX, ndcY, 1.0f };

    Matrix4x4 viewProj    = MultiplyMatrix4x4(view, projection);
    Matrix4x4 invViewProj = Inverse(viewProj);

    Vector3 rayStart = TransformMatrix(nearPoint, invViewProj);
    Vector3 rayEnd   = TransformMatrix(farPoint,  invViewProj);

    Ray ray;
    ray.origin    = rayStart;
    ray.direction = Normalize(rayEnd - rayStart);

    float closestDist = FLT_MAX;
    std::shared_ptr<GameObject> closestObj = nullptr;

    for (auto& obj : gameObjectManager->GetObjects()) {
        if (!obj->GetIsActive()) continue;

        AABB aabb = GetAABB(obj->GetTransform(), 2.0f, 2.0f);

        float dist = 0.0f;
        if (CheckRayAABB(ray, aabb, dist)) {
            if (dist < closestDist) {
                closestDist = dist;
                closestObj  = obj;
            }
        }

        std::shared_ptr<CharacterAnimator> animator = std::dynamic_pointer_cast<CharacterAnimator>(obj);
        if (!animator) {
            if (auto renderObj = std::dynamic_pointer_cast<RenderObject>(obj)) {
                animator = std::dynamic_pointer_cast<CharacterAnimator>(renderObj->GetObjectBase());
            }
        }

        if (animator && animator->GetVisibleBones()) {
            const Skeleton& skeleton = animator->GetSkeleton();
            for (size_t i = 0; i < skeleton.joints.size(); ++i) {
                std::shared_ptr<Sphere> sphereObj = animator->GetJointSphere((int32_t)i);
                if (sphereObj) {
                    AABB boneAabb = GetAABB(sphereObj->GetTransform(), 0.5f, 0.5f);
                    float boneDist = 0.0f;
                    if (CheckRayAABB(ray, boneAabb, boneDist)) {
                        if (boneDist < closestDist) {
                            closestDist = boneDist;
                            closestObj = sphereObj;
                        }
                    }
                }
            }
        }
    }

    if (closestObj) {
        SetSelectedObject(closestObj);
    }
#endif
}

void EditorUI::DrawGizmo(const Matrix4x4& view, const Matrix4x4& projection) {
#ifdef _USE_IMGUI
    if (!selectedObject_ || selectedObject_->GetIsLocked()) return;

    // 現在アクティブなウィンドウのContentRegionの座標を取得
    // (SceneウィンドウのBegin/Endの間から呼ばれることを前提とする)
    ImVec2 vMin = ImGui::GetWindowContentRegionMin();
    ImVec2 vMax = ImGui::GetWindowContentRegionMax();
    ImVec2 windowPos = ImGui::GetWindowPos();

    // Sceneウィンドウの実際の描画領域
    ImVec2 scenePos = EditorManager::s_sceneImagePos;
    ImVec2 sceneSize = EditorManager::s_sceneImageSize;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(scenePos.x, scenePos.y, sceneSize.x, sceneSize.y);
    ImGuizmo::SetGizmoSizeClipSpace(0.15f);

    Transform t = selectedObject_->GetTransform();
    // ギズモ操作時にスケールが小さいとバグるため、スケールは1にする
    Matrix4x4 world = MakeAffineMatrix(t.translate, {1.0f, 1.0f, 1.0f}, t.rotate);

    ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
    if (EditorManager::s_gizmoOp == 1) op = ImGuizmo::ROTATE;
    else if (EditorManager::s_gizmoOp == 2) op = ImGuizmo::SCALE;

    ImGuizmo::Manipulate(
        &view.m[0][0],
        &projection.m[0][0],
        op,
        ImGuizmo::LOCAL,
        &world.m[0][0]
    );

    if (ImGuizmo::IsUsing()) {
        // 分解してTransformに戻す
        float translation[3], rotation[3], scale[3];
        ImGuizmo::DecomposeMatrixToComponents(&world.m[0][0], translation, rotation, scale);

        // ラジアンへ変換
        float pi = 3.1415926535f;
        t.translate = { translation[0], translation[1], translation[2] };
        t.rotate = { rotation[0] * pi / 180.0f, rotation[1] * pi / 180.0f, rotation[2] * pi / 180.0f };
        // スケールはギズモでの操作結果に依存させず元の値を維持（SCALEモード時は必要なら別途対応）
        if (op == ImGuizmo::SCALE) {
            t.scale = { scale[0] * t.scale.x, scale[1] * t.scale.y, scale[2] * t.scale.z };
        }

        selectedObject_->SetTransform(t);
    }
#endif
}

void EditorUI::Draw(GameObjectManager* gameObjectManager, const Matrix4x4& view, const Matrix4x4& projection)
{
#ifdef _USE_IMGUI
    if (!gameObjectManager) return;

    // マウスピッキング
    ProcessMousePicking(gameObjectManager, view, projection);
    // (ギズモはEngineのSceneウィンドウコールバックから描画される)

    // Hierarchy Window
    ImGui::Begin(LanguageManager::Tr("Hierarchy"));
    std::shared_ptr<GameObject> objToDelete = nullptr;
    std::shared_ptr<GameObject> objToCopy = nullptr;

    static std::shared_ptr<GameObject> s_lastSelectedObject = nullptr;
    bool bFocusSelection = false;
    if (selectedObject_ != s_lastSelectedObject) {
        bFocusSelection = true;
        s_lastSelectedObject = selectedObject_;
    }

    auto DrawJoint = [&](auto& self, std::shared_ptr<CharacterAnimator> animator, int32_t jointIndex) -> void {
        const Skeleton& skeleton = animator->GetSkeleton();
        if (jointIndex < 0 || jointIndex >= (int32_t)skeleton.joints.size()) return;
        const Joint& joint = skeleton.joints[jointIndex];
        
        std::shared_ptr<Sphere> sphereObj = animator->GetJointSphere(jointIndex);
        
        ImGuiTreeNodeFlags jointFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (joint.children.empty()) {
            jointFlags |= ImGuiTreeNodeFlags_Leaf;
        }
        
        if (sphereObj && selectedObject_ == sphereObj) {
            jointFlags |= ImGuiTreeNodeFlags_Selected;
        }
        
        bool bContainsSelected = false;
        if (bFocusSelection && selectedObject_) {
            auto checkDescendant = [&](auto& checkSelf, int32_t jIdx) -> bool {
                std::shared_ptr<Sphere> so = animator->GetJointSphere(jIdx);
                if (so && so == selectedObject_) return true;
                const Joint& j = skeleton.joints[jIdx];
                for (int32_t childIdx : j.children) {
                    if (checkSelf(checkSelf, childIdx)) return true;
                }
                return false;
            };
            bContainsSelected = checkDescendant(checkDescendant, jointIndex);
            if (bContainsSelected) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            }
        }
        
        ImGui::PushID(&joint);
        bool isJointOpen = false;
        if (sphereObj) {
            isJointOpen = ImGui::TreeNodeEx((void*)sphereObj.get(), jointFlags, "%s", sphereObj->GetName().c_str());
            if (bContainsSelected && sphereObj == selectedObject_) ImGui::SetScrollHereY();
            if (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1)) {
                SetSelectedObject(sphereObj);
            }
        } else {
            isJointOpen = ImGui::TreeNodeEx((void*)&joint, jointFlags, "%s", joint.name.c_str());
        }
        
        if (isJointOpen) {
            for (int32_t childIndex : joint.children) {
                self(self, animator, childIndex);
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    };


    for (auto& obj : gameObjectManager->GetObjects()) {
        if (!obj) continue;
        
        std::shared_ptr<CharacterAnimator> animator = std::dynamic_pointer_cast<CharacterAnimator>(obj);
        if (!animator) {
            if (auto renderObj = std::dynamic_pointer_cast<RenderObject>(obj)) {
                animator = std::dynamic_pointer_cast<CharacterAnimator>(renderObj->GetObjectBase());
            }
        }

        bool bContainsSelected = false;
        if (bFocusSelection && selectedObject_) {
            if (obj == selectedObject_) {
                bContainsSelected = true;
            } else if (animator) {
                const Skeleton& skeleton = animator->GetSkeleton();
                if (skeleton.joints.size() > 0) {
                    auto checkDescendant = [&](auto& checkSelf, int32_t jIdx) -> bool {
                        std::shared_ptr<Sphere> so = animator->GetJointSphere(jIdx);
                        if (so && so == selectedObject_) return true;
                        const Joint& j = skeleton.joints[jIdx];
                        for (int32_t childIdx : j.children) {
                            if (checkSelf(checkSelf, childIdx)) return true;
                        }
                        return false;
                    };
                    bContainsSelected = checkDescendant(checkDescendant, skeleton.root);
                }
            }
            if (bContainsSelected) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            }
        }

        ImGui::AlignTextToFramePadding();
        bool isLocked = obj->GetIsLocked();
        ImGui::PushID(obj.get());
        if (ImGui::Checkbox("##lock", &isLocked)) {
            obj->SetIsLocked(isLocked);
        }
        ImGui::PopID();
        
        ImGui::SameLine();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!animator || animator->GetSkeleton().joints.empty()) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (selectedObject_ == obj) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (!obj->GetIsActive()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        bool isOpen = ImGui::TreeNodeEx((void*)obj.get(), flags, "%s", obj->GetName().c_str());
        if (bContainsSelected && obj == selectedObject_) ImGui::SetScrollHereY();
        
        if (!obj->GetIsActive()) {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1)) {
            SetSelectedObject(obj);
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem(LanguageManager::Tr("Copy"))) {
                objToCopy = obj;
            }
            if (ImGui::MenuItem(LanguageManager::Tr("Delete"))) {
                objToDelete = obj;
            }
            ImGui::EndPopup();
        }

        if (isOpen) {
            if (animator) {
                const Skeleton& skeleton = animator->GetSkeleton();
                if (skeleton.joints.size() > 0) {
                    DrawJoint(DrawJoint, animator, skeleton.root);
                }
            }
            ImGui::TreePop();
        }
    }

    if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Empty")) {
            auto emptyObj = std::make_shared<GameObject>();
            emptyObj->SetName("GameObject");
            gameObjectManager->AddObject(emptyObj);
        }
        if (ImGui::MenuItem("Create Directional Light")) {
            auto light = std::make_shared<DirectionalLight>();
            gameObjectManager->AddObject(light);
        }
        if (ImGui::MenuItem("Create Point Light")) {
            auto light = std::make_shared<PointLight>();
            gameObjectManager->AddObject(light);
        }
        if (ImGui::MenuItem("Create Spot Light")) {
            auto light = std::make_shared<SpotLight>();
            gameObjectManager->AddObject(light);
        }
        ImGui::EndPopup();
    }

    if (objToDelete) {
        gameObjectManager->RemoveObject(objToDelete);
        if (selectedObject_ == objToDelete) {
            SetSelectedObject(nullptr);
        }
    }
    if (objToCopy) {
        gameObjectManager->CopyObject(objToCopy);
    }

    ImGui::End();

    // Inspector Window
    ImGui::Begin(LanguageManager::Tr("Inspector"));
    if (selectedObject_) {
        ImGui::Text(LanguageManager::Tr("Name: %s"), selectedObject_->GetName().c_str());
        ImGui::Separator();

        bool isActive = selectedObject_->GetIsActive();
        if (ImGui::Checkbox(LanguageManager::Tr("Active"), &isActive)) {
            selectedObject_->SetIsActive(isActive);
        }
        ImGui::SameLine();
        bool isLocked = selectedObject_->GetIsLocked();
        if (ImGui::Checkbox(LanguageManager::Tr("Locked"), &isLocked)) {
            selectedObject_->SetIsLocked(isLocked);
        }
        ImGui::Separator();

        if (selectedObject_->GetIsLocked()) {
            ImGui::BeginDisabled();
        }

        selectedObject_->ImGui();

        if (selectedObject_->GetIsLocked()) {
            ImGui::EndDisabled();
        }
    } else {
        ImGui::Text(LanguageManager::Tr("No object selected."));
    }
    ImGui::End();
#endif
}

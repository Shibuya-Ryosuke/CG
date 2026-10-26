#include "CameraManager.h"
#include "../Core/Base/Logger.h"
#include "../Graphics/3D/ModelManager.h"
#include "../Editor/ModelPicker.h"
#include <algorithm>
#include <map>
#include <vector>

#ifdef NRELEASE
#include <imgui.h>
#endif

namespace RyoEngine {

    namespace {
        struct CameraSlot {
            std::string name; // 空なら名前なし(ImGuiでは "Camera" と表示)
            std::unique_ptr<Camera> camera;

            CameraSlot() = default;
            CameraSlot(std::string slotName, std::unique_ptr<Camera> slotCamera)
                : name(std::move(slotName)), camera(std::move(slotCamera)) {}
            ~CameraSlot() = default;

            // unique_ptrを持つのでコピー不可・ムーブのみ(暗黙の削除警告C4625/C4626を避けるため明示する)
            CameraSlot(const CameraSlot&) = delete;
            CameraSlot& operator=(const CameraSlot&) = delete;
            CameraSlot(CameraSlot&&) = default;
            CameraSlot& operator=(CameraSlot&&) = default;
        };

        // インデックス → カメラ。std::mapなのでImGuiでも番号順に並ぶ
        std::map<uint32_t, CameraSlot> slots;
        // 次に振るインデックス(通し番号)。破棄・Clearしても戻さないので、古いインデックスが別のカメラを指すことはない
        uint32_t nextIndex = 1;

        uint32_t activeIndex = CameraManager::kInvalidIndex;
        std::vector<uint32_t> activeStack; // PushActive()の履歴

        Camera defaultCamera;   // アクティブなカメラが無いときの代わり(落ちないための安全策)
        DebugCamera debugCamera;

        Camera* renderCamera = nullptr; // UpdateAll()で確定した描画カメラ

        bool IsValidIndex(uint32_t index) {
            return index != CameraManager::kInvalidIndex && slots.contains(index);
        }

        // ImGui・ログ用の表示名
        std::string DisplayName(const CameraSlot& slot) {
            return slot.name.empty() ? std::string("Camera") : slot.name;
        }

        // その場で「今描画に使うべきカメラ」を判断する
        Camera& DecideActive() {
#ifdef NRELEASE
            if (debugCamera.GetIsAvailable()) {
                return debugCamera;
            }
#endif
            return CameraManager::GetGameCamera();
        }

        // 描画カメラを確定し、カメラを必要とする描画系へ渡す
        void ApplyRenderCamera() {
            renderCamera = &DecideActive();
            ModelManager::SetViewProjection(renderCamera->GetViewProjectionMatrix());
            ModelPicker::SetCamera(*renderCamera);
        }
    }

    void CameraManager::Initialize() {
        Logger::Log("CameraManager : Initializing...\n");
        slots.clear();
        nextIndex = 1;
        activeIndex = kInvalidIndex;
        activeStack.clear();
        renderCamera = nullptr;

        defaultCamera.Initialize();

        // デバッグカメラ初期化
        debugCamera.Initialize();
        // デバッグ時操作有効、それ以外では無効
#ifdef NRELEASE
        debugCamera.SetAvailable(true);
#else
        debugCamera.SetAvailable(false);
#endif
        Logger::LogSuccess("CameraManager : Initialized\n");
    }

    void CameraManager::Finalize() {
        Logger::Log("CameraManager : Finalizing...\n");
        slots.clear();
        activeIndex = kInvalidIndex;
        activeStack.clear();
        renderCamera = nullptr;
        Logger::LogSuccess("CameraManager : Finalized\n");
    }

    // ------------------------------------------------------------------
    // 生成・破棄
    // ------------------------------------------------------------------
    uint32_t CameraManager::RegisterCamera(const std::string& name, std::unique_ptr<Camera> camera) {
        if (!camera) {
            return kInvalidIndex;
        }
        if (!name.empty() && FindIndex(name) != kInvalidIndex) {
            Logger::LogWarning("[CameraManager] 同名のカメラが既にあります(FindIndexは先に作った方を返します): " + name + "\n");
        }

        const uint32_t index = nextIndex++;
        slots.try_emplace(index, name, std::move(camera));

        // 最初に作ったカメラは自動でアクティブにする
        if (!IsValidIndex(activeIndex)) {
            activeIndex = index;
        }
        return index;
    }

    void CameraManager::DestroyCamera(uint32_t index) {
        auto it = slots.find(index);
        if (it == slots.end()) {
            return;
        }
        Camera* target = it->second.camera.get();

        // 履歴からも取り除く
        activeStack.erase(std::remove(activeStack.begin(), activeStack.end(), index), activeStack.end());

        // アクティブ中なら1つ前に戻す
        if (activeIndex == index) {
            if (!activeStack.empty()) {
                activeIndex = activeStack.back();
                activeStack.pop_back();
            } else {
                activeIndex = kInvalidIndex;
            }
        }

        slots.erase(it);

        // 描画カメラとして確定済みだった場合は、壊れたポインタを残さないよう確定し直す
        if (renderCamera == target) {
            ApplyRenderCamera();
        }
    }

    void CameraManager::Clear() {
        // nextIndexは戻さない(古いインデックスが新しいカメラを指さないように)
        slots.clear();
        activeIndex = kInvalidIndex;
        activeStack.clear();
        // 描画カメラも破棄済みの可能性があるので、デフォルト(またはDebugCamera)で確定し直す
        ApplyRenderCamera();
    }

    // ------------------------------------------------------------------
    // 取得
    // ------------------------------------------------------------------
    Camera* CameraManager::GetCamera(uint32_t index) {
        auto it = slots.find(index);
        return it != slots.end() ? it->second.camera.get() : nullptr;
    }

    uint32_t CameraManager::FindIndex(const std::string& name) {
        if (name.empty()) {
            return kInvalidIndex; // 名前なしのカメラは検索対象にしない
        }
        for (const auto& [index, slot] : slots) {
            if (slot.name == name) {
                return index;
            }
        }
        return kInvalidIndex;
    }

    // ------------------------------------------------------------------
    // 切り替え
    // ------------------------------------------------------------------
    void CameraManager::SetActive(uint32_t index) {
        if (!IsValidIndex(index)) {
            Logger::LogWarning("[CameraManager] SetActive: 無効なインデックスです\n");
            return;
        }
        activeIndex = index;
        activeStack.clear();
    }

    void CameraManager::PushActive(uint32_t index) {
        if (!IsValidIndex(index)) {
            Logger::LogWarning("[CameraManager] PushActive: 無効なインデックスです\n");
            return;
        }
        activeStack.push_back(activeIndex);
        activeIndex = index;
    }

    void CameraManager::PopActive() {
        if (activeStack.empty()) {
            Logger::LogWarning("[CameraManager] PopActive: 戻る先の履歴がありません\n");
            return;
        }
        activeIndex = activeStack.back();
        activeStack.pop_back();
    }

    uint32_t CameraManager::GetActiveIndex() {
        return activeIndex;
    }

    Camera& CameraManager::GetActive() {
        return renderCamera ? *renderCamera : DecideActive();
    }

    Camera& CameraManager::GetGameCamera() {
        auto it = slots.find(activeIndex);
        return it != slots.end() ? *it->second.camera : defaultCamera;
    }

    // ------------------------------------------------------------------
    // デバッグカメラ
    // ------------------------------------------------------------------
    DebugCamera& CameraManager::GetDebugCamera() {
        return debugCamera;
    }

    void CameraManager::UpdateDebugCamera() {
        debugCamera.Update();
    }

    // ------------------------------------------------------------------
    // 毎フレーム
    // ------------------------------------------------------------------
    void CameraManager::UpdateAll() {
        for (auto& [index, slot] : slots) {
            if (slot.camera->IsActive()) {
                slot.camera->Update();
            }
        }
        // 全カメラの行列が確定したので、ここで描画カメラも確定させる
        ApplyRenderCamera();
    }

    void CameraManager::DrawImGui() {
#ifdef NRELEASE
        ImGui::Begin("CameraManager");

        // --- 現在の状態 ---
        std::string activeName = "(デフォルトカメラ)";
        if (auto it = slots.find(activeIndex); it != slots.end()) {
            activeName = DisplayName(it->second) + " [" + std::to_string(activeIndex) + "]";
        }
        ImGui::Text("ゲームカメラ : %s", activeName.c_str());
        ImGui::Text("描画カメラ   : %s", debugCamera.GetIsAvailable() ? "DebugCamera" : activeName.c_str());
        ImGui::Text("Pushの履歴   : %zu", activeStack.size());
        if (ImGui::Button("Pop")) {
            PopActive();
        }

        ImGui::Separator();

        // --- カメラ一覧 ---
        // NOTE: ボタン操作でslotsの中身(アクティブ等)は変わるが、要素の追加・削除はしないのでループ中でも安全
        for (auto& [index, slot] : slots) {
            Camera* camera = slot.camera.get();
            const bool isActive = (index == activeIndex);

            ImGui::PushID(static_cast<int>(index));
            // ラベルが変わってもツリーの開閉状態が保たれるよう、IDは固定の文字列にする
            if (ImGui::TreeNode("camera", "%s%s [%u]", isActive ? "* " : "  ", DisplayName(slot).c_str(), index)) {
                if (ImGui::Button("SetActive")) {
                    SetActive(index);
                }
                ImGui::SameLine();
                if (ImGui::Button("Push")) {
                    PushActive(index);
                }

                bool enabled = camera->IsActive();
                if (ImGui::Checkbox("Updateする", &enabled)) {
                    camera->SetActive(enabled);
                }

                if (camera->IsUsingCustomMatrices()) {
                    ImGui::TextDisabled("カスタム行列モード中");
                    if (ImGui::Button("カスタム行列を解除")) {
                        camera->ReleaseCustomMatrices();
                    }
                } else {
                    Vector3 translate = camera->GetTranslate();
                    if (ImGui::DragFloat3("座標", &translate.x, 0.1f)) {
                        camera->SetTranslate(translate);
                    }
                    Vector3 rotate = camera->GetRotate();
                    if (ImGui::DragFloat3("回転(ラジアン)", &rotate.x, 0.01f)) {
                        camera->SetRotate(rotate);
                    }
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        ImGui::End();
#endif
    }
}
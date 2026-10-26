#include "SceneManager.h"
#include <RyoEngine.h>
#include <cassert>

#ifdef NRELEASE
#include <imgui.h>
#endif

namespace RyoEngine {

    // reloadKey_の初期値(0x3F)がDIK_F5と一致しているかの確認
    static_assert(DIK_F5 == 0x3F, "SceneManager::reloadKey_ の初期値を DIK_F5 に合わせてください");

    SceneManager& SceneManager::GetInstance() {
        static SceneManager instance;
        return instance;
    }

    void SceneManager::SetSceneFactory(std::unique_ptr<AbstractSceneFactory> factory) {
        GetInstance().sceneFactory_ = std::move(factory);
    }

    void SceneManager::ChangeScene(const std::string& sceneName) {
        auto& inst = GetInstance();

        assert(inst.sceneFactory_);
        assert(inst.nextScene_ == nullptr);

        // 次シーンを生成
        inst.nextScene_ = inst.sceneFactory_->CreateScene(sceneName);
        if (!inst.nextScene_) {
            Logger::LogError("[SceneManager] ChangeScene: 該当するシーンがありません: " + sceneName);
            return;
        }
        inst.nextSceneName_ = sceneName;
    }

    void SceneManager::ReloadScene() {
        auto& inst = GetInstance();

        // 既に切り替え予約があるなら無視する(連打対策)
        if (inst.nextScene_) {
            return;
        }
        if (inst.currentSceneName_.empty()) {
            Logger::LogWarning("[SceneManager] ReloadScene: 現在のシーンがありません");
            return;
        }

        Logger::LogSuccess("[SceneManager] Reload: " + inst.currentSceneName_);
        ChangeScene(inst.currentSceneName_);
    }

    void SceneManager::Update() {
        auto& inst = GetInstance();

        // 予約があれば、ここで入れ替える
        if (inst.nextScene_) {
            inst.ReleaseCurrentScene();

            inst.currentScene_ = std::move(inst.nextScene_);
            inst.currentSceneName_ = std::move(inst.nextSceneName_);
            inst.nextSceneName_.clear();

            inst.currentScene_->Initialize();
        }

#ifdef NRELEASE
        if (Input::TriggerKey(inst.reloadKey_)) {
            ReloadScene();
        }
        inst.DrawImGui();
#endif

        if (inst.currentScene_) {
            inst.currentScene_->Update();
        }
    }

    void SceneManager::Draw() {
        auto& inst = GetInstance();
        if (inst.currentScene_) {
            inst.currentScene_->Draw();
        }
    }

    void SceneManager::Finalize() {
        auto& inst = GetInstance();
        inst.ReleaseCurrentScene();
        inst.currentSceneName_.clear();
        inst.nextScene_.reset();
        inst.nextSceneName_.clear();
        inst.sceneFactory_.reset();
    }

    const std::string& SceneManager::GetCurrentSceneName() {
        return GetInstance().currentSceneName_;
    }

    void SceneManager::ReleaseCurrentScene() {
        if (!currentScene_) {
            return;
        }

        // シーンが登録したもの(シーンのメンバへのポインタ)を、シーンが生きているうちに解除する
        // (ParamEditorは解除時に今の値をキャッシュへ退避するので、破棄後だと解放済みのメモリを読んでしまう)
        ParamEditor::ClearEntries();
        GPUParticleManager::ClearRegistrations();

        currentScene_->Finalize();
        currentScene_.reset();

        // カメラはシーンのFinalizeで使う可能性があるので、破棄後に解除する
        CameraManager::Clear();
    }

    void SceneManager::DrawImGui() {
#ifdef NRELEASE
        ImGui::Begin("SceneManager");

        ImGui::Text("現在のシーン : %s", currentSceneName_.empty() ? "(なし)" : currentSceneName_.c_str());

        if (ImGui::Button("リロード (F5)")) {
            ReloadScene();
        }

        ImGui::Separator();

        ImGui::InputText("シーン名", changeSceneNameBuffer_, sizeof(changeSceneNameBuffer_));
        ImGui::SameLine();
        if (ImGui::Button("切り替え")) {
            if (changeSceneNameBuffer_[0] != '\0' && !nextScene_) {
                ChangeScene(changeSceneNameBuffer_);
            }
        }

        ImGui::End();
#endif
    }

}

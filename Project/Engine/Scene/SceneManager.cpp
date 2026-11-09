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

    void SceneManager::SetDefaultScene(const std::string& sceneName) {
        auto& inst = GetInstance();
        inst.defaultSceneName_ = sceneName;
        inst.defaultSceneStarted_ = false;
    }

    void SceneManager::ChangeScene(const std::string& sceneName) {
        auto& inst = GetInstance();

        assert(inst.nextScene_ == nullptr);

        // 次シーンを生成 (登録されたシーンから探す)
        if (auto it = inst.sceneCreators_.find(sceneName); it != inst.sceneCreators_.end()) {
            inst.nextScene_ = it->second();
        }
        if (!inst.nextScene_) {
            Logger::LogError("[SceneManager] ChangeScene: 該当するシーンがありません: " + sceneName);
            return;
        }
        inst.nextSceneName_ = sceneName;
    }

    void SceneManager::ChangeSceneByType(std::type_index type, const char* typeName) {
        auto& inst = GetInstance();
        auto it = inst.sceneNamesByType_.find(type);
        if (it == inst.sceneNamesByType_.end()) {
            Logger::LogError(std::string("[SceneManager] ChangeScene: RegisterScene されていないシーンです: ") + typeName);
            return;
        }
        ChangeScene(it->second);
    }

    void SceneManager::SetDefaultSceneByType(std::type_index type, const char* typeName) {
        auto& inst = GetInstance();
        auto it = inst.sceneNamesByType_.find(type);
        if (it == inst.sceneNamesByType_.end()) {
            Logger::LogError(std::string("[SceneManager] SetDefaultScene: RegisterScene されていないシーンです: ") + typeName);
            return;
        }
        SetDefaultScene(it->second);
    }

    std::string SceneManager::MakeSceneName(const char* typeName) {
        std::string name = typeName;
        // 先頭の "class " / "struct " を取り除く (MSVC の typeid().name() の形)
        for (const char* prefix : { "class ", "struct " }) {
            const std::string p = prefix;
            if (name.compare(0, p.size(), p) == 0) {
                name.erase(0, p.size());
                break;
            }
        }
        // 名前空間を取り除く ("App::Title" → "Title")
        const size_t pos = name.rfind("::");
        if (pos != std::string::npos) {
            name.erase(0, pos + 2);
        }
        return name;
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

        // まだシーンが無ければ、起動時のシーン (SetDefaultScene) に切り替える (1回だけ試す)
        if (!inst.currentScene_ && !inst.nextScene_ && !inst.defaultSceneStarted_ && !inst.defaultSceneName_.empty()) {
            inst.defaultSceneStarted_ = true;
            ChangeScene(inst.defaultSceneName_);
        }

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

        // ゲームオブジェクト (全員の Update → 当たり判定 → 消す) を、シーンより先に更新する。
        // シーンの Update では、全員が動いて当たり判定も済んだ後の状態を見られる
        GameObjectManager::Update();

        if (inst.currentScene_) {
            inst.currentScene_->Update();
        }
    }

    void SceneManager::Draw() {
        auto& inst = GetInstance();
        // ゲームオブジェクトを描いてから、シーン (UIなど) を描く
        GameObjectManager::Draw();
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
        inst.sceneCreators_.clear();
        inst.sceneNamesByType_.clear();
        inst.registeredSceneNames_.clear();
        inst.defaultSceneName_.clear();
        inst.defaultSceneStarted_ = false;
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
        // パーティクル・ライトが参照する座標・フラグの登録
        BindingRegistry::Clear();
        // パーティクルはシーンをまたいで残さない (出ている粒も、連鎖の予約も消す)
        ParticleManager::ClearAll();
        // カスタムポストエフェクトもシーンごとに登録し直す
        PostEffectManager::Clear();

        currentScene_->Finalize();
        // ゲームオブジェクトもシーンごとに作り直す (シーンのFinalizeで使う可能性があるので、その後に消す)
        GameObjectManager::Clear();
        currentScene_.reset();

        // カメラはシーンのFinalizeで使う可能性があるので、破棄後に解除する
        CameraManager::Clear();

        // ParamEditor の登録が全部消えたので、エンジンの項目(当たり判定の表示など)を登録し直す
        GameObjectManager::RegisterDebugParams();
    }

    void SceneManager::DrawImGui() {
#ifdef NRELEASE
        ImGui::Begin("SceneManager");

        ImGui::Text("現在のシーン : %s", currentSceneName_.empty() ? "(なし)" : currentSceneName_.c_str());

        if (ImGui::Button("リロード (F5)")) {
            ReloadScene();
        }

        ImGui::Separator();

        // 登録されたシーンから選んで切り替える
        if (registeredSceneNames_.empty()) {
            ImGui::TextDisabled("登録されたシーンがありません (SceneManager::RegisterScene)");
        } else {
            const int32_t count = static_cast<int32_t>(registeredSceneNames_.size());
            if (selectedSceneIndex_ < 0 || selectedSceneIndex_ >= count) {
                selectedSceneIndex_ = 0;
            }
            if (ImGui::BeginCombo("シーン", registeredSceneNames_[selectedSceneIndex_].c_str())) {
                for (int32_t i = 0; i < count; ++i) {
                    const bool selected = (i == selectedSceneIndex_);
                    if (ImGui::Selectable(registeredSceneNames_[i].c_str(), selected)) {
                        selectedSceneIndex_ = i;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button("切り替え") && !nextScene_) {
                ChangeScene(registeredSceneNames_[selectedSceneIndex_]);
            }
        }

        ImGui::End();
#endif
    }

}

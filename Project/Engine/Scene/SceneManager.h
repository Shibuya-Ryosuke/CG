#pragma once
#include "BaseScene.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace RyoEngine {

    /// <summary>
    /// シーン管理クラス(静的クラス)
    /// 登録されたシーン(RegisterScene)の生成・切り替え・リロード・更新・描画を行う
    ///
    /// 切り替えは予約制。ChangeScene()で次シーンを生成しておき、
    /// 次フレームのUpdate()の先頭で入れ替える(実行中のシーンを途中で破棄しないため)
    ///
    /// 使い方 (アプリの起動時に登録する):
    ///   SceneManager::RegisterScene&lt;Title&gt;();      // 名前はクラス名 ("Title") になる
    ///   SceneManager::RegisterScene&lt;Game&gt;();
    ///   SceneManager::SetDefaultScene&lt;Title&gt;();    // 起動時のシーン
    ///   SceneManager::ChangeScene&lt;Game&gt;();         // 型で切り替える
    /// </summary>
    class SceneManager {
    public:
        // コピー・ムーブ禁止
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;
        SceneManager(SceneManager&&) = delete;
        SceneManager& operator=(SceneManager&&) = delete;

        /// <summary>
        /// シーンを登録する (型と名前の両方で切り替えられるようになる)。同じ名前で登録し直すと上書き
        /// 名前を省略するとクラス名 (名前空間を除く) になる。ImGui のシーン一覧にもこの名前で出る
        /// </summary>
        template <class T>
        static void RegisterScene(const std::string& name = "") {
            static_assert(std::is_base_of_v<BaseScene, T>, "RegisterScene には BaseScene の派生クラスを指定すること");
            auto& inst = GetInstance();
            const std::string sceneName = name.empty() ? MakeSceneName(typeid(T).name()) : name;
            if (inst.sceneCreators_.find(sceneName) == inst.sceneCreators_.end()) {
                inst.registeredSceneNames_.push_back(sceneName);
            }
            inst.sceneCreators_[sceneName] = []() -> std::unique_ptr<BaseScene> { return std::make_unique<T>(); };
            inst.sceneNamesByType_[std::type_index(typeid(T))] = sceneName;
        }

        /// <summary>
        /// 起動時のシーンを決める (まだシーンが無ければ、次の Update で切り替わる)
        /// </summary>
        static void SetDefaultScene(const std::string& sceneName);
        /// <summary>
        /// 起動時のシーンを型で決める。例: SceneManager::SetDefaultScene&lt;Title&gt;();
        /// (RegisterScene の後に呼ぶこと)
        /// </summary>
        template <class T>
        static void SetDefaultScene() {
            static_assert(std::is_base_of_v<BaseScene, T>, "SetDefaultScene には BaseScene の派生クラスを指定すること");
            SetDefaultSceneByType(std::type_index(typeid(T)), typeid(T).name());
        }

        /// <summary>
        /// シーン切り替えを予約する(次フレームの頭で切り替わる)
        /// (RegisterScene していない名前ならエラーログを出して何もしない)
        /// </summary>
        static void ChangeScene(const std::string& sceneName);

        /// <summary>
        /// 型でシーン切り替えを予約する。例: SceneManager::ChangeScene&lt;Game&gt;();
        /// (RegisterScene していない型ならエラーログを出して何もしない)
        /// </summary>
        template <class T>
        static void ChangeScene() {
            static_assert(std::is_base_of_v<BaseScene, T>, "ChangeScene には BaseScene の派生クラスを指定すること");
            ChangeSceneByType(std::type_index(typeid(T)), typeid(T).name());
        }

        /// <summary>
        /// 現在のシーンを作り直す。既に切り替え予約がある場合は無視する
        /// </summary>
        static void ReloadScene();

        /// <summary>
        /// 予約があれば切り替えてから、現在のシーンを更新する
        /// </summary>
        static void Update();

        /// <summary>
        /// 現在のシーンを描画する
        /// </summary>
        static void Draw();

        /// <summary>
        /// 現在のシーンを破棄し、登録も全部消す
        /// </summary>
        static void Finalize();

        static const std::string& GetCurrentSceneName();

    private:
        SceneManager() = default;
        ~SceneManager() = default;

        static SceneManager& GetInstance();

        // 型から登録名を引いて ChangeScene / SetDefaultScene する (typeName はエラー表示用)
        static void ChangeSceneByType(std::type_index type, const char* typeName);
        static void SetDefaultSceneByType(std::type_index type, const char* typeName);
        // typeid().name() ("class App::Title" など) から、クラス名だけ ("Title") を取り出す
        static std::string MakeSceneName(const char* typeName);

        // 現在のシーンを終了・破棄し、シーンが登録したものを解除する
        void ReleaseCurrentScene();

        // デバッグ用ウィンドウ(現在のシーン名・リロード・登録されたシーンから選んで切り替え)
        void DrawImGui();

        // --- 登録式 (RegisterScene) ---
        // 名前 → シーンを作る関数
        std::unordered_map<std::string, std::function<std::unique_ptr<BaseScene>()>> sceneCreators_;
        // 型 → 名前 (ChangeScene<T>() 用)
        std::unordered_map<std::type_index, std::string> sceneNamesByType_;
        // 登録した順の名前 (ImGui の一覧用)
        std::vector<std::string> registeredSceneNames_;
        // 起動時のシーン名と、もう切り替えを試したか
        std::string defaultSceneName_;
        bool defaultSceneStarted_ = false;

        std::unique_ptr<BaseScene> currentScene_;
        std::string currentSceneName_;

        std::unique_ptr<BaseScene> nextScene_;
        std::string nextSceneName_;

        // デバッグ用:リロードキー(DirectInputのキーコード。初期値はDIK_F5)
        uint8_t reloadKey_ = 0x3F;

        // デバッグ用:切り替え先として選んでいるシーン (registeredSceneNames_ の番号)
        int32_t selectedSceneIndex_ = 0;
    };

}

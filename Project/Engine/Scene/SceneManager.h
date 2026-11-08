#pragma once
#include "BaseScene.h"
#include "AbstractSceneFactory.h"
#include <cstdint>
#include <memory>
#include <string>

namespace RyoEngine {

    /// <summary>
    /// シーン管理クラス(静的クラス)
    /// シーンの生成はファクトリーに任せ、切り替え・リロード・更新・描画を行う
    ///
    /// 切り替えは予約制。ChangeScene()で次シーンを生成しておき、
    /// 次フレームのUpdate()の先頭で入れ替える(実行中のシーンを途中で破棄しないため)
    /// </summary>
    class SceneManager {
    public:
        // コピー・ムーブ禁止
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;
        SceneManager(SceneManager&&) = delete;
        SceneManager& operator=(SceneManager&&) = delete;

        /// <summary>
        /// シーン工場をセットする(アプリ側の具象ファクトリーを渡す)
        /// </summary>
        static void SetSceneFactory(std::unique_ptr<AbstractSceneFactory> factory);

        /// <summary>
        /// シーン切り替えを予約する(次フレームの頭で切り替わる)
        /// </summary>
        static void ChangeScene(const std::string& sceneName);

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
        /// 現在のシーンを破棄し、ファクトリーも解放する
        /// </summary>
        static void Finalize();

        static const std::string& GetCurrentSceneName();

    private:
        SceneManager() = default;
        ~SceneManager() = default;

        static SceneManager& GetInstance();

        // 現在のシーンを終了・破棄し、シーンが登録したものを解除する
        void ReleaseCurrentScene();

        // デバッグ用ウィンドウ(現在のシーン名・リロード・名前指定での切り替え)
        void DrawImGui();

        std::unique_ptr<AbstractSceneFactory> sceneFactory_;

        std::unique_ptr<BaseScene> currentScene_;
        std::string currentSceneName_;

        std::unique_ptr<BaseScene> nextScene_;
        std::string nextSceneName_;

        // デバッグ用:リロードキー(DirectInputのキーコード。初期値はDIK_F5)
        uint8_t reloadKey_ = 0x3F;

        // デバッグ用:名前指定で切り替えるときの入力バッファ
        char changeSceneNameBuffer_[64] = {};
    };

}

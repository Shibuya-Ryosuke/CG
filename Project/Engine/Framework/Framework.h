#pragma once
#include <string>

namespace RyoEngine {

    /// <summary>
    /// ゲームの土台となる基底クラス
    /// エンジンの初期化・メインループ・終了処理を受け持ち、
    /// 派生クラスは Initialize / Update / Draw / Finalize だけを実装する
    /// </summary>
    class Framework {
    public:
        explicit Framework(std::wstring windowTitle = L"Application");
        virtual ~Framework() = default;

        Framework(const Framework&) = delete;
        Framework& operator=(const Framework&) = delete;

        /// <summary>
        /// ゲームを実行する(エンジン初期化〜終了までを一括で行う)
        /// 派生クラスでオーバーライドしない
        /// </summary>
        void Run();

    protected:
        // --- 派生クラスで実装するゲーム処理 ---
        virtual void Initialize() = 0;
        // シーンの更新・描画はSceneManagerが行うので、ここはゲーム全体で共通の処理だけ(不要なら空のまま)
        virtual void Update() {}
        virtual void Draw() {}
        virtual void Finalize() {}

    private:
        std::wstring windowTitle_;
    };

}

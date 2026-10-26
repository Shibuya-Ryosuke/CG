#pragma once
#include <RyoEngine.h>
#include "Title.h"
#include "Game.h"
#include "../Fade/Fade.h"
#include "../Background/Background.h"

namespace AL4_1 {
	enum class Scene : int32_t {
		Title,
		Game,
		Result,
		Count,
	};

	class SceneManager {
	public:
		SceneManager();
		~SceneManager();
		// コピー禁止（ユニークポインタを持つので）
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;
		// ムーブは許可
		SceneManager(SceneManager&&) noexcept = default;
		SceneManager& operator=(SceneManager&&) noexcept = default;

		void Initialize(Scene scene);
		void Update();
		void Draw(RyoEngine::Camera& debugCamera);
		void Finalize();

		RyoEngine::Camera& GetCamera() { return game_.GetCamera(); }

	private:
		// シーンを切り替え、切り替え先を最初の状態にする
		void ChangeScene(Scene next);
		// フェードアウトを始め、真っ黒になったら next へ切り替える
		void RequestChangeScene(Scene next);
		// フェードインが終わったときの処理
		void OnFadeInFinished();
		void RegisterInParamEditor();

	private:
		std::unique_ptr<RyoEngine::Model> ground_ = nullptr;
		std::unique_ptr<RyoEngine::Model> sky_ = nullptr;

		Scene scene_ = Scene::Game;

		Title title_{};
		Game game_{};

		// 背景の装飾（タイトルとゲームで共有する。1つだけなのでパラメータも重複しない）
		Background background_{};

		// --- フェード ---
		Fade fade_{};
		// フェードアウトが終わったら切り替える先
		Scene nextScene_ = Scene::Title;
		// 切り替えの途中か（フェードアウト～フェードインが終わるまで）
		bool isChanging_ = false;
		// フェードインの終わりを待っているか
		bool isWaitingFadeIn_ = false;
		// フェードにかける秒数
		float fadeOutTime_ = 0.5f;
		float fadeInTime_ = 0.5f;
	};
}

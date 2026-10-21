#pragma once
#include <RyoEngine.h>
#include "Title.h"
#include "Game.h"
#include "Result.h"

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

	private:
		std::unique_ptr<RyoEngine::Model> ground_ = nullptr;
		std::unique_ptr<RyoEngine::Model> sky_ = nullptr;

		Scene scene_ = Scene::Game;

		Title title_{};
		Game game_{};
		Result result_{};
	};
}

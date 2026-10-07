#pragma once
#include <RyoEngine.h>
#include "Title.h"
#include "Game.h"

namespace AL4_1 {
	enum class Scene : int32_t {
		Title,
		Game,
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
		std::unique_ptr<RyoEngine::Model> ground_ = nullptr;
		std::unique_ptr<RyoEngine::Model> sky_ = nullptr;

		Scene scene_ = Scene::Game;

		Title title_{};
		Game game_{};
	};
}
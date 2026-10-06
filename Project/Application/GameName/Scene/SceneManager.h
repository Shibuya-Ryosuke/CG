#pragma once
#include <RyoEngine.h>
#include "Title.h"
#include "Game.h"

namespace Game1 {
	enum class Scene : int32_t {
		Title,
		Game,
		Count,
	};

	class SceneManager {
	public:
		void Initialize(Scene scene);
		void Update();
		void Draw();
		void Finalize();

	private:
		Scene scene_ = Scene::Title;

		RyoEngine::Camera camera_{};

		Title title_{};
		Game game_{};
	};
}
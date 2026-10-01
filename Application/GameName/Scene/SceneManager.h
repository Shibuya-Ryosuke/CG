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
		void Initialize(RyoEngine::DebugCamera& debugCamera, Scene scene);
		void Update();
		void Draw();
		void Finalize();

	private:
		RyoEngine::DebugCamera debugCamera_{};

		Scene scene_ = Scene::Title;

		Title title_{};
		Game game_{};
	};
}
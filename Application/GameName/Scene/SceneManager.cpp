#include "SceneManager.h"

namespace Game1 {
	void SceneManager::Initialize(RyoEngine::DebugCamera& debugCamera, Scene scene) {
		debugCamera_ = debugCamera;

		title_.Initialize();
		game_.Initialize();

		scene_ = scene;
	}

	void SceneManager::Update() {
		switch (scene_) {
		case Scene::Title:
			title_.Update();
			break;

		case Scene::Game:
			game_.Update();
			break;

		default:
			break;
		}
	}

	void SceneManager::Draw() {
		switch (scene_) {
		case Scene::Title:
			title_.Draw();
			break;

		case Scene::Game:
			game_.Draw();
			break;

		default:
			break;
		}
	}

	void SceneManager::Finalize() {}

}
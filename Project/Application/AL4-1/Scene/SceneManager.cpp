#include "SceneManager.h"

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	SceneManager::SceneManager() = default;
	SceneManager::~SceneManager() = default;

	void SceneManager::Initialize(Scene scene) {
		title_.Initialize();
		game_.Initialize();

		scene_ = scene;

		sky_ = Model::Create("Resources/ApplicationResources/AL4-1/SkyDome/skydome.obj");
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

	void SceneManager::Draw(RyoEngine::Camera& camera) {
		sky_->TransferMatrix(camera);
		sky_->Draw();
		switch (scene_) {
		case Scene::Title:
			title_.Draw();
			break;

		case Scene::Game:
			game_.Draw(camera);
			break;

		default:
			break;
		}
	}

	void SceneManager::Finalize() {}

}
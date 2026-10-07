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

		ground_ = Model::Create("Resources/ApplicationResources/AL4-1/ground.obj");
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
		ground_->TransferMatrix(camera);
		sky_->TransferMatrix(camera);
		ground_->Draw();
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
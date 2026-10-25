#include "SceneManager.h"

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	SceneManager::SceneManager() = default;
	SceneManager::~SceneManager() = default;

	void SceneManager::Initialize(Scene scene) {
		title_.Initialize();
		game_.Initialize();
		result_.Initialize();

		scene_ = scene;

		ground_ = Model::Create("Resources/ApplicationResources/AL4-1/ground.obj");
		sky_ = Model::Create("Resources/ApplicationResources/AL4-1/SkyDome/skydome.obj");
	}

	void SceneManager::Update() {
		GPUParticleManager::GetInstance()->Update(GetDeltaTime());

		switch (scene_) {
		case Scene::Title:
			title_.Update();
			if (title_.IsFinished()) {
				ChangeScene(Scene::Game);
			}
			break;

		case Scene::Game:
			game_.Update();
			if (game_.IsClear()) {
				ChangeScene(Scene::Result);
			} else if (game_.IsRequestTitle()) {
				ChangeScene(Scene::Title);
			}
			break;

		case Scene::Result:
			result_.Update();
			if (result_.IsFinished()) {
				ChangeScene(Scene::Title);
			}
			break;

		default:
			break;
		}
	}

	void SceneManager::Draw(RyoEngine::Camera& debugCamera) {
		Camera& camera = GetActiveCamera(debugCamera);

		GPUParticleManager::GetInstance()->Draw(camera);
		ground_->TransferMatrix(camera);
		sky_->TransferMatrix(camera);
		//ground_->Draw();
		sky_->Draw();
		switch (scene_) {
		case Scene::Title:
			title_.Draw();
			break;

		case Scene::Game:
			game_.Draw(camera);
			break;

		case Scene::Result:
			result_.Draw();
			break;

		default:
			break;
		}
	}

	void SceneManager::Finalize() {}

	void SceneManager::ChangeScene(Scene next) {
		scene_ = next;

		switch (scene_) {
		case Scene::Title:
			title_.Start();
			break;
		case Scene::Game:
			game_.Start();
			break;
		case Scene::Result:
			result_.Start();
			break;
		default:
			break;
		}
	}

}

#include "SceneManager.h"

void SceneManager::Initialize(Scene scene){
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

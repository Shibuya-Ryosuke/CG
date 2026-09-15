#pragma once
#include "../../Original/RyoEngine.h"
#include "Title.h"
#include "Game.h"

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

	Title title_{};
	Game game_{};
};
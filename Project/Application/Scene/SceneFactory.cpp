#include "SceneFactory.h"
#include "Title.h"
#include "Game.h"

using namespace RyoEngine;

namespace App {
	std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName) {
		if (sceneName == "Title") {
			return std::make_unique<Title>();
		}
		if (sceneName == "Game") {
			return std::make_unique<Game>();
		}
		return nullptr;
	}
}

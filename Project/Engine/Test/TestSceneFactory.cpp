#include "TestSceneFactory.h"
#include "TestScene.h"

using namespace RyoEngine;

namespace Test {

	std::unique_ptr<BaseScene> TestSceneFactory::CreateScene(const std::string& sceneName) {
		// 次のシーンを生成
		if (sceneName == "TEST") {
			return std::make_unique<TestScene>();
		}

		return nullptr;
	}

}

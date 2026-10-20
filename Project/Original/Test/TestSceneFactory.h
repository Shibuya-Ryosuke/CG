#pragma once
#include <RyoEngine.h>

namespace Test {

	// このゲーム(Test)用のシーン工場
	class TestSceneFactory : public RyoEngine::AbstractSceneFactory {
	public:
		/// <summary>
		/// シーン生成
		/// </summary>
		/// <param name="sceneName">シーン名</param>
		/// <returns>生成したシーン(該当なしならnullptr)</returns>
		std::unique_ptr<RyoEngine::BaseScene> CreateScene(const std::string& sceneName) override;
	};

}

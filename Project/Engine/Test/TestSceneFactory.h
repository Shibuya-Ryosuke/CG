#pragma once
#include <RyoEngine.h>

namespace Test {
	/// <summary>
	/// Test アプリのシーン工場。シーン名から対応するシーンを作る。
	/// </summary>
	class TestSceneFactory : public RyoEngine::AbstractSceneFactory {
	public:
		/// <summary>シーンを作る。該当する名前がなければ nullptr</summary>
		std::unique_ptr<RyoEngine::BaseScene> CreateScene(const std::string& sceneName) override;
	};
}

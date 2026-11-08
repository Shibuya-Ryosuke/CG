#pragma once
#include <RyoEngine.h>

namespace App {
	/// <summary>
	/// シーン名から対応するシーンを作る。
	/// </summary>
	class SceneFactory : public RyoEngine::AbstractSceneFactory {
	public:
		/// <summary>シーンを作る。該当する名前がなければ nullptr</summary>
		std::unique_ptr<RyoEngine::BaseScene> CreateScene(const std::string& sceneName) override;
	};
}

#pragma once
#include <RyoEngine.h>

// 各シーン
#include "InstancingDemo.h"
#include "Showcase.h"

namespace App {
	/// <summary>
	/// シーンの登録と、起動時のシーンの指定 (Application::Initialize から1回だけ呼ぶ)
	/// シーンを増やしたら、ここに RegisterScene を1行足す
	/// </summary>
	inline void SetupScenes() {
		// 各シーンの登録 (ImGui のシーン一覧には、クラス名で出る)
		RyoEngine::SceneManager::RegisterScene<InstancingDemo>();
		RyoEngine::SceneManager::RegisterScene<Showcase>();

		// 起動時のシーン
		RyoEngine::SceneManager::SetDefaultScene<InstancingDemo>();
	}
}

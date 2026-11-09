#pragma once
#include <RyoEngine.h>

// 各シーン
#include "Title.h"
#include "Game.h"

namespace App {
	/// <summary>
	/// シーンの登録と、起動時のシーンの指定 (Application::Initialize から1回だけ呼ぶ)
	/// シーンを増やしたら、ここに RegisterScene を1行足す
	/// </summary>
	inline void SetupScenes() {
		// 各シーンの登録 (ImGui のシーン一覧には、クラス名で出る)
		RyoEngine::SceneManager::RegisterScene<Title>();
		RyoEngine::SceneManager::RegisterScene<Game>();

		// 起動時のシーン
		RyoEngine::SceneManager::SetDefaultScene<Title>();
	}
}

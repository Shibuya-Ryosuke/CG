#pragma once
#include <RyoEngine.h>

namespace App {
	/// <summary>
	/// ゲームシーン
	/// </summary>
	class Game : public RyoEngine::BaseScene {
	public:
		Game() = default;
		~Game() override = default;

		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;

		void Initialize() override;
		void Update() override;
		void Draw() override;
	};
}

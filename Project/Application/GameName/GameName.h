#pragma once
#include <RyoEngine.h>
#include <memory>

#include "Scene/SceneManager.h"

namespace Game1 {
	class Game1 : public RyoEngine::IGame {
	public:
		// コンストラクタ、デストラクタ
		Game1() = default;
		~Game1() override = default;
		// コピーコンストラクタとコピー代入演算子の禁止
		Game1(const Game1&) = delete;
		Game1& operator=(const Game1&) = delete;


		void Initialize() override;
		void Update() override;
		void Draw() override;
		void Finalize() override;

	private:
		std::unique_ptr<SceneManager> sceneManager_ = nullptr;
	};
}
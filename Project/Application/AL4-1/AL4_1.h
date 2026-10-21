#pragma once
#include <RyoEngine.h>
#include <memory>

#include "Scene/SceneManager.h"

namespace AL4_1 {
	class AL4_1 : public RyoEngine::IGame {
	public:
		// コンストラクタ、デストラクタ
		AL4_1() = default;
		~AL4_1() override = default;
		// コピーコンストラクタとコピー代入演算子の禁止
		AL4_1(const AL4_1&) = delete;
		AL4_1& operator=(const AL4_1&) = delete;


		void Initialize() override;
		void Update() override;
		void Draw() override;
		void Finalize() override;

	private:
		std::unique_ptr<SceneManager> sceneManager_ = nullptr;

		bool stop = false;
	};
}
#pragma once
#include "../Core/Base/IGame.h"

#include "TestManager.h"

namespace Test {
	class Test : public RyoEngine::IGame {
	public:
		Test() = default;
		// コピーコンストラクタ、代入演算子の明示的削除
		Test(const Test&) = delete;
		Test& operator=(const Test&) = delete;

		void Initialize() override;
		void Update() override;
		void Draw() override;
		void Finalize() override;

	private:
		std::unique_ptr<TestManager> testManager_ = nullptr;
	};
}
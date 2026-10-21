#pragma once
#include <RyoEngine.h>

namespace AL4_1 {
	class Title {
	public:
		void Initialize();
		void Update();
		void Draw();
		void Finalize();

		// シーンに入るときに呼ぶ
		void Start() { isFinished_ = false; }
		// ゲームへ進みたいか
		bool IsFinished() const { return isFinished_; }

	private:
		bool isFinished_ = false;
	};
}

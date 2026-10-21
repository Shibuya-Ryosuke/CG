#include "Result.h"

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	void Result::Initialize() {}

	void Result::Update() {
		// Spaceでタイトルへ
		if (Input::TriggerKey(DIK_SPACE)) {
			isFinished_ = true;
		}
	}

	void Result::Draw() {
		// TODO: クリアのUI（CLEAR、Press Space）
	}

	void Result::Finalize() {}
}

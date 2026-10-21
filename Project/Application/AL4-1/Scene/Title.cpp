#include "Title.h"

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	void Title::Initialize() {}

	void Title::Update() {
		// Spaceでゲームへ
		if (Input::TriggerKey(DIK_SPACE)) {
			isFinished_ = true;
		}
	}

	void Title::Draw() {
		// TODO: タイトルのUI（ロゴ、Press Space）
	}

	void Title::Finalize() {}
}

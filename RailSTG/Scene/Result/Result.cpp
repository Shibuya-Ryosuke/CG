#include "Result.h"

using namespace RyoEngine;

void Result::Initialize() {
	clear_.Initialize("resources/RailSTG/UI/Result/clear.png");
	failed_.Initialize("resources/RailSTG/UI/Result/failed.png");
}

void Result::Update() {
	if (isClear_) {
		clear_.Update();
	} else {
		failed_.Update();
	}
}

void Result::Draw() {
	if (isClear_) {
		clear_.Draw();
	} else {
		failed_.Draw();
	}
}
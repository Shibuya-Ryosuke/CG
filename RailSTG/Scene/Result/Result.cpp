#include "Result.h"

using namespace RyoEngine;


void Result::Initialize() {
	clear_.Initialize("resources/RailSTG/UI/Result/clear.png");
	failed_.Initialize("resources/RailSTG/UI/Result/failed.png");

	numbers_.at(0) = LoadTex("resources/RailSTG/UI/Number/0.png");
	numbers_.at(1) = LoadTex("resources/RailSTG/UI/Number/1.png");
	numbers_.at(2) = LoadTex("resources/RailSTG/UI/Number/2.png");
	numbers_.at(3) = LoadTex("resources/RailSTG/UI/Number/3.png");
	numbers_.at(4) = LoadTex("resources/RailSTG/UI/Number/4.png");
	numbers_.at(5) = LoadTex("resources/RailSTG/UI/Number/5.png");
	numbers_.at(6) = LoadTex("resources/RailSTG/UI/Number/6.png");
	numbers_.at(7) = LoadTex("resources/RailSTG/UI/Number/7.png");
	numbers_.at(8) = LoadTex("resources/RailSTG/UI/Number/8.png");
	numbers_.at(9) = LoadTex("resources/RailSTG/UI/Number/9.png");

	spawnEnemies_.Initialize(numbers_.at(0));
	destroyEnemies_.Initialize(numbers_.at(0));
	justEvasionNumber_.Initialize(numbers_.at(0));
	for (auto& hp : remainingHp_) {
		hp.Initialize(numbers_.at(0));
	}
	for (auto& time : clearTime_) {
		time.Initialize(numbers_.at(0));
	}
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
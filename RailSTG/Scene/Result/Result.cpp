#include "Result.h"

using namespace RyoEngine;

void Result::Initialize() {
	result_.Initialize("resources/RailSTG/UI/Result/result.png");
}

void Result::Update() {
	result_.Update();
}

void Result::Draw() {
	result_.Draw();
}
#include "Title.h"

using namespace RyoEngine;

void Title::Initialize() {
	title_.Initialize("resources/RailSTG/UI/Title/title.png");
}

void Title::Update() {
	title_.Update();
}

void Title::Draw() {
	title_.Draw();
}
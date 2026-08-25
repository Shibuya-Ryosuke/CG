#include "Title.h"

using namespace RyoEngine;

void Title::Initialize() {
	title_.Initialize("resources/RailSTG/UI/Title/title.png");
	pressSpace_.Initialize("resources/RailSTG/UI/Input/pressSpace.png",{640.0f,450.0f},Anchor::Center);
}

void Title::Update() {
	title_.Update();
	pressSpace_.Update();
}

void Title::Draw() {
	title_.Draw();
	pressSpace_.Draw();
}
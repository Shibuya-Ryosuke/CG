#include "Title.h"
#include "../../../Original/RyoEngine.h"

using namespace RyoEngine;

void Title::Initialize() {}

void Title::Update() {}

void Title::Draw() {
	// 画面全体を覆う紫の矩形 ※中心座標・サイズは実際の解像度に合わせて調整
	PrimitiveRenderer::DrawRect2D(
		{ 640.0f, 360.0f },
		{ 1280.0f, 720.0f },
		0.0f,
		{ 0.5f, 0.0f, 0.5f, 1.0f }, // 紫
		PrimitiveDrawMode::Fill // ※実際のenum値名に合わせて調整
	);
}
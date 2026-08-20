#include "Result.h"
#include "../../../Original/RyoEngine.h"

using namespace RyoEngine;

void Result::Initialize() {}

void Result::Update() {}

void Result::Draw() {
	// 画面全体を覆う黄の矩形 ※中心座標・サイズは実際の解像度に合わせて調整
	PrimitiveRenderer::DrawRect2D(
		{ 640.0f, 360.0f },
		{ 1280.0f, 720.0f },
		0.0f,
		{ 1.0f, 1.0f, 0.0f, 1.0f }, // 黄色
		PrimitiveDrawMode::Fill // ※実際のenum値名に合わせて調整
	);
}
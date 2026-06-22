#include "AxisIndicator.h"
using namespace RyoEngine;

AxisIndicator::AxisIndicator() {
	isVisible_ = false;
	model_ = Model::Create("resources/AL3-03/axisIndicator.obj");
	light_.color = { 1.0f,1.0f,1.0f,1.0f };
	light_.direction = { 0.0f,0.5f,0.5f };
	light_.intensity = 10.0f;
	model_->SetScale({ 6.0f,6.0f,6.0f });
	model_->SetDirectionalLight(light_);
}
void AxisIndicator::Initialize() {
	AxisIndicator();
}

void AxisIndicator::Update(RyoEngine::Camera& camera) {
	Vector3 cameraRotate{
		camera.GetRotate().x,
		-camera.GetRotate().y,
		-camera.GetRotate().z,
	};
	model_->SetRotate(cameraRotate);

	Matrix4x4 dummyView = MakeIdentity4x4();

	float left = -640.0f;
	float right = 640.0f;
	float top = 360.0f;
	float bottom = -360.0f;
	float nearZ = 0.1f;
	float farZ = 1000.0f;

	Matrix4x4 dummyProj = MakeIdentity4x4();
	dummyProj.m[0][0] = 2.0f / (right - left);
	dummyProj.m[1][1] = 2.0f / (top - bottom);
	dummyProj.m[2][2] = 1.0f / (farZ - nearZ);
	dummyProj.m[3][0] = -(right + left) / (right - left);
	dummyProj.m[3][1] = -(top + bottom) / (top - bottom);
	dummyProj.m[3][2] = -nearZ / (farZ - nearZ);

	model_->SetTranslate({ 550.0f, 270.0f, 10.0f });

	// 4. ダミーカメラにセットして更新
	Camera dummyCamera{};
	// 回転が含まれないビュー行列（単位行列）と、平行投影を渡す
	dummyCamera.SetCustomMatrices(dummyView, dummyProj);

	model_->Update(dummyCamera);
}

void AxisIndicator::Update(RyoEngine::DebugCamera& debugCamera) {
	Vector3 cameraRotate{
		debugCamera.GetRotate().x,
		-debugCamera.GetRotate().y,
		-debugCamera.GetRotate().z,
	};
	model_->SetRotate(cameraRotate);

	Matrix4x4 dummyView = MakeIdentity4x4();

	float left = -640.0f;
	float right = 640.0f;
	float top = 360.0f;
	float bottom = -360.0f;
	float nearZ = 0.1f;
	float farZ = 1000.0f;

	Matrix4x4 dummyProj = MakeIdentity4x4();
	dummyProj.m[0][0] = 2.0f / (right - left);
	dummyProj.m[1][1] = 2.0f / (top - bottom);
	dummyProj.m[2][2] = 1.0f / (farZ - nearZ);
	dummyProj.m[3][0] = -(right + left) / (right - left);
	dummyProj.m[3][1] = -(top + bottom) / (top - bottom);
	dummyProj.m[3][2] = -nearZ / (farZ - nearZ);

	model_->SetTranslate({ 550.0f, 270.0f, 10.0f });

	// 4. ダミーカメラにセットして更新
	Camera dummyCamera{};
	// 回転が含まれないビュー行列（単位行列）と、平行投影を渡す
	dummyCamera.SetCustomMatrices(dummyView, dummyProj);

	model_->Update(dummyCamera);
}

void AxisIndicator::Draw() {
	if (!isVisible_) return;

	model_->Draw();
}

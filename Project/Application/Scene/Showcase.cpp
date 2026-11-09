#include "Showcase.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace RyoEngine;

namespace App {

	// =====================================================================
	// 初期化・終了
	// =====================================================================
	void Showcase::Initialize() {
		CameraManager::GetDebugCamera().SetAvailable(false);
		// --- モデル (エンジンのテスト用素材を借りる) ---
		ground_ = Model::Create("Resources/EngineResources/Test/ground.obj");
		sky_ = Model::Create("Resources/EngineResources/Test/skydome.obj");
		sky_->SetBackground(true);
		sky_->SetEnableLighting(false);

		// --- カメラ ---
		camera_ = CameraManager::CreateCamera("Showcase");

		// --- エフェクトから追従される座標 ---
		BindingRegistry::RegisterPosition(ParticleTemplates::CenterKey(), &center_);
		BindingRegistry::RegisterPosition(ParticleTemplates::TargetKey(), &target_);

		// --- 再生する演出の順番 (0番はチャージ→爆発) ---
		demoNames_ = { "Charge", "Burst", "Fireworks", "Vortex", "Homing", "Curve", "Electric", "Sparks", "Absorb", "RingWave","Original" };

		RegisterParams();
	}

	void Showcase::Finalize() {
		ParticleManager::StopAll();
		BindingRegistry::Unregister(ParticleTemplates::CenterKey());
		BindingRegistry::Unregister(ParticleTemplates::TargetKey());
		SetTimeScale(1.0f);
	}

	void Showcase::RegisterParams() {
		ParamEditor::BeginGroup("【Showcase】カメラ");
		ParamEditor::RegisterValue("距離", &cameraDistance_, 0.1f, 1.0f, 200.0f);
		ParamEditor::RegisterValue("見下ろす角度(ラジアン)", &cameraPitch_, 0.005f, -1.5f, 1.5f);
		ParamEditor::RegisterValue("回る速さ(ラジアン/秒)", &cameraOrbitSpeed_, 0.005f, -3.0f, 3.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【Showcase】動く目標");
		ParamEditor::RegisterValue("中心", &center_, 0.05f, -100.0f, 100.0f);
		ParamEditor::RegisterValue("回る半径", &targetOrbitRadius_, 0.05f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("回る速さ(ラジアン/秒)", &targetOrbitSpeed_, 0.01f, -10.0f, 10.0f);
		ParamEditor::RegisterValue("高さ", &targetHeight_, 0.05f, -100.0f, 100.0f);
		ParamEditor::RegisterValue("上下の揺れ", &targetBobAmplitude_, 0.01f, 0.0f, 20.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【Showcase】演出");
		ParamEditor::RegisterFlag("自動で順番に再生", &autoDemo_);
		ParamEditor::RegisterValue("自動再生の間隔(秒)", &autoDemoInterval_, 0.01f, 0.5f, 30.0f);
		ParamEditor::RegisterValue("チャージの時間(秒)", &sequenceChargeTime_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("ヒットストップの時間(秒)", &hitStopDuration_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("ヒットストップ中の速さ", &hitStopScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("スローの速さ", &slowScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("【Showcase】見た目");
		ParamEditor::RegisterFlag("地面を表示", &showGround_);
		ParamEditor::RegisterColor("空の色", &skyColor_);
		ParamEditor::RegisterColor("地面の色", &groundColor_);
		ParamEditor::RegisterFlag("ブルーム", &bloomEnabled_);
		ParamEditor::RegisterValue("ブルームのしきい値", &bloomThreshold_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("ブルームの強さ", &bloomIntensity_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("露出", &exposure_, 0.01f, 0.0f, 10.0f);
		ParamEditor::EndGroup();
	}

	// =====================================================================
	// 更新
	// =====================================================================
	void Showcase::Update() {
		// カメラ・ヒットストップはスローの影響を受けない時間で、目標はスローに合わせて動かす
		const float unscaledDeltaTime = GetDeltaTime();
		const float scaledDeltaTime = GetScaleTime();

		UpdateInput();
		UpdateSequence(unscaledDeltaTime);
		UpdateAutoDemo(unscaledDeltaTime);
		ApplyTimeScale(unscaledDeltaTime);
		UpdateTarget(scaledDeltaTime);
		UpdateCamera(unscaledDeltaTime);
		ApplyPostProcess();

		ground_->SetColor(groundColor_);
		sky_->SetColor(skyColor_);
		ground_->TransferMatrix();
		sky_->TransferMatrix();

		PrintHelp();
		// NOTE: ParticleManager の Update / Draw はエンジンが毎フレーム呼んでいる
	}

	void Showcase::UpdateInput() {
		// [1]〜[9], [0] で各演出
		const BYTE demoKeys[] = { DIK_1, DIK_2, DIK_3, DIK_4, DIK_5, DIK_6, DIK_7, DIK_8, DIK_9, DIK_0, DIK_MINUS };
		const uint32_t keyCount = static_cast<uint32_t>(sizeof(demoKeys) / sizeof(demoKeys[0]));
		for (uint32_t i = 0; i < keyCount; ++i) {
			if (Input::TriggerKey(demoKeys[i])) {
				autoDemo_ = false; // 手で選んだら自動再生は止める
				PlayDemo(i);
			}
		}

		if (Input::TriggerKey(DIK_A)) {
			autoDemo_ = !autoDemo_;
			autoDemoTimer_ = 0.0f;
		}
		if (Input::TriggerKey(DIK_T)) {
			slowMotion_ = !slowMotion_;
		}
		if (Input::TriggerKey(DIK_C)) {
			ParticleManager::ClearAll();
			sequenceActive_ = false;
		}
		if (Input::TriggerKey(DIK_G)) {
			showGround_ = !showGround_;
		}
	}

	void Showcase::UpdateCamera(float deltaTime) {
		cameraYaw_ += cameraOrbitSpeed_ * deltaTime;

		// rotate = (見下ろす角度, 向き, 0)。カメラは中心を向いたまま、まわりを回る
		const float cosPitch = std::cos(cameraPitch_);
		const float sinPitch = std::sin(cameraPitch_);
		const float cosYaw = std::cos(cameraYaw_);
		const float sinYaw = std::sin(cameraYaw_);
		const Vector3 forward = { cosPitch * sinYaw, -sinPitch, cosPitch * cosYaw };

		Camera* camera = CameraManager::GetCamera(camera_);
		if (!camera) {
			return;
		}
		camera->SetTranslate(center_ - forward * cameraDistance_);
		camera->SetRotate({ cameraPitch_, cameraYaw_, 0.0f });
	}

	void Showcase::UpdateTarget(float deltaTime) {
		targetAngle_ += targetOrbitSpeed_ * deltaTime;
		const float cosAngle = std::cos(targetAngle_);
		const float sinAngle = std::sin(targetAngle_);
		const float bob = std::sin(targetAngle_ * 2.3f) * targetBobAmplitude_;
		target_.x = center_.x + cosAngle * targetOrbitRadius_;
		target_.y = targetHeight_ + bob;
		target_.z = center_.z + sinAngle * targetOrbitRadius_;
	}

	void Showcase::UpdateAutoDemo(float deltaTime) {
		if (!autoDemo_ || demoNames_.empty()) {
			return;
		}
		autoDemoTimer_ -= deltaTime;
		if (autoDemoTimer_ > 0.0f) {
			return;
		}
		autoDemoTimer_ = autoDemoInterval_;

		PlayDemo(autoDemoIndex_);
		autoDemoIndex_ = (autoDemoIndex_ + 1) % static_cast<uint32_t>(demoNames_.size());
	}

	void Showcase::PlayDemo(uint32_t index) {
		if (index >= demoNames_.size()) {
			return;
		}
		// 前の演出の「出し続け」だけ止める (出ている粒は寿命まで残す)
		if (!currentDemo_.empty()) {
			ParticleManager::Stop(currentDemo_);
		}

		if (index == 0) {
			StartSequence();
		} else {
			ParticleManager::Play(demoNames_[index]);
		}
		currentDemo_ = demoNames_[index];
	}

	void Showcase::StartSequence() {
		// チャージ (外周から弧を描いて収束) → 爆発
		ParticleManager::Play("Charge");
		sequenceActive_ = true;
		sequenceTimer_ = sequenceChargeTime_;
	}

	void Showcase::UpdateSequence(float deltaTime) {
		if (!sequenceActive_) {
			return;
		}
		// スロー中はチャージもゆっくりになるので、タイマーもタイムスケールに合わせる
		sequenceTimer_ -= deltaTime * GetTimeScale();
		if (sequenceTimer_ > 0.0f) {
			return;
		}
		sequenceActive_ = false;

		// 爆発 (放射の火花 + 花火 + 衝撃波の輪) は、Charge の「連鎖」でパーティクル側が自動で再生する。
		// ここでは、それに合わせたヒットストップだけ行う (チャージの時間は連鎖の秒数と合わせておくこと)
		hitStopTimer_ = hitStopDuration_;
	}

	void Showcase::ApplyTimeScale(float deltaTime) {
		float scale = slowMotion_ ? slowScale_ : 1.0f;
		if (hitStopTimer_ > 0.0f) {
			hitStopTimer_ -= deltaTime;
			scale *= hitStopScale_;
		}
		SetTimeScale(scale);
	}

	void Showcase::ApplyPostProcess() {
		PostProcess::SetACESEnabled(true);
		PostProcess::SetExposure(exposure_);
		PostProcess::SetBloomEnabled(bloomEnabled_);
		PostProcess::SetBloomThreshold(bloomThreshold_);
		PostProcess::SetBloomIntensity(bloomIntensity_);
	}

	void Showcase::PrintHelp() {
		const std::string current = currentDemo_.empty() ? std::string("-") : currentDemo_;
		PrintText("[ 1~9, 0 , - ] Effects  [A] Auto:{}  [T] Slow:{}  [C] Clear  [G] Ground", Vector2{ 20.0f, 80.0f },
			autoDemo_ ? "ON" : "OFF", slowMotion_ ? "ON" : "OFF");
		PrintText("Now: {}", Vector2{ 20.0f, 110.0f }, current);
	}

	// =====================================================================
	// 描画
	// =====================================================================
	void Showcase::Draw() {
		sky_->Draw();
		if (showGround_) {
			ground_->Draw();
		}
	}
}

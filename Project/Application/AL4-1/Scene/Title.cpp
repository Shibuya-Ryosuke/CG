#include "Title.h"
#include "../GameResources/GameTex.h"
#include "../GameResources/GameSound.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	void Title::Initialize() {
		camera_.Initialize();

		// ロゴとPRESS SPACE（中央を基準にする）
		logo_ = make_unique<Sprite>();
		logo_->Initialize(GameImage::GetImage(GameImage::Image::TitleLogo), logoPosition_, Anchor::Center);
		pressSpace_ = make_unique<Sprite>();
		pressSpace_->Initialize(GameImage::GetImage(GameImage::Image::PressSpace), pressSpacePosition_, Anchor::Center);
		pressButton_ = make_unique<Sprite>();
		pressButton_->Initialize(GameImage::GetImage(GameImage::Image::PressButton), pressButtonPosition_, Anchor::Center);
		controllerSupport_ = make_unique<Sprite>();
		controllerSupport_->Initialize(GameImage::GetImage(GameImage::Image::TextControllerSupport), controllerSupportPosition_, Anchor::Center);

		RegisterInParamEditor();
	}

	void Title::RegisterInParamEditor() {
		ParamEditor::BeginGroup("Title");
		ParamEditor::RegisterValue("cameraTranslate", &cameraTranslate_, 0.1f, -500.0f, 500.0f);
		ParamEditor::RegisterValue("cameraRotate", &cameraRotate_, 0.5f, -180.0f, 180.0f);
		ParamEditor::RegisterValue("logoPosition", &logoPosition_, 1.0f, -2000.0f, 4000.0f);
		ParamEditor::RegisterValue("logoScale", &logoScale_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("flickerInterval", &flickerInterval_, 0.01f, 0.05f, 10.0f);
		ParamEditor::RegisterValue("flickerTime", &flickerTime_, 0.01f, 0.0f, 2.0f);
		ParamEditor::RegisterValue("flickerOffset", &flickerOffset_, 0.1f, 0.0f, 50.0f);
		ParamEditor::RegisterValue("flickerMinAlpha", &flickerMinAlpha_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("pressSpacePosition", &pressSpacePosition_, 1.0f, -2000.0f, 4000.0f);
		ParamEditor::RegisterValue("pressSpaceScale", &pressSpaceScale_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("blinkSpeed", &blinkSpeed_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("blinkMinAlpha", &blinkMinAlpha_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("pressButtonPosition", &pressButtonPosition_, 1.0f, -2000.0f, 4000.0f);
		ParamEditor::RegisterValue("pressButtonScale", &pressButtonScale_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("debugDevice", &debugDevice_, 1.0f, 0, 3);
		ParamEditor::RegisterValue("controllerSupportPosition", &controllerSupportPosition_, 1.0f, -2000.0f, 4000.0f);
		ParamEditor::RegisterValue("controllerSupportScale", &controllerSupportScale_, 0.01f, 0.0f, 10.0f);
		ParamEditor::EndGroup();
	}

	void Title::Update() {
		const float dt = GetDeltaTime();
		timer_ += dt;

		// --- カメラ（ParamEditorの値をそのまま使う） ---
		constexpr float degToRad = std::numbers::pi_v<float> / 180.0f;
		camera_.SetTranslate(cameraTranslate_);
		camera_.SetRotate({ cameraRotate_.x * degToRad, cameraRotate_.y * degToRad, cameraRotate_.z * degToRad });
		camera_.Update();

		// --- ロゴ：ときどきチラつく（パリパリ） ---
		flickerCooldown_ -= dt;
		if (flickerCooldown_ <= 0.0f) {
			flickerTimer_ = flickerTime_;
			flickerCooldown_ = flickerInterval_ * RandomFloat(0.5f, 1.5f);
		}
		Vector2 logoPos = logoPosition_;
		float logoAlpha = 1.0f;
		if (flickerTimer_ > 0.0f) {
			flickerTimer_ -= dt;
			logoPos.x += RandomFloat(-flickerOffset_, flickerOffset_);
			logoPos.y += RandomFloat(-flickerOffset_ * 0.5f, flickerOffset_ * 0.5f);
			logoAlpha = RandomFloat(flickerMinAlpha_, 1.0f);
		}
		if (logo_) {
			logo_->SetTranslate(logoPos);
			logo_->SetScale(logoScale_);
			logo_->SetColor({ 1.0f,1.0f,1.0f,logoAlpha });
			logo_->TransferMatrix();
		}

		// --- PRESS SPACE：ゆっくり点滅 ---
		const float wave = (std::cos(timer_ * blinkSpeed_ * 2.0f * std::numbers::pi_v<float>) + 1.0f) * 0.5f;
		const float pressAlpha = blinkMinAlpha_ + (1.0f - blinkMinAlpha_) * wave;
		if (pressSpace_) {
			pressSpace_->SetTranslate(pressSpacePosition_);
			pressSpace_->SetScale(pressSpaceScale_);
			pressSpace_->SetColor({ 1.0f,1.0f,1.0f,pressAlpha });
			pressSpace_->TransferMatrix();
		}
		if (pressButton_) {
			pressButton_->SetTranslate(pressButtonPosition_);
			pressButton_->SetScale(pressButtonScale_);
			pressButton_->SetColor({ 1.0f,1.0f,1.0f,pressAlpha });
			pressButton_->TransferMatrix();
		}
		// --- コントローラー対応：点滅させずにそのまま出す ---
		if (controllerSupport_) {
			controllerSupport_->SetTranslate(controllerSupportPosition_);
			controllerSupport_->SetScale(controllerSupportScale_);
			controllerSupport_->TransferMatrix();
		}

		// Spaceでゲームへ
		// （ゲームパッドはAかSTART）
		const bool isPadDecide = Input::GetJoystickTrigger(XINPUT_GAMEPAD_A) || Input::GetJoystickTrigger(XINPUT_GAMEPAD_START);
		if (isInputEnabled_ && (Input::TriggerKey(DIK_SPACE) || isPadDecide)) {
			isFinished_ = true;
			GameSound::PlaySE(GameSound::SE::Decide);
		}
	}

	void Title::Draw() {
		if (logo_) logo_->Draw();

		// 最後に触ったデバイスで出し分ける
		int32_t mode = Input::GetDeviceType() == Input::DeviceType::Gamepad ? 2 : 1;
#ifdef _DEBUG
		if (debugDevice_ != 0) { mode = debugDevice_; }
#endif
		if ((mode == 1 || mode == 3) && pressSpace_) pressSpace_->Draw();
		if ((mode == 2 || mode == 3) && pressButton_) pressButton_->Draw();
		// コントローラー対応（デバイスに関係なくいつも出す）
		if (controllerSupport_) controllerSupport_->Draw();
	}

	void Title::Finalize() {
		if (logo_) {
			logo_->Finalize();
			logo_.reset();
		}
		if (pressSpace_) {
			pressSpace_->Finalize();
			pressSpace_.reset();
		}
		if (pressButton_) {
			pressButton_->Finalize();
			pressButton_.reset();
		}
		if (controllerSupport_) {
			controllerSupport_->Finalize();
			controllerSupport_.reset();
		}
	}
}
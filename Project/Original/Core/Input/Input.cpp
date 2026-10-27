#include "Input.h"
#include "../../Core/Base/Logger.h"
#include <cassert>
#include <algorithm>

namespace RyoEngine {
	Input* Input::GetInstance() {
		static Input instance;
		return &instance;
	}

	Input::~Input() {
		// 終了時に振動が残らないように止める
		if (isVibrating_) {
			ApplyVibration(0.0f, 0.0f);
		}
	}

	void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {
		Logger::Log("Input : Initializing...\n");
		// インスタンス取得
		Input* instance = GetInstance();

		// 全体初期化
		HRESULT result = DirectInput8Create(
			hInstance, DIRECTINPUT_VERSION,
			IID_IDirectInput8, (void**)(instance->directInput_.GetAddressOf()),
			nullptr
		);
		assert(SUCCEEDED(result));


		// キーボードデバイス
		result = instance->directInput_->CreateDevice(GUID_SysKeyboard, &instance->keyboard_, NULL);
		assert(SUCCEEDED(result));
		// 入力データ形式セット
		result = instance->keyboard_->SetDataFormat(&c_dfDIKeyboard);
		assert(SUCCEEDED(result));
		// 排他制御レベルセット
		result = instance->keyboard_->SetCooperativeLevel(
			hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
		assert(SUCCEEDED(result));


		// マウスデバイス
		result = instance->directInput_->CreateDevice(GUID_SysMouse, &instance->mouse_, NULL);
		assert(SUCCEEDED(result));
		// 入力データ形式セット
		result = instance->mouse_->SetDataFormat(&c_dfDIMouse);
		assert(SUCCEEDED(result));
		// 排他制御レベルセット
		result = instance->mouse_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
		assert(SUCCEEDED(result));

		Logger::LogSuccess("Input : Initialized\n");
	}

	void Input::Update() {
		// インスタンス取得
		Input* instance = GetInstance();

		// ----- キーボード -----
		// 前フレームの状態退避
		memcpy(instance->preKeys_, instance->keys_, sizeof(keys_));
		// デバイス取得
		HRESULT hrK = instance->keyboard_->Poll();

		if (FAILED(hrK)) {
			instance->keyboard_->Acquire();
		} else {
			// 入力状態の取得
			instance->keyboard_->GetDeviceState(sizeof(keys_), instance->keys_);
		}

		// ----- マウス -----
		// 前フレーム状態
		instance->preMouseState_ = instance->mouseState_;
		// デバイス取得
		HRESULT hrM = instance->mouse_->Poll();
		if (FAILED(hrM)) {
			instance->mouse_->Acquire();
		} else {
			instance->mouse_->GetDeviceState(sizeof(DIMOUSESTATE), &instance->mouseState_);
		}

		// ----- コントローラー -----
		// 状態保存
		instance->joyStatePrevious_ = instance->joyState_;
		// 0番目のコントローラーの状態取得
		DWORD dwResult = XInputGetState(0, &instance->joyState_);
		instance->isConnected_ = (dwResult == ERROR_SUCCESS);

		// デバイスタイプ判定ロジック
		// 1. ゲームパッドの入力判定（ボタンが押された、またはスティックが倒されたか）
		if (instance->isConnected_) {
			// ボタンのトリガー（押された瞬間）をチェック
			WORD currentButtons = instance->joyState_.Gamepad.wButtons;
			WORD prevButtons = instance->joyStatePrevious_.Gamepad.wButtons;
			bool buttonPressed = (currentButtons != prevButtons) && (currentButtons != 0);

			// スティックの傾きをチェック（デッドゾーン考慮）
			float lx = (float)instance->joyState_.Gamepad.sThumbLX / 32768.0f;
			float ly = (float)instance->joyState_.Gamepad.sThumbLY / 32768.0f;
			float rx = (float)instance->joyState_.Gamepad.sThumbRX / 32768.0f;
			float ry = (float)instance->joyState_.Gamepad.sThumbRY / 32768.0f;
			bool stickMoved = (std::abs(lx) > 0.2f || std::abs(ly) > 0.2f || std::abs(rx) > 0.2f || std::abs(ry) > 0.2f);

			if (buttonPressed || stickMoved) {
				instance->deviceType_ = DeviceType::Gamepad;
			}
		}

		// 2. キーボード・マウスの入力判定
		// キーボードのいずれかが押されたか
		for (int i = 0; i < 256; ++i) {
			if ((instance->keys_[i] & 0x80) && !(instance->preKeys_[i] & 0x80)) {
				instance->deviceType_ = DeviceType::Keyboard;
				break;
			}
		}
		// マウスのクリックや移動・ホイールをチェック
		for (int i = 0; i < 3; ++i) {
			if ((instance->mouseState_.rgbButtons[i] & 0x80) && !(instance->preMouseState_.rgbButtons[i] & 0x80)) {
				instance->deviceType_ = DeviceType::Keyboard;
				break;
			}
		}
		// 0だと判定が厳しすぎるようならstd::abs(lX) > 2などで緩める
		if (instance->mouseState_.lX != 0 || instance->mouseState_.lY != 0 || instance->mouseState_.lZ != 0) {
			instance->deviceType_ = DeviceType::Keyboard;
		}

		// ----- 振動 -----
		// 時間が過ぎたら止める（コントローラーが外れたときも止めた扱いにする）
		if (instance->isVibrating_) {
			if (!instance->isConnected_ || std::chrono::steady_clock::now() >= instance->vibrationEndTime_) {
				StopVibration();
			}
		}
	}

	void Input::Vibrate(float low, float high, float duration) {
		Input* instance = GetInstance();
		if (!instance->isConnected_) return;

		low = std::clamp(low, 0.0f, 1.0f);
		high = std::clamp(high, 0.0f, 1.0f);
		const auto now = std::chrono::steady_clock::now();
		const auto endTime = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
			std::chrono::duration<float>((std::max)(duration, 0.0f)));

		// 振動中なら、強い方・長い方で上書きする（小さい振動で大きい振動が途切れないように）
		if (instance->isVibrating_ && now < instance->vibrationEndTime_) {
			low = (std::max)(low, instance->vibrationLow_);
			high = (std::max)(high, instance->vibrationHigh_);
			instance->vibrationEndTime_ = (std::max)(endTime, instance->vibrationEndTime_);
		} else {
			instance->vibrationEndTime_ = endTime;
		}
		instance->vibrationLow_ = low;
		instance->vibrationHigh_ = high;
		instance->isVibrating_ = true;
		instance->ApplyVibration(low, high);
	}

	void Input::StopVibration() {
		Input* instance = GetInstance();
		instance->isVibrating_ = false;
		instance->vibrationLow_ = 0.0f;
		instance->vibrationHigh_ = 0.0f;
		instance->ApplyVibration(0.0f, 0.0f);
	}

	void Input::ApplyVibration(float low, float high) {
		XINPUT_VIBRATION vibration{};
		vibration.wLeftMotorSpeed = static_cast<WORD>(low * 65535.0f);
		vibration.wRightMotorSpeed = static_cast<WORD>(high * 65535.0f);
		// つながっていないときは失敗するだけなので、戻り値は見ない
		XInputSetState(0, &vibration);
	}
}
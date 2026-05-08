#pragma once
#define DIRECTINPUT_VERSION 0x0800  // DirectInputのバージョン指定
#include <Windows.h>
#include <dinput.h>
#include <wrl.h>
#include <Xinput.h>
#include <cmath>

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"xinput.lib")

namespace Engine {
	class Input {
	public:

		static void Initialize(HINSTANCE hInstance, HWND hwnd);

		static void Update();

        // --- キーボード ---
        static bool PushKey(BYTE keyNumber) {
            return GetInstance()->keys_[keyNumber] & 0x80;
        }

        static bool TriggerKey(BYTE keyNumber) {
            return (GetInstance()->keys_[keyNumber] & 0x80) && !(GetInstance()->preKeys_[keyNumber] & 0x80);
        }

        // --- マウス ---
        static bool IsMousePush(int buttonNumber) {
            // 0:左, 1:右, 2:中
            return GetInstance()->mouseState_.rgbButtons[buttonNumber] & 0x80;
        }

        static long GetMouseRelX() {
            return GetInstance()->mouseState_.lX;
        }

        static long GetMouseRelY() {
            return GetInstance()->mouseState_.lY;
        }

        static long GetMouseWheel() {
            return GetInstance()->mouseState_.lZ;
        }

        // --- コントローラー (ボタン) ---
        static bool GetJoystickButton(WORD button) {
            if (!GetInstance()->isConnected_) return false;
            return (GetInstance()->joyState_.Gamepad.wButtons & button);
        }

        static bool GetJoystickTrigger(WORD button) {
            Input* instance = GetInstance();
            if (!instance->isConnected_) return false;
            return (instance->joyState_.Gamepad.wButtons & button) &&
                !(instance->joyStatePrevious_.Gamepad.wButtons & button);
        }

        // --- コントローラー (スティック) ---
        static float GetLeftStickX() {
            if (!GetInstance()->isConnected_) return 0.0f;
            float val = (float)GetInstance()->joyState_.Gamepad.sThumbLX / 32768.0f;
            return (std::abs(val) < 0.1f) ? 0.0f : val;
        }

        static float GetLeftStickY() {
            if (!GetInstance()->isConnected_) return 0.0f;
            float val = (float)GetInstance()->joyState_.Gamepad.sThumbLY / 32768.0f;
            return (std::abs(val) < 0.1f) ? 0.0f : val;
        }

        static float GetRightStickX() {
            if (!GetInstance()->isConnected_) return 0.0f;
            float val = (float)GetInstance()->joyState_.Gamepad.sThumbRX / 32768.0f;
            return (std::abs(val) < 0.1f) ? 0.0f : val;
        }

        static float GetRightStickY() {
            if (!GetInstance()->isConnected_) return 0.0f;
            float val = (float)GetInstance()->joyState_.Gamepad.sThumbRY / 32768.0f;
            return (std::abs(val) < 0.1f) ? 0.0f : val;
        }

	private:
		Input() = default;
		~Input() = default;
		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;

		static Input* GetInstance();

		Microsoft::WRL::ComPtr<IDirectInput8> directInput_;

		Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;
		BYTE keys_[256] = {};
		BYTE preKeys_[256] = {};

		Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_;
		DIMOUSESTATE mouseState_ = {};

		XINPUT_STATE joyState_ = {};
		XINPUT_STATE joyStatePrevious_ = {};
		bool isConnected_ = false;
	};
}
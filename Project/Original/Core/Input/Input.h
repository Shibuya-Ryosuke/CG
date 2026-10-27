#pragma once
#define DIRECTINPUT_VERSION 0x0800  // DirectInputのバージョン指定
#include <Windows.h>
#include <dinput.h>
#include <wrl.h>
#include <Xinput.h>
#include <cmath>
#include <chrono>

#include "../Base/WinApp.h"
#include "../../Core/Math/Math.h"

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"xinput.lib")

namespace RyoEngine {
    class Input {
    public:
        enum class DeviceType {
            Keyboard,
            Gamepad,
        };

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
        // 0:左, 1:右, 2:中
        static bool IsMousePush(int buttonNumber) {
            return GetInstance()->mouseState_.rgbButtons[buttonNumber] & 0x80;
        }
        static bool IsMouseTrigger(int buttonNumber) {
            Input* instance = GetInstance();
            bool current = instance->mouseState_.rgbButtons[buttonNumber] & 0x80;
            bool previous = instance->preMouseState_.rgbButtons[buttonNumber] & 0x80;

            return current && !previous;
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

        static Vector2 GetMouseScreenPos() {
            POINT cursolPos;
            GetCursorPos(&cursolPos);

            HWND hwnd = WinApp::GetInstance()->GetHwnd();
            ScreenToClient(hwnd, &cursolPos);

            return { static_cast<float>(cursolPos.x),static_cast<float>(cursolPos.y) };
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

        // デバイスタイプ
        static DeviceType GetDeviceType() {
            return GetInstance()->deviceType_;
        }

        // --- コントローラー (振動) ---
        /// <summary>
        /// 振動させる。時間は実時間で計るので、スロー中でも長さは変わらない。
        /// 振動中に呼ばれたら、強い方の値と長い方の終了時刻で上書きする
        /// </summary>
        /// <param name="low">左の重いモーターの強さ（0～1）</param>
        /// <param name="high">右の軽いモーターの強さ（0～1）</param>
        /// <param name="duration">振動させる時間（秒）</param>
        static void Vibrate(float low, float high, float duration);
        // 振動を止める
        static void StopVibration();

    private:
        Input() = default;
        // 終了時にモーターが回りっぱなしにならないよう止める
        ~Input();
        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;

        static Input* GetInstance();

        Microsoft::WRL::ComPtr<IDirectInput8> directInput_;

        Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;
        BYTE keys_[256] = {};
        BYTE preKeys_[256] = {};

        Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_;
        DIMOUSESTATE mouseState_ = {};
        DIMOUSESTATE preMouseState_ = {};

        XINPUT_STATE joyState_ = {};
        XINPUT_STATE joyStatePrevious_ = {};
        bool isConnected_ = false;

        DeviceType deviceType_ = DeviceType::Keyboard;

        // 振動
        bool isVibrating_ = false;
        float vibrationLow_ = 0.0f;
        float vibrationHigh_ = 0.0f;
        std::chrono::steady_clock::time_point vibrationEndTime_{};
        // モーターに強さを送る（0～1）
        void ApplyVibration(float low, float high);
    };
}
#pragma once
#include "../../Original/RyoEngine.h"

#include "InputEnum.h"
#include <cstdint>
#include <array>

class InputManager {
public:
	InputManager() = default;
	~InputManager() = default;
	InputManager(const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;

	/// <summary>
	/// インスタンスの取得
	/// </summary>
	/// <returns>instance</returns>
	static InputManager& GetInstance() {
		static InputManager instance;
		return instance;
	}

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// デバイスの種類をセット
	/// </summary>
	/// <param name="deviceType">デバイスの種類</param>
	void SetDeviceType(DeviceType deviceType) { GetInstance().request_ = deviceType; }
	
	// ここにその行動が押されたときの関数群が追加されていく
	// IsPushMainShot() みたいな

	/// <summary>
	/// 押してる間のアクション
	/// </summary>
	/// <param name="action">アクション名</param>
	/// <returns>bool</returns>
	static bool IsPushAction(InputAction action)  {
		return RyoEngine::Input::PushKey(InputManager::GetInstance().keyBindings_[static_cast<size_t>(action)]);
	}

	/// <summary>
	/// 押した瞬間のアクション
	/// </summary>
	/// <param name="action">アクション名</param>
	/// <returns>bool</returns>
	static bool IsTriggerAction(InputAction action)  {
		return RyoEngine::Input::TriggerKey(InputManager::GetInstance().keyBindings_[static_cast<size_t>(action)]);
	}

private:
	// デバイスの種類
	DeviceType deviceType_ = DeviceType::Keyboard;
	DeviceType request_ = DeviceType::None;

	// キーの登録
	std::array<uint8_t, static_cast<size_t>(InputAction::Count)> keyBindings_ = {
		{
			DIK_W,      // MoveUp
		    DIK_S,      // MoveDown
		    DIK_A,      // MoveLeft
		    DIK_D,      // MoveRight
		    DIK_SPACE,  // MainShot
		}
	};

	// ボタンの登録もいつかやる
	
};
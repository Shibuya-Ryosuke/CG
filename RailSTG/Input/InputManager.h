#pragma once
#include "InputEnum.h"

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



private:
	// デバイスの種類
	DeviceType deviceType_ = DeviceType::None;
	DeviceType request_ = DeviceType::None;
};
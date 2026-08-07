#pragma once
#include<cstdint>

enum class DeviceType : int32_t {
	None,          // 未接続
	Keyboard,      // キーマウ
	Controller,    // パッド
};

enum class InputAction : int32_t {
	MoveUp,
	MoveDown,
	MoveLeft,
	MoveRight,
	MainShot,
	Count,
};
#pragma once
#include<cstdint>

enum class DeviceType : int32_t {
	None,          // 未接続
	Keyboard,      // キーマウ
	Controller,    // パッド
};
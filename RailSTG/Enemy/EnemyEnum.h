#pragma once
#include <cstdint>

enum class MobState : int32_t {
	None,         // リクエスト待機時に使用
	Standard,     // 通常時（射撃を含む）
};
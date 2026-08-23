#pragma once
#include<cstdint>

enum class TimeState : int32_t {
	None,             // リクエストの待機時に使用
	Default,          // 通常時
	JustEvasion,      // ジャスト回避時
	Targeting,        // プレイヤーの特殊攻撃においてターゲティングしている時
	Ready,            // ゲーム開始時
	Pause,
};
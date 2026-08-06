#pragma once
#include<cstdint>

enum class PlayerState : int32_t{
	None,                // リクエストの待機時に使用
	Standard,            // 通常時（メイン射撃、サブ射撃を含む）
	SpecialAttack1,      // 特殊攻撃1
	SpecialAttack2,      // 特殊攻撃2
	Ultimate,            // 必殺技
};
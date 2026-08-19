#pragma once
#include <cstdint>

enum class MobState : int32_t {
	None,         // リクエスト待機時に使用
	Standard,     // 通常時（射撃を含む）
};

enum class LockOnState : int32_t {
	None,         // 無し
	Hoverd,       // カーソルが重なっているとき
	Locked,       // ロックオンされている
};

enum class ReticleState : int32_t {
	None,       // 未配置
	Following,  // プレイヤーに追従中
	Locked,     // 座標固定
	Ready,      // 予告SE再生済み、判定待ち
	Shot,       // 判定発生済み
	End,        // 攻撃後の演出用
};
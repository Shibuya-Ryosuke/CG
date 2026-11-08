#pragma once
#include <cstdint>

namespace Test {
	/// <summary>
	/// 当たり判定のレイヤー (自分は何者か)。ビットを1つずつ割り当てる。
	/// mask には「当たりたい相手」のレイヤーを | でつないで指定する。
	/// (レイヤーの種類はゲームごとに違うので、アプリ側で定義する)
	/// </summary>
	enum TestLayer : uint32_t {
		kLayerPlayer = 1u << 0,
		kLayerEnemy = 1u << 1,
		kLayerPlayerBullet = 1u << 2,
	};
}

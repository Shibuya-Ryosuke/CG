#pragma once
#include "../../../Original/RyoEngine.h"

class Title {
public:
	Title() = default;
	~Title() = default;
	Title(const Title&) = delete;
	Title& operator=(const Title&) = delete;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

private:
	RyoEngine::Sprite title_;
};
#pragma once
#include "../../../Original/RyoEngine.h"

class Result {
public:
	Result() = default;
	~Result() = default;
	Result(const Result&) = delete;
	Result& operator=(const Result&) = delete;

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

	void SetIsClear(bool isClear) { isClear_ = isClear; }

private:
	RyoEngine::Sprite clear_;
	RyoEngine::Sprite failed_;

	bool isClear_ = true;
};
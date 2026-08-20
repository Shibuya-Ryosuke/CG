#pragma once

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
};
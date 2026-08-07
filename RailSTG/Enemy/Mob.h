#pragma once
#include "BaseEnemy.h"

class Mob : public BaseEnemy {
public:
	Mob();
	~Mob();
	// コピーコンストラクタと代入演算子の明示的削除
	Mob(const Mob&) = delete;
	Mob& operator=(const Mob&) = delete;

	/// <summary>
	/// 更新
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize() override;

	/// <summary>
	/// 更新
	/// </summary>
	/// <param name="camera"></param>
	void Update(const RyoEngine::Camera& camera) override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw() override;

private:

};
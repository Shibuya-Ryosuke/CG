#pragma once
#include "../../Original/RyoEngine.h"
#include "../BaseObject/BaseObject.h"

class BaseBullet : public BaseObject {
public:
	~BaseBullet() override = default;
	// コピーコンストラクタと代入演算子の明示的削除
	BaseBullet(const BaseBullet&) = delete;
	BaseBullet& operator=(const BaseBullet&) = delete;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize() override;

	/// <summary>
	///  終了
	/// </summary>
	void Finalize() override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update(const RyoEngine::Camera& camera) override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw() override;

protected:
	// 速度
	RyoEngine::Vector3 velocity_{};

	// 死亡
	bool isDead_ = false;

	// 衝突判定用
	RyoEngine::OBB obb_{};
};
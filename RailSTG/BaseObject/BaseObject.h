#pragma once
#include "../../Original/RyoEngine.h"
#include <memory>

class BaseObject {
public:
	virtual ~BaseObject() = default;

	// コピーコンストラクタと代入演算子の明示的削除
	BaseObject(const BaseObject&) = delete;
	BaseObject& operator=(const BaseObject&) = delete;

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize() = 0;

	/// <summary>
	///  終了
	/// </summary>
	virtual void Finalize() = 0;

	/// <summary>
	/// 更新
	/// </summary>
	virtual void Update(RyoEngine::Camera& camera) = 0;

	/// <summary>
	/// 描画
	/// </summary>
	virtual void Draw() = 0;

protected:
	// 自身
	std::unique_ptr<RyoEngine::Model> model_ = nullptr;
};
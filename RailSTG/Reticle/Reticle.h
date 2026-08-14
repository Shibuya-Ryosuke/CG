#pragma once
#include <memory>
#include <vector>

#include "../../Original/RyoEngine.h"
#include "../BaseObject/BaseObject.h"

class Reticle {
public:
	Reticle();
	~Reticle();
	Reticle(const Reticle&) = delete;
	Reticle& operator=(const Reticle&) = delete;

	void Initialize();
	void Update(const RyoEngine::Camera& camera, const RyoEngine::Vector3& playerPos);
	void Draw();

	// 弾の狙う基準点(一番手前のレティクル位置)
	RyoEngine::Vector3 GetWorldPos() const { return models_.front()->GetWorldPos(); }
	RyoEngine::Vector3 GetFarWorldPos() const { return models_.back()->GetWorldPos(); }

	// 弾が飛ぶべき方向(カメラ基準)。全レティクルが同一直線上にあるので誤差が出ない
	RyoEngine::Vector3 GetAimDirection(const RyoEngine::Camera& camera, const RyoEngine::Vector3& playerPos) const;

private:
	void Move();

private:
	// レティクルを置く距離(手前から奥へ)。3つ以上に増やしたければ配列を伸ばすだけでいい
	std::vector<float> kDistances = { 25.0f, 45.0f, 65.0f };
	// マウス感度(tanの増分なので数値は小さめになる。要調整)
	float kMouseSensitivity = 0.001f;
	// 画面端ぎりぎりに張り付かせないための余裕係数(1.0で端ちょうど)
	float kMaxTanMargin = 0.9f;

private:
	std::vector<std::unique_ptr<RyoEngine::Model>> models_;

	// 狙いの角度をtan(横/縦)で表現。距離に依存しない"比率"にすることで、
	// 複数のレティクルを距離だけ変えて同じ直線上に並べられる。
	float tanX_ = 0.0f;
	float tanY_ = 0.0f;
};
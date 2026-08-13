#pragma once
#include "../../Original/RyoEngine.h"
#include <vector>

/// <summary>
/// レールシューティング用の自動移動カメラ。
/// 設定されたウェイポイント(軌道)に沿って自動で位置・向きが更新される。
/// Player側はこのカメラのGetForward()/GetRight()/GetUp()/GetTranslate()/GetFovY()/GetAspectRatio()
/// を使って自身のワールド座標を計算する(詳細はPlayer::UpdateFollowTransform参照)。
/// </summary>
class RailCameraController : public RyoEngine::Camera {
public:
	RailCameraController() = default;
	~RailCameraController() override = default;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 更新(ウェイポイントに沿って自動的に位置・向きを進める)
	/// </summary>
	void Update() override;

	/// <summary>
	/// 軌道のウェイポイントを設定する(ワールド座標。最低2点必要)
	/// 呼び出した時点で、開始位置は先頭のウェイポイントにリセットされる。
	/// </summary>
	/// <param name="wayPoints">通過点のリスト(順番通りに通過する)</param>
	void SetWayPoints(const std::vector<RyoEngine::Vector3>& wayPoints) {
		wayPoints_ = wayPoints;
		currentIndex_ = 0;
		segmentT_ = 0.0f;
	}

	/// <summary>
	/// 軌道上を進む速度(1フレームあたりに進むワールド距離)
	/// </summary>
	void SetMoveSpeed(float speed) { moveSpeed_ = speed; }
	float GetMoveSpeed() const { return moveSpeed_; }

	/// <summary>
	/// 最終ウェイポイントまで到達しきったか
	/// </summary>
	bool IsFinished() const;

private:
	/// <summary>
	/// 現在区間内の進行度を進め、区間をまたいだらインデックスを送る
	/// </summary>
	void AdvanceProgress();

private:
	// 軌道のウェイポイント(ワールド座標)
	std::vector<RyoEngine::Vector3> wayPoints_;

	// 現在の区間 (wayPoints_[currentIndex_] -> wayPoints_[currentIndex_ + 1])
	size_t currentIndex_ = 0;

	// 現在区間内の進行度 (0.0 ～ 1.0)
	float segmentT_ = 0.0f;

	// 1フレームあたりに進むワールド距離
	float moveSpeed_ = 0.1f;
};

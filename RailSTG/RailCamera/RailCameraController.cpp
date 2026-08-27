#include <cmath>
#include <utility>

#include "RailCameraController.h"
#include "../Time/TimeManager.h"

using namespace RyoEngine;

namespace {
	// Catmull-Romスプラインの位置を計算する(P1→P2の間をt(0～1)で補間する)
	// P0, P3はP1, P2の前後にある点(補間曲線の"引っ張り具合"を決めるための参考点で、
	// この区間自体は通過しない)
	Vector3 CatmullRomPosition(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
		float t2 = t * t;
		float t3 = t2 * t;

		// 各項の係数をまとめておく(Vector3に単項マイナスが無いため、引き算の順序を工夫している)
		Vector3 coeffT2 = (p0 * 2.0f) - (p1 * 5.0f) + (p2 * 4.0f) - p3;
		Vector3 coeffT3 = (p1 * 3.0f + p3) - (p0 + p2 * 3.0f);

		return (p1 * 2.0f + (p2 - p0) * t + coeffT2 * t2 + coeffT3 * t3) * 0.5f;
	}

	// Catmull-Romスプラインの接線(その地点での進行方向)を計算する。
	// 位置の式をtで微分したもの。カメラの向きはこれを使って求める。
	Vector3 CatmullRomTangent(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
		Vector3 coeffT2 = (p0 * 2.0f) - (p1 * 5.0f) + (p2 * 4.0f) - p3;
		Vector3 coeffT3 = (p1 * 3.0f + p3) - (p0 + p2 * 3.0f);

		return ((p2 - p0) + coeffT2 * (2.0f * t) + coeffT3 * (3.0f * t * t)) * 0.5f;
	}
}

void RailCameraController::Initialize() {
	// FOV/アスペクト比/NearZ/FarZなど基底クラスの初期値をそのまま流用する
	Camera::Initialize();

	currentIndex_ = 0;
	segmentT_ = 0.0f;
}

bool RailCameraController::IsFinished() const {
	if (wayPoints_.size() < 2) {
		// 軌道が設定されていない場合は「終わっている」扱いにしておく
		return true;
	}
	return (currentIndex_ + 2 >= wayPoints_.size()) && (segmentT_ >= 1.0f);
}

void RailCameraController::ConnectToNextPhase(const std::vector<RyoEngine::Vector3>& relativeWayPoints) {
	if (relativeWayPoints.empty()) {
		return;
	}

	// 現在の実位置を「次フェーズのローカル原点(0,0,0)」に対応させるオフセットとして使う
	const Vector3 offset = translate_;

	// 次区間(現在位置→次フェーズ2点目)の長さを求め、仮想P0を置く距離の基準にする
	float nextSegmentLength = 1.0f; // フォールバック(2点目が無い場合など)
	if (relativeWayPoints.size() >= 2) {
		const Vector3 p1 = relativeWayPoints[0] + offset; // 通常は現在位置と一致する
		const Vector3 p2 = relativeWayPoints[1] + offset;
		float length = Length(p2 - p1);
		if (length > 0.0001f) {
			nextSegmentLength = length;
		}
	}
	const float virtualP0Distance = nextSegmentLength * kVirtualP0Ratio_;

	// 直前の進行方向を逆に延ばした点を仮想P0として先頭に置く
	// (これにより、切替直後の接線計算に「直前の進行方向」が反映され、向きが滑らかに繋がる)
	const Vector3 virtualP0 = translate_ - currentDirection_ * virtualP0Distance;

	std::vector<Vector3> newWayPoints;
	newWayPoints.reserve(relativeWayPoints.size() + 1);
	newWayPoints.push_back(virtualP0);
	for (const auto& relativePoint : relativeWayPoints) {
		newWayPoints.push_back(relativePoint + offset);
	}

	wayPoints_ = std::move(newWayPoints);
	currentIndex_ = 1; // P1 = 現在位置(実際の経路の始点)。P0(=仮想P0)はwayPoints_[0]
	segmentT_ = 0.0f;
}

void RailCameraController::EnterChangingStraight(float duration) {
	// 直進させたい距離(時間切れちょうどで終点に到達するよう計算。多少余裕を持たせたい場合は
	// 呼び出し側でdurationを少し多めに渡してもOK)
	float distance = moveSpeed_ * duration;
	if (distance <= 0.0001f) {
		distance = 1.0f; // 0除算・ゼロ距離を避ける
	}

	// 現在位置から進行方向へdistance分進んだ点を終点とする、2点だけの直線経路
	// (ConnectToNextPhaseは「相対座標」を受け取る仕様なので、先頭は{0,0,0}=現在位置、
	//  2点目は現在位置からのオフセット量で表す)
	std::vector<Vector3> straightRelativeWayPoints = {
		{ 0.0f, 0.0f, 0.0f },
		currentDirection_ * distance,
	};

	// 仮想P0(直前方向の延長)を含めた滑らかな接続は ConnectToNextPhase がそのまま面倒を見てくれる
	ConnectToNextPhase(straightRelativeWayPoints);
}

void RailCameraController::AdvanceProgress() {
	if (wayPoints_.size() < 2) {
		return;
	}

	// 既に最終区間の終点まで到達していたら、それ以上は進めない
	if (currentIndex_ + 2 >= wayPoints_.size() && segmentT_ >= 1.0f) {
		return;
	}

	// 進行度の増分は今まで通り「区間の直線距離」を目安に計算する。
	// (スプラインの実際の弧長ではないので、カーブがきついとやや速度にムラが出るが、
	//  気になるレベルになったらここを弧長ベースの計算に置き換える)
	const Vector3& p1 = wayPoints_[currentIndex_];
	const Vector3& p2 = wayPoints_[currentIndex_ + 1];

	float segmentLength = Length(p2 - p1);
	if (segmentLength <= 0.0001f) {
		// 同じ位置が2つ連続している場合など、0除算を避ける
		segmentLength = 1.0f;
	}

	// 1. TimeManagerからスロー反映済みのDeltaTimeを取得する
	float deltaTime = TimeManager::GetDeltaTime();

	// 2. 移動量に deltaTime を掛ける
	// （※これに合わせて moveSpeed_ の数値の大きさの調整が必要になる場合があります）
	segmentT_ += (moveSpeed_ / segmentLength) * deltaTime;

	// 区間をまたいだ分だけ次の区間へ進める
	while (segmentT_ >= 1.0f && currentIndex_ + 2 < wayPoints_.size()) {
		segmentT_ -= 1.0f;
		currentIndex_++;
	}

	// 最終区間では1.0を超えないようにクランプし、終点でぴったり停止させる
	if (currentIndex_ + 2 >= wayPoints_.size() && segmentT_ > 1.0f) {
		segmentT_ = 1.0f;
	}
}

void RailCameraController::Update() {
	if (isIdleRotating_) {
		// レール移動はせず、その場でY軸回転だけ進める
		float deltaTime = TimeManager::GetDeltaTime();
		rotate_.y += idleRotateSpeed_ * deltaTime;

		// 1回転(2π)を超えたら即座に0へ戻す(見た目上は巻き戻りが分からないのでOK)
		constexpr float kTwoPi = 6.28318530718f;
		if (rotate_.y >= kTwoPi) {
			rotate_.y = 0.0f;
		}

		// レール移動中の傾き(ピッチ/ロール)を引き継がないよう水平に固定
		rotate_.x = 0.0f;
		rotate_.z = 0.0f;
	} else {
		AdvanceProgress();

		if (wayPoints_.size() >= 2) {
			// Catmull-Romは前後2点ずつ、計4点(P0, P1, P2, P3)を使ってP1→P2の間を滑らかに補間する。
			// 先頭/末尾の区間で「前後の点」が存在しない場合は、無い方をP1(またはP2)自身で
			// 複製して代用する(これが一番シンプルなCatmull-Romの端点処理)。
			size_t count = wayPoints_.size();

			size_t i1 = currentIndex_;
			size_t i2 = currentIndex_ + 1;
			size_t i0 = (i1 == 0) ? i1 : i1 - 1;
			size_t i3 = (i2 + 1 < count) ? i2 + 1 : i2;

			const Vector3& p0 = wayPoints_[i0];
			const Vector3& p1 = wayPoints_[i1];
			const Vector3& p2 = wayPoints_[i2];
			const Vector3& p3 = wayPoints_[i3];

			// 位置: Catmull-Romスプラインで滑らかに補間(以前のLerpからここが変わった部分)
			translate_ = CatmullRomPosition(p0, p1, p2, p3, segmentT_);

			// 向き: スプラインの接線方向(その瞬間の進行方向)を向かせる。
			// 区間の切り替わりでも接線が連続的に変化するので、以前のような
			// 「区間ごとの向きがカクッと切り替わる」現象が起きなくなる。
			Vector3 direction = Normalize(CatmullRomTangent(p0, p1, p2, p3, segmentT_));
			currentDirection_ = direction;
			rotate_.y = atan2f(direction.x, direction.z);
			float horizontalLength = sqrtf(direction.x * direction.x + direction.z * direction.z);
			rotate_.x = atan2f(-direction.y, horizontalLength);
			// ロール(Z軸の傾き)は今回も未使用。
			rotate_.z = 0.0f;
		}
	}



	// SetCustomMatrices()で外部から行列を直接指定されていた場合は自動計算をスキップする
	if (isOverride_) {
		isOverride_ = false;
		return;
	}

	// ビュー行列の生成（カメラのワールド行列の逆行列）
	Matrix4x4 cameraMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate_, translate_);
	viewMatrix_ = Inverse(cameraMatrix);

	// プロジェクション行列の生成
	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearZ_, farZ_);

	// ViewProjection行列の合成
	viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
}

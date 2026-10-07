#pragma once
#include <RyoEngine.h>
#include "../MapChipField//MapChipField.h"

namespace AL4_1 {
	/// <summary>
	/// プレイヤーに追従する横スクロール用カメラ
	/// 
	/// ParamEditor（グループ "FollowCamera"）で編集できる項目
	///  ・yaw / pitch   : プレイヤーをどの方向から見るか（度）。yaw=0,pitch=0 で正面(-Z側)から
	///  ・distance      : プレイヤー（注視点）からカメラまでの距離
	///  ・followDelay   : 追従遅延（秒）。0で遅延なし、大きいほどゆっくり追いかける
	///  ・lookOffsetX/Y : 足元からどれだけずらした点を見るか（体の中心を見たいのでYを上げる）
	/// 
	/// 画面端の対策
	///  視界(画面の四隅)が z=0 のマップ面に映る範囲を毎フレーム計算し、その範囲が
	///  マップの外に出ないよう注視点を制限する。マップの大きさには依存しない。
	///  マップが視界より小さい場合は、距離を自動で縮めて視界をマップ内に収める。
	/// </summary>
	class FollowCamera {
	public:
		void Initialize(const MapChipField* map);

		/// <summary>プレイヤーの足元位置を渡して毎フレーム呼ぶ</summary>
		void Update(const RyoEngine::Vector3& playerFeet);

		/// <summary>次のUpdateで遅延なしに注視点へ瞬間移動する（リスポーン時などに使う）</summary>
		void Snap() { needSnap_ = true; }

		/// <summary>描画に使うカメラ</summary>
		RyoEngine::Camera& GetCamera() { return camera_; }

		/// <summary>
		/// 画角と縦横比。実際のカメラの設定と同じ値にすること（画面端の計算に使う）
		/// </summary>
		void SetProjection(float fovY, float aspect) {
			camera_.SetFovY(fovY);
			camera_.SetAspectRatio(aspect);
		}

		/// <summary>マップが視界より小さいとき、距離を自動で縮めるか（デフォルトtrue）</summary>
		void SetLimitDistanceToMap(bool enable) { limitDistanceToMap_ = enable; }

	private:
		void RegisterInParamEditor();

		// 注視点を原点・カメラ距離1.0としたときの、視界の四隅がマップ面(z=0)に映る範囲を求める
		// 映らない向き（空を見上げすぎ等）の場合は false
		bool CalcUnitFootprint(float& minX, float& maxX, float& minY, float& maxY) const;

		// ここだけがCameraクラスのAPIに依存する
		void Apply(const RyoEngine::Vector3& position, const RyoEngine::Vector3& rotate);

	private:
		RyoEngine::Camera camera_{};
		const MapChipField* map_ = nullptr;

		// --- ParamEditorで編集する値 ---
		float yawDeg_ = 0.0f;
		float pitchDeg_ = 10.0f;
		float distance_ = 35.0f;
		float followDelay_ = 0.15f;
		float lookOffsetX_ = 0.0f;
		float lookOffsetY_ = 1.0f;

		bool limitDistanceToMap_ = true;

		// 遅延をかけた後の注視点（z=0固定）
		float targetX_ = 0.0f;
		float targetY_ = 0.0f;
		bool needSnap_ = true;
	};
}
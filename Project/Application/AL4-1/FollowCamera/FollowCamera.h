#pragma once
#include <RyoEngine.h>
#include "../MapChipField//MapChipField.h"

namespace AL4_1 {
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

		/// <summary>
		/// プレイヤーの代わりに target をアップで映す（ボスの登場・撃破演出など）。
		/// target のアドレスは演出中ずっと有効であること
		/// </summary>
		void SetFocus(const RyoEngine::Vector3* target) {
			focusTarget_ = target;
			isZoomFocus_ = false;
			CancelPunchZoom();
		}
		/// <summary>
		/// target をズームで映す（クリア演出など）。zoomOffsetX/Y の分だけ注視点をずらすので、
		/// 対象の横に情報を出す余白が作れる。target のアドレスは演出中ずっと有効であること
		/// </summary>
		void SetZoomFocus(const RyoEngine::Vector3* target) {
			focusTarget_ = target;
			isZoomFocus_ = true;
			CancelPunchZoom();
		}
		/// <summary>
		/// target へ素早く寄って punchZoomTime_ の間保ち、元の距離へ戻る（フルパリィの瞬間など）。
		/// スロー中でも止まらないよう、寄る・戻る間は実時間で動く。target のアドレスは演出中ずっと有効であること
		/// </summary>
		void StartPunchZoom(const RyoEngine::Vector3* target) {
			punchTarget_ = target;
			punchTimer_ = punchZoomTime_;
			punchReturnTimer_ = 0.0f;
		}
		bool IsPunchZooming() const { return punchTarget_ != nullptr; }
		/// <summary>アップをやめてプレイヤーへ戻る（focusDelay_ でゆっくり戻る）</summary>
		void ClearFocus() {
			if (focusTarget_) { returnTimer_ = focusReturnTime_; }
			focusTarget_ = nullptr;
			isZoomFocus_ = false;
		}
		bool IsFocusing() const { return focusTarget_ != nullptr; }

	private:
		// パンチズームを途中でやめる（ほかの注目演出が始まったとき）
		void CancelPunchZoom() {
			punchTarget_ = nullptr;
			punchTimer_ = 0.0f;
			punchReturnTimer_ = 0.0f;
		}

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

		// --- 注目演出（ボスのアップなど） ---
		float focusDistance_ = 14.0f;	// アップのときの距離
		float focusOffsetY_ = 0.0f;		// アップのときの注視点の高さのずれ
		float focusDelay_ = 0.35f;		// アップへ寄る・プレイヤーへ戻るときの遅延（followDelay_ より遅くする）
		float focusReturnTime_ = 1.0f;	// アップをやめてから focusDelay_ を使い続ける時間

		// --- ズーム演出（クリア時のプレイヤーのアップ） ---
		float zoomDistance_ = 9.0f;		// ズームのときの距離
		float zoomOffsetX_ = 3.0f;		// 注視点の横のずれ（プラスで対象が画面の左に寄る）
		float zoomOffsetY_ = 1.2f;		// 注視点の高さのずれ
		bool isZoomFocus_ = false;

		// --- パンチズーム（フルパリィの瞬間にプレイヤーへ素早く寄る。スロー中でも動くよう実時間で進める） ---
		float punchZoomDistance_ = 8.0f;	// 寄ったときの距離
		float punchZoomOffsetY_ = 1.0f;		// 寄ったときの注視点の高さのずれ
		float punchZoomTime_ = 0.7f;		// 寄っている時間（実時間）
		float punchZoomInDelay_ = 0.06f;	// 寄るときの遅延（小さいほど一瞬で寄る）
		float punchZoomOutDelay_ = 0.3f;	// 戻るときの遅延
		const RyoEngine::Vector3* punchTarget_ = nullptr;
		float punchTimer_ = 0.0f;			// 寄っている残り時間
		float punchReturnTimer_ = 0.0f;		// 戻っている途中の残り時間

		bool limitDistanceToMap_ = true;

		// 遅延をかけた後の注視点（z=0固定）
		float targetX_ = 0.0f;
		float targetY_ = 0.0f;
		bool needSnap_ = true;

		// 注目演出の状態
		const RyoEngine::Vector3* focusTarget_ = nullptr;
		float returnTimer_ = 0.0f;		// プレイヤーへ戻っている途中の残り時間
		float currentDistance_ = 35.0f;	// 遅延をかけた後の距離
	};
}
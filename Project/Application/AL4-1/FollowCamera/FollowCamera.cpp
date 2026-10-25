#include "FollowCamera.h"
#include <algorithm>
#include <cmath>

using namespace RyoEngine;

namespace AL4_1 {
	namespace {
		constexpr float kDegToRad = 3.14159265f / 180.0f;

		// 範囲が成立していれば値を収め、成立しない（マップが視界より小さい）なら中央に置く
		float ClampOrCenter(float v, float lo, float hi) {
			if (lo > hi) return (lo + hi) * 0.5f;
			return std::clamp(v, lo, hi);
		}
	}

	void FollowCamera::Initialize(const MapChipField* map) {
		camera_.Initialize();
		map_ = map;
		needSnap_ = true;
		RegisterInParamEditor();
	}

	void FollowCamera::RegisterInParamEditor() {
		ParamEditor::BeginGroup("FollowCamera");
		ParamEditor::RegisterValue("yaw", &yawDeg_, 0.5f, -80.0f, 80.0f);
		ParamEditor::RegisterValue("pitch", &pitchDeg_, 0.5f, -80.0f, 80.0f);
		ParamEditor::RegisterValue("distance", &distance_, 0.1f, 1.0f, 200.0f);
		ParamEditor::RegisterValue("followDelay", &followDelay_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("lookOffsetX", &lookOffsetX_, 0.1f, -20.0f, 20.0f);
		ParamEditor::RegisterValue("lookOffsetY", &lookOffsetY_, 0.1f, -20.0f, 20.0f);
		ParamEditor::RegisterValue("focusDistance", &focusDistance_, 0.1f, 1.0f, 200.0f);
		ParamEditor::RegisterValue("focusOffsetY", &focusOffsetY_, 0.1f, -20.0f, 20.0f);
		ParamEditor::RegisterValue("focusDelay", &focusDelay_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("focusReturnTime", &focusReturnTime_, 0.01f, 0.0f, 5.0f);
		ParamEditor::RegisterValue("zoomDistance", &zoomDistance_, 0.1f, 1.0f, 200.0f);
		ParamEditor::RegisterValue("zoomOffsetX", &zoomOffsetX_, 0.1f, -20.0f, 20.0f);
		ParamEditor::RegisterValue("zoomOffsetY", &zoomOffsetY_, 0.1f, -20.0f, 20.0f);
		ParamEditor::RegisterValue("punchZoomDistance", &punchZoomDistance_, 0.1f, 1.0f, 200.0f);
		ParamEditor::RegisterValue("punchZoomOffsetY", &punchZoomOffsetY_, 0.1f, -20.0f, 20.0f);
		ParamEditor::RegisterValue("punchZoomTime", &punchZoomTime_, 0.01f, 0.0f, 5.0f);
		ParamEditor::RegisterValue("punchZoomInDelay", &punchZoomInDelay_, 0.005f, 0.0f, 2.0f);
		ParamEditor::RegisterValue("punchZoomOutDelay", &punchZoomOutDelay_, 0.005f, 0.0f, 2.0f);
		ParamEditor::EndGroup();
	}

	bool FollowCamera::CalcUnitFootprint(float& minX, float& maxX, float& minY, float& maxY) const {
		const float yaw = yawDeg_ * kDegToRad;
		const float pitch = pitchDeg_ * kDegToRad;
		const float sinYaw = std::sin(yaw), cosYaw = std::cos(yaw);
		const float sinPit = std::sin(pitch), cosPit = std::cos(pitch);

		// 注視点を原点としたカメラの位置（距離1.0）
		const float camX = sinYaw * cosPit;
		const float camY = sinPit;
		const float camZ = -cosYaw * cosPit;

		// カメラの回転は rotate = { pitch, -yaw, 0 }（X回転→Y回転の順）
		const float cosPsi = cosYaw;   // cos(-yaw)
		const float sinPsi = -sinYaw;  // sin(-yaw)

		const float tanHalf = std::tan(camera_.GetFovY() * 0.5f);

		bool first = true;
		for (int ix = 0; ix < 2; ++ix) {
			for (int iy = 0; iy < 2; ++iy) {
				// 画面の四隅を通るレイ（カメラ空間）
				const float lx = (ix == 0 ? -1.0f : 1.0f) * tanHalf * camera_.GetAspectRatio();
				const float ly = (iy == 0 ? -1.0f : 1.0f) * tanHalf;
				const float lz = 1.0f;

				// X回転(pitch)
				const float x1 = lx;
				const float y1 = ly * cosPit - lz * sinPit;
				const float z1 = ly * sinPit + lz * cosPit;
				// Y回転(-yaw)
				const float wx = x1 * cosPsi + z1 * sinPsi;
				const float wy = y1;
				const float wz = -x1 * sinPsi + z1 * cosPsi;

				// マップ面(z=0)に届かないレイがあれば計算不能
				if (wz <= 1e-4f) return false;
				const float t = -camZ / wz;
				if (t <= 0.0f) return false;

				const float px = camX + wx * t;
				const float py = camY + wy * t;

				if (first) {
					minX = maxX = px;
					minY = maxY = py;
					first = false;
				} else {
					minX = std::min(minX, px);
					maxX = std::max(maxX, px);
					minY = std::min(minY, py);
					maxY = std::max(maxY, py);
				}
			}
		}
		return true;
	}

	void FollowCamera::Update(const Vector3& playerFeet) {
		const float dt = GetScaleTime();
		// パンチズームはスロー中でも止まらないよう実時間で進める
		const float realDt = GetDeltaTime();
		if (punchTarget_) {
			punchTimer_ -= realDt;
			if (punchTimer_ <= 0.0f) {
				// 寄り終わったら戻り始める（遅延の数倍の時間で、ほぼ元の距離に戻る）
				punchTarget_ = nullptr;
				punchReturnTimer_ = punchZoomOutDelay_ * 4.0f;
			}
		} else if (punchReturnTimer_ > 0.0f) {
			punchReturnTimer_ -= realDt;
		}

		// --- 追いかけたい注視点 ---
		float desiredX = playerFeet.x + lookOffsetX_;
		float desiredY = playerFeet.y + lookOffsetY_;
		float desiredDist = distance_;
		// 注目対象がいればそちらをアップで映す
		if (focusTarget_) {
			desiredX = focusTarget_->x;
			desiredY = focusTarget_->y + focusOffsetY_;
			desiredDist = focusDistance_;
			// ズーム演出のときは専用の距離とずれを使う
			if (isZoomFocus_) {
				desiredX = focusTarget_->x + zoomOffsetX_;
				desiredY = focusTarget_->y + zoomOffsetY_;
				desiredDist = zoomDistance_;
			}
		}
		// パンチズーム中は対象へ寄る
		if (punchTarget_) {
			desiredX = punchTarget_->x;
			desiredY = punchTarget_->y + punchZoomOffsetY_;
			desiredDist = punchZoomDistance_;
		}

		// アップへ寄る・戻る途中は、いつもよりゆっくり追いかける
		if (returnTimer_ > 0.0f) {
			returnTimer_ -= dt;
		}
		float delay = (focusTarget_ || returnTimer_ > 0.0f) ? focusDelay_ : followDelay_;
		float stepDt = dt;
		// パンチズームの寄る・戻る間は、専用の遅延を実時間でかける
		if (punchTarget_) {
			delay = punchZoomInDelay_;
			stepDt = realDt;
		} else if (punchReturnTimer_ > 0.0f) {
			delay = punchZoomOutDelay_;
			stepDt = realDt;
		}
		const float alpha = needSnap_ ? 1.0f : (delay <= 0.0f ? 1.0f : 1.0f - std::exp(-stepDt / delay));

		// 距離も遅延をかけて変える
		currentDistance_ += (desiredDist - currentDistance_) * alpha;

		// --- 画面端の対策：視界がマップ外に出ない範囲へ収める ---
		float dist = currentDistance_;
		float rx0 = 0.0f, rx1 = 0.0f, ry0 = 0.0f, ry1 = 0.0f;
		const bool canLimit = map_ && CalcUnitFootprint(rx0, rx1, ry0, ry1);

		Vector3 mapMin{}, mapMax{};
		if (canLimit) {
			mapMin = map_->GetWorldMin();
			mapMax = map_->GetWorldMax();

			// マップが視界より小さいときは、視界がマップに収まる距離まで縮める
			if (limitDistanceToMap_) {
				const float unitW = rx1 - rx0;
				const float unitH = ry1 - ry0;
				float maxDist = dist;
				if (unitW > 1e-6f) maxDist = std::min(maxDist, (mapMax.x - mapMin.x) / unitW);
				if (unitH > 1e-6f) maxDist = std::min(maxDist, (mapMax.y - mapMin.y) / unitH);
				dist = std::max(maxDist * 0.999f, 0.1f);
			}
		}

		// 注視点が動ける範囲に収める（距離に比例して視界の広さが変わる）
		auto clampTarget = [&](float& tx, float& ty) {
			if (!canLimit) return;
			tx = ClampOrCenter(tx, mapMin.x - rx0 * dist, mapMax.x - rx1 * dist);
			ty = ClampOrCenter(ty, mapMin.y - ry0 * dist, mapMax.y - ry1 * dist);
			};

		// 先に目標を範囲内へ収めてから遅延をかける
		// （遅延をかけた後だけで収めると、折り返したときに壁際で一瞬止まって見える）
		clampTarget(desiredX, desiredY);

		// --- 追従遅延（フレームレートに依存しない指数平滑） ---
		if (needSnap_) {
			targetX_ = desiredX;
			targetY_ = desiredY;
			needSnap_ = false;
		} else {
			targetX_ += (desiredX - targetX_) * alpha;
			targetY_ += (desiredY - targetY_) * alpha;
		}
		// ParamEditorで値を変えた直後などに範囲外へ出ないよう、最後にもう一度収める
		clampTarget(targetX_, targetY_);

		// --- カメラの位置と向き ---
		const float yaw = yawDeg_ * kDegToRad;
		const float pitch = pitchDeg_ * kDegToRad;

		Vector3 position = {
			targetX_ + std::sin(yaw) * std::cos(pitch) * dist,
			targetY_ + std::sin(pitch) * dist,
			-std::cos(yaw) * std::cos(pitch) * dist
		};
		Vector3 rotate = { pitch, -yaw, 0.0f };

		Apply(position, rotate);
	}

	void FollowCamera::Apply(const Vector3& position, const Vector3& rotate) {
		// ★ Cameraクラスの実際のAPIに合わせてここだけ書き換える
		camera_.SetTranslate(position);
		camera_.SetRotate(rotate);
		camera_.Update();
	}
}
#pragma once
#include "../ImGui/ImGuiAllInclude.h"

namespace RyoEngine {

	class AnimEdit {
	public:
		AnimEdit() = default;
		~AnimEdit() = default;

		void Initialize();

		void DrawUI();

	private:
		struct CurveDelegate : public ImCurveEdit::Delegate {
			// データを保持する配列（初期値）
			ImVec2 mPoints[3] = {
				ImVec2(0.0f, 0.0f),
				ImVec2(0.5f, 0.5f),
				ImVec2(1.0f, 1.0f)
			};

			// 1. 表示範囲の指定
			ImVec2& GetMin() override { static ImVec2 min(0.0f, 0.0f); return min; }
			ImVec2& GetMax() override { static ImVec2 max(1.0f, 1.0f); return max; }

			// 2. カーブの基本情報
			size_t GetCurveCount() override { return 1; }
			bool IsVisible(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return true;
			}
			size_t GetPointCount(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return 3;
			}

			// 3. キーフレーム（点）の座標
			ImVec2* GetPoints(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return mPoints;
			}

			// 4. エラーの出ていた編集系関数（引数・戻り値をライブラリの定義に完全一致）
			uint32_t GetCurveColor(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return 0xFF00FFFF;  // 紫色
			}

			// EditPoint の引数は (size_t, int, ImVec2) などの組み合わせになっているため、型を合わせています
			int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) override {
				static_cast<void>(curveIndex);
				mPoints[pointIndex] = value;
				return pointIndex;
			}

			void AddPoint(size_t curveIndex, ImVec2 value) override {
				static_cast<void>(curveIndex);
				static_cast<void>(value);
			}

			// DelPoint の第2引数も int に変更
			///void DelPoint(size_t curveIndex, int pointIndex) override {}
		};

		CurveDelegate m_CurveDelegate;
	};
}
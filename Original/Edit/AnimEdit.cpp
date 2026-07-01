#include "AnimEdit.h"

#ifdef _DEBUG
#include "../ImGui/ImGuiAllInclude.h"

namespace RyoEngine {

	struct AnimEdit::Impl {
		struct CurveDelegate : public ImCurveEdit::Delegate {
			// データを保持する配列（初期値）
			ImVec2 mPoints[3] = {
				ImVec2(0.0f, 0.0f),
				ImVec2(0.5f, 0.5f),
				ImVec2(1.0f, 1.0f)
			};


			ImVec2& GetMin() override { static ImVec2 min(0.0f, 0.0f); return min; }
			ImVec2& GetMax() override { static ImVec2 max(1.0f, 1.0f); return max; }
			size_t GetCurveCount() override { return 1; }
			bool IsVisible(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return true;
			}
			size_t GetPointCount(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return 3;
			}
			ImVec2* GetPoints(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return mPoints;
			}
			uint32_t GetCurveColor(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return 0xFF00FFFF;  // 紫色
			}

			
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
			// void DelPoint(size_t curveIndex, size_t pointIndex) override {}
			// 存在しない可能性あり
		};

		CurveDelegate m_CurveDelegate;
	};

	// コンストラクタで生成
	AnimEdit::AnimEdit() {
		m_pImpl = new Impl();
	}
	// デストラクタで破棄
	AnimEdit::~AnimEdit() {
		delete m_pImpl;
	}

	void AnimEdit::Initialize(){}

	void AnimEdit::DrawUI() {
		AnimEdit& instance = GetInstance();

		// Animation Editor ウィンドウ生成
		ImGui::Begin("Animation Editor");
		{
			// ウィンドウいっぱいにグラフを表示させるサイズ指定
			ImVec2 size = ImGui::GetContentRegionAvail();

            if (size.x < 100.0f) size.x = 100.0f;
            if (size.y < 200.0f) size.y = 200.0f;

            ImCurveEdit::Edit(instance.m_pImpl->m_CurveDelegate, size, 1);
		}
		ImGui::End();
	}
}

#else
namespace RyoEngine {
	AnimEdit::AnimEdit() = default;
	AnimEdit::~AnimEdit() = default;
}
#endif

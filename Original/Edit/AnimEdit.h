#pragma once

namespace RyoEngine {

	class AnimEdit {
	public:

		// コピーと代入を禁止
		AnimEdit(const AnimEdit&) = delete;
		AnimEdit& operator=(const AnimEdit&) = delete;
		AnimEdit(AnimEdit&&) = delete;
		AnimEdit& operator=(AnimEdit&&) = delete;

#ifdef _DEBUG
		static void Initialize();
		static void DrawUI();
#else
		static void Initialize(){}
		static void DrawUI(){}
#endif


	private:
		/// <summary>
		/// インスタンス取得
		/// </summary>
		/// <returns>インスタンス</returns>
		static AnimEdit& GetInstance() {
			static AnimEdit instance;
			return instance;
		}

		AnimEdit();
		~AnimEdit();

#ifdef _DEBUG
		struct Impl;
		Impl* m_pImpl = nullptr;
#endif
	};
}
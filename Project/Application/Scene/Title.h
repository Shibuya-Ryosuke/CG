#pragma once
#include <RyoEngine.h>

namespace App {
	/// <summary>
	/// タイトルシーン
	/// </summary>
	class Title : public RyoEngine::BaseScene {
	public:
		Title() = default;
		~Title() override = default;

		Title(const Title&) = delete;
		Title& operator=(const Title&) = delete;

		void Initialize() override;
		void Update() override;
		void Draw() override;
	};
}

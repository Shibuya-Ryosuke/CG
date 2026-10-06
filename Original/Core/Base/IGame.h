#pragma once

namespace RyoEngine {
	class IGame {
	public:
		virtual ~IGame() = default;
		virtual void Initialize() = 0;
		virtual void Update() = 0;
		virtual void Draw() = 0;
		virtual void Finalize() = 0;
	};
}
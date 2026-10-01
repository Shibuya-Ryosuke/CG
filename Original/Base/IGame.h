#pragma once
#include "../Camera/Camera.h"
#include "../Camera/DebugCamera.h"

namespace RyoEngine {
	class IGame {
	public:
		virtual ~IGame() = default;
		virtual void Initialize(RyoEngine::DebugCamera& debugCamera) = 0;
		virtual void Update() = 0;
		virtual void Draw() = 0;
		virtual void Finalize() = 0;
	};
}
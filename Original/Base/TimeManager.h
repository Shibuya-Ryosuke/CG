#pragma once
#include <chrono>

namespace RyoEngine {
	class TimeManager {
	public:
		static void Initialize();
		static void NewFrame();
		static void EndFrame();

		static float GetDeltaTime() {
			return GetInstance()->deltaTime_;
		}
		static float GetScaleTime() {
			return GetInstance()->deltaTime_ * GetInstance()->scale_;
		}
		static float GetFps() {
			return GetInstance()->fps_;
		}
		static float GetSmoothedFps() {
			return GetInstance()->smoothedFps_;
		}
		static float GetCpuFrameTime() {
			return GetInstance()->cpuFrameTime_;
		}
		static float GetCpuFps() {
			return GetInstance()->cpuFps_;
		}
		static float GetScale() {
			return GetInstance()->scale_;
		}
		static float GetTargetFps() {
			return GetInstance()->targetFps_;
		}
		static bool IsFpsLimitEnabled() {
			return GetInstance()->isFpsLimitEnabled_;
		}


		static void SetScale(float scale) {
			GetInstance()->scale_ = scale;
		}
		static void SetTragetFps(float fps) {
			TimeManager* instance = GetInstance();
			instance->targetFps_ = fps;

			// 目標のフレーム時間 (例: 60FPSなら 1,000,000μs / 60 ≒ 16666μs)
			instance->targetFrameTime_ = std::chrono::microseconds(static_cast<uint64_t>(1000000.0f / fps));

			// VSync二重待機回避用の閾値時間 (例: 60FPSなら 1,000,000μs / 65 ≒ 15384μs)
			// 目標FPSより少し高いFPS(+5.0f程度)で計算してマージンを作る
			instance->checkFrameTime_ = std::chrono::microseconds(static_cast<uint64_t>(1000000.0f / (fps + 5.0f)));
		}
		static void SetFpsLimitEnabled(bool enable) {
			GetInstance()->isFpsLimitEnabled_ = enable;
		}

	private:
		static TimeManager* GetInstance() {
			static TimeManager instance;
			return &instance;
		};

		TimeManager() = default;
		~TimeManager() = default;
		TimeManager(const TimeManager&) = delete;
		TimeManager& operator=(const TimeManager&) = delete;

		std::chrono::high_resolution_clock::time_point lastTime_;
		float deltaTime_ = 0.0f;
		float scale_ = 1.0f;
		float fps_ = 0.0f;
		float smoothedFps_ = 0.0f;

		std::chrono::steady_clock::time_point cpuStart_;
		float cpuFrameTime_ = 0.0f;
		float cpuFps_ = 0.0f;

		float targetFps_ = 60.0f;
		std::chrono::duration<double> targetFrameTime_{ 1.0 / 60.0 };
		// VSync二重待機回避のための閾値時間
		std::chrono::microseconds checkFrameTime_{};
		bool isFpsLimitEnabled_ = true;
	};
}
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

		static void SetScale(float scale) {
			GetInstance()->scale_ = scale;
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
	};
}
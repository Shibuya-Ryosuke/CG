#include "TimeManager.h"

namespace RyoEngine {
    void TimeManager::Initialize() {
        GetInstance()->lastTime_ = std::chrono::high_resolution_clock::now();
    }

    void TimeManager::NewFrame() {
        TimeManager* instance = GetInstance();

        instance->cpuStart_ = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - instance->lastTime_;
        instance->deltaTime_ = elapsed.count();
        instance->lastTime_ = currentTime;
        
        // 念のためゼロ除算（クラッシュ）防止
        if (instance->deltaTime_ > 0.0f) {
            instance->fps_ = 1.0f / instance->deltaTime_;
            // 毎フレーム数値がガタガタ動くと見づらいので、10%ずつ近づけて滑らかにする（お好みで）
            instance->smoothedFps_ = (instance->smoothedFps_ * 0.9f) + (instance->fps_ * 0.1f);
        }
    }

    void TimeManager::EndFrame() {
        TimeManager* instance = GetInstance();
        
        auto cpuEnd = std::chrono::high_resolution_clock::now();

        // CPUの処理時間を計算 (秒単位)
        std::chrono::duration<float> cpuElapsed = cpuEnd - instance->cpuStart_;
        instance->cpuFrameTime_ = cpuElapsed.count();

        // FPS換算 (もしこの処理だけでループしたら何FPS出るか)
        instance->cpuFps_ = (instance->cpuFrameTime_ > 0.0f) ? (1.0f / instance->cpuFrameTime_) : 0.0f;
    }
}
#include "TimeManager.h"
#include <windows.h>
#include <timeapi.h>
#include <thread>
#pragma comment(lib, "winmm.lib")

namespace RyoEngine {
    void TimeManager::Initialize() {
        timeBeginPeriod(1);
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

        // FPS制限処理（制限が有効な場合のみ実行）
        if (instance->isFpsLimitEnabled_) {

            auto currentTime = std::chrono::high_resolution_clock::now();
            // NewFrameで記録した開始時間からの経過時間を算出
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - instance->lastTime_);

            // 規定時間より少し短い閾値（kMinCheckTime相当）を超えていないかチェック
            // これを超えていれば、待機せずに直接VSync（Present）へ向かうことで二重待機を防ぐ
            if (elapsed < instance->checkFrameTime_) {

                // 目標とするフレーム終了時刻
                auto targetTime = instance->lastTime_ + instance->targetFrameTime_;

                // 目標時刻に到達するまで待機
                while (true) {
                    currentTime = std::chrono::high_resolution_clock::now();
                    if (currentTime >= targetTime) {
                        break;
                    }

                    auto remaining = targetTime - currentTime;

                    // 残り時間が1.5ms以上あれば、スリープしてCPU使用率を抑える
                    if (remaining > std::chrono::microseconds(1500)) {
                        std::this_thread::sleep_for(remaining - std::chrono::microseconds(1000));
                    } else {
                        // 1マイクロ秒スリープ
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                }
            }
        }
    }
}
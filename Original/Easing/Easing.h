#pragma once
#include <stdint.h>

namespace RyoEngine {

    enum class EasingType : int32_t {
        None,          // 無し
        Lerp,          // 線形補間（等速）
        EaseInQuad,    // 2次加速
        EaseOutQuad,   // 2次減速
        EaseInOutQuad, // 2次加速減速
        EaseOutBounce  // バウンド
    };

    // クランプ（0～1）
    inline float Clamp(float t) {
        if (t < 0.0f) {
            t = 0.0f;
        }
        if (t > 1.0f) {
            t = 1.0f;
        }
        return t;
    }

    //線形補完
    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    float EaseInQuad(float t, float start, float end);

    float EaseOutQuad(float t, float start, float end);

    float EaseInOutQuad(float t, float start, float end);

    float EaseInCubic(float t, float start, float end);

    float EaseOutCubic(float t, float start, float end);

    float EaseInOutCubic(float t, float start, float end);

    float EaseInQuart(float t, float start, float end);

    float EaseOutQuart(float t, float start, float end);

    float EaseInOutQuart(float t, float start, float end);

    float EaseInQuint(float t, float start, float end);

    float EaseOutQuint(float t, float start, float end);

    float EaseInOutQuint(float t, float start, float end);

    float EaseInSine(float t, float start, float end);

    float EaseOutSine(float t, float start, float end);

    float EaseInOutSine(float t, float start, float end);

    float EaseInExpo(float t, float start, float end);

    float EaseOutExpo(float t, float start, float end);

    float EaseInOutExpo(float t, float start, float end);

    float EaseInCirc(float t, float start, float end);

    float EaseOutCirc(float t, float start, float end);

    float EaseInOutCirc(float t, float start, float end);

    float EaseInBack(float t, float start, float end, float s = 1.70158f);

    float EaseOutBack(float t, float start, float end, float s = 1.70158f);

    float EaseInOutBack(float t, float start, float end, float s = 1.70158f);

    float EaseOutBounce(float t, float start, float end);

    float EaseInBounce(float t, float start, float end);

    float EaseInOutBounce(float t, float start, float end);

    float EaseInElastic(float t, float start, float end, float amplitude = 1.0f, float period = 0.3f);

    float EaseOutElastic(float t, float start, float end, float amplitude = 1.0f, float period = 0.3f);

    float EaseInOutElastic(float t, float start, float end, float amplitude = 1.0f, float period = 0.45f);


}

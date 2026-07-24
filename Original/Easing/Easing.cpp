#include "Easing.h"
#define _USE_MATH_DEFINES
#include <math.h>

namespace RyoEngine {
    float ApplyEasing(EasingType type, float t) {
        switch (type) {
        case EasingType::Lerp:          return t;
        case EasingType::EaseInQuad:    return t * t;
        case EasingType::EaseOutQuad:   return t * (2.0f - t);
        case EasingType::EaseInOutQuad: return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        case EasingType::EaseOutBounce:
            if (t < (1.0f / 2.75f)) {
                return 7.5625f * t * t;
            } else if (t < (2.0f / 2.75f)) {
                t -= (1.5f / 2.75f);
                return 7.5625f * t * t + 0.75f;
            } else if (t < (2.5f / 2.75f)) {
                t -= (2.25f / 2.75f);
                return 7.5625f * t * t + 0.9375f;
            } else {
                t -= (2.625f / 2.75f);
                return 7.5625f * t * t + 0.984375f;
            }
        case EasingType::None:
        default:
            return (t >= 1.0f) ? 1.0f : 0.0f;
        }
    }

    // （2次：ゆっくり始まる）
    float EaseInQuad(float t, float start, float end) {
        t = Clamp(t);
        float easedT = t * t;
        return start + (end - start) * easedT;
    }

    // （2次：速く始まってゆっくり止まる）
    float EaseOutQuad(float t, float start, float end) {
        t = Clamp(t);
        float inv = 1.0f - t;
        float easedT = 1.0f - inv * inv;
        return start + (end - start) * easedT;
    }

    // （2次：前半ゆっくり → 中 → 終盤ゆっくり）
    float EaseInOutQuad(float t, float start, float end) {
        t = Clamp(t);
        float easedT;
        if (t < 0.5f) {
            easedT = 2.0f * t * t;
        } else {
            float inv = -2.0f * t + 2.0f;    // = 2*(1?t)
            easedT = 1.0f - (inv * inv) / 2.0f;
        }
        return start + (end - start) * easedT;
    }

    // （3次：ゆっくり始まる）
    float EaseInCubic(float t, float start, float end) {
        t = Clamp(t);
        float easedT = t * t * t;
        return start + (end - start) * easedT;
    }

    // （3次：速く始まってゆっくり止まる）
    float EaseOutCubic(float t, float start, float end) {
        t = Clamp(t);
        float inv = 1.0f - t;
        float easedT = 1.0f - inv * inv * inv;
        return start + (end - start) * easedT;
    }

    // （3次：ゆらぎを持たせた加減速）
    float EaseInOutCubic(float t, float start, float end) {
        t = Clamp(t);
        float easedT;
        if (t < 0.5f) {
            easedT = 4.0f * t * t * t;
        } else {
            float inv = (2.0f * t) - 2.0f;
            easedT = 0.5f * inv * inv * inv + 1.0f;
        }
        return start + (end - start) * easedT;
    }

    // （4次）
    float EaseInQuart(float t, float start, float end) {
        t = Clamp(t);
        float easedT = t * t * t * t;
        return start + (end - start) * easedT;
    }

    // （4次：速→減速）
    float EaseOutQuart(float t, float start, float end) {
        t = Clamp(t);
        float inv = 1.0f - t;
        float easedT = 1.0f - inv * inv * inv * inv;
        return start + (end - start) * easedT;
    }

    // （4次：加速と減速両方）
    float EaseInOutQuart(float t, float start, float end) {
        t = Clamp(t);
        float easedT;
        if (t < 0.5f) {
            easedT = 8.0f * t * t * t * t;
        } else {
            float inv = -2.0f * t + 2.0f;
            easedT = 1.0f - (inv * inv * inv * inv) / 2.0f;
        }
        return start + (end - start) * easedT;
    }

    // （5次：ゆっくり始まる、強めの加速）
    float EaseInQuint(float t, float start, float end) {
        t = Clamp(t);
        float easedT = t * t * t * t * t;
        return start + (end - start) * easedT;
    }

    // （5次：速→ゆっくり止まる）
    float EaseOutQuint(float t, float start, float end) {
        t = Clamp(t);
        float inv = 1.0f - t;
        float easedT = 1.0f - powf(inv, 5.0f);
        return start + (end - start) * easedT;
    }

    // （5次：加速・定速・減速の三段構成）
    float EaseInOutQuint(float t, float start, float end) {
        t = Clamp(t);
        float easedT;
        if (t < 0.5f) {
            easedT = 16.0f * t * t * t * t * t;
        } else {
            float inv = (2.0f * t) - 2.0f;
            easedT = 1.0f - powf(fabsf(inv), 5.0f) / 2.0f;
            // こちらは少し工夫が要る実装パターンもある
        }
        return start + (end - start) * easedT;
    }

    // ??? 三角／円／指数／オーバーシュート系 ???

    // （サイン波を使ってゆるやか開始）
    float EaseInSine(float t, float start, float end) {
        t = Clamp(t);
        float easedT = 1.0f - cosf((t * (float)M_PI) / 2.0f);  // M_PI は π
        return start + (end - start) * easedT;
    }

    // （サイン波で終盤ゆるやか）
    float EaseOutSine(float t, float start, float end) {
        t = Clamp(t);
        float easedT = sinf((t * (float)M_PI) / 2.0f);
        return start + (end - start) * easedT;
    }

    //（サインを使った滑らかな加減速）
    float EaseInOutSine(float t, float start, float end) {
        t = Clamp(t);
        float easedT = -(cosf((float)M_PI * t) - 1.0f) / 2.0f;
        return start + (end - start) * easedT;
    }

    // （指数関数を使って急加速）
    float EaseInExpo(float t, float start, float end) {
        t = Clamp(t);
        float easedT = (t == 0.0f) ? 0.0f : powf(2.0f, 10.0f * (t - 1.0f));
        return start + (end - start) * easedT;
    }

    // （終盤急激に変化 → 減速）
    float EaseOutExpo(float t, float start, float end) {
        t = Clamp(t);
        float easedT = (t == 1.0f) ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
        return start + (end - start) * easedT;
    }

    // （両端がゆるやか、中盤で急変化）
    float EaseInOutExpo(float t, float start, float end) {
        t = Clamp(t);
        float easedT;
        if (t == 0.0f) {
            easedT = 0.0f;
        } else if (t == 1.0f) {
            easedT = 1.0f;
        } else if (t < 0.5f) {
            easedT = powf(2.0f, 10.0f * (2.0f * t - 1.0f)) / 2.0f;
        } else {
            easedT = (2.0f - powf(2.0f, -10.0f * (2.0f * t - 1.0f))) / 2.0f;
        }
        return start + (end - start) * easedT;
    }

    // （円関数：ゆるやかに始まりつつ自然なカーブ）
    float EaseInCirc(float t, float start, float end) {
        t = Clamp(t);
        float easedT = 1.0f - sqrtf(1.0f - t * t);
        return start + (end - start) * easedT;
    }

    // （速→ゆるやかになる円曲線）
    float EaseOutCirc(float t, float start, float end) {
        t = Clamp(t);
        float inv = t - 1.0f;
        float easedT = sqrtf(1.0f - inv * inv);
        return start + (end - start) * easedT;
    }

    // （前後ゆるやか、中盤自然な速度）
    float EaseInOutCirc(float t, float start, float end) {
        t = Clamp(t);
        float easedT;
        if (t < 0.5f) {
            easedT = (1.0f - sqrtf(1.0f - 4.0f * (t * t))) / 2.0f;
        } else {
            float inv = -2.0f * t + 2.0f;
            easedT = (sqrtf(1.0f - inv * inv) + 1.0f) / 2.0f;
        }
        return start + (end - start) * easedT;
    }

    //（最初少し逆方向に引っ張ってから加速）
    float EaseInBack(float t, float start, float end, float s) {
        t = Clamp(t);
        float easedT = t * t * ((s + 1.0f) * t - s);
        return start + (end - start) * easedT;
    }

    // （終盤少し飛び出して戻るような動き）
    float EaseOutBack(float t, float start, float end, float s) {
        t = Clamp(t);
        float inv = t - 1.0f;
        float easedT = inv * inv * ((s + 1.0f) * inv + s) + 1.0f;
        return start + (end - start) * easedT;
    }

    // （前後に少し引っ張り／戻しを含む自然な S 字）
    float EaseInOutBack(float t, float start, float end, float s) {
        t = Clamp(t);
        float s2 = s * 1.525f;
        float easedT;
        if (t < 0.5f) {
            easedT = (t * 2.0f * t * 2.0f * ((s2 + 1.0f) * t * 2.0f - s2)) / 2.0f;
        } else {
            float inv = t * 2.0f - 2.0f;
            easedT = (inv * inv * ((s2 + 1.0f) * inv + s2) + 2.0f) / 2.0f;
        }
        return start + (end - start) * easedT;
    }

    // （バウンス：最後ではねる感じで止まる）
    float EaseOutBounce(float t, float start, float end) {
        t = Clamp(t);
        float c = end - start;
        float easedT;
        if (t < 1.0f / 2.75f) {
            easedT = 7.5625f * t * t;
        } else if (t < 2.0f / 2.75f) {
            t = t - (1.5f / 2.75f);
            easedT = 7.5625f * t * t + 0.75f;
        } else if (t < 2.5f / 2.75f) {
            t = t - (2.25f / 2.75f);
            easedT = 7.5625f * t * t + 0.9375f;
        } else {
            t = t - (2.625f / 2.75f);
            easedT = 7.5625f * t * t + 0.984375f;
        }
        return start + c * easedT;
    }

    //（開始時にはねてから目的地へ）
    float EaseInBounce(float t, float start, float end) {
        t = Clamp(t);
        // “反転”で Out を使う
        float c = end - start;
        float easedT = 1.0f - EaseOutBounce(1.0f - t, 0.0f, 1.0f);
        return start + c * easedT;
    }

    //（前後バウンスを組み合わせた動き）
    float EaseInOutBounce(float t, float start, float end) {
        t = Clamp(t);
        float c = end - start;
        float easedT;
        if (t < 0.5f) {
            easedT = (1.0f - EaseOutBounce(1.0f - 2.0f * t, 0.0f, 1.0f)) / 2.0f;
        } else {
            easedT = (1.0f + EaseOutBounce(2.0f * t - 1.0f, 0.0f, 1.0f)) / 2.0f;
        }
        return start + c * easedT;
    }

    // ゴムのように伸びて揺れながら加速）
    float EaseInElastic(float t, float start, float end, float amplitude, float period) {
        t = Clamp(t);
        float c = end - start;
        if (t == 0.0f) return start;
        if (t == 1.0f) return end;
        float s;
        if (amplitude < fabsf(c)) {
            amplitude = c;
            s = period / 4.0f;
        } else {
            s = period / (2.0f * (float)M_PI) * asinf(c / amplitude);
        }
        t -= 1.0f;
        float easedT = -(amplitude * powf(2.0f, 10.0f * t) * sinf((t * 1.0f - s) * (2.0f * (float)M_PI) / period));
        return start + c * easedT;
    }

    //（最後で弾むように揺れて止まる）
    float EaseOutElastic(float t, float start, float end, float amplitude, float period) {
        t = Clamp(t);
        float c = end - start;
        if (t == 0.0f) return start;
        if (t == 1.0f) return end;
        float s;
        if (amplitude < fabsf(c)) {
            amplitude = c;
            s = period / 4.0f;
        } else {
            s = period / (2.0f * (float)M_PI) * asinf(c / amplitude);
        }
        float easedT = amplitude * powf(2.0f, -10.0f * t) * sinf((t * 1.0f - s) * (2.0f * (float)M_PI) / period) + 1.0f;
        return start + c * easedT;
    }

    //（始まりと終わりで弾む効果を出す）
    float EaseInOutElastic(float t, float start, float end, float amplitude, float period) {
        t = Clamp(t);
        float c = end - start;
        if (t == 0.0f) return start;
        if (t == 1.0f) return end;
        float s;
        if (amplitude < fabsf(c)) {
            amplitude = c;
            s = period / 4.0f;
        } else {
            s = period / (2.0f * (float)M_PI) * asinf(c / amplitude);
        }
        float easedT;
        t = t * 2.0f;
        if (t < 1.0f) {
            t -= 1.0f;
            easedT = -0.5f * (amplitude * powf(2.0f, 10.0f * t) * sinf((t * 1.0f - s) * (2.0f * (float)M_PI) / period));
        } else {
            t -= 1.0f;
            easedT = amplitude * powf(2.0f, -10.0f * t) * sinf((t * 1.0f - s) * (2.0f * (float)M_PI) / period) * 0.5f + 1.0f;
        }
        return start + c * easedT;
    }

}
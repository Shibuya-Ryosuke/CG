#pragma once
#include <string>
#include <vector>
#include "ParticleFxTypes.h"

namespace App {

    /// <summary>
    /// エフェクトのテンプレ (動き方・消え方・形の組み合わせの見本)。
    /// エディタの「新規作成」「テンプレを適用」から選べる。
    /// 値はここで決め打ちしているが、作った後はエディタで自由に変えてJSONに保存できる。
    /// </summary>
    class ParticleFxTemplates {
    public:
        struct Entry {
            std::string label;       // エディタでの表示名
            std::string defaultName; // 見本のエフェクトとして作るときの名前
            ParticleFxConfig (*make)();
        };

        /// <summary>全テンプレ (先頭は「空」)</summary>
        static const std::vector<Entry>& GetAll();

        static ParticleFxConfig Blank();
        static ParticleFxConfig Charge();     // 外周の輪から弧を描いて収束 (縮み出し)
        static ParticleFxConfig Burst();      // 放射の火花 (点滅で消える)
        static ParticleFxConfig Fireworks();  // 寿命で分裂する花火
        static ParticleFxConfig RingWave();   // 画面に正対した輪が広がる衝撃波
        static ParticleFxConfig Vortex();     // 渦を巻いて上昇 (色変化で消える)
        static ParticleFxConfig Homing();     // 目標へ曲がりながら追尾
        static ParticleFxConfig Curve();      // ベジェで目標へ飛ぶ (伸びて消える)
        static ParticleFxConfig Electric();   // ジッターでパリパリする電気
        static ParticleFxConfig Sparks();     // 重力で落ちて床で跳ねる火花
        static ParticleFxConfig Absorb();     // 漂ってから吸い込まれる

        // 見本で使う、BindingRegistry のキー名 (Showcaseシーンが登録する)
        static const char* ShowcaseCenterKey() { return "showcase center"; }
        static const char* ShowcaseTargetKey() { return "showcase target"; }
    };
}

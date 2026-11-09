#pragma once
#include <string>
#include <vector>
#include "ParticleTypes.h"

namespace RyoEngine {

    /// <summary>
    /// エフェクトのテンプレ (動き方・消え方・形の組み合わせの見本)。
    /// エディタの「新規作成」「テンプレを適用」から選べる。
    /// 値はここで決め打ちしているが、作った後はエディタで自由に変えてJSONに保存できる。
    /// </summary>
    class ParticleTemplates {
    public:
        struct Entry {
            std::string label;       // エディタでの表示名
            std::string defaultName; // 見本のエフェクトとして作るときの名前
            ParticleConfig (*make)();
        };

        /// <summary>全テンプレ (先頭は「空」)</summary>
        static const std::vector<Entry>& GetAll();

        static ParticleConfig Blank();
        static ParticleConfig Charge();     // 外周の輪から弧を描いて収束 (縮み出し)。連鎖で爆発する
        static ParticleConfig Burst();      // 放射の火花 (点滅で消える)
        static ParticleConfig Fireworks();  // 寿命で分裂する花火
        static ParticleConfig RingWave();   // 画面に正対した輪が広がる衝撃波
        static ParticleConfig Vortex();     // 渦を巻いて上昇 (色変化で消える)
        static ParticleConfig Homing();     // 目標へ曲がりながら追尾
        static ParticleConfig Curve();      // ベジェで目標へ飛ぶ (伸びて消える)
        static ParticleConfig Electric();   // ジッターでパリパリする電気
        static ParticleConfig Sparks();     // 重力で落ちて床で跳ねる火花
        static ParticleConfig Absorb();     // 漂ってから吸い込まれる

        // 見本のエフェクトが追従する、BindingRegistry のキー名。
        // アプリ側で BindingRegistry::RegisterPosition(ParticleTemplates::CenterKey(), &座標) すると、そこから出る
        // (登録しなければ、基準座標や Play で渡した場所から出る)
        static const char* CenterKey() { return "ParticleCenter"; }
        static const char* TargetKey() { return "ParticleTarget"; }
    };
}

#include "ParticleTemplates.h"

namespace RyoEngine {

    const std::vector<ParticleTemplates::Entry>& ParticleTemplates::GetAll() {
        static const std::vector<Entry> entries = {
            { "空 (初期値)", "New", &ParticleTemplates::Blank },
            { "収束チャージ (輪→弧→中心→連鎖で爆発)", "Charge", &ParticleTemplates::Charge },
            { "放射の火花 (点滅で消える)", "Burst", &ParticleTemplates::Burst },
            { "花火 (分裂)", "Fireworks", &ParticleTemplates::Fireworks },
            { "衝撃波の輪", "RingWave", &ParticleTemplates::RingWave },
            { "渦 (色変化で消える)", "Vortex", &ParticleTemplates::Vortex },
            { "追尾", "Homing", &ParticleTemplates::Homing },
            { "カーブ (伸びて消える)", "Curve", &ParticleTemplates::Curve },
            { "電気 (ジッター)", "Electric", &ParticleTemplates::Electric },
            { "火花のバウンド (重力)", "Sparks", &ParticleTemplates::Sparks },
            { "吸い込み", "Absorb", &ParticleTemplates::Absorb },
        };
        return entries;
    }

    ParticleConfig ParticleTemplates::Blank() {
        return ParticleConfig{};
    }

    ParticleConfig ParticleTemplates::Charge() {
        ParticleConfig c{};
        c.emitMode = ParticleEmitMode::Continuous;
        c.maxParticles = 1024;
        c.rate = 280.0f;
        c.duration = 1.0f;
        c.positionKey = CenterKey();

        c.spawnShape = ParticleSpawnShape::Ring;
        c.radius = 6.0f;
        c.innerRatio = 1.0f;
        c.axisFacesCamera = true;
        c.shrinkTime = 1.0f;
        c.shrinkEndRatio = 0.4f;

        c.motion = ParticleMotion::Converge;
        c.arcAngle = 120.0f;
        c.easePower = 2.2f;

        c.lifeMin = 0.7f;
        c.lifeMax = 0.9f;
        c.scaleMin = 0.08f;
        c.scaleMax = 0.16f;
        c.startScale = 0.6f;
        c.endScale = 1.4f;

        c.death = ParticleDeath::Shrink;
        c.deathRatio = 0.25f;

        c.look = ParticleLook::SoftCircle;
        c.colorA = { 0.3f, 0.8f, 1.0f, 1.0f };
        c.colorB = { 0.7f, 0.4f, 1.0f, 1.0f };
        c.endColor = { 1.5f, 1.5f, 1.5f, 1.0f };
        c.intensity = 3.0f;
        c.softness = 1.5f;
        c.stretch = 0.04f;
        c.speedBrightness = 0.05f;

        // 連鎖：集まりきったところで爆発 (放射の火花 + 衝撃波の輪 + 花火)
        c.followUps = {
            ParticleFollowUp{ "Burst", 1.05f, true },
            ParticleFollowUp{ "RingWave", 1.05f, true },
            ParticleFollowUp{ "Fireworks", 1.05f, true },
        };
        return c;
    }

    ParticleConfig ParticleTemplates::Burst() {
        ParticleConfig c{};
        c.emitMode = ParticleEmitMode::Burst;
        c.maxParticles = 1024;
        c.burstCount = 260;
        c.positionKey = CenterKey();

        c.spawnShape = ParticleSpawnShape::Sphere;
        c.radius = 0.2f;

        c.motion = ParticleMotion::Explode;
        c.speedMin = 6.0f;
        c.speedMax = 20.0f;
        c.drag = 3.0f;
        c.gravity = 3.0f;

        c.lifeMin = 0.5f;
        c.lifeMax = 1.0f;
        c.scaleMin = 0.05f;
        c.scaleMax = 0.12f;

        c.death = ParticleDeath::Blink;
        c.deathRatio = 0.5f;
        c.blinkFrequencyMin = 10.0f;
        c.blinkFrequencyMax = 30.0f;
        c.blinkFade = 0.3f;

        c.look = ParticleLook::Spark;
        c.colorA = { 1.0f, 0.9f, 0.4f, 1.0f };
        c.colorB = { 1.0f, 0.5f, 0.1f, 1.0f };
        c.endColor = { 1.0f, 0.6f, 0.3f, 1.0f };
        c.intensity = 4.0f;
        c.softness = 2.0f;
        c.stretch = 0.06f;
        c.speedBrightness = 0.03f;
        return c;
    }

    ParticleConfig ParticleTemplates::Fireworks() {
        ParticleConfig c{};
        c.emitMode = ParticleEmitMode::Burst;
        c.maxParticles = 2048;
        c.burstCount = 36;
        c.positionKey = CenterKey();

        c.spawnShape = ParticleSpawnShape::Sphere;
        c.radius = 0.1f;

        c.motion = ParticleMotion::Explode;
        c.speedMin = 8.0f;
        c.speedMax = 11.0f;
        c.gravity = 5.0f;
        c.drag = 1.2f;

        c.lifeMin = 0.9f;
        c.lifeMax = 1.1f;
        c.scaleMin = 0.12f;
        c.scaleMax = 0.16f;

        c.death = ParticleDeath::Split;
        c.splitCount = 10;
        c.splitSpeed = 4.0f;
        c.splitLife = 0.7f;
        c.splitScale = 0.5f;

        c.look = ParticleLook::Star;
        c.colorA = { 1.0f, 0.3f, 0.8f, 1.0f };
        c.colorB = { 0.6f, 0.4f, 1.0f, 1.0f };
        c.intensity = 3.0f;
        c.stretch = 0.03f;
        return c;
    }

    ParticleConfig ParticleTemplates::RingWave() {
        ParticleConfig c{};
        c.emitMode = ParticleEmitMode::Burst;
        c.maxParticles = 512;
        c.burstCount = 160;
        c.positionKey = CenterKey();

        c.spawnShape = ParticleSpawnShape::Ring;
        c.radius = 0.3f;
        c.innerRatio = 1.0f;
        c.axisFacesCamera = true;

        c.motion = ParticleMotion::Explode;
        c.speedMin = 14.0f;
        c.speedMax = 14.0f;
        c.drag = 3.5f;

        c.lifeMin = 0.6f;
        c.lifeMax = 0.7f;
        c.scaleMin = 0.1f;
        c.scaleMax = 0.14f;
        c.endScale = 0.3f;

        c.death = ParticleDeath::Fade;
        c.deathRatio = 0.7f;

        c.look = ParticleLook::SoftCircle;
        c.colorA = { 1.0f, 1.0f, 1.0f, 1.0f };
        c.colorB = { 0.8f, 0.9f, 1.0f, 1.0f };
        c.intensity = 3.0f;
        c.stretch = 0.02f;
        return c;
    }

    ParticleConfig ParticleTemplates::Vortex() {
        ParticleConfig c{};
        c.emitMode = ParticleEmitMode::Continuous;
        c.maxParticles = 1024;
        c.rate = 150.0f;
        c.duration = 3.0f;
        c.positionKey = CenterKey();
        c.positionOffset = { 0.0f, -1.5f, 0.0f };

        c.spawnShape = ParticleSpawnShape::Ring;
        c.radius = 3.0f;
        c.innerRatio = 0.8f;
        c.shapeAxis = { 0.0f, 1.0f, 0.0f };

        c.motion = ParticleMotion::Vortex;
        c.motionAxis = { 0.0f, 1.0f, 0.0f };
        c.vortexAngularSpeed = 220.0f;
        c.vortexRadialSpeed = -0.6f;
        c.vortexRiseSpeed = 2.5f;

        c.lifeMin = 1.6f;
        c.lifeMax = 2.0f;
        c.scaleMin = 0.06f;
        c.scaleMax = 0.12f;

        c.death = ParticleDeath::ColorShift;
        c.deathRatio = 0.6f;
        c.deathColor = { 0.2f, 0.4f, 1.0f, 1.0f };

        c.look = ParticleLook::SoftCircle;
        c.colorA = { 1.0f, 1.0f, 1.0f, 1.0f };
        c.colorB = { 0.6f, 0.9f, 1.0f, 1.0f };
        c.intensity = 2.5f;
        c.stretch = 0.03f;
        return c;
    }

    ParticleConfig ParticleTemplates::Homing() {
        ParticleConfig c{};
        c.emitMode = ParticleEmitMode::Burst;
        c.maxParticles = 512;
        c.burstCount = 50;
        c.burstRepeat = 4;
        c.burstInterval = 0.2f;
        c.positionKey = CenterKey();
        c.targetKey = TargetKey();

        c.spawnShape = ParticleSpawnShape::Sphere;
        c.radius = 0.3f;

        c.velocityMode = ParticleVelocityMode::Random;
        c.velocityMin = { -12.0f, 4.0f, -12.0f };
        c.velocityMax = { 12.0f, 14.0f, 12.0f };

        c.motion = ParticleMotion::Homing;
        c.homingSpeed = 16.0f;
        c.homingTurn = 3.5f;
        c.arriveRadius = 0.6f;
        c.noiseStrength = 4.0f;
        c.noiseFrequency = 0.5f;

        c.lifeMin = 2.5f;
        c.lifeMax = 3.0f;
        c.scaleMin = 0.08f;
        c.scaleMax = 0.12f;

        c.death = ParticleDeath::Fade;
        c.deathRatio = 0.2f;

        c.look = ParticleLook::Spark;
        c.colorA = { 0.4f, 1.0f, 0.6f, 1.0f };
        c.colorB = { 0.1f, 1.0f, 0.9f, 1.0f };
        c.intensity = 3.0f;
        c.stretch = 0.06f;
        return c;
    }

    ParticleConfig ParticleTemplates::Curve() {
        ParticleConfig c{};
        c.emitMode = ParticleEmitMode::Burst;
        c.maxParticles = 512;
        c.burstCount = 70;
        c.positionKey = CenterKey();
        c.targetKey = TargetKey();

        c.spawnShape = ParticleSpawnShape::Sphere;
        c.radius = 0.4f;

        c.motion = ParticleMotion::Bezier;
        c.easePower = 1.6f;
        c.controlOffsetMin = { -7.0f, 3.0f, -7.0f };
        c.controlOffsetMax = { 7.0f, 9.0f, 7.0f };

        c.lifeMin = 1.0f;
        c.lifeMax = 1.5f;
        c.scaleMin = 0.1f;
        c.scaleMax = 0.16f;

        c.death = ParticleDeath::Stretch;
        c.deathRatio = 0.3f;
        c.deathStretch = 4.0f;

        c.look = ParticleLook::Star;
        c.colorA = { 1.0f, 0.8f, 0.3f, 1.0f };
        c.colorB = { 1.0f, 0.4f, 0.6f, 1.0f };
        c.intensity = 3.0f;
        c.stretch = 0.02f;
        return c;
    }

    ParticleConfig ParticleTemplates::Electric() {
        ParticleConfig c{};
        c.emitMode = ParticleEmitMode::Continuous;
        c.maxParticles = 512;
        c.rate = 160.0f;
        c.duration = 2.5f;
        c.positionKey = TargetKey();

        c.spawnShape = ParticleSpawnShape::Sphere;
        c.radius = 1.0f;
        c.innerRatio = 0.3f;

        c.velocityMode = ParticleVelocityMode::Random;
        c.velocityMin = { -0.5f, -0.5f, -0.5f };
        c.velocityMax = { 0.5f, 0.5f, 0.5f };
        c.motion = ParticleMotion::Physics;
        c.jitterStrength = 0.3f;
        c.jitterFrequency = 30.0f;

        c.lifeMin = 0.12f;
        c.lifeMax = 0.3f;
        c.scaleMin = 0.06f;
        c.scaleMax = 0.14f;

        c.death = ParticleDeath::Blink;
        c.deathRatio = 0.8f;
        c.blinkFrequencyMin = 20.0f;
        c.blinkFrequencyMax = 40.0f;
        c.blinkFade = 0.0f;

        c.look = ParticleLook::Spark;
        c.colorA = { 0.5f, 0.8f, 1.0f, 1.0f };
        c.colorB = { 0.9f, 0.95f, 1.0f, 1.0f };
        c.intensity = 5.0f;
        c.softness = 3.0f;
        return c;
    }

    ParticleConfig ParticleTemplates::Sparks() {
        ParticleConfig c{};
        c.emitMode = ParticleEmitMode::Continuous;
        c.maxParticles = 1024;
        c.rate = 90.0f;
        c.duration = 3.0f;
        c.positionKey = CenterKey();

        c.spawnShape = ParticleSpawnShape::Point;

        c.velocityMode = ParticleVelocityMode::Random;
        c.velocityMin = { -5.0f, 8.0f, -5.0f };
        c.velocityMax = { 5.0f, 14.0f, 5.0f };
        c.motion = ParticleMotion::Physics;
        c.gravity = 20.0f;
        c.drag = 0.2f;
        c.bounce = true;
        c.floorY = 0.0f;
        c.restitution = 0.45f;
        c.friction = 0.7f;

        c.lifeMin = 1.6f;
        c.lifeMax = 2.2f;
        c.scaleMin = 0.04f;
        c.scaleMax = 0.08f;

        c.death = ParticleDeath::ColorShift;
        c.deathRatio = 0.5f;
        c.deathColor = { 1.0f, 0.25f, 0.05f, 1.0f };

        c.look = ParticleLook::Spark;
        c.colorA = { 1.0f, 1.0f, 0.8f, 1.0f };
        c.colorB = { 1.0f, 0.85f, 0.4f, 1.0f };
        c.intensity = 4.0f;
        c.stretch = 0.05f;
        return c;
    }

    ParticleConfig ParticleTemplates::Absorb() {
        ParticleConfig c{};
        c.emitMode = ParticleEmitMode::Burst;
        c.maxParticles = 1024;
        c.burstCount = 500;
        c.positionKey = CenterKey();

        c.spawnShape = ParticleSpawnShape::Sphere;
        c.radius = 7.0f;
        c.innerRatio = 0.0f;

        c.velocityMode = ParticleVelocityMode::Random;
        c.velocityMin = { -0.5f, -0.5f, -0.5f };
        c.velocityMax = { 0.5f, 0.5f, 0.5f };
        c.motion = ParticleMotion::Physics;
        c.noiseStrength = 5.0f;
        c.noiseFrequency = 0.4f;
        c.drag = 1.0f;

        c.lifeMin = 1.6f;
        c.lifeMax = 2.2f;
        c.scaleMin = 0.06f;
        c.scaleMax = 0.12f;

        c.death = ParticleDeath::Absorb;
        c.deathRatio = 0.55f;
        c.absorbStrength = 25.0f;

        c.look = ParticleLook::SoftCircle;
        c.colorA = { 0.7f, 0.3f, 1.0f, 1.0f };
        c.colorB = { 0.3f, 0.5f, 1.0f, 1.0f };
        c.intensity = 2.5f;
        c.stretch = 0.02f;
        return c;
    }
}

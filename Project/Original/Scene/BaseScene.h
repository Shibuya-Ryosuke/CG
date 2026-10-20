#pragma once

namespace RyoEngine {

    /// <summary>
    /// シーン基底クラス
    /// シーンはSceneManagerが生成・破棄する(切り替え・リロードのたびに作り直される)
    /// </summary>
    class BaseScene {
    public:
        virtual ~BaseScene() = default;

        virtual void Initialize() = 0;
        virtual void Update() = 0;
        virtual void Draw() = 0;
        virtual void Finalize() {}
    };

}

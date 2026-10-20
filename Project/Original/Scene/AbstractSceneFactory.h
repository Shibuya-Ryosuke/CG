#pragma once
#include "BaseScene.h"
#include <memory>
#include <string>

namespace RyoEngine {

    /// <summary>
    /// シーン工場(概念)
    /// 具体的なシーンの生成はアプリ側の具象クラスで行う
    /// </summary>
    class AbstractSceneFactory {
    public:
        /// 仮想デストラクタ
        virtual ~AbstractSceneFactory() = default;

        /// <summary>
        /// シーン生成
        /// </summary>
        /// <param name="sceneName">シーン名</param>
        /// <returns>生成したシーン(該当なしならnullptr)</returns>
        virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;
    };

}

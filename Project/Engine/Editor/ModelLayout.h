#pragma once
#include <string>
#include "../Core/Math/Transform.h"

namespace RyoEngine {
    class Model;

    /// <summary>
    /// 全Modelの Transform を、エンジンが一括で保存・復元するための仕組み。
    ///
    /// ・Model を Create すると自動で登録され、キー ("objのパス#何番目" か SetName で付けた名前") が付く
    /// ・ギズモで動かした Model(=編集済み) だけを、保存ボタンで JSON へ書き出す
    /// ・次回以降、同じキーの Model は、最初の TransferMatrix で保存した Transform が適用される
    /// ・使う側のコードは何も要らない (名前を付けたいときだけ model->SetName)
    ///
    /// 保存先: Resources/EditorData/ModelLayout.json  { "シーン名": { "キー": {scale, rotate(度数法), translate} } }
    /// 「何番目」は、同じobjを生成した順番。毎回同じ順で生成されるものだけが、正しく復元される。
    /// 実行中に増減するもの(敵の出現など)は、保存した番号の後ろに続くので影響しない。
    /// </summary>
    class ModelLayout {
    public:
        // Model が持つ、自分の識別情報
        struct Slot {
            std::string key;   // JSONのキー
            std::string path;  // 自動キーのとき、そのobjパス (カウンタ管理用)。名前付きなら空
        };

        /// <summary>
        /// 保存先の区切り(シーン名など)。省略時は "Default"。ゲーム側から呼ぶなら、シーン開始時に1回。
        /// </summary>
        static void SetScope(const std::string& scope);
        static const std::string& GetScope();

        // ---- Model から呼ばれる (ゲーム側は呼ばない) ----
        static Slot Register(Model* model, const std::string& objPath);
        static void Unregister(Model* model, const Slot& slot);
        static Slot Rename(Model* model, const Slot& old, const std::string& name);
        /// <summary>保存された Transform があれば返す</summary>
        static bool TryGetSaved(const std::string& key, Transform& out);

        // ---- エディタ用 ----
        /// <summary>編集済みの Model の Transform を JSON へ書き出す。成功したら true</summary>
        static bool Save();
        /// <summary>このキーの保存値を削除 (ファイルへは次の Save で反映)</summary>
        static void Erase(const std::string& key);
        static bool HasSaved(const std::string& key);
    };
}

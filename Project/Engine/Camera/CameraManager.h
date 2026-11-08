#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include "Camera.h"
#include "DebugCamera.h"

namespace RyoEngine {

    class CameraManager {
    public:
        static constexpr uint32_t kInvalidIndex = 0;

        CameraManager() = delete; // 静的クラスなので実体は作らない

        static void Initialize();
        static void Finalize();

        // ---------------- 生成・破棄 ----------------

        /// <summary>
        /// カメラを生成し、Initialize()を呼んだうえで登録する。派生クラスも指定できる。
        /// 最初に生成したカメラは自動でアクティブになる。
        /// 名前は省略可(ImGuiでの表示とFindIndex()用)。コンストラクタ引数を渡す場合は名前も指定すること(""でも可)。
        /// 例: CreateCamera() / CreateCamera("Main") / CreateCamera&lt;MyCamera&gt;("Boss", 引数...)
        /// </summary>
        /// <returns>インデックス(1始まりの通し番号)</returns>
        template <class T = Camera, class... Args>
        static uint32_t CreateCamera(const std::string& name, Args&&... args) {
            static_assert(std::is_base_of_v<Camera, T>, "CreateCameraにはCameraの派生クラスを指定すること");
            std::unique_ptr<Camera> camera = std::make_unique<T>(std::forward<Args>(args)...);
            camera->Initialize();
            return RegisterCamera(name, std::move(camera));
        }

        /// <summary>名前なしでカメラを生成する(ImGuiでは "Camera [番号]" と表示される)</summary>
        template <class T = Camera>
        static uint32_t CreateCamera() {
            return CreateCamera<T>(std::string{});
        }

        /// <summary>
        /// カメラを破棄する。アクティブ中なら履歴から1つ前に戻る(履歴が無ければデフォルトカメラ)。
        /// </summary>
        static void DestroyCamera(uint32_t index);

        /// <summary>
        /// 全カメラを破棄する。シーンを抜けるときに呼ぶこと(呼ばないと前のシーンのカメラが残る)。
        /// NOTE: インデックスは通し番号なので、Clear後も番号は続きから振られる。
        ///       破棄済みの古いインデックスでGetCamera()するとnullptrが返る(別のカメラを指すことはない)。
        /// </summary>
        static void Clear();

        // ---------------- 取得 ----------------

        /// <summary>インデックスからカメラを取得する。無効・破棄済みならnullptr。ポインタはDestroy/Clearまで有効</summary>
        static Camera* GetCamera(uint32_t index);

        /// <summary>派生クラスとして取得する。型が違う・無効ならnullptr</summary>
        template <class T>
        static T* GetCamera(uint32_t index) {
            return dynamic_cast<T*>(GetCamera(index));
        }

        /// <summary>名前からインデックスを探す。無い・空文字ならkInvalidIndex</summary>
        static uint32_t FindIndex(const std::string& name);

        // ---------------- 切り替え ----------------

        /// <summary>即座に切り替える(Pushの履歴はクリアされる)</summary>
        static void SetActive(uint32_t index);
        /// <summary>今のカメラを履歴に積んでから切り替える(演出カメラなど、後で戻したいとき)</summary>
        static void PushActive(uint32_t index);
        /// <summary>履歴から1つ前のカメラに戻す</summary>
        static void PopActive();
        /// <summary>ゲーム側のアクティブカメラのインデックス(DebugCameraは含まない)</summary>
        static uint32_t GetActiveIndex();

        /// <summary>
        /// 描画に使うカメラ。DebugCameraが有効ならDebugCamera、それ以外はゲーム側のアクティブカメラ。
        /// UpdateAll()で確定した値を返すので、同じフレーム内ではModel/パーティクル等が全員同じカメラを見る。
        /// (一度もUpdateAll()を呼んでいない間は、その場で判断した結果を返す)
        /// </summary>
        static Camera& GetActive();

        /// <summary>
        /// ゲームロジック用のカメラ。DebugCameraの有効/無効に関係なく、常にゲーム側のアクティブカメラを返す。
        /// カメラ基準のプレイヤー移動など「デバッグ中もゲームカメラ基準でないと困る」計算に使う。
        /// 確定前の最新状態を返す(UpdateAll()前に呼んでも、SetRotate()等の結果が反映された向きが取れる)。
        /// </summary>
        static Camera& GetGameCamera();

        // ---------------- デバッグカメラ ----------------

        static DebugCamera& GetDebugCamera();
        /// <summary>デバッグカメラの操作を反映する(エンジンのNewFrame()から呼ばれる)</summary>
        static void UpdateDebugCamera();

        // ---------------- 毎フレーム ----------------

        /// <summary>
        /// 全カメラのUpdate()を行い、描画カメラを確定させる。
        /// エンジンのEndUpdate()から毎フレーム呼ばれる(ゲーム側から呼ぶ必要はない)。
        /// Camera::SetActive(false)にしたカメラはUpdate()をスキップする。
        /// </summary>
        static void UpdateAll();

        /// <summary>カメラ一覧・切り替えボタンのImGuiパネル(デバッグ用)</summary>
        static void DrawImGui();

    private:
        static uint32_t RegisterCamera(const std::string& name, std::unique_ptr<Camera> camera);
    };
}
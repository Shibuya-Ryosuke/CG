#pragma once
#include <cstdint>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include "../Math/Math.h"
#include "../Camera/Camera.h"

namespace RyoEngine {

    // 描画モード
    enum class PrimitiveDrawMode {
        Wireframe,  // 線だけ描画(中身は透けて見える)
        Fill,       // 面を塗りつぶして描画(color.wで半透明可)
    };

    /// <summary>
    /// 即時描画(イミディエイトモード)で図形を描く静的クラス。
    /// AABB/OBBなどの当たり判定形状の可視化や、デバッグ用の図形・線描画に使う。
    ///
    /// - new/インスタンス化は禁止。PrimitiveRenderer::DrawSphere(...) のように、
    ///   すべて static 関数として呼び出す。
    /// - 内部で「今フレーム描画するように頼まれた形状」をCPU側に一時的に溜め込み、
    ///   実際のGPUへの送信は Flush() でまとめて行う(1形状=1DrawCallにならないようにするため)。
    /// </summary>
    class PrimitiveRenderer {
    public:
        // --- インスタンス化禁止 ---
        PrimitiveRenderer() = delete;
        ~PrimitiveRenderer() = delete;
        PrimitiveRenderer(const PrimitiveRenderer&) = delete;
        PrimitiveRenderer& operator=(const PrimitiveRenderer&) = delete;

        /// <summary>
        /// エンジン起動時に1度だけ呼ぶ(GPUリソース確保)
        /// </summary>
        static void Initialize();

        /// <summary>
        /// 終了時に1度だけ呼ぶ
        /// </summary>
        static void Finalize();

        /// <summary>
        /// 毎フレームの先頭で1度呼ぶ。前フレームに溜め込んだ描画データをクリアする。
        /// DrawXXX() より前に呼ぶこと。
        /// </summary>
        static void NewFrame();

        /// <summary>
        /// 3D用のカメラを設定する。DrawSphere/DrawBox/DrawLine3Dより前に、毎フレーム1度呼ぶこと。
        /// </summary>
        static void SetCamera(const Camera& camera);

        /// <summary>
        /// 2D用のスクリーンサイズを設定する(省略時は1280x720)。
        /// </summary>
        static void SetScreenSize(float width, float height);

        /// <summary>
        /// このフレームに溜め込んだ描画データをまとめてGPUに送る(描画コマンドを積む)。
        /// 3D描画のところで1度だけ呼ぶこと。
        /// </summary>
        static void Flush();

        // ===================== 3D =====================

        /// <summary>
        /// 球を描画する
        /// </summary>
        /// <param name="center">中心座標</param>
        /// <param name="radius">半径</param>
        /// <param name="subdivision">分割数(大きいほど滑らか)</param>
        /// <param name="color">色(w=アルファ。半透明可)</param>
        /// <param name="mode">ワイヤーフレーム/塗りつぶし</param>
        static void DrawSphere(const Vector3& center, float radius, uint32_t subdivision,
            const Vector4& color, PrimitiveDrawMode mode);

        /// <summary>
        /// 直方体を描画する。AABBの可視化ならrotateを{0,0,0}に、OBBの可視化ならrotateにOBBの回転を渡す。
        /// </summary>
        /// <param name="center">中心座標</param>
        /// <param name="rotate">回転(ラジアン)</param>
        /// <param name="size">縦横高さ(1辺の全長。半径ではない)</param>
        /// <param name="color">色(w=アルファ。半透明可)</param>
        /// <param name="mode">ワイヤーフレーム/塗りつぶし</param>
        static void DrawBox(const Vector3& center, const Vector3& rotate, const Vector3& size,
            const Vector4& color, PrimitiveDrawMode mode);

        /// <summary>
        /// AABB構造体をそのまま渡して描画する(rotateは常に0)
        /// </summary>
        static void DrawAABB(const AABB& aabb, const Vector4& color, PrimitiveDrawMode mode);

        /// <summary>
        /// OBB構造体をそのまま渡して描画する
        /// </summary>
        static void DrawOBB(const OBB& obb, const Vector4& color, PrimitiveDrawMode mode);

        /// <summary>
        /// 3D空間上に直線を1本描画する
        /// </summary>
        static void DrawLine3D(const Vector3& start, const Vector3& end, const Vector4& color);




        /// <summary>
        /// 2D矩形を描画する
        /// </summary>
        /// <param name="center">中心座標(ピクセル)</param>
        /// <param name="size">横幅・縦幅(ピクセル)</param>
        /// <param name="rotate">回転(ラジアン)</param>
        static void DrawRect2D(const Vector2& center, const Vector2& size, float rotate,
            const Vector4& color, PrimitiveDrawMode mode);

        /// <summary>
        /// 2D円を描画する
        /// </summary>
        static void DrawCircle2D(const Vector2& center, float radius, uint32_t subdivision,
            const Vector4& color, PrimitiveDrawMode mode);

        /// <summary>
        /// 2D空間上に直線を1本描画する
        /// </summary>
        static void DrawLine2D(const Vector2& start, const Vector2& end, const Vector4& color);

    private:
        // GPUに送る最終的な頂点フォーマット。
        // ワールド(または2Dスクリーン)座標まで変換済みの状態で溜め込み、
        // VS側ではカメラのViewProjection(または2D用の正射影行列)を掛けるだけで済むようにする。
        struct PrimitiveVertex {
            Vector4 position;
            Vector4 color;
        };

        // ---- 形状 -> 頂点列 への変換ヘルパー(CPU側のみで完結) ----
        static void AppendSphere(const Vector3& center, float radius, uint32_t subdivision,
            const Vector4& color, PrimitiveDrawMode mode);
        static void AppendBox(const Matrix4x4& worldMatrix, const Vector3& halfSize,
            const Vector4& color, PrimitiveDrawMode mode);

        // ---- GPUリソース生成(Initialize()から呼ぶ) ----
        static void CreateRootSignature();
        static void CreatePipelineStates();

        // 蓄積バッファ (3D/2D、線/塗りつぶしで4本に分ける。PSOの切り替え回数を最小にするため)
        static std::vector<PrimitiveVertex> lineVertices3D_;
        static std::vector<PrimitiveVertex> triVertices3D_;
        static std::vector<PrimitiveVertex> lineVertices2D_;
        static std::vector<PrimitiveVertex> triVertices2D_;

        static Matrix4x4 viewProjectionMatrix_;   // 3D用(SetCameraで更新)
        static Matrix4x4 orthographicMatrix_;     // 2D用(SetScreenSizeで更新)
        static float screenWidth_;
        static float screenHeight_;

        // ---- GPU側リソース ----
        // NOTE: 頂点色をそのまま使い、テクスチャ/ライトを使わない専用のPSO・RootSignature・Shaderが必要。
        //       詳細は実装(.cpp)側の先頭コメントを参照。
        static Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        static Microsoft::WRL::ComPtr<ID3D12PipelineState> linePSO_;      // D3D_PRIMITIVE_TOPOLOGY_LINELIST
        static Microsoft::WRL::ComPtr<ID3D12PipelineState> trianglePSO_;  // D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST(アルファブレンド有効)

        // 動的頂点バッファ(毎フレーム書き換えるUpload Heap。常時Map)
        static Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
        static PrimitiveVertex* mappedVertexData_;
        static uint32_t vertexCapacity_;

        // カメラ用定数バッファ(3D用/2D用)
        static Microsoft::WRL::ComPtr<ID3D12Resource> vpResource3D_;
        static Matrix4x4* vpData3D_;
        static Microsoft::WRL::ComPtr<ID3D12Resource> vpResource2D_;
        static Matrix4x4* vpData2D_;
    };
}

#include "./Original/OriginalEngine.h"

using namespace Engine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    Initialize();

    // テクスチャ
    uint32_t textureHandle = textureManager_->Load("resources/uvChecker.png");

    // 3d
    Object3d* model = Object3d::Create("resources/axis.obj");
    model->SetTexture(textureHandle);
    model->SetReflectionMode(ReflectionMode::HALF_LAMBERT);

    // 画像
    Sprite* sprite = new Sprite();
    sprite->Initialize(textureHandle, {100.0f,100.0f});

    // 音
    uint32_t alarm = Audio::LoadAudio("resources/Alarm01.wav");
    Audio::PlayAudio(alarm, 1.0f);

    // カメラ
    Camera* camera = new Camera();




    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // --- 更新処理 (Update) ---
        // 入力受付
        Input::Update();

        if (Input::IsMousePush(0)) {
            break;
        }

        model->Update(*camera);
        sprite->Update();

        // --- 描画処理 (Draw) ---
        GetDxCommon()->PreDraw();

        // [3D描画フェーズ]
        GetObject3dCommon()->BeginDraw();

        model->Draw();
       

        // [2D描画フェーズ]
        GetSpriteCommon()->BeginDraw();

        sprite->Draw();

        // 画面表示（PostDraw、コマンドリスト実行、スワップチェーン入れ替え）
        GetDxCommon()->PostDraw();
    }

    // エンジン終了
    Finalize();

    return 0;
}
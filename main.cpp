#include "./Original/OriginalEngine.h"

using namespace Engine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

    winApp_ = WinApp::GetInstance();
    winApp_->Initialize(L"test");

    // 2. DirectX基盤の初期化[cite: 2]
    dxCommon_ = DirectXCommon::GetInstance();
    dxCommon_->Initialize(winApp_);

    shaderCompiler_ = Engine::ShaderCompiler::GetInstance();
    shaderCompiler_->Initialize();

    // 3. テクスチャマネージャーの初期化[cite: 6]
    textureManager_ = TextureManager::GetInstance();
    textureManager_->Initialize();

    // 4. 各種描画共通部の初期化[cite: 9, 11]
    object3dCommon_ = Object3dCommon::GetInstance();
    object3dCommon_->Initialize();

    spriteCommon_ = SpriteCommon::GetInstance();
    spriteCommon_->Initialize();

    // 5. オーディオの初期化[cite: 15]
    audio_ = Audio::GetInstance();
    audio_->Initialize();

    // 2. 共通クラスのインスタンス取得（シングルトン）
    Object3dCommon* object3dCommon = Object3dCommon::GetInstance();
    SpriteCommon* spriteCommon = SpriteCommon::GetInstance();

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
    while (winApp_->ProcessMessage()) {


        // --- 更新処理 (Update) ---
        // 入力受付、座標計算、行列更新などをここで行う
        model->Update(*camera);
        sprite->Update();

        // --- 描画処理 (Draw) ---
        // 画面クリア（PreDraw）
        dxCommon_->PreDraw();

        // [3D描画フェーズ]
        // 3D用のパイプラインとルートシグネチャを1回だけセット
        object3dCommon->BeginDraw();

        // あとは描画したい3Dモデルを並べるだけ
        model->Draw();
       

        // [2D描画フェーズ]
        // スプライト用の設定に切り替え（上書き）
        spriteCommon->BeginDraw();

        // UIや背景などのスプライトを描画
        sprite->Draw();

        // 画面表示（PostDraw、コマンドリスト実行、スワップチェーン入れ替え）
        dxCommon_->PostDraw();
    }

    // 4. 終了処理
    // 各リソースの解放、WinAppのUnregisterClassなどが走る
       // 初期化と逆の順序で解放
    audio_->Finalize();
    Audio::DestroyInstance();

    spriteCommon_->Finalize();
    object3dCommon_->Finalize();
    textureManager_->Finalize();
    shaderCompiler_->Finalize();

    // DirectXの基盤を止める（デバイスなどの破棄）
    dxCommon_->Finalize();

    // 最後に WindowsAPI の登録を解除する
    winApp_->Finalize();

    return 0;
}
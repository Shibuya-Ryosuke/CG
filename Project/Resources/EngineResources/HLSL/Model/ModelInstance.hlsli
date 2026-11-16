// C++側 ModelInstanceData (ModelManager.h) と1:1でレイアウトを合わせること。全体で176byte。
// StructuredBuffer は要素が詰めて並ぶので、フィールドの順番とサイズだけ合っていればよい。
struct ModelInstance
{
    float4x4 world;         // 64
    float4 color;           // 16  RGBA
    float4 emissive;        // 16  rgb=発光色, a=強度
    float4 uvMat;           // 16  UV変換の2x2 (x=m00, y=m01, z=m10, w=m11)
    float2 uvOffset;        //  8  UV変換の平行移動 (m30, m31)
    int enableLighting;     //  4
    int shadingMode;        //  4  0:None 1:Lambert 2:HalfLambert 3:Phong 4:BlinnPhong
    float4 params[2];       // 32  自由な値 (C++の Model::SetShaderParam。0〜3番 = params[0].xyzw、4〜7番 = params[1].xyzw)
    float4 specular;        // 16  rgb=鏡面反射の色×強さ, a=鋭さ(shininess)。Phong / BlinnPhong のときだけ使う (Model::SetSpecular)
};

// インスタンス配列。C++側 root param 1 (Root SRV, ALL)。
// グループごとにアドレスをずらして渡すので、SV_InstanceID は常に「そのグループの先頭から数えた番号」になる。
StructuredBuffer<ModelInstance> gInstances : register(t3);

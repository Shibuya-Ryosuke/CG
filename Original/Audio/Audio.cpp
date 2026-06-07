#include "Audio.h"
#include <mfapi.h>
#include <mfidl.h>  // これがないとエラーになる
#include <mfreadwrite.h>
#include <string>

#include <cassert>

namespace RyoEngine {
	// インスタンス初期化
	Audio* Audio::instance = nullptr;

	Audio* Audio::GetInstance() {
		if (instance == nullptr) {
			instance = new Audio();
		}
		return instance;
	}

	void Audio::DestroyInstance() {
		if (instance) {
			delete instance;
			instance = nullptr;
		}
	}

	void Audio::Initialize() {
		HRESULT hr = S_OK;

		// MF初期化
		hr = MFStartup(MF_VERSION);
		assert(SUCCEEDED(hr));

		// インスタンスを取得
		Audio* inst = GetInstance();

		// XAudio2作成
		hr = XAudio2Create(&inst->xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
		assert(SUCCEEDED(hr));

		// Mastering Voice作成
		hr = inst->xAudio2->CreateMasteringVoice(&inst->masterVoice);
		assert(SUCCEEDED(hr));
	}

	void Audio::Finalize() {
		// インスタンスを取得
		Audio* inst = GetInstance();

		// 各音声が持ってるボイスを解放
		for (auto& data : inst->soundDatas) {
			if (data.pSourceVoice) {
				data.pSourceVoice->DestroyVoice();
				data.pSourceVoice = nullptr;
			}
		}

		// vectorをクリア
		inst->soundDatas.clear();

		// XAudio2やMF終了
		if (inst->masterVoice) {
			inst->masterVoice->DestroyVoice();
			inst->masterVoice = nullptr;
		}
		inst->xAudio2.Reset();
		MFShutdown();
	}

	uint32_t Audio::LoadAudio(const char* filename) {
		// インスタンス取得
		Audio* inst = GetInstance();
		HRESULT hr = S_OK;

		// char*からwchar_t*へ変換
		// 必要なバッファサイズを取得
		int32_t size = MultiByteToWideChar(CP_ACP, 0, filename, -1, nullptr, 0);
		// 変換
		std::wstring wstr(static_cast<size_t>(size), L'\0');
		MultiByteToWideChar(CP_ACP, 0, filename, -1, &wstr[0], size);

		// SourceReaderの作成
		Microsoft::WRL::ComPtr<IMFSourceReader> pReader;
		hr = MFCreateSourceReaderFromURL(wstr.c_str(), nullptr, &pReader);
		assert(SUCCEEDED(hr));

		// 出力形式の設定
		Microsoft::WRL::ComPtr<IMFMediaType> pPartialType;
		MFCreateMediaType(&pPartialType);
		pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);  // 音声であることを指定
		pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);  // PCM形式(非圧縮)を指定
		pReader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr, pPartialType.Get());

		// 最終的なフォーマット情報を取得
		Microsoft::WRL::ComPtr<IMFMediaType> pUncompressedAudioType;
		pReader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &pUncompressedAudioType);
		WAVEFORMATEX* pWfex = nullptr;
		uint32_t cbFormat = 0;
		// MFから構造体を取り出す
		hr = MFCreateWaveFormatExFromMFMediaType(pUncompressedAudioType.Get(), &pWfex, &cbFormat);
		assert(SUCCEEDED(hr));

		// 構造体準備
		SoundData soundData = {};
		soundData.wfex = *pWfex;
		CoTaskMemFree(pWfex);  // 構造体コピー後解放

		// データの読み込みループ
		std::vector<BYTE> audioData;
		while (true) {
			DWORD dwFlags = 0;
			Microsoft::WRL::ComPtr<IMFSample> pSample;
			pReader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, nullptr, &dwFlags, nullptr, &pSample);

			if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM) break;  // ファイルの終わり

			if (pSample) {
				Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;
				// サンプルからバッファを取り出す
				pSample->ConvertToContiguousBuffer(&pBuffer);

				BYTE* pRawData = nullptr;
				DWORD currentLength = 0;

				// バッファをロックし、生データのポインタを取得
				pBuffer->Lock(&pRawData, nullptr, &currentLength);

				// audioDataベクトルに読み込んだサイズ分を一気にコピーして展開
				if (currentLength > 0) {
					// 直接メンバ変数のvectorへ追加
					soundData.buffer.insert(soundData.buffer.end(), pRawData, pRawData + currentLength);
				}

				// ロックを解除
				pBuffer->Unlock();
			}
		}

		// soundDatasに保存
		inst->soundDatas.push_back(soundData);

		// 保存した場所のインデックスを返す
		return static_cast<uint32_t>(inst->soundDatas.size() - 1);
	}

	void Audio::PlayAudio(uint32_t handle, float volume, bool loop) {
		// インスタンス取得
		Audio* inst = GetInstance();
		HRESULT hr = S_OK;

		// ハンドルが有効かチェック
		if (handle >= static_cast<uint32_t>(inst->soundDatas.size())) {
			return;
		}

		// データを取り出す
		SoundData& data = inst->soundDatas[static_cast<size_t>(handle)];

		// SourceVoiceがなければ作成
		if (data.pSourceVoice == nullptr) {
			hr = inst->xAudio2->CreateSourceVoice(&data.pSourceVoice, &data.wfex);
			assert(SUCCEEDED(hr));
		}

		// 音量設定
		data.pSourceVoice->SetVolume(volume);

		// 再生する波形データの設定
		XAUDIO2_BUFFER buf{};
		buf.pAudioData = data.buffer.data();
		buf.AudioBytes = static_cast<UINT32>(data.buffer.size());
		buf.Flags = XAUDIO2_END_OF_STREAM;

		// ループの設定
		if (loop) {
			buf.LoopCount = XAUDIO2_LOOP_INFINITE;  // 無限ループ(INFINITEなので)
		} else {
			buf.LoopCount = 0;  // ループ無し
		}

		// 再生
		// 最初から鳴らしなおす
		// (BGMはシーン遷移時か、シーンの1f目でだけ呼んでおかないと、毎フレーム最初の音しかならなくなってしまう。
		// 効果音は呼ばれるたびに最初から再生してくれるため、1周鳴っていなくても最初からにしてくれる)
		data.pSourceVoice->Stop();
		data.pSourceVoice->FlushSourceBuffers();

		hr = data.pSourceVoice->SubmitSourceBuffer(&buf);
		assert(SUCCEEDED(hr));

		hr = data.pSourceVoice->Start();
		assert(SUCCEEDED(hr));
	}

}

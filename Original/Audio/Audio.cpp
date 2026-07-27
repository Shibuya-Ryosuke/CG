#include "Audio.h"
#include "../Base/Logger.h"
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
		Logger::Log("Audio : Initializing...\n");
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

		Logger::LogSuccess("Audio : Initialized\n");
	}

	void Audio::Finalize() {
		Logger::Log("Audio : Finalizing...\n");
		// インスタンスを取得
		Audio* inst = GetInstance();

		// BGMが持ってるボイスを解放
		for (auto& data : inst->bgmDatas) {
			if (data.pSourceVoice) {
				data.pSourceVoice->DestroyVoice();
				data.pSourceVoice = nullptr;
			}
		}
		inst->bgmDatas.clear();

		// 再生中のSEボイスを全て解放
		for (auto& playing : inst->playingSEs) {
			if (playing.pSourceVoice) {
				playing.pSourceVoice->DestroyVoice();
			}
			delete playing.pCallback;
		}
		inst->playingSEs.clear();
		inst->seDatas.clear();

		// XAudio2やMF終了
		if (inst->masterVoice) {
			inst->masterVoice->DestroyVoice();
			inst->masterVoice = nullptr;
		}
		inst->xAudio2.Reset();
		MFShutdown();
		Logger::LogSuccess("Audio : Finalized\n");
	}

	void Audio::Update() {
		GetInstance()->CleanupFinishedSE();
	}

	void Audio::CleanupFinishedSE() {
		for (auto it = playingSEs.begin(); it != playingSEs.end(); ) {
			if (it->pCallback->isFinished) {
				it->pSourceVoice->DestroyVoice();
				delete it->pCallback;
				it = playingSEs.erase(it);
			} else {
				++it;
			}
		}
	}

	Audio::SoundData Audio::LoadWaveFile(const char* filename) {
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

				// 読み込んだサイズ分を一気にコピーして展開
				if (currentLength > 0) {
					soundData.buffer.insert(soundData.buffer.end(), pRawData, pRawData + currentLength);
				}

				// ロックを解除
				pBuffer->Unlock();
			}
		}

		return soundData;
	}

	uint32_t Audio::LoadBGM(const char* filename) {
		Audio* inst = GetInstance();

		BGMData data;
		data.sound = LoadWaveFile(filename);
		inst->bgmDatas.push_back(std::move(data));

		return static_cast<uint32_t>(inst->bgmDatas.size() - 1);
	}

	uint32_t Audio::LoadSE(const char* filename) {
		Audio* inst = GetInstance();

		inst->seDatas.push_back(LoadWaveFile(filename));

		return static_cast<uint32_t>(inst->seDatas.size() - 1);
	}

	void Audio::PlayBGM(uint32_t handle, float volume, bool loop) {
		// インスタンス取得
		Audio* inst = GetInstance();
		HRESULT hr = S_OK;

		// ハンドルが有効かチェック
		if (handle >= static_cast<uint32_t>(inst->bgmDatas.size())) {
			return;
		}

		// データを取り出す
		BGMData& data = inst->bgmDatas[static_cast<size_t>(handle)];

		// SourceVoiceがなければ作成
		if (data.pSourceVoice == nullptr) {
			hr = inst->xAudio2->CreateSourceVoice(&data.pSourceVoice, &data.sound.wfex);
			assert(SUCCEEDED(hr));
		}

		// 音量設定
		data.pSourceVoice->SetVolume(volume);

		// 再生する波形データの設定
		XAUDIO2_BUFFER buf{};
		buf.pAudioData = data.sound.buffer.data();
		buf.AudioBytes = static_cast<UINT32>(data.sound.buffer.size());
		buf.Flags = XAUDIO2_END_OF_STREAM;
		buf.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

		// 再生
		// 最初から鳴らしなおす
		// (BGMはシーン遷移時か、シーンの1f目でだけ呼んでおかないと、毎フレーム最初の音しかならなくなってしまう。
		// BGMは1データ=1SourceVoiceなので、重ねて鳴らすことはできない)
		data.pSourceVoice->Stop();
		data.pSourceVoice->FlushSourceBuffers();

		hr = data.pSourceVoice->SubmitSourceBuffer(&buf);
		assert(SUCCEEDED(hr));

		hr = data.pSourceVoice->Start();
		assert(SUCCEEDED(hr));
	}

	void Audio::SetBGMVolume(uint32_t handle, float volume) {
		Audio* inst = GetInstance();

		if (handle >= static_cast<uint32_t>(inst->bgmDatas.size())) {
			return;
		}

		BGMData& data = inst->bgmDatas[static_cast<size_t>(handle)];
		if (data.pSourceVoice) {
			data.pSourceVoice->SetVolume(volume);
		}
	}

	void Audio::PlaySE(uint32_t handle, float volume) {
		// インスタンス取得
		Audio* inst = GetInstance();
		HRESULT hr = S_OK;

		// ハンドルが有効かチェック
		if (handle >= static_cast<uint32_t>(inst->seDatas.size())) {
			return;
		}

		// 再生完了済みのボイスを先に片付ける
		inst->CleanupFinishedSE();

		SoundData& sound = inst->seDatas[static_cast<size_t>(handle)];

		// 呼ぶたびに新しいSourceVoiceを作成するため、同じSEを重ねて再生できる
		PlayingSE playing;
		playing.pCallback = new SEVoiceCallback();

		hr = inst->xAudio2->CreateSourceVoice(
			&playing.pSourceVoice,
			&sound.wfex,
			0,
			XAUDIO2_DEFAULT_FREQ_RATIO,
			playing.pCallback);
		assert(SUCCEEDED(hr));

		// 音量設定
		playing.pSourceVoice->SetVolume(volume);

		// 再生する波形データの設定
		XAUDIO2_BUFFER buf{};
		buf.pAudioData = sound.buffer.data();
		buf.AudioBytes = static_cast<UINT32>(sound.buffer.size());
		buf.Flags = XAUDIO2_END_OF_STREAM;
		
		hr = playing.pSourceVoice->SubmitSourceBuffer(&buf);
		assert(SUCCEEDED(hr));

		hr = playing.pSourceVoice->Start();
		assert(SUCCEEDED(hr));

		inst->playingSEs.push_back(playing);
	}

}

#include <Windows.h>
#include "CWasapi.h"
#include <combaseapi.h>
#include "Util/logger.h"
#include "Network/Socket.h"
#include "utilfun.h"
#include "CoTaskMemPtr.hpp"
#include <functiondiscoverykeys.h>
#include "obs.h"
#include "platform.h"
#include "util_uint64.h"

#define BUFFER_TIME_100NS (5 * 10000000)
#define OBS_KSAUDIO_SPEAKER_4POINT1 \
	(KSAUDIO_SPEAKER_SURROUND | SPEAKER_LOW_FREQUENCY)

#define RECONNECT_INTERVAL 3000

CWasapi::CWasapi()
{
	//int i = std::atomic_fetch_add(&s_thread_index, 1);

	mmdevapi_module = LoadLibrary(L"Mmdevapi");
	if (mmdevapi_module) {
		//activate_audio_interface_async =(PFN_ActivateAudioInterfaceAsync)GetProcAddress(mmdevapi_module, "ActivateAudioInterfaceAsync");
	}

	idleSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!idleSignal.Valid()) 
		PrintE("Could not create idle signal");
		//throw "Could not create idle signal";

	stopSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!stopSignal.Valid())
		PrintE("Could not create stop signal");
		//throw "Could not create stop signal";

	receiveSignal = CreateEvent(nullptr, false, false, nullptr);
	if (!receiveSignal.Valid())
		PrintE("Could not create receive signal");
		//throw "Could not create receive signal";

	restartSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!restartSignal.Valid())
		PrintE("Could not create restart signal");

	reconnectExitSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!reconnectExitSignal.Valid())
		PrintE("Could not create reconnect exit signal");

	exitSignal = CreateEvent(nullptr, true, false, nullptr);
	if (!exitSignal.Valid())
		PrintE("Could not create exit signal");

	initSignal = CreateEvent(nullptr, false, false, nullptr);
	if (!initSignal.Valid())
		PrintE("Could not create init signal");

	reconnectSignal = CreateEvent(nullptr, false, false, nullptr);
	if (!reconnectSignal.Valid())
		PrintE("Could not create reconnect signal");

	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
		CLSCTX_ALL,
		IID_PPV_ARGS(enumerator.Assign()));
	if (FAILED(hr))
		PrintE("Failed to create enumerator", hr);

	//throw HRError("Failed to create enumerator", hr);

	captureThread = CreateThread(nullptr, 0, CWasapi::CaptureThread, this,0, nullptr);

	if (!captureThread.Valid())
	{
			//throw "Failed to create capture thread";
		PrintE("Failed to create capture thread");
	}

	/*auto notify = GetNotify();
	if (notify) {
		notify->AddDefaultDeviceChangedCallback(
			this,
			std::bind(&WASAPISource::SetDefaultDevice, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3));
	}*/

	Start();
}

CWasapi::~CWasapi()
{

}

bool CWasapi::init()
{
	bool b = true;
	if (!mmdevapi_module.Valid())
	{
		mmdevapi_module = LoadLibrary(L"Mmdevapi");
		if (!mmdevapi_module.Valid())
		{
			PrintE("load dll error for Mmdevapi");
			b = false;
		}
	}
	if (!enumerator)
	{
		HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(enumerator.Assign()));
		if (FAILED(hr))
		{
			PrintE("CoCreateInstance error for m_enumerator:%s", HRESULT2utf8(hr).c_str());
			b = false;
		}
	}
	return b;
}

std::vector<CWasapi::AudioDeviceInfo> CWasapi::getMicrophoneDevices()
{
	std::vector<CWasapi::AudioDeviceInfo>devices;
	getAudioDevices(eCapture, devices);
	return devices;
}

std::vector<CWasapi::AudioDeviceInfo> CWasapi::getDesktopAudioDevices()
{
	std::vector<CWasapi::AudioDeviceInfo>devices;
	getAudioDevices(eRender, devices);
	return devices;
}

bool CWasapi::getAudioDevices(EDataFlow type, std::vector<CWasapi::AudioDeviceInfo>&devices)
{
	ComPtr<IMMDeviceCollection>collection;
	devices.clear();
	if (!enumerator) 
		init();
	if (enumerator)
	{
		HRESULT res = enumerator->EnumAudioEndpoints(type, DEVICE_STATE_ACTIVE, collection.Assign());
		if (FAILED(res))
		{
			PrintE("IMMDeviceEnumerator::EnumAudioEndpoints failed:%s", HRESULT2utf8(res).c_str());
			return false;
		}
		UINT count;
		res = collection->GetCount(&count);
		if (FAILED(res))
		{
			PrintE("IMMDeviceCollection::EnumAudioEndpoints failed:%s", HRESULT2utf8(res).c_str());
			return false;
		}

		devices.reserve(count);
		for (UINT i = 0; i < count; i++) {
			ComPtr<IMMDevice> device;
			CoTaskMemPtr<WCHAR> w_id;
			AudioDeviceInfo info;

			res = collection->Item(i, device.Assign());
			if (FAILED(res))
				continue;

			res = device->GetId(&w_id);
			if (FAILED(res) || !w_id || !*w_id)
				continue;

			std::string device_name;
			ComPtr<IPropertyStore> store;
			PROPVARIANT nameVar;

			res = device->OpenPropertyStore(STGM_READ, store.Assign());
			if (FAILED(res))
			{
				PrintW("IMMDevice::OpenPropertyStore failed:%s", HRESULT2utf8(res).c_str());
				continue;
			}
			PropVariantInit(&nameVar);
			res = store->GetValue(PKEY_Device_FriendlyName, &nameVar);
			if (FAILED(res))
			{
				PrintW("IPropertyStore::GetValue failed:%s", HRESULT2utf8(res).c_str());
				continue;
			}
			info.id = wchar_t2uft8(w_id);
			info.name = wchar_t2uft8(nameVar.pwszVal);
			PropVariantClear(&nameVar);
			devices.push_back(std::move(info));
		}
		return true;
	}
	return false;
}



DWORD WINAPI CWasapi::CaptureThread(LPVOID param)
{
	//s_thread_index
	

	//os_set_thread_name("win-wasapi: capture thread");
	const HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED); 
	const bool com_initialized = SUCCEEDED(hr);
	if (!com_initialized)
	{
		PrintE("[WASAPISource::CaptureThread] CoInitializeEx failed:%s", HRESULT2utf8(hr).c_str());
		//////setEevent(errorSignal)?=>join thread///////
	}
	DWORD unused = 0;
	//const HANDLE handle = AvSetMmThreadCharacteristics(L"Audio", &unused);

	CWasapi* source = (CWasapi*)param;

	const HANDLE inactive_sigs[] = {
		source->exitSignal,
		source->stopSignal,
		source->initSignal,
	};

	const HANDLE active_sigs[] = {
		source->exitSignal,
		source->stopSignal,
		source->receiveSignal,
		source->restartSignal,
	};

	DWORD sig_count = _countof(inactive_sigs);
	const HANDLE* sigs = inactive_sigs;

	bool exit = false;
	while (!exit) {
		bool idle = false;
		bool stop = false;
		bool reconnect = false;
		do {
			/* Windows 7 does not seem to wake up for LOOPBACK */
			const DWORD dwMilliseconds =
				((sigs == active_sigs) &&
					(source->sourceType != SourceType::Input))
				? 10
				: INFINITE;

			const DWORD ret = WaitForMultipleObjects(
				sig_count, sigs, false, dwMilliseconds);
			switch (ret) {
			case WAIT_OBJECT_0: {
				exit = true;
				stop = true;
				idle = true;
				break;
			}

			case WAIT_OBJECT_0 + 1:
				stop = true;
				idle = true;
				break;

			case WAIT_OBJECT_0 + 2:
			case WAIT_TIMEOUT:
				if (sigs == inactive_sigs) {
					assert(ret != WAIT_TIMEOUT);

					if (source->TryInitialize()) {
						sig_count =
							_countof(active_sigs);
						sigs = active_sigs;
					}
					else {
						if (source->reconnectDuration ==0) 
						{
							PrintE("WASAPI: Device '%s' failed to start", source->device_name.c_str());
							/*blog(LOG_INFO,
								"WASAPI: Device '%s' failed to start (source: %s)",
								source->device_id
								.c_str(),
								obs_source_get_name(
									source->source));*/
						}
						stop = true;
						reconnect = true;
						source->reconnectDuration =
							RECONNECT_INTERVAL;
					}
				}
				else {
					stop = !source->ProcessCaptureData();
					if (stop) {
						PrintD("Device '%s' invalidated.  Retrying ",
							source->device_name.c_str());

						/*if (source->sourceType ==
							SourceType::
							ProcessOutput &&
							source->hooked) {
							source->hooked = false;
							signal_handler_t* sh =
								obs_source_get_signal_handler(
									source->source);
							calldata_t data = { 0 };
							calldata_set_ptr(
								&data, "source",
								source->source);
							signal_handler_signal(
								sh, "unhooked",
								&data);
							calldata_free(&data);
						}*/
						stop = true;
						reconnect = true;
						source->reconnectDuration =
							RECONNECT_INTERVAL;
					}
				}
				break;

			default:
				assert(sigs == active_sigs);
				assert(ret == WAIT_OBJECT_0 + 3);
				stop = true;
				reconnect = true;
				source->reconnectDuration = 0;
				ResetEvent(source->restartSignal);
			}
		} while (!stop);

		sig_count = _countof(inactive_sigs);
		sigs = inactive_sigs;

		if (source->client) {
			source->client->Stop();

			source->capture.Clear();
			source->client.Clear();
		}

		if (idle) {
			SetEvent(source->idleSignal);
		}
		else if (reconnect) {
			PrintD(
				"Device '%s' invalidated.  Retrying",
				source->device_name.c_str());
			/*if (source->sourceType == SourceType::ProcessOutput &&
				source->hooked) {
				source->hooked = false;
				signal_handler_t* sh =
					obs_source_get_signal_handler(
						source->source);
				calldata_t data = { 0 };
				calldata_set_ptr(&data, "source",
					source->source);
				signal_handler_signal(sh, "unhooked", &data);
				calldata_free(&data);
			}*/
			SetEvent(source->reconnectSignal);
		}
	}

	//if (handle)
	//	AvRevertMmThreadCharacteristics(handle);

	if (com_initialized)
		CoUninitialize();

	return 0;
}


ComPtr<IMMDevice> CWasapi::InitDevice(IMMDeviceEnumerator* enumerator, bool isDefaultDevice, SourceType type, const std::string& device_id)
{
	ComPtr<IMMDevice> device;
	if (isDefaultDevice) {
		const bool input = type == SourceType::Input;
		HRESULT res = enumerator->GetDefaultAudioEndpoint(
			input ? eCapture : eRender,
			input ? eCommunications : eConsole, device.Assign());
		if (FAILED(res))
		{
			PrintW("Failed GetDefaultAudioEndpoint:%s", HRESULT2utf8(res).c_str());
			return device;
		}
	}
	else {
		std::wstring w_id = utf82wchar_t(device_id.c_str(), device_id.size());
		if (w_id.empty()) 
		{
			PrintE("Failed to widen device id string");
			return device;
		}
		const HRESULT res = enumerator->GetDevice(w_id.c_str(), device.Assign());

		if (FAILED(res))
		{
			PrintE("Failed to enumerate device:%s", HRESULT2utf8(res));
			return device;
		}
	}
	return device;
}


std::string CWasapi::GetDeviceName(IMMDevice* device)
{
	std::string device_name;
	ComPtr<IPropertyStore> store;
	HRESULT res;

	if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, store.Assign()))) {
		PROPVARIANT nameVar;
		PropVariantInit(&nameVar);
		res = store->GetValue(PKEY_Device_FriendlyName, &nameVar);
		if (SUCCEEDED(res) && nameVar.pwszVal && *nameVar.pwszVal) 
		{
			device_name = wchar_t2uft8(nameVar.pwszVal);
			PropVariantClear(&nameVar);
		}
	}
	return device_name;
}


ComPtr<IAudioClient> CWasapi::InitClient(
	IMMDevice* device, SourceType type, DWORD process_id,
	PFN_ActivateAudioInterfaceAsync activate_audio_interface_async,
	speaker_layout& speakers, audio_format& format,
	uint32_t& samples_per_sec)
{
	//WAVEFORMATEXTENSIBLE wfextensible;
	CoTaskMemPtr<WAVEFORMATEX> wfex;
	const WAVEFORMATEX* pFormat;
	HRESULT res;
	ComPtr<IAudioClient> client;

	/*if (type == SourceType::ProcessOutput) {
		if (activate_audio_interface_async == NULL)
			throw "ActivateAudioInterfaceAsync is not available";

		struct obs_audio_info oai;
		obs_get_audio_info(&oai);

		const WORD nChannels = (WORD)get_audio_channels(oai.speakers);
		const DWORD nSamplesPerSec = oai.samples_per_sec;
		constexpr WORD wBitsPerSample = 32;
		const WORD nBlockAlign = nChannels * wBitsPerSample / 8;

		WAVEFORMATEX& wf = wfextensible.Format;
		wf.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wf.nChannels = nChannels;
		wf.nSamplesPerSec = nSamplesPerSec;
		wf.nAvgBytesPerSec = nSamplesPerSec * nBlockAlign;
		wf.nBlockAlign = nBlockAlign;
		wf.wBitsPerSample = wBitsPerSample;
		wf.cbSize = sizeof(wfextensible) - sizeof(wf);
		wfextensible.Samples.wValidBitsPerSample = wBitsPerSample;
		wfextensible.dwChannelMask =
			GetSpeakerChannelMask(oai.speakers);
		wfextensible.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

		AUDIOCLIENT_ACTIVATION_PARAMS audioclientActivationParams;
		audioclientActivationParams.ActivationType =
			AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
		audioclientActivationParams.ProcessLoopbackParams
			.TargetProcessId = process_id;
		audioclientActivationParams.ProcessLoopbackParams
			.ProcessLoopbackMode =
			PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;
		PROPVARIANT activateParams{};
		activateParams.vt = VT_BLOB;
		activateParams.blob.cbSize =
			sizeof(audioclientActivationParams);
		activateParams.blob.pBlobData =
			reinterpret_cast<BYTE*>(&audioclientActivationParams);

		{
			Microsoft::WRL::ComPtr<
				WASAPIActivateAudioInterfaceCompletionHandler>
				handler = Microsoft::WRL::Make<
				WASAPIActivateAudioInterfaceCompletionHandler>();
			ComPtr<IActivateAudioInterfaceAsyncOperation> asyncOp;
			res = activate_audio_interface_async(
				VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK,
				__uuidof(IAudioClient), &activateParams,
				handler.Get(), &asyncOp);
			if (FAILED(res))
				throw HRError(
					"Failed to get activate audio client",
					res);

			res = handler->GetActivateResult(client.Assign());
			if (FAILED(res))
				throw HRError("Async activation failed", res);
		}

		pFormat = &wf;
	}
	else {*/
		res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
			nullptr, (void**)client.Assign());
		if (FAILED(res))
		{
			PrintE("Failed to activate client context:%s", HRESULT2utf8(res).c_str());
			return client;
		}
			

		res = client->GetMixFormat(&wfex);
		if (FAILED(res)) 
		{
			PrintE("Failed to get mix format:%s", HRESULT2utf8(res).c_str());
			return client;
		}

		pFormat = wfex.Get();
	//}

	InitFormat(pFormat, speakers, format, samples_per_sec);

	DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
	if (type != SourceType::Input)
		flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
	res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags,
		BUFFER_TIME_100NS, 0, pFormat, nullptr);
	if (FAILED(res)) 
	{
		PrintE("Failed to initialize audio client:%s", HRESULT2utf8(res).c_str());
		return client;
	}
		

	return client;
}

void CWasapi::InitFormat(const WAVEFORMATEX* wfex,
	enum speaker_layout& speakers,
	enum audio_format& format, uint32_t& sampleRate)
{
	DWORD layout = 0;

	if (wfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
		WAVEFORMATEXTENSIBLE* ext = (WAVEFORMATEXTENSIBLE*)wfex;
		layout = ext->dwChannelMask;
	}

	/* WASAPI is always float */
	speakers = ConvertSpeakerLayout(layout, wfex->nChannels);
	format = AUDIO_FORMAT_FLOAT;
	sampleRate = wfex->nSamplesPerSec;
}

speaker_layout CWasapi::ConvertSpeakerLayout(DWORD layout, WORD channels)
{
	switch (layout) {
	case KSAUDIO_SPEAKER_2POINT1:
		return SPEAKERS_2POINT1;
	case KSAUDIO_SPEAKER_SURROUND:
		return SPEAKERS_4POINT0;
	case OBS_KSAUDIO_SPEAKER_4POINT1:
		return SPEAKERS_4POINT1;
	case KSAUDIO_SPEAKER_5POINT1_SURROUND:
		return SPEAKERS_5POINT1;
	case KSAUDIO_SPEAKER_7POINT1_SURROUND:
		return SPEAKERS_7POINT1;
	}
	return (speaker_layout)channels;
}

ComPtr<IAudioCaptureClient> CWasapi::InitCapture(IAudioClient* client,
	HANDLE receiveSignal)
{
	ComPtr<IAudioCaptureClient> capture;
	HRESULT res = client->GetService(IID_PPV_ARGS(capture.Assign()));
	if (FAILED(res))
	{
		PrintE("Failed to create capture context:%s", HRESULT2utf8(res).c_str());
		return capture;
	}
		//throw HRError("Failed to create capture context", res);

	res = client->SetEventHandle(receiveSignal);
	if (FAILED(res)) 
	{
		PrintE("Failed to set event handle:%s", HRESULT2utf8(res).c_str());
		return capture;
	}
		//throw HRError("Failed to set event handle", res);

	res = client->Start();
	if (FAILED(res)) 
	{
		PrintE("Failed to start capture client:%s", HRESULT2utf8(res).c_str());
		return capture;
	}
		//throw HRError("Failed to start capture client", res);

	return capture;
}

void CWasapi::ClearBuffer(IMMDevice* device)
{
	CoTaskMemPtr<WAVEFORMATEX> wfex;
	HRESULT res;
	LPBYTE buffer;
	UINT32 frames;
	ComPtr<IAudioClient> client;

	res = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
		(void**)client.Assign());
	if (FAILED(res))
	{
		PrintE("Failed to activate client context:%s", HRESULT2utf8(res).c_str());
		return;
	}
		//throw HRError("Failed to activate client context", res);

	res = client->GetMixFormat(&wfex);
	if (FAILED(res)) 
	{
		PrintE("Failed to get mix format:%s", HRESULT2utf8(res).c_str());
		return;
	}
		//throw HRError("Failed to get mix format", res);

	res = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, BUFFER_TIME_100NS,
		0, wfex, nullptr);
	if (FAILED(res))
	{
		PrintE("Failed to initialize audio client:%s", HRESULT2utf8(res).c_str());
		return;
	}
		//throw HRError("Failed to initialize audio client", res);

	/* Silent loopback fix. Prevents audio stream from stopping and */
	/* messing up timestamps and other weird glitches during silence */
	/* by playing a silent sample all over again. */

	res = client->GetBufferSize(&frames);
	if (FAILED(res))
	{
		PrintE("Failed to get buffer size:%s", HRESULT2utf8(res).c_str());
		return;
	}
		//throw HRError("Failed to get buffer size", res);

	ComPtr<IAudioRenderClient> render;
	res = client->GetService(IID_PPV_ARGS(render.Assign()));
	if (FAILED(res))
	{
		PrintE("Failed to get render client:%s", HRESULT2utf8(res).c_str());
		return;
	}
		//throw HRError("Failed to get render client", res);

	res = render->GetBuffer(frames, &buffer);
	if (FAILED(res))
	{
		PrintE("Failed to get buffer:%s", HRESULT2utf8(res).c_str());
		return;
	}
		//throw HRError("Failed to get buffer", res);

	memset(buffer, 0, (size_t)frames * (size_t)wfex->nBlockAlign);

	render->ReleaseBuffer(frames, 0);
}

bool CWasapi::Initialize()
{
	ComPtr<IMMDevice> device;
	device = InitDevice(enumerator, isDefaultDevice, sourceType, device_id);
	device_name = GetDeviceName(device);
	ResetEvent(receiveSignal);

	ComPtr<IAudioClient> temp_client = InitClient(
		device, sourceType, 0/*process_id*/, NULL/*activate_audio_interface_async*/,
		speakers, format, sampleRate);
	if (sourceType == SourceType::DeviceOutput)
		ClearBuffer(device);
	ComPtr<IAudioCaptureClient> temp_capture =
		InitCapture(temp_client, receiveSignal);

	client = std::move(temp_client);
	capture = std::move(temp_capture);

	/*if (rtwq_supported) {
		HRESULT hr = rtwq_put_waiting_work_item(
			receiveSignal, 0, sampleReadyAsyncResult, nullptr);
		if (FAILED(hr)) {
			capture.Clear();
			client.Clear();
			throw HRError("RtwqPutWaitingWorkItem failed", hr);
		}

		hr = rtwq_put_waiting_work_item(restartSignal, 0,
			restartAsyncResult, nullptr);
		if (FAILED(hr)) {
			capture.Clear();
			client.Clear();
			throw HRError("RtwqPutWaitingWorkItem failed", hr);
		}
	}*/

	PrintD("WASAPI: Device '%s' [%d Hz] initialized ",device_name.c_str(), sampleRate);

	return (!!client) && (!!capture);

	/*if (sourceType == SourceType::ProcessOutput && !hooked) {
		hooked = true;

		signal_handler_t* sh = obs_source_get_signal_handler(source);
		calldata_t data = { 0 };
		struct dstr title = { 0 };
		struct dstr window_class = { 0 };
		struct dstr executable = { 0 };

		ms_get_window_title(&title, hwnd);
		ms_get_window_class(&window_class, hwnd);
		ms_get_window_exe(&executable, hwnd);

		calldata_set_ptr(&data, "source", source);
		calldata_set_string(&data, "title", title.array);
		calldata_set_string(&data, "class", window_class.array);
		calldata_set_string(&data, "executable", executable.array);
		signal_handler_signal(sh, "hooked", &data);

		dstr_free(&title);
		dstr_free(&window_class);
		dstr_free(&executable);
		calldata_free(&data);
	}*/
}

bool CWasapi::TryInitialize()
{
	bool success = Initialize();
	previouslyFailed = !success;
	return success;
}

bool CWasapi::ProcessCaptureData()
{
	HRESULT res;
	LPBYTE buffer;
	UINT32 frames;
	DWORD flags;
	UINT64 pos, ts;
	UINT captureSize = 0;

	while (true) {
		if ((sourceType == SourceType::ProcessOutput) /*&& !IsWindow(hwnd)*/) 
		{
			PrintE("[WASAPISource::ProcessCaptureData] doesn't realize");
			return false;
		}

		res = capture->GetNextPacketSize(&captureSize);
		if (FAILED(res)) {
			if (res != AUDCLNT_E_DEVICE_INVALIDATED)
				PrintW("[WASAPISource::ProcessCaptureData]"
					" capture->GetNextPacketSize"
					" failed: %lX",
					res);
			return false;
		}

		if (!captureSize)
			break;

		res = capture->GetBuffer(&buffer, &frames, &flags, &pos, &ts);
		if (FAILED(res)) {
			if (res != AUDCLNT_E_DEVICE_INVALIDATED)
				PrintW("[WASAPISource::ProcessCaptureData]"
					" capture->GetBuffer"
					" failed: %lX",
					res);
			return false;
		}

		if (!sawBadTimestamp &&
			flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) {
			PrintW("[WASAPISource::ProcessCaptureData]"
				" Timestamp error!");
			sawBadTimestamp = true;
		}

		if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
			/* libobs should handle discontinuities fine. */
			PrintD("[WASAPISource::ProcessCaptureData]"
				" Discontinuity flag is set.");
		}

		if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
			PrintD("[WASAPISource::ProcessCaptureData]"
				" Silent flag is set.");

			/* buffer size = frame size * number of frames
			 * frame size = channels * sample size
			 * sample size = 4 bytes (always float per InitFormat) */
			uint32_t requiredBufSize =
				get_audio_channels(speakers) * frames * 4;
			if (silence.size() < requiredBufSize)
				silence.resize(requiredBufSize);

			buffer = silence.data();
		}

		obs_source_audio data = {};
		data.data[0] = buffer;
		data.frames = frames;
		data.speakers = speakers;
		data.samples_per_sec = sampleRate;
		data.format = format;
		if (sourceType == SourceType::ProcessOutput) {
			data.timestamp = ts * 100;
		}
		else {
			data.timestamp = useDeviceTiming ? ts * 100
				: os_gettime_ns();

			if (!useDeviceTiming)
				data.timestamp -= util_mul_div64(
					frames, UINT64_C(1000000000),
					sampleRate);
		}

		/*if (reroute_target) {
			obs_source_t* target =
				obs_weak_source_get_source(reroute_target);

			if (target) {
				obs_source_output_audio(target, &data);
				obs_source_release(target);
			}
		}
		else {*/
			source_output_audio(&data);
		//}

		capture->ReleaseBuffer(frames);
	}
	return true;
}



//std::atomic_int  CWasapi::s_thread_index = 0;
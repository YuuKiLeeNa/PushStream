#ifndef __CaptureEvents_h__
#define __CaptureEvents_h__

class CaptureEvents
{
public:
	enum class CaptureEventsType:int
	{
		Type_Audio
		,Type_Video
	};

	void RestartVideo(int index) { Restart(CaptureEventsType::Type_Video,index); }
	void RestartAudio(int index) { Restart(CaptureEventsType::Type_Audio, index); }
	void CloseVideo(int index) { Close(CaptureEventsType::Type_Audio, index); }
	void CloseAudio(int index) { Close(CaptureEventsType::Type_Audio, index); }
private:
	virtual void Restart(CaptureEvents::CaptureEventsType type, int index) = 0;
	virtual void Close(CaptureEvents::CaptureEventsType type, int index) = 0;


};

#endif
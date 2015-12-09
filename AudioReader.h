#pragma once
#include "Reader.h"

class CAudioReader : public CReader
{
public:
	CAudioReader(bool bStreamMode = false);
	~CAudioReader();
	HRESULT            Init(IKinectSensor* pKinectSensor);
	HRESULT            Update();

private:
	BOOL               m_bStreamMode; // event non supported;
	IAudioBeam*        m_pAudioBeam;  // A single audio beam off the Kinect sensor.
	IStream*           m_pAudioStream;// An IStream derived from the audio beam, used to read audio samples
	IAudioBeamFrameReader* m_pAudioBeamFrameReader;

	HRESULT            StreamProcess();
	HRESULT            Process(IAudioBeamSubFrame* pAudioBeamSubFrame);
};
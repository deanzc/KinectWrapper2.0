#pragma once
#include "Reader.h"

class CLongExposureInfraredReader : public CReader
{
public:
	CLongExposureInfraredReader();
	~CLongExposureInfraredReader();
	HRESULT       Init(IKinectSensor* pKinectSensor);
	HRESULT       Update();

private:
	ILongExposureInfraredFrameReader* m_pLongExposureInfraredFrameReader;
	HRESULT       Process(ILongExposureInfraredFrame* pLongExposureInfraredFrame);
};
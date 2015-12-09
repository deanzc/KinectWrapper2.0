#pragma once
#include "Reader.h"

class CDepthReader : public CReader
{
public:
	CDepthReader();
	~CDepthReader();
	HRESULT            Init(IKinectSensor* pKinectSensor);
	HRESULT            Update();

private:
	IDepthFrameReader* m_pDepthFrameReader;
	HRESULT            Process(IDepthFrame* pDepthFrame);
};
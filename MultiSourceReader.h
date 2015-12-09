#pragma once
#include "Reader.h"

class CMultiSourceReader : public CReader
{
public:
	CMultiSourceReader();
	~CMultiSourceReader();
	HRESULT                  Init(IKinectSensor* pKinectSensor);
	HRESULT                  Update();

private:
	IMultiSourceFrameReader* m_pMultiSourceFrameReader;
	HRESULT                  Process(IMultiSourceFrame* pMultiSourceFrame);
};
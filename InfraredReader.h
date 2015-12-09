#pragma once
#include "Reader.h"

class CInfraredReader : public CReader
{
public:
	CInfraredReader();
	~CInfraredReader();
	HRESULT               Init(IKinectSensor* pKinectSensor);
	HRESULT               Update();

private:
	IInfraredFrameReader* m_pInfraredFrameReader;
	HRESULT               Process(IInfraredFrame* pInfraredFrame);
};
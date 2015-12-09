#pragma once
#include "Reader.h"

class CBodyReader : public CReader
{
public:
	CBodyReader();
	~CBodyReader();
	HRESULT           Init(IKinectSensor* pKinectSensor);
	HRESULT           Update();

private:
	IBodyFrameReader* m_pBodyFrameReader;
	HRESULT           Process(IBodyFrame* pBodyFrame);
};
#pragma once
#include "Reader.h"

class CBodyIndexReader : public CReader
{
public:
	CBodyIndexReader();
	~CBodyIndexReader();
	HRESULT                Init(IKinectSensor* pKinectSensor);
	HRESULT                Update();

private:
	IBodyIndexFrameReader* m_pBodyIndexFrameReader;
	HRESULT                Process(IBodyIndexFrame* pBodyIndexFrame);
};
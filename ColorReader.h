#pragma once
#include "Reader.h"

class CColorReader : public CReader
{
public:
	CColorReader();
	~CColorReader();
	HRESULT            Init(IKinectSensor* pKinectSensor);
	HRESULT            Update();

private:  
	IColorFrameReader* m_pColorFrameReader;
	HRESULT            Process(IColorFrame* pColorFrame);
};
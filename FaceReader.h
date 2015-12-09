#pragma once
#include "ReaderEx.h"

class CFaceReader : public CReaderEx
{
public:
	CFaceReader(UINT id);
	~CFaceReader();
	HRESULT           Init(IKinectSensor* pKinectSensor);
	HRESULT           Update();

private:
	IFaceFrameSource* m_pFaceFrameSource;
	IFaceFrameReader* m_pFaceFrameReader;	
	HRESULT           PutBodyTrackingId(UINT64 uBodyTId);
	HRESULT           Process(IFaceFrame* pFaceFrame);
};
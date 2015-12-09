#pragma once
#include "ReaderEx.h"

class CVGBReader : public CReaderEx
{
public:
	CVGBReader(UINT id, PCWSTR pGestureFile);
	~CVGBReader();
	HRESULT          Init(IKinectSensor* pKinectSensor);
	HRESULT          Update();

private:
	PCWSTR           m_pGestureDataFile;
	IGesture*        m_pGestures[cGestureMaxSize];
	IVisualGestureBuilderFrameSource* m_pVGBFrameSource;
	IVisualGestureBuilderFrameReader* m_pVGBFrameReader;	

	HRESULT          AddGestureDataFromFile(PCWSTR fileName);
	HRESULT          RemoveGestureData();
	HRESULT          PutBodyTrackingId(UINT64 uBodyTId);
	HRESULT          Process(IVisualGestureBuilderFrame* pVGBFrame);
};
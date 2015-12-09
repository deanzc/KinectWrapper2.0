#pragma once
#include "KinectData.h"

enum ReaderMode
{
	Poll                 = 0,
	Event                = 1
};

enum ReaderType
{
	None                 = 0x0,
	Color                = 0x1,
	Infrared             = 0x2,
	LongExposureInfrared = 0x4,
	Depth                = 0x8,
	BodyIndex            = 0x10,
	Body                 = 0x20,
	Audio                = 0x40,
	Multi                = 0x80,  //or Speech/Controls/Pointer
	Face                 = 0x100,
	HDFace               = 0x200,
	VGB                  = 0x400, 
	Fusion               = 0x800, // NUI
	All                  = 0xFFF
};

class CReader
{
public:
	static CKinectData   m_sKinectData;

public:
	CReader();
	virtual ~CReader();
	virtual HRESULT      Init(IKinectSensor* pKinectSensor);
	virtual HRESULT      Update();
	ReaderMode           GetMode();
	VOID                 SetMode(ReaderMode readerMode);
	HANDLE               GetHandle();

protected:
	ReaderMode           m_ReaderMode;
	WAITABLE_HANDLE      m_hFrameArrived;
};
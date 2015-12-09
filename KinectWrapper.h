#pragma once
#include <vector>
#include "Reader.h"

template class __declspec(dllexport) std::vector<CReader*>;

#ifdef KINECTWRAPPER_DLLEXPORT
class _declspec(dllexport) CKinectWrapper
#else
class _declspec(dllimport) CKinectWrapper
#endif
{
public:
	CKinectWrapper();
	~CKinectWrapper();

	HRESULT               Init(DWORD enableReaderType, ReaderMode readerMode = Poll, 
														 PCWSTR pGestureFile = L"Database\\Seated.gdb");
	HRESULT               Update();

	// data
	unsigned char*        GetColorData();
	unsigned char*        GetInfraredData();
	unsigned char*        GetLongExposureInfraredData();
	unsigned char*        GetDepthData();
	unsigned char*        GetBodyIndexData();
	BodyData*             GetBodyData();
	AudioData             GetAudioData();
	FaceData*             GetFaceData();
	HDFaceData*           GetHDFaceData();
	VGBData*              GetVGBData();

	void                  SetTrackBodyId(ReaderType readerType, UINT readerId, UINT iBody);

	// tool
	HRESULT               CameraToColor(const CameraSpacePoint& cameraPoint, ColorSpacePoint* colorPoint);
	HRESULT               CameraToDepth(const CameraSpacePoint& cameraPoint, DepthSpacePoint* depthPoint);
	
private:
	IKinectSensor*        m_pKinectSensor;
	ICoordinateMapper*    m_pCoordinateMapper;
	std::vector<CReader*> m_vReader;

	///////////////////// worker thread //////////////////////////
	CRITICAL_SECTION      m_csLock;
	HANDLE                m_hWorkerThread;
	HANDLE                m_hTerminateWorkerThread;
	HRESULT               StartWorkerThread();
	VOID                  CloseWorkerThread();
	HRESULT               WorkerThread();		
	static DWORD WINAPI   WorkerThread(_In_ LPVOID lpParameter);
};
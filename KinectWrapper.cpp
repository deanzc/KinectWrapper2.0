#include "stdafx.h"
#include "ColorReader.h"
#include "InfraredReader.h"
#include "LongExposureInfraredReader.h"
#include "DepthReader.h"
#include "BodyIndexReader.h"
#include "BodyReader.h"
#include "AudioReader.h"
#include "MultiSourceReader.h"
#include "FaceReader.h"
#include "HDFaceReader.h"
#include "VGBReader.h"
#include "KinectWrapper.h"

//#define PI_F    3.1415926f
//#define PI      3.14159265358979323846

CKinectWrapper::CKinectWrapper() :
	m_pKinectSensor(nullptr),
	m_pCoordinateMapper(nullptr),
	m_hTerminateWorkerThread(NULL),
	m_hWorkerThread(NULL)
{
	InitializeCriticalSection(&m_csLock);
	m_vReader.clear();
}

CKinectWrapper::~CKinectWrapper()
{
	CloseWorkerThread();

	std::vector<CReader*>::iterator iter;
	for (iter = m_vReader.begin(); iter != m_vReader.end(); ++iter)
	{
		if (*iter != nullptr)
		{
			delete *iter;
			*iter = nullptr;
		}
	}	

	SafeRelease(m_pCoordinateMapper);

	if (m_pKinectSensor != nullptr)
	{
		m_pKinectSensor->Close();
		SafeRelease(m_pKinectSensor);
	}

	DeleteCriticalSection(&m_csLock);
}

HRESULT CKinectWrapper::Init(DWORD enableReaderType, ReaderMode readerMode, PCWSTR pGestureFile)
{
	HRESULT hr = E_FAIL;

	// Get Sensor
	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (SUCCEEDED(hr))
	{
		BOOLEAN bOpen = true;
		hr = m_pKinectSensor->get_IsOpen(&bOpen);
		if (SUCCEEDED(hr) && !bOpen)
		{
			hr = m_pKinectSensor->Open();
		}
	}

	// Get Mapper
	if (SUCCEEDED(hr))
	{
		hr = m_pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
	}

	// Get Reader
	if (SUCCEEDED(hr))
	{	
		// Create Reader
		if (enableReaderType & Color)
		{
			m_vReader.push_back(new CColorReader()); 
		}

		if (enableReaderType & Infrared)
		{
			m_vReader.push_back(new CInfraredReader());
		}

		if (enableReaderType & LongExposureInfrared)
		{
			m_vReader.push_back(new CLongExposureInfraredReader());
		}

		if (enableReaderType & Depth)
		{
			m_vReader.push_back(new CDepthReader());
		}

		if (enableReaderType & BodyIndex)
		{
			m_vReader.push_back(new CBodyIndexReader());
		}

		if ((enableReaderType & Body)
			| (enableReaderType & Face)
			| (enableReaderType & HDFace)
			| (enableReaderType & VGB))
		{
			m_vReader.push_back(new CBodyReader());
		}

		if (enableReaderType & Audio)
		{
			m_vReader.push_back(new CAudioReader(false));
		}

		if (enableReaderType & Multi)
		{
			m_vReader.push_back(new CMultiSourceReader());
		}

		// Create ReaderEx
		if (enableReaderType & Face)
		{
			for (int id = 0; id < BODY_COUNT; id++)
			{
				m_vReader.push_back(new CFaceReader(id));
			}
		}

		if (enableReaderType & HDFace)
		{
			for (int id = 0; id < BODY_COUNT; id++)
			{
				m_vReader.push_back(new CHDFaceReader(id, false));
			}
		}

		if (enableReaderType & VGB)
		{
			for (int id = 0; id < BODY_COUNT; id++)
			{
				m_vReader.push_back(new CVGBReader(id, pGestureFile));
			}
		}
	}	

	// Init Reader
	std::vector<CReader*>::iterator iter;
	for (iter = m_vReader.begin(); iter != m_vReader.end(); ++iter)
	{
		(*iter)->SetMode(readerMode);
		hr = (*iter)->Init(m_pKinectSensor);
	}

	//// Work Thread
	//	StartWorkerThread();

	return hr;
}

HRESULT CKinectWrapper::Update()
{
	HRESULT hr = E_FAIL;
	BOOLEAN bAvailable = false;
	hr = m_pKinectSensor->get_IsAvailable(&bAvailable);
	if (!bAvailable) return hr;

	std::vector<CReader*>::iterator iter;
	for (iter = m_vReader.begin(); iter != m_vReader.end(); ++iter)
	{
		if (Poll == (*iter)->GetMode())
		{
			hr = (*iter)->Update();
		} 
		else // Event Mode
		{
			if (WAIT_OBJECT_0 == 
				WaitForSingleObject((*iter)->GetHandle(), 0)) 
			{
				hr = (*iter)->Update();
			}
		}
	}

	return hr;
}

///////////////////////// get data //////////////////////////////

unsigned char* CKinectWrapper::GetColorData()
{
	return CReader::m_sKinectData.m_pColorRGBX;
}

unsigned char* CKinectWrapper::GetInfraredData()
{
	return CReader::m_sKinectData.m_pInfraredRGBX;
}

unsigned char* CKinectWrapper::GetLongExposureInfraredData()
{
	return CReader::m_sKinectData.m_pLongExposureInfraredRGBX;
}

unsigned char* CKinectWrapper::GetDepthData()
{
	return CReader::m_sKinectData.m_pDepthRGBX;
}

unsigned char* CKinectWrapper::GetBodyIndexData()
{
	return CReader::m_sKinectData.m_pBodyIndexRGBX;
}

BodyData* CKinectWrapper::GetBodyData()
{
	return CReader::m_sKinectData.m_BodyData;
}

AudioData CKinectWrapper::GetAudioData()
{
	return CReader::m_sKinectData.m_AudioData;
}

FaceData* CKinectWrapper::GetFaceData()
{
	return CReader::m_sKinectData.m_FaceData;
}

HDFaceData* CKinectWrapper::GetHDFaceData()
{
	return CReader::m_sKinectData.m_HDFaceData;
}

VGBData* CKinectWrapper::GetVGBData()
{
	return CReader::m_sKinectData.m_VGBData;
}

void CKinectWrapper::SetTrackBodyId(ReaderType readerType, UINT readerId, UINT iBody)
{
	if (readerId < 0 || readerId > 5 || iBody < 0 || iBody > 5) return;

	switch (readerType)
	{
	case Face:
		CReader::m_sKinectData.m_FaceData[readerId].iTrackTarget = iBody;
		break;
	case HDFace:
		CReader::m_sKinectData.m_HDFaceData[readerId].iTrackTarget = iBody;
		break;
	case VGB:
		CReader::m_sKinectData.m_VGBData[readerId].iTrackTarget = iBody;
		break;
	}
}

/////////////////////// tool ////////////////////////////////////

HRESULT CKinectWrapper::CameraToColor(const CameraSpacePoint& cameraPoint, ColorSpacePoint* colorPoint)
{
	HRESULT hr = E_FAIL;

	if (m_pCoordinateMapper)
	{
		hr = m_pCoordinateMapper->MapCameraPointToColorSpace(cameraPoint, colorPoint);
	}

	return hr;
}

HRESULT CKinectWrapper::CameraToDepth(const CameraSpacePoint& cameraPoint, DepthSpacePoint* depthPoint)
{
	HRESULT hr = E_FAIL;

	if (m_pCoordinateMapper)
	{
		hr = m_pCoordinateMapper->MapCameraPointToDepthSpace(cameraPoint, depthPoint);
	}

	return hr;
}

//void CKinectWrapper::ExtractFaceRotationInDegrees(const Vector4* pQuaternion, int* pPitch, int* pYaw, int* pRoll)
//{
//	double x = pQuaternion->x;
//	double y = pQuaternion->y;
//	double z = pQuaternion->z;
//	double w = pQuaternion->w;
//
//	// convert face rotation quaternion to Euler angles in degrees		
//	double dPitch, dYaw, dRoll;
//	dPitch = atan2(2 * (y * z + w * x), w * w - x * x - y * y + z * z) / PI * 180.0;
//	dYaw = asin(2 * (w * y - x * z)) / PI * 180.0;
//	dRoll = atan2(2 * (x * y + w * z), w * w + x * x - y * y - z * z) / PI * 180.0;
//
//	// clamp rotation values in degrees to a specified range of values to control the refresh rate
//	double increment = 5.0f; //c_FaceRotationIncrementInDegrees;
//	*pPitch = static_cast<int>(floor((dPitch + increment / 2.0 * (dPitch > 0 ? 1.0 : -1.0)) / increment) * increment);
//	*pYaw = static_cast<int>(floor((dYaw + increment / 2.0 * (dYaw > 0 ? 1.0 : -1.0)) / increment) * increment);
//	*pRoll = static_cast<int>(floor((dRoll + increment / 2.0 * (dRoll > 0 ? 1.0 : -1.0)) / increment) * increment);
//}
//
//void CKinectWrapper::ExtractHDFaceRotationInDegrees(const Vector4* pQuaternion, float* pPitch, float* pYaw, float* pRoll)
//{
//	float x = pQuaternion->x;
//	float y = pQuaternion->y;
//	float z = pQuaternion->z;
//	float w = pQuaternion->w;
//
//	//double dPitch, dYaw, dRoll;
//	*pPitch = atan2f(2.f * (y * z + w * x), w * w - x * x - y * y + z * z) / PI_F * 180.f;
//	*pYaw   = asinf(2.f * (w * y - x * z)) / PI_F * 180.f;
//	*pRoll  = atan2f(2.f * (x * y + w * z), w * w + x * x - y * y - z * z) / PI_F * 180.f;
//}

/////////////////////// worker thread ///////////////////////////

HRESULT CKinectWrapper::StartWorkerThread()
{
	HRESULT hr = S_OK;

	if (SUCCEEDED(hr))
	{
		m_hTerminateWorkerThread = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (NULL == m_hTerminateWorkerThread)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (SUCCEEDED(hr))
	{
		m_hWorkerThread = CreateThread(NULL, 0, &CKinectWrapper::WorkerThread, this, 0, NULL);
		if (NULL == m_hWorkerThread)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	return hr;
}

VOID CKinectWrapper::CloseWorkerThread()
{
	if (NULL != m_hTerminateWorkerThread)
	{
		SetEvent(m_hTerminateWorkerThread);
	}

	if (NULL != m_hWorkerThread)
	{
		WaitForSingleObject(m_hWorkerThread, INFINITE);
		CloseHandle(m_hWorkerThread);
	}

	if (NULL != m_hTerminateWorkerThread)
	{
		CloseHandle(m_hTerminateWorkerThread);
	}
}

HRESULT CKinectWrapper::WorkerThread()
{
	HRESULT hr = S_OK;
	DWORD timeout = 2000;
	BOOL bThreadRunning = TRUE;

	HANDLE handles[] = { m_hTerminateWorkerThread };
	//m_vHandle.push_back(m_hTerminateWorkerThread);

	while (bThreadRunning)
	{
		DWORD result = WaitForMultipleObjects(_countof(handles), handles, FALSE, timeout);
		//DWORD result = WaitForMultipleObjects(m_vHandle.size(), &m_vHandle[0], FALSE, timeout);

		if (WAIT_OBJECT_0 + 0 == result) // Terminate worker thread
		{
			break;
		}
		else
		if (WAIT_TIMEOUT) // Timeout
		{
			OutputDebugString(L"timeout\n");
		}
		else
		{
			hr = E_FAIL;
			break;
		}
	}

	return hr;
}

DWORD WINAPI CKinectWrapper::WorkerThread(_In_ LPVOID lpParameter)
{
	HRESULT hr = S_OK;
	CKinectWrapper* pThis = static_cast<CKinectWrapper*>(lpParameter);
	hr = pThis->WorkerThread();

	return SUCCEEDED(hr) ? 0 : 1;
}

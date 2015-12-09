#include "stdafx.h"
#include "VGBReader.h"

CVGBReader::CVGBReader(UINT id, PCWSTR pGestureFile)
: m_pGestureDataFile(pGestureFile),
	m_pVGBFrameSource(nullptr),
	m_pVGBFrameReader(nullptr)
{
	m_iReaderExId = id;
}

CVGBReader::~CVGBReader()
{
	RemoveGestureData();

	if (m_pVGBFrameReader && m_hFrameArrived)
	{
		m_pVGBFrameReader->UnsubscribeFrameArrived(m_hFrameArrived);
		m_hFrameArrived = 0;
	}

	SafeRelease(m_pVGBFrameReader);
	SafeRelease(m_pVGBFrameSource);
}

HRESULT CVGBReader::AddGestureDataFromFile(PCWSTR fileName)
{
	HRESULT hr = E_FAIL;
	UINT    uGestureSize = 0;
	IVisualGestureBuilderDatabase* pVGBDatabase = NULL;

	if (!fileName) { return hr; }

	hr = CreateVisualGestureBuilderDatabaseInstanceFromFile(fileName, &pVGBDatabase);

	if (SUCCEEDED(hr))
	{
		hr = pVGBDatabase->get_AvailableGesturesCount(&uGestureSize);
	}

	if (SUCCEEDED(hr))
	{
		hr = pVGBDatabase->get_AvailableGestures(uGestureSize, m_pGestures);
	}

	if (SUCCEEDED(hr) && m_pVGBFrameSource)
	{
		hr = m_pVGBFrameSource->AddGestures(uGestureSize, m_pGestures);
	}

	SafeRelease(pVGBDatabase);
	return hr;
}

HRESULT CVGBReader::RemoveGestureData()
{
	HRESULT hr = E_FAIL;
	UINT    uGestureSize = 0;
	hr = m_pVGBFrameSource->get_GestureCount(&uGestureSize);
	if (FAILED(hr)) { return hr; }

	for (UINT i = 0; i < uGestureSize; i++)
	{
		hr = m_pVGBFrameSource->RemoveGesture(m_pGestures[i]);
		SafeRelease(m_pGestures[i]);
	}

	return hr;
}

HRESULT CVGBReader::PutBodyTrackingId(UINT64 uBodyTId)
{
	HRESULT hr = E_FAIL;
	BOOLEAN bTracked = TRUE;
	hr = m_pVGBFrameSource->get_IsTrackingIdValid(&bTracked);

	if (SUCCEEDED(hr) && !bTracked)
	{
		hr = m_pVGBFrameSource->put_TrackingId(uBodyTId);
	}

	return hr;
}

HRESULT CVGBReader::Init(IKinectSensor* pKinectSensor)
{
	HRESULT hr = E_FAIL;
	hr = CreateVisualGestureBuilderFrameSource(pKinectSensor, 0, &m_pVGBFrameSource);

	if (SUCCEEDED(hr))
	{
		hr = m_pVGBFrameSource->OpenReader(&m_pVGBFrameReader);
	}

	if (SUCCEEDED(hr) && m_ReaderMode == Event)
	{
		hr = m_pVGBFrameReader->SubscribeFrameArrived(&m_hFrameArrived);
	}

	if (SUCCEEDED(hr))
	{
		AddGestureDataFromFile(m_pGestureDataFile);
	}

	return hr;
}

HRESULT CVGBReader::Update()
{
	HRESULT hr = E_FAIL;
	IVisualGestureBuilderFrame* pVGBFrame = nullptr;

	if (!m_pVGBFrameReader) { return hr; }

	if (m_ReaderMode == Event)
	{
		IVisualGestureBuilderFrameReference* pVGBFrameReference = nullptr;
		IVisualGestureBuilderFrameArrivedEventArgs* pVGBFrameArrivedEventArgs = nullptr;

		hr = m_pVGBFrameReader->GetFrameArrivedEventData(m_hFrameArrived, &pVGBFrameArrivedEventArgs);

		if (SUCCEEDED(hr))
		{
			hr = pVGBFrameArrivedEventArgs->get_FrameReference(&pVGBFrameReference);
		}

		if (SUCCEEDED(hr))
		{
			hr = pVGBFrameReference->AcquireFrame(&pVGBFrame);
		}

		SafeRelease(pVGBFrameReference);
		SafeRelease(pVGBFrameArrivedEventArgs);
	}
	else
	{
		hr = m_pVGBFrameReader->CalculateAndAcquireLatestFrame(&pVGBFrame);
	}

	if (SUCCEEDED(hr))
	{
		hr = Process(pVGBFrame);
	}

	SafeRelease(pVGBFrame);

	return hr;
}

HRESULT CVGBReader::Process(IVisualGestureBuilderFrame* pVGBFrame)
{
	INT64 nTime = 0;
	BOOLEAN bGestureTracked = false;

	HRESULT hr = E_FAIL;
	if (!pVGBFrame) { return hr; }
	hr = pVGBFrame->get_RelativeTime(&nTime);

	if (SUCCEEDED(hr))
	{
		hr = pVGBFrame->get_IsTrackingIdValid(&bGestureTracked);
	}

	if (SUCCEEDED(hr) && bGestureTracked)
	{
		m_sKinectData.m_VGBData[m_iReaderExId].bTracked = TRUE;

		IDiscreteGestureResult *pDGResult = nullptr;
		IContinuousGestureResult *pCGResult = nullptr;

		float fProgress = 0.f;
		float fConfidence = 0.f;
		BOOLEAN bDetected = false;
		BOOLEAN bFFDetected = true;

		UINT uGestureSize = 0;
		hr = m_pVGBFrameSource->get_GestureCount(&uGestureSize);
		if (FAILED(hr)) { return hr; }

		for (UINT i = 0; i < uGestureSize; i++)
		{
			//WCHAR gName[100] = L"";
			//hr = m_pGestures[iBody][i]->get_Name(_countof(gName), gName);
			GestureType gType = GestureType_None;
			hr = m_pGestures[i]->get_GestureType(&gType);

			if (SUCCEEDED(hr))
			{
				m_sKinectData.m_VGBData[m_iReaderExId].gData[i].gType = gType; //save
			}

			if (gType == GestureType_Discrete)
			{
				if (SUCCEEDED(hr))
				{
					hr = pVGBFrame->get_DiscreteGestureResult(m_pGestures[i], &pDGResult);
				}

				if (pDGResult)
				{
					if (SUCCEEDED(hr))
					{
						hr = pDGResult->get_Detected(&bDetected);
					}

					if (SUCCEEDED(hr))
					{
						hr = pDGResult->get_Confidence(&fConfidence);
					}

					if (SUCCEEDED(hr))
					{
						hr = pDGResult->get_FirstFrameDetected(&bFFDetected);
					}
				}

				m_sKinectData.m_VGBData[m_iReaderExId].gData[i].bDetected = bDetected;
				m_sKinectData.m_VGBData[m_iReaderExId].gData[i].fConfidence = fConfidence;
				m_sKinectData.m_VGBData[m_iReaderExId].gData[i].bFFDetected = bFFDetected;
			}
			else if (gType == GestureType_Continuous)
			{
				if (SUCCEEDED(hr))
				{
					hr = pVGBFrame->get_ContinuousGestureResult(m_pGestures[i], &pCGResult);
				}

				if (SUCCEEDED(hr) && pCGResult)
				{
					hr = pCGResult->get_Progress(&fProgress);
				}

				m_sKinectData.m_VGBData[m_iReaderExId].gData[i].fProgress = fProgress;
			}
		}

		SafeRelease(pCGResult);
		SafeRelease(pDGResult);
	}
	else
	{
		m_sKinectData.m_VGBData[m_iReaderExId].bTracked = FALSE;

		UINT64 uTrackedId = m_sKinectData.m_BodyData[m_sKinectData.m_VGBData[m_iReaderExId].iTrackTarget].uTrackedId;

		if (uTrackedId != 0)
		{
			PutBodyTrackingId(uTrackedId);
		}
	}

	return hr;
}
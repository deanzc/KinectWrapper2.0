#include "stdafx.h"
#include "MultiSourceReader.h"

CMultiSourceReader::CMultiSourceReader()
: m_pMultiSourceFrameReader(nullptr)
{
}

CMultiSourceReader::~CMultiSourceReader()
{
	if (m_pMultiSourceFrameReader && m_hFrameArrived)
	{
		m_pMultiSourceFrameReader->UnsubscribeMultiSourceFrameArrived(m_hFrameArrived);
		m_hFrameArrived = 0;
	}

	SafeRelease(m_pMultiSourceFrameReader);
}

HRESULT CMultiSourceReader::Init(IKinectSensor* pKinectSensor)
{
	HRESULT	hr = E_FAIL;

	hr = pKinectSensor->OpenMultiSourceFrameReader(
		FrameSourceTypes::FrameSourceTypes_Color |
		FrameSourceTypes::FrameSourceTypes_Infrared |
		FrameSourceTypes::FrameSourceTypes_LongExposureInfrared |
		FrameSourceTypes::FrameSourceTypes_Depth |
		FrameSourceTypes::FrameSourceTypes_BodyIndex |
		FrameSourceTypes::FrameSourceTypes_Body,
		//FrameSourceTypes::FrameSourceTypes_Audio, // error
		&m_pMultiSourceFrameReader);

	if (SUCCEEDED(hr) && m_ReaderMode == Event)
	{
		hr = m_pMultiSourceFrameReader->SubscribeMultiSourceFrameArrived(&m_hFrameArrived);
	}

	return hr;
}

HRESULT CMultiSourceReader::Update()
{
	HRESULT hr = E_FAIL;
	IMultiSourceFrame* pMultiSourceFrame = nullptr;

	if (!m_pMultiSourceFrameReader) { return hr; }

	if (m_ReaderMode == Event)
	{
		IMultiSourceFrameReference* pMultiSourceFrameReference = nullptr;
		IMultiSourceFrameArrivedEventArgs* pMultiSourceFrameArrivedEventArgs = nullptr;

		hr = m_pMultiSourceFrameReader->GetMultiSourceFrameArrivedEventData(m_hFrameArrived, &pMultiSourceFrameArrivedEventArgs);

		if (SUCCEEDED(hr))
		{
			hr = pMultiSourceFrameArrivedEventArgs->get_FrameReference(&pMultiSourceFrameReference);
		}

		if (SUCCEEDED(hr))
		{
			hr = pMultiSourceFrameReference->AcquireFrame(&pMultiSourceFrame);
		}

		SafeRelease(pMultiSourceFrameReference);
		SafeRelease(pMultiSourceFrameArrivedEventArgs);
	}
	else
	{
		hr = m_pMultiSourceFrameReader->AcquireLatestFrame(&pMultiSourceFrame);
	}

	if (SUCCEEDED(hr))
	{
		hr = Process(pMultiSourceFrame);
	}

	SafeRelease(pMultiSourceFrame);
	return hr;
}

HRESULT CMultiSourceReader::Process(IMultiSourceFrame* pMultiSourceFrame)
{	
	HRESULT hr = E_FAIL;

	IColorFrame*     pColorFrame = NULL;
	IInfraredFrame*  pInfraredFrame = NULL;
	IDepthFrame*     pDepthFrame = NULL;
	IBodyIndexFrame* pBodyIndexFrame = NULL;
	IBodyFrame*      pBodyFrame = NULL;

	if (!pMultiSourceFrame) { return hr; }

	hr = m_pMultiSourceFrameReader->AcquireLatestFrame(&pMultiSourceFrame);

	if (SUCCEEDED(hr)) // Color Frame
	{
		IColorFrameReference* pColorFrameReference = NULL;
		hr = pMultiSourceFrame->get_ColorFrameReference(&pColorFrameReference);
		if (SUCCEEDED(hr))
		{
			hr = pColorFrameReference->AcquireFrame(&pColorFrame);
		}

		SafeRelease(pColorFrameReference);
	}

	if (SUCCEEDED(hr)) // Infrared Frame
	{
		IInfraredFrameReference* pInfraredFrameReference = NULL;
		hr = pMultiSourceFrame->get_InfraredFrameReference(&pInfraredFrameReference);
		if (SUCCEEDED(hr))
		{
			hr = pInfraredFrameReference->AcquireFrame(&pInfraredFrame);
		}

		SafeRelease(pInfraredFrameReference);
	}

	if (SUCCEEDED(hr)) // Depth Frame
	{
		IDepthFrameReference* pDepthFrameReference = NULL;
		hr = pMultiSourceFrame->get_DepthFrameReference(&pDepthFrameReference);
		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameReference->AcquireFrame(&pDepthFrame);
		}

		SafeRelease(pDepthFrameReference);
	}

	if (SUCCEEDED(hr)) // BodyIndex Frame
	{
		IBodyIndexFrameReference* pBodyIndexFrameReference = NULL;
		hr = pMultiSourceFrame->get_BodyIndexFrameReference(&pBodyIndexFrameReference);
		if (SUCCEEDED(hr))
		{
			hr = pBodyIndexFrameReference->AcquireFrame(&pBodyIndexFrame);
		}

		SafeRelease(pBodyIndexFrameReference);
	}

	if (SUCCEEDED(hr)) // Body Frame
	{
		IBodyFrameReference* pBodyFrameReference = NULL;
		hr = pMultiSourceFrame->get_BodyFrameReference(&pBodyFrameReference);
		if (SUCCEEDED(hr))
		{
			hr = pBodyFrameReference->AcquireFrame(&pBodyFrame);
		}

		SafeRelease(pBodyFrameReference);
	}

	//if (SUCCEEDED(hr)) // Process
	//{
	//	ProcessColorFrame(pColorFrame);
	//	ProcessInfraredFrame(pInfraredFrame);
	//	ProcessDepthFrame(pDepthFrame);
	//	ProcessBodyIndexFrame(pBodyIndexFrame);
	//	ProcessBodyFrame(pBodyFrame);
	//}

	SafeRelease(pBodyFrame);
	SafeRelease(pBodyIndexFrame);
	SafeRelease(pDepthFrame);
	SafeRelease(pInfraredFrame);
	SafeRelease(pColorFrame);

	return hr;
}
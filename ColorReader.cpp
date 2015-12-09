#include "stdafx.h"
#include "ColorReader.h"

CColorReader::CColorReader()
: m_pColorFrameReader(nullptr)
{
}

CColorReader::~CColorReader()
{
	if (m_pColorFrameReader && m_hFrameArrived)
	{
		m_pColorFrameReader->UnsubscribeFrameArrived(m_hFrameArrived);
		m_hFrameArrived = 0;
	}

	SafeRelease(m_pColorFrameReader);
}

HRESULT CColorReader::Init(IKinectSensor* pKinectSensor)
{	
	HRESULT	hr = E_FAIL;
	IColorFrameSource* pColorFrameSource = nullptr;

	hr = pKinectSensor->get_ColorFrameSource(&pColorFrameSource);

	if (SUCCEEDED(hr))
	{
		hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
	}

	if (SUCCEEDED(hr) && m_ReaderMode == Event)
	{
		hr = m_pColorFrameReader->SubscribeFrameArrived(&m_hFrameArrived);
	}

	SafeRelease(pColorFrameSource);

	return hr;
}

HRESULT CColorReader::Update()
{	
	HRESULT hr = E_FAIL;
	IColorFrame* pColorFrame = nullptr;

	if (!m_pColorFrameReader) { return hr; }

	if (m_ReaderMode == Event)
	{
		IColorFrameReference* pColorFrameReference = nullptr;
		IColorFrameArrivedEventArgs* pColorFrameArrivedEventArgs = nullptr;

		hr = m_pColorFrameReader->GetFrameArrivedEventData(m_hFrameArrived, &pColorFrameArrivedEventArgs);

		if (SUCCEEDED(hr))
		{
			hr = pColorFrameArrivedEventArgs->get_FrameReference(&pColorFrameReference);
		}

		if (SUCCEEDED(hr))
		{
			hr = pColorFrameReference->AcquireFrame(&pColorFrame);
		}

		SafeRelease(pColorFrameReference);
		SafeRelease(pColorFrameArrivedEventArgs);
	}
	else
	{
		hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);
	}

	if (SUCCEEDED(hr))
	{
		hr = Process(pColorFrame);
	}

	SafeRelease(pColorFrame);
	return hr;
}

HRESULT CColorReader::Process(IColorFrame* pColorFrame)
{
	INT64 nTime = 0;
	IFrameDescription* pFrameDescription = NULL;
	int nWidth = 0;
	int nHeight = 0;
	ColorImageFormat imageFormat = ColorImageFormat_None;
	UINT nBufferSize = 0;
	RGBQUAD *pBuffer = NULL;

	HRESULT hr = E_FAIL;
	if (!pColorFrame) { return hr; }
	hr = pColorFrame->get_RelativeTime(&nTime);

	if (SUCCEEDED(hr))
	{
		hr = pColorFrame->get_FrameDescription(&pFrameDescription);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrameDescription->get_Width(&nWidth);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrameDescription->get_Height(&nHeight);
	}

	if (SUCCEEDED(hr))
	{
		hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
	}

	if (SUCCEEDED(hr))
	{
		if (imageFormat == ColorImageFormat_Bgra)
		{
			hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));
			memcpy(m_sKinectData.m_pColorRGBX, pBuffer, nBufferSize);
		}
		else if (m_sKinectData.m_pColorRGBX)
		{
			pBuffer = reinterpret_cast<RGBQUAD*>(m_sKinectData.m_pColorRGBX);
			nBufferSize = cColorWidth * cColorHeight * 4;
			hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize, reinterpret_cast<BYTE*>(pBuffer), ColorImageFormat_Bgra);
		}
		else
		{
			hr = E_FAIL;
		}
	}

	SafeRelease(pFrameDescription);

	return hr;
}
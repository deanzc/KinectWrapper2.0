#include "stdafx.h"
#include "DepthReader.h"

CDepthReader::CDepthReader()
: m_pDepthFrameReader(nullptr)
{
}

CDepthReader::~CDepthReader()
{
	if (m_pDepthFrameReader && m_hFrameArrived)
	{
		m_pDepthFrameReader->UnsubscribeFrameArrived(m_hFrameArrived);
		m_hFrameArrived = 0;
	}

	SafeRelease(m_pDepthFrameReader);
}

HRESULT CDepthReader::Init(IKinectSensor* pKinectSensor)
{
	HRESULT	hr = E_FAIL;
	IDepthFrameSource* pDepthFrameSource = nullptr;

	hr = pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);

	if (SUCCEEDED(hr))
	{
		hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
	}

	if (SUCCEEDED(hr) && m_ReaderMode == Event)
	{
		hr = m_pDepthFrameReader->SubscribeFrameArrived(&m_hFrameArrived);
	}

	SafeRelease(pDepthFrameSource);

	return hr;
}

HRESULT CDepthReader::Update()
{
	HRESULT hr = E_FAIL;
	IDepthFrame* pDepthFrame = nullptr;

	if (!m_pDepthFrameReader) { return hr; }

	if (m_ReaderMode == Event)
	{
		IDepthFrameReference* pDepthFrameReference = nullptr;
		IDepthFrameArrivedEventArgs* pDepthFrameArrivedEventArgs = nullptr;

		hr = m_pDepthFrameReader->GetFrameArrivedEventData(m_hFrameArrived, &pDepthFrameArrivedEventArgs);

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameArrivedEventArgs->get_FrameReference(&pDepthFrameReference);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameReference->AcquireFrame(&pDepthFrame);
		}

		SafeRelease(pDepthFrameReference);
		SafeRelease(pDepthFrameArrivedEventArgs);
	}
	else
	{
		hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
	}

	if (SUCCEEDED(hr))
	{
		hr = Process(pDepthFrame);
	}

	SafeRelease(pDepthFrame);
	return hr;
}

HRESULT CDepthReader::Process(IDepthFrame* pDepthFrame)
{
	INT64 nTime = 0;
	IFrameDescription* pFrameDescription = NULL;
	int nWidth = 0;
	int nHeight = 0;
	USHORT nDepthMinReliableDistance = 0;
	USHORT nDepthMaxDistance = 0;
	UINT nBufferSize = 0;
	UINT16 *pBuffer = NULL;

	HRESULT hr = E_FAIL;
	if (!pDepthFrame) { return hr; }
	hr = pDepthFrame->get_RelativeTime(&nTime);

	if (SUCCEEDED(hr))
	{
		hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
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
		hr = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);
	}

	if (SUCCEEDED(hr))
	{
		// In order to see the full range of depth (including the less reliable far field depth)
		// we are setting nDepthMaxDistance to the extreme potential depth threshold
		nDepthMaxDistance = USHRT_MAX;

		// Note: If you wish to filter by reliable depth distance, uncomment the following line.
		//// hr = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxDistance);
	}

	if (SUCCEEDED(hr))
	{
		hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
	}

	if (SUCCEEDED(hr))
	{
		if (m_sKinectData.m_pDepthRGBX && pBuffer && (nWidth == cDepthWidth) && (nHeight == cDepthHeight))
		{
			RGBQUAD* pRGBX = reinterpret_cast<RGBQUAD*>(m_sKinectData.m_pDepthRGBX);

			// end pixel is start + width*height -1
			const UINT16* pBufferEnd = pBuffer + (nWidth * nHeight);

			while (pBuffer < pBufferEnd)
			{
				USHORT depth = *pBuffer;

				BYTE intensity = static_cast<BYTE>((depth >= nDepthMinReliableDistance) && (depth <= nDepthMaxDistance) ? (depth % 256) : 0);

				pRGBX->rgbRed = intensity;
				pRGBX->rgbGreen = intensity;
				pRGBX->rgbBlue = intensity;

				++pRGBX;
				++pBuffer;
			}
		}
	}

	SafeRelease(pFrameDescription);

	return hr;
}
#include "stdafx.h"
#include "BodyIndexReader.h"

CBodyIndexReader::CBodyIndexReader()
: m_pBodyIndexFrameReader(nullptr)
{
}

CBodyIndexReader::~CBodyIndexReader()
{
	if (m_pBodyIndexFrameReader && m_hFrameArrived)
	{
		m_pBodyIndexFrameReader->UnsubscribeFrameArrived(m_hFrameArrived);
		m_hFrameArrived = 0;
	}

	SafeRelease(m_pBodyIndexFrameReader);
}

HRESULT CBodyIndexReader::Init(IKinectSensor* pKinectSensor)
{
	HRESULT	hr = E_FAIL;
	IBodyIndexFrameSource* pBodyIndexFrameSource = nullptr;

	hr = pKinectSensor->get_BodyIndexFrameSource(&pBodyIndexFrameSource);

	if (SUCCEEDED(hr))
	{
		hr = pBodyIndexFrameSource->OpenReader(&m_pBodyIndexFrameReader);
	}

	if (SUCCEEDED(hr) && m_ReaderMode == Event)
	{
		hr = m_pBodyIndexFrameReader->SubscribeFrameArrived(&m_hFrameArrived);
	}

	SafeRelease(pBodyIndexFrameSource);

	return hr;
}

HRESULT CBodyIndexReader::Update()
{
	HRESULT hr = E_FAIL;
	IBodyIndexFrame* pBodyIndexFrame = nullptr;

	if (!m_pBodyIndexFrameReader) { return hr; }

	if (m_ReaderMode == Event)
	{
		IBodyIndexFrameReference* pBodyIndexFrameReference = nullptr;
		IBodyIndexFrameArrivedEventArgs* pBodyIndexFrameArrivedEventArgs = nullptr;

		hr = m_pBodyIndexFrameReader->GetFrameArrivedEventData(m_hFrameArrived, &pBodyIndexFrameArrivedEventArgs);

		if (SUCCEEDED(hr))
		{
			hr = pBodyIndexFrameArrivedEventArgs->get_FrameReference(&pBodyIndexFrameReference);
		}

		if (SUCCEEDED(hr))
		{
			hr = pBodyIndexFrameReference->AcquireFrame(&pBodyIndexFrame);
		}

		SafeRelease(pBodyIndexFrameReference);
		SafeRelease(pBodyIndexFrameArrivedEventArgs);
	}
	else
	{
		hr = m_pBodyIndexFrameReader->AcquireLatestFrame(&pBodyIndexFrame);
	}

	if (SUCCEEDED(hr))
	{
		hr = Process(pBodyIndexFrame);
	}

	SafeRelease(pBodyIndexFrame);
	return hr;
}

HRESULT CBodyIndexReader::Process(IBodyIndexFrame* pBodyIndexFrame)
{
	INT64 nTime = 0;
	IFrameDescription* pFrameDescription = NULL;
	int nWidth = 0;
	int nHeight = 0;
	UINT nBufferSize = 0;
	BYTE *pBuffer = NULL;

	HRESULT hr = E_FAIL;
	if (!pBodyIndexFrame) { return hr; }
	hr = pBodyIndexFrame->get_RelativeTime(&nTime);

	if (SUCCEEDED(hr))
	{
		hr = pBodyIndexFrame->get_FrameDescription(&pFrameDescription);
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
		hr = pBodyIndexFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
	}

	if (SUCCEEDED(hr))
	{
		if (m_sKinectData.m_pBodyIndexRGBX && pBuffer && (nWidth == cDepthWidth) && (nHeight == cDepthHeight))
		{
			RGBQUAD* pRGBX = reinterpret_cast<RGBQUAD*>(m_sKinectData.m_pBodyIndexRGBX);

			const RGBQUAD BodyColor[6] =
			{
				{ 0x00, 0x00, 0xFF, 0xAA },
				{ 0x00, 0xFF, 0x00, 0xAA },
				{ 0xFF, 0xFF, 0x40, 0xAA },
				{ 0x40, 0xFF, 0xFF, 0xAA },
				{ 0xFF, 0x40, 0xFF, 0xAA },
				{ 0xFF, 0x80, 0x80, 0xAA }
			};

			for (UINT i = 0; i < nBufferSize; i++) {
				//pRGBX->rgbBlue  = *pBuffer & 0x01 ? 0x00 : 0xFF;
				//pRGBX->rgbGreen = *pBuffer & 0x02 ? 0x00 : 0xFF;
				//pRGBX->rgbRed   = *pBuffer & 0x04 ? 0x00 : 0xFF;
				if (*pBuffer < BODY_COUNT) {
					*pRGBX = BodyColor[*pBuffer];
				}
				else {
					*pRGBX = { 0x00, 0x00, 0x00, 0x00 };
				}
				++pRGBX;
				++pBuffer;
			}
		}
	}

	SafeRelease(pFrameDescription);

	return hr;
}
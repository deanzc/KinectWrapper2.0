#include "stdafx.h"
#include "InfraredReader.h"

#define InfraredSourceValueMaximum static_cast<float>(USHRT_MAX)
#define InfraredOutputValueMinimum 0.01f
#define InfraredOutputValueMaximum 1.0f
#define InfraredSceneValueAverage 0.08f
#define InfraredSceneStandardDeviations 3.0f

CInfraredReader::CInfraredReader()
: m_pInfraredFrameReader(nullptr)
{
}

CInfraredReader::~CInfraredReader()
{
	if (m_pInfraredFrameReader && m_hFrameArrived)
	{
		m_pInfraredFrameReader->UnsubscribeFrameArrived(m_hFrameArrived);
		m_hFrameArrived = 0;
	}

	SafeRelease(m_pInfraredFrameReader);
}

HRESULT CInfraredReader::Init(IKinectSensor* pKinectSensor)
{
	HRESULT	hr = E_FAIL;
	IInfraredFrameSource* pInfraredFrameSource = nullptr;

	hr = pKinectSensor->get_InfraredFrameSource(&pInfraredFrameSource);

	if (SUCCEEDED(hr))
	{
		hr = pInfraredFrameSource->OpenReader(&m_pInfraredFrameReader);
	}

	if (SUCCEEDED(hr) && m_ReaderMode == Event)
	{
		hr = m_pInfraredFrameReader->SubscribeFrameArrived(&m_hFrameArrived);
	}

	SafeRelease(pInfraredFrameSource);

	return hr;
}

HRESULT CInfraredReader::Update()
{
	HRESULT hr = E_FAIL;
	IInfraredFrame* pInfraredFrame = nullptr;

	if (!m_pInfraredFrameReader) { return hr; }

	if (m_ReaderMode == Event)
	{
		IInfraredFrameReference* pInfraredFrameReference = nullptr;
		IInfraredFrameArrivedEventArgs* pInfraredFrameArrivedEventArgs = nullptr;

		hr = m_pInfraredFrameReader->GetFrameArrivedEventData(m_hFrameArrived, &pInfraredFrameArrivedEventArgs);

		if (SUCCEEDED(hr))
		{
			hr = pInfraredFrameArrivedEventArgs->get_FrameReference(&pInfraredFrameReference);
		}

		if (SUCCEEDED(hr))
		{
			hr = pInfraredFrameReference->AcquireFrame(&pInfraredFrame);
		}

		SafeRelease(pInfraredFrameReference);
		SafeRelease(pInfraredFrameArrivedEventArgs);
	}
	else
	{
		hr = m_pInfraredFrameReader->AcquireLatestFrame(&pInfraredFrame);
	}

	if (SUCCEEDED(hr))
	{
		hr = Process(pInfraredFrame);
	}

	SafeRelease(pInfraredFrame);
	return hr;
}

HRESULT CInfraredReader::Process(IInfraredFrame* pInfraredFrame)
{
	INT64 nTime = 0;
	IFrameDescription* pFrameDescription = NULL;
	int nWidth = 0;
	int nHeight = 0;
	UINT nBufferSize = 0;
	UINT16 *pBuffer = NULL;

	HRESULT hr = E_FAIL;
	if (!pInfraredFrame) { return hr; }
	hr = pInfraredFrame->get_RelativeTime(&nTime);

	if (SUCCEEDED(hr))
	{
		hr = pInfraredFrame->get_FrameDescription(&pFrameDescription);
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
		hr = pInfraredFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
	}

	if (SUCCEEDED(hr))
	{
		if (m_sKinectData.m_pInfraredRGBX && pBuffer && (nWidth == cDepthWidth) && (nHeight == cDepthHeight))
		{
			RGBQUAD* pDest = reinterpret_cast<RGBQUAD*>(m_sKinectData.m_pInfraredRGBX);

			// end pixel is start + width*height -1
			const UINT16* pBufferEnd = pBuffer + (nWidth * nHeight);

			while (pBuffer < pBufferEnd)
			{
				// normalize the incoming infrared data (ushort) to a float ranging from
				// [InfraredOutputValueMinimum, InfraredOutputValueMaximum] by
				// 1. dividing the incoming value by the source maximum value
				float intensityRatio = static_cast<float>(*pBuffer) / InfraredSourceValueMaximum;

				// 2. dividing by the (average scene value * standard deviations)
				intensityRatio /= InfraredSceneValueAverage * InfraredSceneStandardDeviations;

				// 3. limiting the value to InfraredOutputValueMaximum
				intensityRatio = min(InfraredOutputValueMaximum, intensityRatio);

				// 4. limiting the lower value InfraredOutputValueMinium
				intensityRatio = max(InfraredOutputValueMinimum, intensityRatio);

				// 5. converting the normalized value to a byte and using the result
				// as the RGB components required by the image
				byte intensity = static_cast<byte>(intensityRatio * 255.0f);
				pDest->rgbRed = intensity;
				pDest->rgbGreen = intensity;
				pDest->rgbBlue = intensity;

				++pDest;
				++pBuffer;
			}
		}
	}

	SafeRelease(pFrameDescription);

	return hr;
}
#include "stdafx.h"
#include "FaceReader.h"

// define the face frame features required to be computed by this application
static const DWORD c_FaceFrameFeatures =
  FaceFrameFeatures::FaceFrameFeatures_BoundingBoxInColorSpace
| FaceFrameFeatures::FaceFrameFeatures_PointsInColorSpace
| FaceFrameFeatures::FaceFrameFeatures_RotationOrientation
| FaceFrameFeatures::FaceFrameFeatures_Happy
| FaceFrameFeatures::FaceFrameFeatures_RightEyeClosed
| FaceFrameFeatures::FaceFrameFeatures_LeftEyeClosed
| FaceFrameFeatures::FaceFrameFeatures_MouthOpen
| FaceFrameFeatures::FaceFrameFeatures_MouthMoved
| FaceFrameFeatures::FaceFrameFeatures_LookingAway
| FaceFrameFeatures::FaceFrameFeatures_Glasses
| FaceFrameFeatures::FaceFrameFeatures_FaceEngagement;

CFaceReader::CFaceReader(UINT id)
: m_pFaceFrameSource(nullptr),
	m_pFaceFrameReader(nullptr)
{
	m_iReaderExId = id;
}

CFaceReader::~CFaceReader()
{
	if (m_pFaceFrameReader && m_hFrameArrived)
	{
		m_pFaceFrameReader->UnsubscribeFrameArrived(m_hFrameArrived);
		m_hFrameArrived = 0;
	}

	SafeRelease(m_pFaceFrameReader);
	SafeRelease(m_pFaceFrameSource);
}

HRESULT CFaceReader::PutBodyTrackingId(UINT64 uBodyTId)
{
	HRESULT hr = E_FAIL;
	BOOLEAN bTracked = TRUE;
	hr = m_pFaceFrameSource->get_IsTrackingIdValid(&bTracked);

	if (SUCCEEDED(hr) && !bTracked)
	{
		hr = m_pFaceFrameSource->put_TrackingId(uBodyTId);
	}

	return hr;
}

HRESULT CFaceReader::Init(IKinectSensor* pKinectSensor)
{
	HRESULT hr = E_FAIL;

	hr = CreateFaceFrameSource(pKinectSensor, 0, c_FaceFrameFeatures, &m_pFaceFrameSource);

	if (SUCCEEDED(hr))
	{
		hr = m_pFaceFrameSource->OpenReader(&m_pFaceFrameReader);
	}

	if (SUCCEEDED(hr) && m_ReaderMode == Event)
	{
		hr = m_pFaceFrameReader->SubscribeFrameArrived(&m_hFrameArrived);
	}

	return hr;
}

HRESULT CFaceReader::Update()
{
	HRESULT hr = E_FAIL;
	IFaceFrame* pFaceFrame = nullptr;

	if (!m_pFaceFrameReader) { return hr; }

	if (m_ReaderMode == Event)
	{
		IFaceFrameReference* pFaceFrameReference = nullptr;
		IFaceFrameArrivedEventArgs* pFaceFrameArrivedEventArgs = nullptr;

		hr = m_pFaceFrameReader->GetFrameArrivedEventData(m_hFrameArrived, &pFaceFrameArrivedEventArgs);

		if (SUCCEEDED(hr))
		{
			hr = pFaceFrameArrivedEventArgs->get_FrameReference(&pFaceFrameReference);
		}

		if (SUCCEEDED(hr))
		{
			hr = pFaceFrameReference->AcquireFrame(&pFaceFrame);
		}

		SafeRelease(pFaceFrameReference);
		SafeRelease(pFaceFrameArrivedEventArgs);
	}
	else
	{
		hr = m_pFaceFrameReader->AcquireLatestFrame(&pFaceFrame);
	}

	if (SUCCEEDED(hr))
	{
		hr = Process(pFaceFrame);
	}

	SafeRelease(pFaceFrame);

	return hr;
}

HRESULT CFaceReader::Process(IFaceFrame* pFaceFrame)
{
	INT64 nTime = 0;
	BOOLEAN bFaceTracked = false;

	HRESULT hr = E_FAIL;
	if (!pFaceFrame) { return hr; }
	hr = pFaceFrame->get_RelativeTime(&nTime);

	if (SUCCEEDED(hr))
	{
		hr = pFaceFrame->get_IsTrackingIdValid(&bFaceTracked);
	}

	if (SUCCEEDED(hr) && bFaceTracked)
	{
		IFaceFrameResult* pFaceFrameResult = nullptr;
		RectI faceBox = { 0 };
		PointF facePoints[FacePointType_Count];
		Vector4 faceRotation;
		DetectionResult faceProperties[FaceProperty_Count];

		hr = pFaceFrame->get_FaceFrameResult(&pFaceFrameResult);

		if (SUCCEEDED(hr) && pFaceFrameResult != nullptr)
		{
			hr = pFaceFrameResult->get_FaceBoundingBoxInColorSpace(&faceBox);

			if (SUCCEEDED(hr))
			{
				hr = pFaceFrameResult->GetFacePointsInColorSpace(FacePointType::FacePointType_Count, facePoints);
			}

			if (SUCCEEDED(hr))
			{
				hr = pFaceFrameResult->get_FaceRotationQuaternion(&faceRotation);
			}

			if (SUCCEEDED(hr))
			{
				hr = pFaceFrameResult->GetFaceProperties(FaceProperty::FaceProperty_Count, faceProperties);
			}
		}

		if (SUCCEEDED(hr)) // save
		{
			m_sKinectData.m_FaceData[m_iReaderExId].bTracked = TRUE;
			m_sKinectData.m_FaceData[m_iReaderExId].faceBox = faceBox;
			m_sKinectData.m_FaceData[m_iReaderExId].faceRotation = faceRotation;

			for (int i = 0; i < FacePointType_Count; i++)
			{
				m_sKinectData.m_FaceData[m_iReaderExId].facePoints[i] = facePoints[i];
			}

			for (int j = 0; j < FaceProperty_Count; j++)
			{
				m_sKinectData.m_FaceData[m_iReaderExId].faceProperties[j] = faceProperties[j];
			}
		}

		SafeRelease(pFaceFrameResult);
	}
	else
	{
		m_sKinectData.m_FaceData[m_iReaderExId].bTracked = FALSE;

		UINT64 uTrackedId = m_sKinectData.m_BodyData[m_sKinectData.m_FaceData[m_iReaderExId].iTrackTarget].uTrackedId;

		if (uTrackedId != 0)
		{
			PutBodyTrackingId(uTrackedId);
		}
	}

	return hr;
}
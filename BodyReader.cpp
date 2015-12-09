#include "stdafx.h"
#include "BodyReader.h"

CBodyReader::CBodyReader()
: m_pBodyFrameReader(nullptr)
{
}

CBodyReader::~CBodyReader()
{
	if (m_pBodyFrameReader && m_hFrameArrived)
	{
		m_pBodyFrameReader->UnsubscribeFrameArrived(m_hFrameArrived);
		m_hFrameArrived = 0;
	}

	SafeRelease(m_pBodyFrameReader);
}

HRESULT CBodyReader::Init(IKinectSensor* pKinectSensor)
{	
	HRESULT hr = E_FAIL;
	IBodyFrameSource* pBodyFrameSource = nullptr;

	hr = pKinectSensor->get_BodyFrameSource(&pBodyFrameSource);

	if (SUCCEEDED(hr))
	{
		hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
	}

	if (SUCCEEDED(hr) && m_ReaderMode == Event)
	{
		hr = m_pBodyFrameReader->SubscribeFrameArrived(&m_hFrameArrived);
	}

	SafeRelease(pBodyFrameSource);

	return hr;
}

HRESULT CBodyReader::Update()
{
	HRESULT hr = E_FAIL;
	IBodyFrame* pBodyFrame = nullptr;

	if (!m_pBodyFrameReader) { return hr; }

	if (m_ReaderMode == Event)
	{
		IBodyFrameReference* pBodyFrameReference = nullptr;
		IBodyFrameArrivedEventArgs* pBodyFrameArrivedEventArgs = nullptr;

		hr = m_pBodyFrameReader->GetFrameArrivedEventData(m_hFrameArrived, &pBodyFrameArrivedEventArgs);

		if (SUCCEEDED(hr))
		{
			hr = pBodyFrameArrivedEventArgs->get_FrameReference(&pBodyFrameReference);
		}

		if (SUCCEEDED(hr))
		{
			hr = pBodyFrameReference->AcquireFrame(&pBodyFrame);
		}

		SafeRelease(pBodyFrameReference);
		SafeRelease(pBodyFrameArrivedEventArgs);
	}
	else
	{
		hr = m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);
	}

	if (SUCCEEDED(hr))
	{
		hr = Process(pBodyFrame);
	}

	SafeRelease(pBodyFrame);
	return hr;
}

HRESULT CBodyReader::Process(IBodyFrame* pBodyFrame)
{
	INT64 nTime = 0;
	IBody* ppBodies[BODY_COUNT] = { 0 };

	HRESULT hr = E_FAIL;
	if (!pBodyFrame) { return hr; }
	hr = pBodyFrame->get_RelativeTime(&nTime);

	if (SUCCEEDED(hr))
	{
		hr = pBodyFrame->GetAndRefreshBodyData(_countof(ppBodies), ppBodies);
	}

	if (SUCCEEDED(hr))
	{
		for (int i = 0; i < BODY_COUNT; ++i)
		{
			IBody* pBody = ppBodies[i];

			if (pBody)
			{
				BOOLEAN bTracked = false;
				HRESULT hr = pBody->get_IsTracked(&bTracked);

				if (SUCCEEDED(hr) && bTracked)
				{
					UINT64 uBodyTId = 0;
					Joint joints[JointType_Count];
					HandState leftHandState = HandState_Unknown;
					HandState rightHandState = HandState_Unknown;

					hr = pBody->get_TrackingId(&uBodyTId);
					hr = pBody->get_HandLeftState(&leftHandState);
					hr = pBody->get_HandRightState(&rightHandState);
					hr = pBody->GetJoints(_countof(joints), joints);

					if (SUCCEEDED(hr)) //save
					{
						m_sKinectData.m_BodyData[i].uTrackedId = uBodyTId;
						m_sKinectData.m_BodyData[i].leftHandState = leftHandState;
						m_sKinectData.m_BodyData[i].rightHandState = rightHandState;

						for (int j = 0; j < JointType_Count; ++j)
						{
							m_sKinectData.m_BodyData[i].jointStates[j] = joints[j].TrackingState;
							m_sKinectData.m_BodyData[i].jointPositions[j] = joints[j].Position;
						}
					}
				}
				else
				{
					m_sKinectData.m_BodyData[i].uTrackedId = 0;
				}
			}
		}
	}

	for (int i = 0; i < _countof(ppBodies); ++i)
	{
		SafeRelease(ppBodies[i]);
	}

	return hr;
}
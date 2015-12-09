#include "stdafx.h"
#include "KinectData.h"

CKinectData::CKinectData()
: m_pColorRGBX(nullptr),
	m_pInfraredRGBX(nullptr),
	m_pLongExposureInfraredRGBX(nullptr),
	m_pDepthRGBX(nullptr),
	m_pBodyIndexRGBX(nullptr)
{
	//image data
	m_pColorRGBX = new unsigned char[cColorWidth * cColorHeight * 4];
	m_pInfraredRGBX = new unsigned char[cDepthWidth * cDepthHeight * 4];
	m_pLongExposureInfraredRGBX = new unsigned char[cDepthWidth * cDepthHeight * 4];
	m_pDepthRGBX = new unsigned char[cDepthWidth * cDepthHeight * 4];
	m_pBodyIndexRGBX = new unsigned char[cDepthWidth * cDepthHeight * 4];

	//audio data
	m_AudioData.fBeamAngle = 0.f;
	m_AudioData.fBeamAngleConfidence = 0.f;
	m_AudioData.bHaveNewEnergy = false;
	memset(m_AudioData.fEnergyBuffer, 0, sizeof(float)*sizeof(m_AudioData.fEnergyBuffer));

	for (int i = 0; i < BODY_COUNT; i++)
	{	
		//body data
		m_BodyData[i].uTrackedId = 0;
		m_BodyData[i].leftHandState = HandState_Unknown;
		m_BodyData[i].rightHandState = HandState_Unknown;

		for (int j = 0; j < JointType_Count; ++j)
		{
			m_BodyData[i].jointStates[j] = TrackingState_NotTracked;
			m_BodyData[i].jointPositions[j] = { 0.f, 0.f, 0.f };
		}

		//face data
		m_FaceData[i].iTrackTarget = i;
		m_FaceData[i].bTracked = false;
		m_FaceData[i].faceBox = {0, 0, 0, 0};
		m_FaceData[i].faceRotation = { 0.f, 0.f, 0.f, 0.f };

		for (int j = 0; j < FacePointType_Count; ++j)
		{
			m_FaceData[i].facePoints[j] = { 0.f, 0.f };
		}

		for (int j = 0; j < FaceProperty_Count; ++j)
		{
			m_FaceData[i].faceProperties[j] = DetectionResult_Unknown;
		}

		//hd face data
		m_HDFaceData[i].iTrackTarget = i;
		m_HDFaceData[i].bTracked = false;
		m_HDFaceData[i].faceBox = { 0, 0, 0, 0 };
		m_HDFaceData[i].faceRotation = { 0.f, 0.f, 0.f, 0.f };
		m_HDFaceData[i].headPivot = { 0.f, 0.f };

		for (int j = 0; j < FaceShapeAnimations_Count; ++j)
		{ 
			m_HDFaceData[i].fsa[j] = 0.0f; 
		}

		for (int j = 0; j < FaceShapeDeformations_Count; ++j)
		{
			m_HDFaceData[i].fsd[j] = 0.0f;
		}

		//vgb data
		m_VGBData[i].iTrackTarget = i;
		m_VGBData[i].bTracked = false;

		for (int j = 0; j < cGestureMaxSize; ++j)
		{
			m_VGBData[i].gData[j].gType = GestureType_None;
			m_VGBData[i].gData[j].bDetected = false;
			m_VGBData[i].gData[j].fConfidence = 0.0f;
			m_VGBData[i].gData[j].bFFDetected = true;
			m_VGBData[i].gData[j].fProgress = 0.0f;
		}
	}
}

CKinectData::~CKinectData()
{
	if (m_pColorRGBX)
	{
		delete[] m_pColorRGBX;
		m_pColorRGBX = nullptr;
	}

	if (m_pInfraredRGBX)
	{
		delete[] m_pInfraredRGBX;
		m_pInfraredRGBX = nullptr;
	}

	if (m_pLongExposureInfraredRGBX)
	{
		delete[] m_pLongExposureInfraredRGBX;
		m_pLongExposureInfraredRGBX = nullptr;
	}

	if (m_pDepthRGBX)
	{
		delete[] m_pDepthRGBX;
		m_pDepthRGBX = nullptr;
	}

	if (m_pBodyIndexRGBX)
	{
		delete[] m_pBodyIndexRGBX;
		m_pBodyIndexRGBX = nullptr;
	}
}
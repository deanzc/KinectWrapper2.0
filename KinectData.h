#pragma once

static const int	  cColorWidth     = 1920;
static const int	  cColorHeight    = 1080;
static const int	  cDepthWidth     = 512;  //Depth | Infrared | BodyIndex
static const int	  cDepthHeight    = 424;  //Depth | Infrared | BodyIndex
static const int    cGestureMaxSize = 64;

struct AudioData
{
	float             fBeamAngle;
	float             fBeamAngleConfidence;	
	bool              bHaveNewEnergy;         //clear by user
	float             fEnergyBuffer[1000];    //cEnergyBufferLength
};

struct BodyData
{
	UINT64            uTrackedId;
	HandState         leftHandState;
	HandState         rightHandState;
	TrackingState     jointStates[JointType_Count];
	CameraSpacePoint  jointPositions[JointType_Count];
};

struct FaceData
{
	UINT              iTrackTarget; //user
	BOOLEAN           bTracked;
	RectI             faceBox;
	Vector4           faceRotation;
	PointF            facePoints[FacePointType_Count];
	DetectionResult   faceProperties[FaceProperty_Count];
};

struct HDFaceData
{
	UINT              iTrackTarget; //user
	BOOLEAN           bTracked;
	RectI             faceBox;
	Vector4           faceRotation;
	CameraSpacePoint  headPivot;
	float             fsa[FaceShapeAnimations_Count];
	float             fsd[FaceShapeDeformations_Count];
};

struct VGBData
{
	UINT              iTrackTarget; //user
	BOOLEAN           bTracked;
	struct GestureData
	{
		GestureType     gType;
		BOOLEAN         bDetected;
		float           fConfidence;
		BOOLEAN         bFFDetected;
		float           fProgress;
	}	gData[cGestureMaxSize];
};

struct FusionData
{
};

struct PointerData
{
};

class CKinectData
{
public:
	CKinectData();
	~CKinectData();

	unsigned char*    m_pColorRGBX;
	unsigned char*    m_pInfraredRGBX;
	unsigned char*    m_pLongExposureInfraredRGBX;
	unsigned char*    m_pDepthRGBX;
	unsigned char*    m_pBodyIndexRGBX;
	AudioData         m_AudioData;
	BodyData          m_BodyData[BODY_COUNT];
	FaceData          m_FaceData[BODY_COUNT];
	HDFaceData        m_HDFaceData[BODY_COUNT];
	VGBData           m_VGBData[BODY_COUNT];
};
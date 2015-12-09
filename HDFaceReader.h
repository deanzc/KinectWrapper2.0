#pragma once
#include "ReaderEx.h"

class CHDFaceReader :	public CReaderEx
{
public:
	CHDFaceReader(UINT id, bool bBuildModel = false);
	~CHDFaceReader();
	HRESULT             Init(IKinectSensor* pKinectSensor);
	HRESULT             Update();

private:	
	BOOL                m_bNeedBuildModel;
	UINT                m_uFaceVerticeCount;
	CameraSpacePoint*   m_pFaceVertices; //dynamic
	UINT32              m_uFaceModelTriangleCount;
	UINT32*             m_pFaceModelTriangles; //dynamic

	IFaceAlignment*     m_pFaceAlignment;
	IFaceModelBuilder*  m_pFaceModelBuilder;
	IFaceModel*         m_pFaceModel;
	IHighDefinitionFaceFrameSource* m_pHDFaceFrameSource;
	IHighDefinitionFaceFrameReader* m_pHDFaceFrameReader;

	HRESULT             PutBodyTrackingId(UINT64 uBodyTId);
	HRESULT             Process(IHighDefinitionFaceFrame* pFaceFrame);	
	BOOLEAN             SaveModelFile();
	BOOLEAN             ReadFSDFile(PCWSTR filename, float* fsd);
	BOOLEAN             WriteFSDFile(PCWSTR filename, float* fsd);
};
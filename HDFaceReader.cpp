#include "stdafx.h"
#include <string>
#include "HDFaceReader.h"

CHDFaceReader::CHDFaceReader(UINT id, bool bBuildModel)
: m_bNeedBuildModel(bBuildModel),
  m_pHDFaceFrameSource(nullptr),
  m_pHDFaceFrameReader(nullptr)
{
	m_iReaderExId = id;
}

CHDFaceReader::~CHDFaceReader()
{
	if (m_pFaceVertices)
	{
		free(m_pFaceVertices);
		m_pFaceVertices = nullptr;
	}

	if (m_pFaceModelTriangles)
	{
		free(m_pFaceModelTriangles);
		m_pFaceModelTriangles = nullptr;
	}

	if (m_bNeedBuildModel)
	{
		SafeRelease(m_pFaceModelBuilder);
	}
	SafeRelease(m_pFaceModel);
	SafeRelease(m_pFaceAlignment);

	if (m_pHDFaceFrameReader && m_hFrameArrived)
	{
		m_pHDFaceFrameReader->UnsubscribeFrameArrived(m_hFrameArrived);
		m_hFrameArrived = 0;
	}

	SafeRelease(m_pHDFaceFrameSource);
	SafeRelease(m_pHDFaceFrameReader);
}

HRESULT CHDFaceReader::PutBodyTrackingId(UINT64 uBodyTId)
{
	HRESULT hr = E_FAIL;
	BOOLEAN bTracked = TRUE;
	hr = m_pHDFaceFrameSource->get_IsTrackingIdValid(&bTracked);

	if (SUCCEEDED(hr) && !bTracked)
	{
		hr = m_pHDFaceFrameSource->put_TrackingId(uBodyTId);
	}

	return hr;
}

HRESULT CHDFaceReader::Init(IKinectSensor* pKinectSensor)
{
	HRESULT hr = E_FAIL;
	hr = CreateHighDefinitionFaceFrameSource(pKinectSensor, &m_pHDFaceFrameSource);

	if (SUCCEEDED(hr))
	{
		hr = m_pHDFaceFrameSource->OpenReader(&m_pHDFaceFrameReader);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	if (SUCCEEDED(hr))
	{
		hr = CreateFaceAlignment(&m_pFaceAlignment);
	}

	if (m_bNeedBuildModel)
	{
		if (SUCCEEDED(hr))
		{
			hr = m_pHDFaceFrameSource->OpenModelBuilder(FaceModelBuilderAttributes_None, &m_pFaceModelBuilder);
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pFaceModelBuilder->BeginFaceDataCollection();
		}
	}

	if (SUCCEEDED(hr))
	{
		float fsd[FaceShapeDeformations_Count] = { 0.0f };
		if (!ReadFSDFile(L"FaceShapeDeformations.txt", fsd)) //≥¢ ‘∂¡»°
		{
			for (int i = 0; i < FaceShapeDeformations_Count; i++) { fsd[i] = 0.0f; }
		}
		hr = CreateFaceModel(1.f, FaceShapeDeformations::FaceShapeDeformations_Count, fsd, &m_pFaceModel);
	}

	if (SUCCEEDED(hr))
	{
		hr = GetFaceModelVertexCount(&m_uFaceVerticeCount);
	}

	if (SUCCEEDED(hr))
	{
		m_pFaceVertices = reinterpret_cast<CameraSpacePoint*>( // ∂•µ„ª∫¥Ê 3D+2D
			malloc((sizeof(CameraSpacePoint)+sizeof(PointF)) * m_uFaceVerticeCount));

		if (!m_pFaceVertices) hr = E_OUTOFMEMORY;
	}

	if (SUCCEEDED(hr))
	{
		hr = GetFaceModelTriangleCount(&m_uFaceModelTriangleCount);

		m_pFaceModelTriangles = reinterpret_cast<UINT32*>(malloc(sizeof(UINT32)* m_uFaceModelTriangleCount * 3));
		if (!m_pFaceModelTriangles) hr = E_OUTOFMEMORY;
	}

	if (SUCCEEDED(hr))
	{
		hr = GetFaceModelTriangles(m_uFaceModelTriangleCount * 3, m_pFaceModelTriangles);
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////

	if (SUCCEEDED(hr) && m_ReaderMode == Event)
	{
		hr = m_pHDFaceFrameReader->SubscribeFrameArrived(&m_hFrameArrived);
	}

	return hr;
}

HRESULT CHDFaceReader::Update()
{
	HRESULT hr = E_FAIL;
	IHighDefinitionFaceFrame* pHDFaceFrame = nullptr;

	if (!m_pHDFaceFrameReader) { return hr; }

	if (m_ReaderMode == Event)
	{
		IHighDefinitionFaceFrameReference* pHDFaceFrameReference = nullptr;
		IHighDefinitionFaceFrameArrivedEventArgs* pHDFaceFrameArrivedEventArgs = nullptr;

		hr = m_pHDFaceFrameReader->GetFrameArrivedEventData(m_hFrameArrived, &pHDFaceFrameArrivedEventArgs);

		if (SUCCEEDED(hr))
		{
			hr = pHDFaceFrameArrivedEventArgs->get_FrameReference(&pHDFaceFrameReference);
		}

		if (SUCCEEDED(hr))
		{
			hr = pHDFaceFrameReference->AcquireFrame(&pHDFaceFrame);
		}

		SafeRelease(pHDFaceFrameReference);
		SafeRelease(pHDFaceFrameArrivedEventArgs);
	}
	else
	{
		hr = m_pHDFaceFrameReader->AcquireLatestFrame(&pHDFaceFrame);
	}

	if (SUCCEEDED(hr))
	{
		m_sKinectData.m_HDFaceData[m_iReaderExId].bTracked = TRUE;
		hr = Process(pHDFaceFrame);
	}
	else
	{
		m_sKinectData.m_HDFaceData[m_iReaderExId].bTracked = FALSE;
		UINT64 uTrackedId = m_sKinectData.m_BodyData[m_sKinectData.m_HDFaceData[m_iReaderExId].iTrackTarget].uTrackedId;
		if (uTrackedId != 0)
		{
			hr = PutBodyTrackingId(uTrackedId);
		}
	}

	SafeRelease(pHDFaceFrame);

	return hr;
}

HRESULT CHDFaceReader::Process(IHighDefinitionFaceFrame* pHDFaceFrame)
{
	INT64 nTime = 0;
	BOOLEAN bHDFaceTracked = false;
	float fsa[FaceShapeAnimations_Count];
	float fsd[FaceShapeDeformations_Count];
	RectI faceBox = { 0, 0, 0, 0 };
	Vector4 faceRotation = { 0.f, 0.f, 0.f, 0.f };
	CameraSpacePoint headPivot = { 0.f, 0.f };
	PointF* screenPoints = reinterpret_cast<PointF*>(m_pFaceVertices + m_uFaceVerticeCount);

	HRESULT hr = E_FAIL;
	if (!pHDFaceFrame) { return hr; }
	hr = pHDFaceFrame->get_RelativeTime(&nTime);

	if (SUCCEEDED(hr))
	{
		hr = pHDFaceFrame->get_IsTrackingIdValid(&bHDFaceTracked);
	}

	if (SUCCEEDED(hr) && bHDFaceTracked)
	{
		hr = pHDFaceFrame->GetAndRefreshFaceAlignmentResult(m_pFaceAlignment);

		if (SUCCEEDED(hr))
		{
			hr = m_pFaceAlignment->get_FaceBoundingBox(&faceBox);
			m_sKinectData.m_HDFaceData[m_iReaderExId].faceBox = faceBox;
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pFaceAlignment->get_FaceOrientation(&faceRotation);
			m_sKinectData.m_HDFaceData[m_iReaderExId].faceRotation = faceRotation;
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pFaceAlignment->get_HeadPivotPoint(&headPivot);
			m_sKinectData.m_HDFaceData[m_iReaderExId].headPivot = headPivot;
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pFaceAlignment->GetAnimationUnits(FaceShapeAnimations_Count, fsa);

			for (int i = 0; i < FaceShapeAnimations_Count; i++)
			{
				m_sKinectData.m_HDFaceData[m_iReaderExId].fsa[i] = fsa[i];
			}
		}

		if (SUCCEEDED(hr) && m_bNeedBuildModel && m_pFaceModelBuilder)
		{
			FaceModelBuilderCollectionStatus co_status = FaceModelBuilderCollectionStatus(-1);
			FaceModelBuilderCaptureStatus    ca_status = FaceModelBuilderCaptureStatus_GoodFrameCapture;

			hr = m_pFaceModelBuilder->get_CollectionStatus(&co_status);

			if (co_status == FaceModelBuilderCollectionStatus_Complete)
			{
				IFaceModelData* pFaceModelData = nullptr;

				if (SUCCEEDED(hr))
				{
					hr = m_pFaceModelBuilder->GetFaceData(&pFaceModelData);
				}

				if (SUCCEEDED(hr))
				{
					SafeRelease(m_pFaceModel); //?
					hr = pFaceModelData->ProduceFaceModel(&m_pFaceModel);
				}

				if (SUCCEEDED(hr))
				{
					m_bNeedBuildModel = FALSE;
					hr = m_pFaceModel->GetFaceShapeDeformations(FaceShapeDeformations_Count, fsd);
					WriteFSDFile(L"FaceShapeDeformations.txt", fsd);
				}

				SafeRelease(pFaceModelData);
			}
			else
			{
				if (SUCCEEDED(hr))
				{
					hr = m_pFaceModelBuilder->get_CaptureStatus(&ca_status);
				}
			}
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pFaceModel->CalculateVerticesForAlignment(m_pFaceAlignment, m_uFaceVerticeCount, m_pFaceVertices);
		}

		if (SUCCEEDED(hr))
		{
			//m_pCoordinateMapper->MapCameraPointsToColorSpace(m_uFaceVerticeCount, m_pFaceVertices,
			//	m_uFaceVerticeCount, const_cast<ColorSpacePoint*>(screenPoints));
			//
			//for (UINT i = 0U; i < m_uFaceVerticeCount; ++i)
			//{
			//	hr = CameraToScreen(m_pFaceVertices[i], &screenPoints[i], m_uRTargetWidth, m_uRTargetHeight);
			//}
		}
	}

	return hr;
}

BOOLEAN CHDFaceReader::ReadFSDFile(PCWSTR filename, float* fsd)
{
	FILE* file = nullptr;
	BYTE buffer[FaceShapeDeformations_Count * 64];
	size_t count = 0;

	if (filename != nullptr)
	{
		errno_t err;
		err = _wfopen_s(&file, filename, L"rb");
		if (file)
		{
			fseek(file, 0, SEEK_END);
			long size = ftell(file);
			fseek(file, 0, 0);
			fread(buffer, 1, size, file);

			int j = 0;
			char fsd_swap[64] = { 0 };
			for (int i = 0; i < size; i++)
			{
				if (buffer[i] != '\r')
				{
					fsd_swap[j++] = buffer[i];
				}
				else
				{
					sscanf_s(fsd_swap, "%f", fsd++);
					memset(fsd_swap, 0, sizeof(fsd_swap)*sizeof(char));
					j = 0; i++; //'\n'
					count++;
				}
			}
			fclose(file);
		}
	}

	return count == FaceShapeDeformations_Count;
}

BOOLEAN CHDFaceReader::WriteFSDFile(PCWSTR filename, float* fsd)
{
	FILE* file = nullptr;
	BYTE buffer[FaceShapeDeformations_Count * 64];

	if (!_wfopen_s(&file, filename, L"wb"))
	{
		char* buf_index = reinterpret_cast<char*>(buffer);
		float* fsd_index = fsd;
		size_t length = 0;
		size_t total_length = 0;

		for (int i = 0; i < FaceShapeDeformations_Count; i++)
		{
			length = sprintf_s(buf_index, 100, "%f\r\n", *fsd_index++);
			buf_index += length;
			total_length += length;
		}

		fwrite(buffer, 1, total_length, file);
		fclose(file);
		return TRUE;
	}

	return FALSE;
}

BOOLEAN CHDFaceReader::SaveModelFile()
{
	FILE* file = nullptr;
	if (!_wfopen_s(&file, L"FaceModel.obj", L"w"))
	{
		// ÃÓ–¥∂•µ„◊¯±Í
		for (UINT i = 0u; i < m_uFaceVerticeCount; ++i)
		{
			fprintf(file, "v %f %f %f\n", m_pFaceVertices[i].X, m_pFaceVertices[i].Y, m_pFaceVertices[i].Z);
		}

		// ÃÓ–¥À˜“˝–≈œ¢
		fprintf(file, "\n\n\n");
		for (UINT32 i = 0u; i < m_uFaceModelTriangleCount * 3; i += 3)
		{
			fprintf(file, "f %d %d %d\n", m_pFaceModelTriangles[i + 0] + 1, m_pFaceModelTriangles[i + 1] + 1, m_pFaceModelTriangles[i + 2] + 1);
		}

		if (file)
		{
			fclose(file);
			file = nullptr;
		}
		return TRUE;
	}

	return FALSE;
}
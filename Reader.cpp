#include "stdafx.h"
#include "Reader.h"

// static data init
CKinectData CReader::m_sKinectData = CKinectData();

CReader::CReader()
: m_ReaderMode(Poll),
	m_hFrameArrived(NULL)
{
}

CReader::~CReader()
{
}

HRESULT CReader::Init(IKinectSensor* pKinectSensor)
{
	OutputDebugString(L"This is CReader Class!");
	return S_OK;
}

HRESULT CReader::Update()
{
	OutputDebugString(L"This is CReader Class!");
	return S_OK;
}

ReaderMode CReader::GetMode()
{
	return m_ReaderMode;
}

VOID CReader::SetMode(ReaderMode readerMode)
{
	m_ReaderMode = readerMode;
}

HANDLE CReader::GetHandle()
{
	return (HANDLE)m_hFrameArrived;
}
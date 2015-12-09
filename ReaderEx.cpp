#include "stdafx.h"
#include "ReaderEx.h"

CReaderEx::CReaderEx()
: m_iReaderExId(0)
{
}

CReaderEx::~CReaderEx()
{
}

HRESULT CReaderEx::PutBodyTrackingId(UINT64 uBodyTId)
{
	OutputDebugString(L"This is CReaderEx Class!");
	return S_OK;
}

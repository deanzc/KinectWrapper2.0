#pragma once
#include "Reader.h"

class CReaderEx : public CReader
{
public:
	CReaderEx();
	virtual ~CReaderEx();
	virtual HRESULT PutBodyTrackingId(UINT64 uBodyTId);

protected:
	UINT    m_iReaderExId;
};


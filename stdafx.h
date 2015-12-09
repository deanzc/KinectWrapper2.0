// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  �� Windows ͷ�ļ����ų�����ʹ�õ���Ϣ
// Windows ͷ�ļ�: 
#include <windows.h>

// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�

#ifndef KINECTWRAPPER_DLLEXPORT
#define KINECTWRAPPER_DLLEXPORT
#endif

#include <Kinect.h>
#include <Kinect.Face.h>
#include <Kinect.VisualGestureBuilder.h>

#pragma comment(lib, "kinect20.lib")
#pragma comment(lib, "Kinect20.Face.lib")
#pragma comment(lib, "Kinect20.VisualGestureBuilder.lib")

// Safe release for interfaces
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != nullptr)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = nullptr;
	}
}
// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息
// Windows 头文件: 
#include <windows.h>

// TODO:  在此处引用程序需要的其他头文件

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
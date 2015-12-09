#include "stdafx.h"
#include "math.h"
#include "AudioReader.h"

// ID of timer that drives energy stream display.
//static const int   cEnergyRefreshTimerId = 2;
// Time interval, in milliseconds, for timer that drives energy stream display.
//static const int   cEnergyRefreshTimerInterval = 10;
// Number of energy samples that will be visible in display at any given time.
//static const int   cEnergySamplesToDisplay = 780;

// Number of audio samples captured from Kinect audio stream accumulated into a single
// energy measurement that will get displayed.
static const int    cAudioSamplesPerEnergySample = 40;

// Audio samples per second in Kinect audio stream
static const int    cAudioSamplesPerSecond = 16000;

// Time interval, in milliseconds, for timer that drives audio capture.
static const int    cAudioReadTimerInterval = 50;

// Number of float samples in the audio beffer we allocate for reading every time the audio capture timer fires
// (should be larger than the amount of audio corresponding to cAudioReadTimerInterval msec).
static const int    cAudioBufferLength = 2 * cAudioReadTimerInterval * cAudioSamplesPerSecond / 1000;

// Minimum energy of audio to display (in dB value, where 0 dB is full scale)
static const int    cMinEnergy = -90;

// Number of energy samples that will be stored in the circular buffer.
// Always keep it higher than the energy display length to avoid overflow.
static const int    cEnergyBufferLength = 1000;


CAudioReader::CAudioReader(bool bStreamMode)
: m_bStreamMode(bStreamMode),
	m_pAudioBeam(nullptr),
	m_pAudioStream(nullptr),
	m_pAudioBeamFrameReader(nullptr)
{
}

CAudioReader::~CAudioReader()
{	
	if (m_pAudioBeamFrameReader && m_hFrameArrived)
	{
		m_pAudioBeamFrameReader->UnsubscribeFrameArrived(m_hFrameArrived);
		m_hFrameArrived = 0;
	}

	SafeRelease(m_pAudioBeam);
	SafeRelease(m_pAudioStream);
	SafeRelease(m_pAudioBeamFrameReader);
}

HRESULT CAudioReader::Init(IKinectSensor* pKinectSensor)
{
	HRESULT	hr = E_FAIL;
	IAudioSource* pAudioSource = nullptr;

	hr = pKinectSensor->get_AudioSource(&pAudioSource);

	if (m_bStreamMode)
	{
		IAudioBeamList* pAudioBeamList = nullptr;

		hr = pKinectSensor->get_AudioSource(&pAudioSource);

		if (SUCCEEDED(hr))
		{
			hr = pAudioSource->get_AudioBeams(&pAudioBeamList);
		}

		if (SUCCEEDED(hr))
		{
			hr = pAudioBeamList->OpenAudioBeam(0, &m_pAudioBeam);
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pAudioBeam->OpenInputStream(&m_pAudioStream);
		}

#if 0
		// To overwrite the automatic mode of the audio beam, change it to
		// manual and set the desired beam angle. In this example, point it
		// straight forward.
		// Note that setting beam mode and beam angle will only work if the
		// application window is in the foreground. However, the operations below will
		// return S_OK even if the application window is in the background.
		// Furthermore, setting these values is an asynchronous operation --
		// it may take a short period of time for the beam to adjust.
		if (SUCCEEDED(hr))
		{
			hr = m_pAudioBeam->put_AudioBeamMode(AudioBeamMode_Manual);
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pAudioBeam->put_BeamAngle(0);
		}
#endif

		SafeRelease(pAudioBeamList);
	}
	else //AudioBeamFrame
	{
		if (SUCCEEDED(hr))
		{
			hr = pAudioSource->OpenReader(&m_pAudioBeamFrameReader);
		}

#if 0
		// To overwrite the automatic mode of the audio beam, change it to
		// manual and set the desired beam angle. In this example, point it
		// straight forward.
		{
			IAudioBeam* pAudioBeam = NULL;
			IAudioBeamList* pAudioBeamList = NULL;

			if (SUCCEEDED(hr))
			{
				hr = pAudioSource->get_AudioBeams(&pAudioBeamList);
			}

			if (SUCCEEDED(hr))
			{
				hr = pAudioBeamList->OpenAudioBeam(0, &pAudioBeam);
			}

			// Note that setting beam mode and beam angle will only work if the
			// application window is in the foreground. However, the operations below will
			// return S_OK even if the application window is in the background.
			// Furthermore, setting these values is an asynchronous operation --
			// it may take a short period of time for the beam to adjust.
			if (SUCCEEDED(hr))
			{
				hr = pAudioBeam->put_AudioBeamMode(AudioBeamMode_Manual);
			}

			if (SUCCEEDED(hr))
			{
				hr = pAudioBeam->put_BeamAngle(0);
			}

			SafeRelease(pAudioBeamList);
			SafeRelease(pAudioBeam);
		}
#endif

		if (SUCCEEDED(hr) && m_ReaderMode == Event)
		{
			hr = m_pAudioBeamFrameReader->SubscribeFrameArrived(&m_hFrameArrived);
		}
	}

	SafeRelease(pAudioSource);

	return hr;
}

HRESULT CAudioReader::Update()
{
	if (m_bStreamMode)
	{
		return StreamProcess();
	}

	HRESULT hr = E_FAIL;
	IAudioBeamFrameList* pAudioBeamFrameList = NULL;
	IAudioBeamFrame* pAudioBeamFrame = NULL;
	UINT32 subFrameCount = 0;

	if (!m_pAudioBeamFrameReader) { return hr; }

	if (m_ReaderMode == Event)
	{
		IAudioBeamFrameReference* pAudioBeamFrameReference = nullptr;
		IAudioBeamFrameArrivedEventArgs* pAudioBeamFrameArrivedEventArgs = nullptr;
		hr = m_pAudioBeamFrameReader->GetFrameArrivedEventData(m_hFrameArrived, &pAudioBeamFrameArrivedEventArgs);
		if (SUCCEEDED(hr))
		{
			hr = pAudioBeamFrameArrivedEventArgs->get_FrameReference(&pAudioBeamFrameReference);
		}

		if (SUCCEEDED(hr))
		{
			hr = pAudioBeamFrameReference->AcquireBeamFrames(&pAudioBeamFrameList);
		}

		SafeRelease(pAudioBeamFrameReference);
		SafeRelease(pAudioBeamFrameArrivedEventArgs);
	}
	else
	{
		hr = m_pAudioBeamFrameReader->AcquireLatestBeamFrames(&pAudioBeamFrameList);
	}

	if (SUCCEEDED(hr))
	{
		// Only one audio beam is currently supported
		hr = pAudioBeamFrameList->OpenAudioBeamFrame(0, &pAudioBeamFrame);
	}

	if (SUCCEEDED(hr))
	{
		hr = pAudioBeamFrame->get_SubFrameCount(&subFrameCount);
	}

	if (SUCCEEDED(hr) && subFrameCount > 0)
	{
		for (UINT32 i = 0; i < subFrameCount; i++)
		{
			// Process all subframes
			IAudioBeamSubFrame* pAudioBeamSubFrame = NULL;
			hr = pAudioBeamFrame->GetSubFrame(i, &pAudioBeamSubFrame);

			if (SUCCEEDED(hr))
			{
				hr = Process(pAudioBeamSubFrame);
			}

			SafeRelease(pAudioBeamSubFrame);
		}
	}

	SafeRelease(pAudioBeamFrame);
	SafeRelease(pAudioBeamFrameList);
	return hr;
}

HRESULT CAudioReader::StreamProcess()
{
	static float fAccumulatedSquareSum = 0.0f;
	static int   nAccumulatedSampleCount = 0;
	static int   nEnergyIndex = 0;
	static float fEnergyBuffer[cEnergyBufferLength];

	DWORD cbRead = 0;
	float audioBuffer[cAudioBufferLength];

	// S_OK will be returned when cbRead == sizeof(audioBuffer).
	// E_PENDING will be returned when cbRead < sizeof(audioBuffer).
	// For both return codes we will continue to process the audio written into the buffer.
	HRESULT hr = m_pAudioStream->Read((void *)audioBuffer, sizeof(audioBuffer), &cbRead);

	if (FAILED(hr) && hr != E_PENDING)
	{
		//SetStatusMessage(L"Failed to read from audio stream.");
	}
	else if (cbRead > 0)
	{
		DWORD nSampleCount = cbRead / sizeof(float);
		float fBeamAngle = 0.f;
		float fBeamAngleConfidence = 0.f;

		// Get most recent audio beam angle and confidence
		m_pAudioBeam->get_BeamAngle(&fBeamAngle);
		m_pAudioBeam->get_BeamAngleConfidence(&fBeamAngleConfidence);

		m_sKinectData.m_AudioData.fBeamAngle = fBeamAngle;
		m_sKinectData.m_AudioData.fBeamAngleConfidence = fBeamAngleConfidence;

		// Calculate energy from audio
		for (UINT i = 0; i < nSampleCount; i++)
		{
			// Compute the sum of squares of audio samples that will get accumulated
			// into a single energy value.
			fAccumulatedSquareSum += audioBuffer[i] * audioBuffer[i];
			++nAccumulatedSampleCount;

			if (nAccumulatedSampleCount < cAudioSamplesPerEnergySample)
			{
				continue;
			}

			// Each energy value will represent the logarithm of the mean of the
			// sum of squares of a group of audio samples.
			float fMeanSquare = fAccumulatedSquareSum / cAudioSamplesPerEnergySample;

			if (fMeanSquare > 1.0f)
			{
				// A loud audio source right next to the sensor may result in mean square values
				// greater than 1.0. Cap it at 1.0f for display purposes.
				fMeanSquare = 1.0f;
			}

			float fEnergy = cMinEnergy;
			if (fMeanSquare > 0.f)
			{
				fEnergy = 10.0f*log10(fMeanSquare); // Convert to dB
			}

			{
				// Renormalize signal above noise floor to [0,1] range for visualization.
				fEnergyBuffer[nEnergyIndex] = (cMinEnergy - fEnergy) / cMinEnergy;
				nEnergyIndex = (nEnergyIndex + 1) % cEnergyBufferLength;

				if (nEnergyIndex == 0)
				{
					m_sKinectData.m_AudioData.bHaveNewEnergy = true;
					memcpy(m_sKinectData.m_AudioData.fEnergyBuffer, fEnergyBuffer, cEnergyBufferLength);
				}
			}

			fAccumulatedSquareSum = 0.f;
			nAccumulatedSampleCount = 0;
		}
	}

	return hr;
}

HRESULT CAudioReader::Process(IAudioBeamSubFrame* pAudioBeamSubFrame)
{
	static float fAccumulatedSquareSum = 0.0f;
	static int   nAccumulatedSampleCount = 0;
	static int   nEnergyIndex = 0;
	static float fEnergyBuffer[cEnergyBufferLength];

	INT64 nTime = 0;
	UINT cbRead = 0;
	float* pAudioBuffer = NULL;

	HRESULT hr = E_FAIL;
	if (!pAudioBeamSubFrame) { return hr; }
	hr = pAudioBeamSubFrame->get_RelativeTime(&nTime);

	if (SUCCEEDED(hr))
	{
		hr = pAudioBeamSubFrame->AccessUnderlyingBuffer(&cbRead, (BYTE **)&pAudioBuffer);
	}

	if (FAILED(hr))
	{
		//Failed to read buffer from audio subframe.
	}
	else if (cbRead > 0)
	{
		DWORD nSampleCount = cbRead / sizeof(float);
		float fBeamAngle = 0.f;
		float fBeamAngleConfidence = 0.0f;

		// Get audio beam angle and confidence
		pAudioBeamSubFrame->get_BeamAngle(&fBeamAngle);
		pAudioBeamSubFrame->get_BeamAngleConfidence(&fBeamAngleConfidence);

		// save
		m_sKinectData.m_AudioData.fBeamAngle = fBeamAngle;
		m_sKinectData.m_AudioData.fBeamAngleConfidence = fBeamAngleConfidence;

		// Calculate energy from audio
		for (UINT i = 0; i < nSampleCount; i++)
		{
			// Compute the sum of squares of audio samples that will get accumulated
			// into a single energy value.
			__pragma(warning(push))
				__pragma(warning(disable:6385)) // Suppress warning about the range of i. The range is correct.
				fAccumulatedSquareSum += pAudioBuffer[i] * pAudioBuffer[i];
			__pragma(warning(pop))
				++nAccumulatedSampleCount;

			if (nAccumulatedSampleCount < cAudioSamplesPerEnergySample)
			{
				continue;
			}

			// Each energy value will represent the logarithm of the mean of the
			// sum of squares of a group of audio samples.
			float fMeanSquare = fAccumulatedSquareSum / cAudioSamplesPerEnergySample;

			if (fMeanSquare > 1.0f)
			{
				// A loud audio source right next to the sensor may result in mean square values
				// greater than 1.0. Cap it at 1.0f for display purposes.
				fMeanSquare = 1.0f;
			}

			float fEnergy = cMinEnergy;
			if (fMeanSquare > 0.f)
			{
				fEnergy = 10.0f * log10(fMeanSquare); // Convert to dB
			}

			{ 
				// Renormalize signal above noise floor to [0, 1] range for visualization.
				fEnergyBuffer[nEnergyIndex] = (cMinEnergy - fEnergy) / cMinEnergy;
				nEnergyIndex = (nEnergyIndex + 1) % cEnergyBufferLength;

				if (nEnergyIndex == 0)
				{
					m_sKinectData.m_AudioData.bHaveNewEnergy = true;
					memcpy(m_sKinectData.m_AudioData.fEnergyBuffer, fEnergyBuffer, cEnergyBufferLength);
				}
			}

			fAccumulatedSquareSum = 0.f;
			nAccumulatedSampleCount = 0;
		}
	}

	return hr;
}

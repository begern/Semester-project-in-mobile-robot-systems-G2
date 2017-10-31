#include "WaveBuffer.h"


const double WaveBuffer::piProducts[16][2] = {
	{ piProduct(findLoDTMFFreq(0)), piProduct(findHiDTMFFreq(0)) },
	{ piProduct(findLoDTMFFreq(1)), piProduct(findHiDTMFFreq(1)) },
	{ piProduct(findLoDTMFFreq(2)), piProduct(findHiDTMFFreq(2)) },
	{ piProduct(findLoDTMFFreq(3)), piProduct(findHiDTMFFreq(3)) },
	{ piProduct(findLoDTMFFreq(4)), piProduct(findHiDTMFFreq(4)) },
	{ piProduct(findLoDTMFFreq(5)), piProduct(findHiDTMFFreq(5)) },
	{ piProduct(findLoDTMFFreq(6)), piProduct(findHiDTMFFreq(6)) },
	{ piProduct(findLoDTMFFreq(7)), piProduct(findHiDTMFFreq(7)) },
	{ piProduct(findLoDTMFFreq(8)), piProduct(findHiDTMFFreq(8)) },
	{ piProduct(findLoDTMFFreq(9)), piProduct(findHiDTMFFreq(9)) },
	{ piProduct(findLoDTMFFreq(10)), piProduct(findHiDTMFFreq(10)) },
	{ piProduct(findLoDTMFFreq(11)), piProduct(findHiDTMFFreq(11)) },
	{ piProduct(findLoDTMFFreq(12)), piProduct(findHiDTMFFreq(12)) },
	{ piProduct(findLoDTMFFreq(13)), piProduct(findHiDTMFFreq(13)) },
	{ piProduct(findLoDTMFFreq(14)), piProduct(findHiDTMFFreq(14)) },
	{ piProduct(findLoDTMFFreq(15)), piProduct(findHiDTMFFreq(15)) }
};

int WaveBuffer::sampleRate = 44100;

WaveBuffer::WaveBuffer()
{
	iPointer = 0;
	oPointer = 0;
	sPointer = 0;
	waveNumOfSamples = 4410;
	bufferSize = 200;
	phase = 0;
	buffer = std::vector<int>(bufferSize, 0);
}


WaveBuffer::~WaveBuffer()
{

}

int WaveBuffer::getBufferLength()
{
	return bufferSize;
}

bool WaveBuffer::put(int b)
{
	if ((iPointer + 1) % bufferSize == oPointer % bufferSize)
		return false;
	buffer[iPointer++] = b;
	if (iPointer == bufferSize)
		iPointer = 0;
	return true;
}

double WaveBuffer::getNext()
{
	if (oPointer != iPointer) {
		double r = 0.2 * sin( sPointer * piProducts[ buffer[oPointer] ][0] + phase) + 0.2 * sin( sPointer * piProducts[ buffer[oPointer] ][1] + phase);

		if (++sPointer == waveNumOfSamples) {
			sPointer = 0;
			if (++oPointer == bufferSize)
				oPointer = 0;
		}
		return r;
	}
	return 0;
}

int WaveBuffer::findLoDTMFFreq(int d) {
	int f;
	switch (d / 4)
	{
	case 0: f = 697; break; case 1: f = 770; break;
	case 2: f = 852; break; case 3: f = 941; break; default: f = 0;
	}
	return f;
}

int WaveBuffer::findHiDTMFFreq(int d) {
	int f;
	switch (d % 4)
	{
	case 0: f = 1209; break; case 1: f = 1336; break;
	case 2: f = 1477; break; case 3: f = 1633; break; default: f = 0;
	}
	return f;
}

double WaveBuffer::piProduct(int freq)
{
	return (double)((2.0 * M_PI * freq) / sampleRate);
}

bool WaveBuffer::open(PaDeviceIndex index)
{
	PaStreamParameters outputParameters;

	outputParameters.device = index;
	if (outputParameters.device == paNoDevice) {
		return false;
	}

	const PaDeviceInfo* pInfo = Pa_GetDeviceInfo(index);
	if (pInfo != 0)
	{
		printf("Output device name: '%s'\r", pInfo->name);
	}

	outputParameters.channelCount = 1;       /* mono output */
	outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	PaError err = Pa_OpenStream(
		&stream,
		NULL, /* no input */
		&outputParameters,
		44100,
		paFramesPerBufferUnspecified,
		paClipOff,      /* we won't output out of range samples so don't bother clipping them */
		&WaveBuffer::paCallback,
		this            /* Using 'this' for userData so we can cast to Sine* in paCallback method */
	);

	if (err != paNoError)
	{
		/* Failed to open stream to device !!! */
		return false;
	}

	err = Pa_SetStreamFinishedCallback(stream, &WaveBuffer::paStreamFinished);

	if (err != paNoError)
	{
		Pa_CloseStream(stream);
		stream = 0;

		return false;
	}

	return true;
}

bool WaveBuffer::close()
{
	if (stream == 0)
		return false;

	PaError err = Pa_CloseStream(stream);
	stream = 0;

	return (err == paNoError);
}


bool WaveBuffer::start()
{
	if (stream == 0)
		return false;

	PaError err = Pa_StartStream(stream);

	return (err == paNoError);
}

bool WaveBuffer::stop()
{
	if (stream == 0)
		return false;

	PaError err = Pa_StopStream(stream);

	return (err == paNoError);
}
int WaveBuffer::paCallbackMethod(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags)
{
	float *out = (float*)outputBuffer;
	unsigned long i;

	(void)timeInfo; /* Prevent unused variable warnings. */
	(void)statusFlags;
	(void)inputBuffer;

	for (i = 0; i<framesPerBuffer; i++)
	{
		*out++ = getNext();
	}

	return paContinue;

}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
int WaveBuffer::paCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	/* Here we cast userData to Sine* type so we can call the instance method paCallbackMethod, we can do that since
	we called Pa_OpenStream with 'this' for userData */
	return ((WaveBuffer*)userData)->paCallbackMethod(inputBuffer, outputBuffer,
		framesPerBuffer,
		timeInfo,
		statusFlags);
}


void WaveBuffer::paStreamFinishedMethod()
{
	printf("Stream Completed: %s\n", message);
}

/*
* This routine is called by portaudio when playback is done.
*/
void WaveBuffer::paStreamFinished(void* userData)
{
	return ((WaveBuffer*)userData)->paStreamFinishedMethod();
}


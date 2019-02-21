#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "nx-ecnr.h"

#define MAX_NUMBER_INTERFACES 2
#define MAX_NUMBER_INPUT_DEVICES 3
#define POSITION_UPDATE_PERIOD 1000 /* 1 sec */
#define AUDIO_IN_MAX_LEN 128

/* if you want to save audio rawdata, uncomment below line
   do following step
   - su
   - mkdir /data/tmp
   - chmod 777 /data/tmp
 */
//#define FILE_DUMP

typedef struct
{
	uint32_t sampleRate;
	uint32_t format;
	uint32_t flags;
	uint32_t bytesPerPacket;
	uint32_t framesPerPacket;
	uint32_t bytesPerFrame;
	uint32_t channelsPerFrame;
	uint32_t bitsPerChannel;
	uint32_t reserved;
} Audioformat;

typedef struct
{
	uint32_t inputSampleTime;
    	void *inCtx;
	uint8_t *audioInBuffer;
	size_t audioInMaxLen;
	
	Audioformat format;
	SLRecordItf recordItf;
	SLAndroidSimpleBufferQueueItf recBuffQueueItf;
} AudioRef;

FILE *file;
AudioRef ref;

/* Checks for error. If any errors exit the application! */
void CheckErr( SLresult res )
{
	if ( res != SL_RESULT_SUCCESS )
	{
		// Debug printing to be placed here
		exit(1);
	}
}

void RecordEventCallback(SLRecordItf caller __unused, void *pContext,
							SLuint32 recordevent __unused)
{
	AudioRef *ref = (AudioRef *) pContext;
	SLresult res;

	/* Callback code goes here */
	printf("Recording...\n");
}

/*==============================================================================
 *  AndroidAudioRecordCallbackFunction
 *=============================================================================*/

void AndroidAudioRecordCallbackFunction(SLAndroidSimpleBufferQueueItf queueItf, void *pContext)
{
	(void)queueItf;
	AudioRef *ref = (AudioRef *) pContext;
	SLresult res;
	int i;

	res=(*(ref->recBuffQueueItf))->Enqueue(ref->recBuffQueueItf,
						ref->audioInBuffer,ref->audioInMaxLen);
	CheckErr(res);

#ifdef FILE_DUMP
	if (file)
		fwrite(ref->audioInBuffer, 1, ref->audioInMaxLen, file);
#endif

exit:
    return;
}

/*
 * Test recording of audio from a microphone into a specified file
 */
void TestAudioRecording(SLObjectItf sl, Audioformat format)
{
	SLObjectItf recorder;
	SLEngineItf EngineItf;
	SLAudioIODeviceCapabilitiesItf AudioIODeviceCapabilitiesItf;
	SLAudioInputDescriptor AudioInputDescriptor;
	SLresult res;
	SLDataSource audioSource;
	SLDataLocator_IODevice locator_mic;
	SLDeviceVolumeItf devicevolumeItf;
	SLDataSink audioSink;
	SLDataLocator_AndroidSimpleBufferQueue recBuffQueue;
	SLDataFormat_PCM recPcm;
	SLDataSink recSink;
	SLboolean required[MAX_NUMBER_INTERFACES];
	SLInterfaceID iidArray[MAX_NUMBER_INTERFACES];
	SLuint32 InputDeviceIDs[MAX_NUMBER_INPUT_DEVICES];
	SLint32 numInputs = 0;
	SLboolean mic_available = SL_BOOLEAN_FALSE;
	SLuint32 mic_deviceID = 0;
	int i;

	ref.audioInMaxLen=AUDIO_IN_MAX_LEN;
	ref.audioInBuffer = (uint8_t *) malloc( ref.audioInMaxLen );
	memset(ref.audioInBuffer, 0, sizeof(ref.audioInMaxLen));
	ref.format = format;

	/* Get the SL Engine Interface which is implicit */
	res = (*sl)->GetInterface(sl, SL_IID_ENGINE, (void*)&EngineItf);
	CheckErr(res);
	mic_deviceID = SL_DEFAULTDEVICEID_AUDIOINPUT;
	mic_available = SL_BOOLEAN_TRUE;

	/* If neither of the preferred input audio devices is available,
		no point in continuing */
	if (!mic_available) {
		/* Appropriate error message here */
		exit(1);
	}

	/* Initialize arrays required[] and iidArray[] */
	for (i=0;i<MAX_NUMBER_INTERFACES;i++)
	{
		required[i] = SL_BOOLEAN_FALSE;
		iidArray[i] = SL_IID_NULL;
	}

	required[0] = SL_BOOLEAN_TRUE;
	iidArray[0] = SL_IID_ANDROIDSIMPLEBUFFERQUEUE;
	iidArray[1] = SL_IID_ANDROIDCONFIGURATION;

	/* Setup the data source structure */
	locator_mic.locatorType = SL_DATALOCATOR_IODEVICE;
	locator_mic.deviceType = SL_IODEVICE_AUDIOINPUT;
	locator_mic.deviceID = mic_deviceID;
	locator_mic.device= NULL; 

	audioSource.pLocator = (void *)&locator_mic;
	audioSource.pFormat = NULL;

	/* Setup the data sink structure */
	recBuffQueue.locatorType=SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	recBuffQueue.numBuffers=4;

	recPcm.formatType=SL_DATAFORMAT_PCM;

	recPcm.numChannels=1;
	recPcm.samplesPerSec=(SLuint32) format.sampleRate*1000;
	recPcm.bitsPerSample=SL_PCMSAMPLEFORMAT_FIXED_16;
	recPcm.containerSize=16;
	recPcm.channelMask=SL_SPEAKER_FRONT_LEFT;
	recPcm.endianness=SL_BYTEORDER_LITTLEENDIAN;

	audioSink.pLocator = (void *)&recBuffQueue;
	audioSink.pFormat = (void *)&recPcm;

	/* Create audio recorder */
	res = (*EngineItf)->CreateAudioRecorder(EngineItf, &recorder,
						&audioSource, &audioSink, 2,
						iidArray, required);
	CheckErr(res);
	/* Realizing the recorder in synchronous mode. */
	res = (*recorder)->Realize(recorder, SL_BOOLEAN_FALSE);
	CheckErr(res);
	/* Get the RECORD interface - it is an implicit interface */
	res = (*recorder)->GetInterface(recorder, SL_IID_RECORD, (void*)&ref.recordItf);
	CheckErr(res);
	res= (*recorder)->GetInterface(recorder, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
					(void*)&ref.recBuffQueueItf);
	CheckErr(res);
	/* Setup to receive position event callbacks */
	res = (*ref.recordItf)->RegisterCallback(ref.recordItf, RecordEventCallback, NULL);
	CheckErr(res);
	res = (*ref.recBuffQueueItf)->RegisterCallback(ref.recBuffQueueItf,
					 AndroidAudioRecordCallbackFunction, &ref);
	/* Set notifications to occur after every second - may be useful in
		updating a recording progress bar */
	res = (*ref.recordItf)->SetPositionUpdatePeriod(ref.recordItf,
							POSITION_UPDATE_PERIOD);
	CheckErr(res);
	res = (*ref.recordItf)->SetCallbackEventsMask(ref.recordItf,
							SL_RECORDEVENT_HEADATNEWPOS);
	CheckErr(res);
	/* Set the duration of the recording - 30 seconds (30,000 milliseconds) */
	res = (*ref.recordItf)->SetDurationLimit(ref.recordItf, 10000);
	CheckErr(res);
	/* Record the audio */
	res = (*ref.recordItf)->SetRecordState(ref.recordItf,SL_RECORDSTATE_RECORDING);

	AndroidAudioRecordCallbackFunction(ref.recBuffQueueItf,&ref);
	/* Destroy the recorder object */
	//(*recorder)->Destroy(recorder);
}

static void printUsage(char **argv)
{
	fprintf(stderr, "Usage: %s [-r samplerate] [-f filename]\n", argv[0]);
	fprintf(stderr, "default: %s [-r 48000] [-f /data/tmp/recdata.raw]\n", argv[0]);
}

static const char *defaultFileName = "/data/tmp/recdata.raw";
int main(int argc __unused, char *argv[])
{
	char input = 0;
	const char *fileName = defaultFileName;
	Audioformat format;
	SLresult res;
	SLObjectItf sl;
	/* Create OpenSL ES engine in thread-safe mode */
	SLEngineOption EngineOption[] = {{(SLuint32)SL_ENGINEOPTION_THREADSAFE,
					 (SLuint32) SL_BOOLEAN_TRUE}};
	res = slCreateEngine( &sl, 1, EngineOption, 0, NULL, NULL);
	CheckErr(res);
	/* Realizing the SL Engine in synchronous mode. */
	res = (*sl)->Realize(sl, SL_BOOLEAN_FALSE);
	CheckErr(res);

	format.sampleRate = 48000;

	/* parse command line */
	char **myArgv = argv;
	myArgv++;
	while (*myArgv) {
		if (strcmp(*myArgv, "-r") == 0) {
			myArgv++;
			if (*myArgv) {
				format.sampleRate = atoi(*myArgv);
			}
		} else if (strcmp(*myArgv, "-f") == 0) {
			myArgv++;
			if (*myArgv) {
				fileName = *myArgv;
			} else {
				fprintf(stderr, "No filename given, using %s\n", defaultFileName);
				fileName = defaultFileName;
			}
		} else if (strcmp(*myArgv, "-h") == 0) {
			printUsage(argv);
			exit(0);
		}

		if (*myArgv)
			myArgv++;
	}

#ifdef FILE_DUMP
	if (fileName) {
		file = fopen(fileName, "wb");
		if (!file) {
			 printf("%s: failed to create\n", __func__);
		}
	}
#endif
	// Init ecnr feature
	ecnr_init();
        printf("Init ecnr   : status %d\n", ecnr_is_enabled()); 

	// Enable ecnr feature
        set_ecnr(1);
        printf("Enable ecnr : status %d\n", ecnr_is_enabled()); 

	TestAudioRecording(sl, format);

	printf("If you want to stop app, Enter 'q'\n");
	while (input != 'q') {
		usleep(500000);
                input = getchar();
	};

#ifdef FILE_DUMP
	fclose(file);
#endif
	res = (*ref.recordItf)->SetRecordState(ref.recordItf,
							SL_RECORDSTATE_STOPPED);
	CheckErr(res);

	// Disable ecnr feature
        set_ecnr(0);
        printf("Disable ecnr : status %d\n", ecnr_is_enabled()); 

	/* Shutdown OpenSL ES */
	(*sl)->Destroy(sl);
	exit(0);
}

/**
 * This is a basic example to show how to use the virtual device.
 * This sample will create a new thread to generate dummy frame, and read by OpenNI API.
 *
 * http://viml.nchc.org.tw/home/
 */

// STL Header
#include <iostream>

// OpenNI Header
#include <OpenNI.h>

// XnLib in OpenNI Source Code, use for threading
#include "XnLib.h"

// Virtual Device Header
#include "..\..\VirtualDevice\VirtualDevice.h"

// namespace
using namespace std;
using namespace openni;

// global object
bool bRunning = true;

// The function to generate dummy frame
XN_THREAD_PROC GenerateDummyFrame( XN_THREAD_PARAM pThreadParam )
{
	VideoStream* pVStream = (VideoStream*)pThreadParam;

	while( bRunning )
	{
		if( pVStream->isValid() )
		{
			// get a frame form virtual video stream
			OniFrame* pFrame = NULL;
			if( pVStream->invoke( GET_VIRTUAL_STREAM_IMAGE, pFrame ) == openni::STATUS_OK )
			{
				// type casting
				DepthPixel* pVirData = reinterpret_cast<DepthPixel*>( pFrame->data );

				// Fill dummy data
				for( int y = 0; y < pFrame->height; ++ y )
				{
					for( int x = 0; x < pFrame->width; ++ x )
					{
						int idx = x + y * pFrame->width;
						pVirData[idx] = 100;
					}
				}

				// write data to form virtual video stream
				pVStream->invoke( SET_VIRTUAL_STREAM_IMAGE, pFrame );

			}
		}

		// sleep
		xnOSSleep(33);
	}

	XN_THREAD_PROC_RETURN(XN_STATUS_OK);
}

int main( int, char** )
{
	#pragma region OpenNI initialize
	// Initial OpenNI
	if( OpenNI::initialize() != STATUS_OK )
	{
		cerr << "OpenNI Initial Error: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// Open Virtual Device
	Device	devVirDevice;
	if( devVirDevice.open( "\\OpenNI2\\VirtualDevice\\TEST" ) != STATUS_OK )
	{
		cerr << "Can't create virtual device: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// create virtual color video stream
	VideoStream vsVirDepth;
	if( vsVirDepth.create( devVirDevice, SENSOR_DEPTH ) == STATUS_OK )
	{
		VideoMode mMode;
		mMode.setFps( 30 );
		mMode.setResolution( 320, 240 );
		mMode.setPixelFormat( PIXEL_FORMAT_DEPTH_1_MM );
		vsVirDepth.setVideoMode( mMode );
	}
	else
	{
		cerr << "Can't create depth stream on device: " << OpenNI::getExtendedError() << endl;
		return -1;
	}
	#pragma endregion

	#pragma region main loop
	// start data generate
	vsVirDepth.start();

	// create a new thread to generate dummy data
	XN_THREAD_HANDLE mThreadHandle;
	xnOSCreateThread( GenerateDummyFrame, &vsVirDepth, &mThreadHandle );

	// use for-loop to read 100 frames
	for( int i = 0; i < 100; ++ i )
	{
		VideoFrameRef mFrame;
		if( vsVirDepth.readFrame( &mFrame ) == STATUS_OK )
		{
			const DepthPixel* pData = reinterpret_cast<const DepthPixel*>( mFrame.getData() );
			cout << pData[ mFrame.getWidth() / 2 + ( mFrame.getHeight() / 2 ) * mFrame.getWidth() ] << endl;
		}
	}

	// stop data generate
	bRunning = false;
	vsVirDepth.stop();

	// close device
	vsVirDepth.destroy();
	devVirDevice.close();

	// shutdown
	OpenNI::shutdown();

	#pragma endregion

	return 0;
}

/***
 * This is a simple example to show how to use the virtual device.
 * This sample will open an existed real device, read the depth frame with listener,
 * then copy to the virtual device.
 */

// STL Header
#include <iostream>

// OpenNI Header
#include <OpenNI.h>

// namespace
using namespace std;
using namespace openni;

// definition of customized property
#define GET_VIRTUAL_STREAM_IMAGE	100000
#define SET_VIRTUAL_STREAM_IMAGE	100001

// the NewFrameListener
class CFrameModifer : public VideoStream::NewFrameListener
{
public:
	CFrameModifer( VideoStream& rStream ) : m_rVStream( rStream )
	{
	}

	void onNewFrame( openni::VideoStream& rStream )
	{
		openni::VideoFrameRef mFrame;
		// read frame from real video stream
		if( rStream.readFrame( &mFrame ) == openni::STATUS_OK )
		{
			// get a frame form virtual video stream
			OniFrame* pFrame = NULL;
			if( m_rVStream.invoke( GET_VIRTUAL_STREAM_IMAGE, pFrame ) == openni::STATUS_OK )
			{
				// type casting
				const DepthPixel* pRealData = reinterpret_cast<const DepthPixel*>( mFrame.getData() );
				DepthPixel* pVirData = reinterpret_cast<DepthPixel*>( pFrame->data );

				// read data from the frame of real sensor, and write to the frame of virtual sensor
				for( int y = 0; y < mFrame.getHeight(); ++ y )
				{
					for( int x = 0; x < mFrame.getWidth(); ++ x )
					{
						int idx = x + y * mFrame.getWidth();
						pVirData[idx] = pRealData[idx];
					}
				}

				// write data to form virtual video stream
				m_rVStream.invoke( SET_VIRTUAL_STREAM_IMAGE, pFrame );
			}
		}
	}

protected:
	openni::VideoStream&	m_rVStream;
};

int main( int, char** )
{
	#pragma region OpenNI initialize
	// Initial OpenNI
	if( OpenNI::initialize() != STATUS_OK )
	{
		cerr << "OpenNI Initial Error: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// Open a real device
	Device devRealSensor;
	if( devRealSensor.open( ANY_DEVICE ) != STATUS_OK )
	{
		cerr << "Can't Open Device: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// Create depth stream
	VideoStream vsRealDepth;
	if( devRealSensor.hasSensor( SENSOR_DEPTH ) )
	{
		if( vsRealDepth.create( devRealSensor, SENSOR_DEPTH ) != STATUS_OK )
		{
			cerr << "Can't create depth stream on device: " << OpenNI::getExtendedError() << endl;
			return -1;
		}
	}
	else
	{
		cerr << "ERROR: This device does not have depth sensor" << endl;
		return -1;
	}
	#pragma endregion

	#pragma region Virtual Device
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
		vsVirDepth.setVideoMode( vsRealDepth.getVideoMode() );

		// add new frame listener to real video stream
		vsRealDepth.addNewFrameListener( new CFrameModifer(vsVirDepth) );
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
	vsRealDepth.start();

	// use for-loop to read 100 frames
	for( int i = 0; i < 100; ++ i )
	{
		VideoFrameRef mFrame;
		if( vsRealDepth.readFrame( &mFrame ) == STATUS_OK )
		{
			const DepthPixel* pData = reinterpret_cast<const DepthPixel*>( mFrame.getData() );
			cout << pData[ mFrame.getWidth() / 2 + ( mFrame.getHeight() / 2 ) * mFrame.getWidth() ] << endl;
		}
	}

	// stop data generate
	vsVirDepth.stop();
	vsRealDepth.stop();

	// close device
	vsVirDepth.destroy();
	vsRealDepth.destroy();
	devVirDevice.close();
	devRealSensor.close();

	// shutdown
	OpenNI::shutdown();

	#pragma endregion

	return 0;
}

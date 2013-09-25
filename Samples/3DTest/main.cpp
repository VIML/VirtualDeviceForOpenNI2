/**
 * This is a simple example to show how to use the virtual device.
 * This sample will open an existed real device, read the depth frame with listener,
 * then copy to the virtual device.
 *
 * http://viml.nchc.org.tw/home/
 */

// STL Header
#include <iostream>

// glut Header
#include <GL/glut.h>

// OpenNI Header
#include <OpenNI.h>

#include "OpenGLCamera.h"

// namespace
using namespace std;
using namespace openni;

#pragma region global objects

// global OpenNI object
struct SDevice
{
	Device		devDevice;
	VideoStream	vsDepth;
	VideoStream	vsColor;
} g_Device[2];

// global object
SimpleCamera	g_Camera;

#pragma endregion

#pragma region used for Virtual Device

// Virtual Device Header
#include "..\..\VirtualDevice\VirtualDevice.h"

// The NewFrameListener to set the new frame of virtual device
template<typename PIXEL_TYPE>
class CCopyFrame : public VideoStream::NewFrameListener
{
public:
	CCopyFrame( VideoStream& rStream ) : m_rVStream( rStream ){}

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
				const PIXEL_TYPE* pRealData = reinterpret_cast<const PIXEL_TYPE*>( mFrame.getData() );
				PIXEL_TYPE* pVirData = reinterpret_cast<PIXEL_TYPE*>( pFrame->data );

				// read data from the frame of real sensor, and write to the frame of virtual sensor
				int w = mFrame.getWidth(), h = mFrame.getHeight();
				for( int y = 0; y < h; ++ y )
				{
					for( int x = 0; x < w; ++ x )
					{
						int idx = x + y * w;
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

	CCopyFrame& operator=(const CCopyFrame&);
};

#pragma endregion

#pragma region GLUT callback functions

// glut display function(draw)
void display()
{
	// clear previous screen
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	SDevice& virDevice = g_Device[1];

	// get depth and color frame
	VideoFrameRef	vfDepthFrame;
	VideoFrameRef	vfColorFrame;
	if( virDevice.vsDepth.readFrame( &vfDepthFrame ) == STATUS_OK && 
		virDevice.vsColor.readFrame( &vfColorFrame ) == STATUS_OK )
	{
		// type cast
		const DepthPixel* pDepthArray = reinterpret_cast<const DepthPixel*>( vfDepthFrame.getData() );
		const RGB888Pixel* pColorArray = reinterpret_cast<const RGB888Pixel*>( vfColorFrame.getData() );

		int iW = vfDepthFrame.getWidth(),
			iH = vfDepthFrame.getHeight();

		// draw point cloud
		glBegin( GL_POINTS );
		float vPos[3];
		for( int y = 0; y < iH; ++ y )
		{
			for( int x = 0; x < iW; ++ x )
			{
				int idx = x + y * iW;

				// update points array
				const DepthPixel& rDepth = pDepthArray[idx];
				if( rDepth > 0 )
				{
					// read color
					const RGB888Pixel& rColor = pColorArray[idx];

					// convert coordinate
					CoordinateConverter::convertDepthToWorld( virDevice.vsDepth, x, y, rDepth, &vPos[0], &vPos[1], &vPos[2] );

					// draw point
					glColor3ub( rColor.r,  rColor.g, rColor.b );
					glVertex3fv( vPos );
				}
			}
		}
		glEnd();
	}

	// swap buffer
	glutSwapBuffers();
}

// glut idle function
void idle()
{
	glutPostRedisplay();
}

// glut keyboard function
void keyboard( unsigned char key, int, int )
{
	float fSpeed = 50.0f;
	switch( key )
	{
	case 'q':
		// stop OpenNI
		g_Device[0].vsDepth.destroy();
		g_Device[0].vsColor.destroy();
		g_Device[1].vsDepth.destroy();
		g_Device[1].vsColor.destroy();

		g_Device[0].devDevice.close();
		g_Device[1].devDevice.close();

		OpenNI::shutdown();
		exit( 0 );

	case 's':
		g_Camera.MoveForward( -fSpeed );
		break;

	case 'w':
		g_Camera.MoveForward( fSpeed );
		break;

	case 'a':
		g_Camera.MoveSide( -fSpeed );
		break;

	case 'd':
		g_Camera.MoveSide( fSpeed );
		break;

	case 'z':
		g_Camera.MoveUp( -fSpeed );
		break;

	case 'x':
		g_Camera.MoveUp( fSpeed );
		break;

	case 'p':
		{
			static bool bPoly = true;
			if( bPoly )
			{
				bPoly = false;
				glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
			}
			else
			{
				bPoly = true;
				glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			}
		}
		break;
	}
}

// glut special keyboard function
void specialKey( int key, int, int )
{
	float fRotateScale = 0.01f;
	switch( key )
	{
	case GLUT_KEY_DOWN:
		g_Camera.RotateUp( -fRotateScale );
		break;

	case GLUT_KEY_UP:
		g_Camera.RotateUp( fRotateScale );
		break;

	case GLUT_KEY_RIGHT:
		g_Camera.RotateSide( fRotateScale );
		break;

	case GLUT_KEY_LEFT:
		g_Camera.RotateSide( -fRotateScale );
		break;
	}
}

#pragma endregion

int main( int argc, char** argv )
{
	#pragma region OpenNI initialize
	// Initial OpenNI
	if( OpenNI::initialize() != STATUS_OK )
	{
		cerr << "OpenNI Initial Error: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// Open Device
	SDevice& phyDevice = g_Device[0];
	if( phyDevice.devDevice.open( ANY_DEVICE ) != STATUS_OK )
	{
		cerr << "Can't Open Device: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// Create depth stream
	if( phyDevice.devDevice.hasSensor( SENSOR_DEPTH ) )
	{
		if( phyDevice.vsDepth.create( phyDevice.devDevice, SENSOR_DEPTH ) != STATUS_OK )
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

	// Create color stream
	if( phyDevice.devDevice.hasSensor( SENSOR_COLOR ) )
	{
		if( phyDevice.vsColor.create( phyDevice.devDevice, SENSOR_COLOR ) == STATUS_OK )
		{
			// image registration
			if( phyDevice.devDevice.isImageRegistrationModeSupported( IMAGE_REGISTRATION_DEPTH_TO_COLOR ) )
			{
				phyDevice.devDevice.setImageRegistrationMode( IMAGE_REGISTRATION_DEPTH_TO_COLOR );
			}
			else
			{
				cout << "This device doesn't support IMAGE_REGISTRATION_DEPTH_TO_COLOR, will use CoordinateConverter." << endl;
			}
		}
		else
		{
			cerr <<  "Can't create color stream on device: " << OpenNI::getExtendedError() << endl;
		}
	}
	else
	{
		cerr << "This device does not have depth sensor" << endl;
		return -1;
	}
	#pragma endregion

	#pragma region Virtual Device
	// Open Virtual Device
	SDevice& virDevice = g_Device[1];
	if( virDevice.devDevice.open( "\\OpenNI2\\VirtualDevice\\TEST" ) != STATUS_OK )
	{
		cerr << "Can't create virtual device: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// create virtual depth video stream
	if( virDevice.vsDepth.create( virDevice.devDevice, SENSOR_DEPTH ) == STATUS_OK )
	{
		// set the FOV for depth, which is required for
		virDevice.vsDepth.setProperty( ONI_STREAM_PROPERTY_VERTICAL_FOV,		phyDevice.vsDepth.getVerticalFieldOfView() );
		virDevice.vsDepth.setProperty( ONI_STREAM_PROPERTY_HORIZONTAL_FOV,	phyDevice.vsDepth.getHorizontalFieldOfView() );

		virDevice.vsDepth.setVideoMode( phyDevice.vsDepth.getVideoMode() );

		phyDevice.vsDepth.addNewFrameListener( new CCopyFrame<DepthPixel>( virDevice.vsDepth ) );
	}
	else
	{
		cerr << "Virtual Depth Stream Create error: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// create virtual color video stream
	if( virDevice.vsColor.create( virDevice.devDevice, SENSOR_COLOR ) == STATUS_OK )
	{
		virDevice.vsColor.setVideoMode( phyDevice.vsColor.getVideoMode() );
		phyDevice.vsColor.addNewFrameListener( new CCopyFrame<RGB888Pixel>( virDevice.vsColor ) );
	}
	#pragma endregion

	#pragma region OpenGL Initialize
	// initial glut
	glutInit(&argc, argv);
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB );

	// create glut window
	glutInitWindowSize( 640, 480 );
	glutCreateWindow( "OpenNI 3D Point Cloud" );

	// set OpenGL environment
	glEnable( GL_DEPTH_TEST );
	glDisable( GL_LIGHTING );

	// OpenGL projection
	glMatrixMode( GL_PROJECTION );
	gluPerspective( 40.0, 1.0, 100.0, 20000.0 );	// FOV, aspect ration, near, far
	g_Camera.vCenter	= Vector3( 0.0, 0.0, 1000.0 );
	g_Camera.vPosition	= Vector3( 0.0, 0.0, -2000.0 );
	g_Camera.vUpper		= Vector3( 0.0, 1.0, 0.0 );
	g_Camera.SetCamera();

	// register glut callback functions
	glutDisplayFunc( display );
	glutIdleFunc( idle );
	glutKeyboardFunc( keyboard );
	glutSpecialFunc( specialKey );
	#pragma endregion

	// start
	if( phyDevice.vsDepth.start() == STATUS_OK )
	{
		phyDevice.vsColor.start();
		virDevice.vsDepth.start();
		virDevice.vsColor.start();

		glutMainLoop();
	}
	else
	{
		cerr << "Stream start error: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	return 0;
}

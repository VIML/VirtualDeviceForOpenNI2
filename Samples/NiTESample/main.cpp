/**
 * This is a simple example to show how to use the virtual device.
 * This sample will open an existed real device, read the depth frame with listener,
 * then copy to the virtual device.
 *
 * http://viml.nchc.org.tw/home/
 */

// STL Header
#include <iostream>

// OpenCV Header
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// NiTE Header
#include <NiTE.h>

#include "VirtualDeviceHelper.h"

// namespace
using namespace std;
using namespace nite;

int main( int, char** )
{
	openni::OpenNI::initialize();
	openni::Device mDevice;
	mDevice.open( openni::ANY_DEVICE );
	auto x = mDevice.getDeviceInfo();

	openni::VideoStream vsDepth;
	vsDepth.create( mDevice, openni::SENSOR_DEPTH );
	vsDepth.setMirroringEnabled(true);

	openni::Device virDevice;
	virDevice.open( "\\OpenNI2\\VirtualDevice\\Kinect" );
	openni::VideoStream* virDepth = CreateVirtualStream( virDevice, vsDepth, []( const OniFrame& rF1,OniFrame& rF2 ){
		openni::DepthPixel* pImg1 = reinterpret_cast<openni::DepthPixel*>(rF1.data);
		openni::DepthPixel* pImg2 = reinterpret_cast<openni::DepthPixel*>(rF2.data);

		for( int y = 0; y < rF2.height; ++ y )
		{
			for( int x = 0; x < rF2.width; ++ x )
			{
				int idx = x + y * rF2.width;
				const openni::DepthPixel& rDepth = pImg1[ idx ];
				if( rDepth > 3000 )
					pImg2[ idx ] = 0;
				else
					pImg2[ idx ] = rDepth;
			}
		}
	} );

	vsDepth.start();
	virDepth->start();

	// Initial NiTE
	if( NiTE::initialize() != STATUS_OK )
	{
		cerr << "NiTE initial error" << endl;
		return -1;
	}

	// create user tracker
	UserTracker mUserTracker;
	if( mUserTracker.create( &virDevice ) != STATUS_OK )
	{
		cerr << "Can't create user tracker" << endl;
		return -1;
	}

	// create OpenCV Window
	cv::namedWindow( "User Image",  CV_WINDOW_AUTOSIZE );

	// start
	while( true )
	{
		// get user frame
		UserTrackerFrameRef	mUserFrame;
		if( mUserTracker.readFrame( &mUserFrame )== nite::STATUS_OK )
		{
			// get depth data and convert to OpenCV format
			openni::VideoFrameRef vfDepthFrame = mUserFrame.getDepthFrame();
			const cv::Mat mImageDepth( vfDepthFrame.getHeight(), vfDepthFrame.getWidth(), CV_16UC1, const_cast<void*>( vfDepthFrame.getData() ) );
			// re-map depth data [0,Max] to [0,255]
			cv::Mat mScaledDepth;
			mImageDepth.convertTo( mScaledDepth, CV_8U, 255.0 / 10000 );

			// convert gray-scale to color
			cv::Mat mImageBGR;
			cv::cvtColor( mScaledDepth, mImageBGR, CV_GRAY2BGR );

			// get users data
			const nite::Array<UserData>& aUsers = mUserFrame.getUsers();
			for( int i = 0; i < aUsers.getSize(); ++ i )
			{
				const UserData& rUser = aUsers[i];

				// check user status
				if( rUser.isNew() )
				{
					cout << "New User [" << rUser.getId() << "] found." << endl;
					// 5a. start tracking for new user
					mUserTracker.startSkeletonTracking( rUser.getId() );
				}
				else if( rUser.isLost() )
				{
					cout << "User [" << rUser.getId()  << "] lost." << endl;
				}

				if( rUser.isVisible() )
				{
					// get user skeleton
					const Skeleton& rSkeleton = rUser.getSkeleton();
					if( rSkeleton.getState() == SKELETON_TRACKED )
					{
						// build joints array
						nite::SkeletonJoint aJoints[15];
						aJoints[ 0] = rSkeleton.getJoint( JOINT_HEAD );
						aJoints[ 1] = rSkeleton.getJoint( JOINT_NECK );
						aJoints[ 2] = rSkeleton.getJoint( JOINT_LEFT_SHOULDER );
						aJoints[ 3] = rSkeleton.getJoint( JOINT_RIGHT_SHOULDER );
						aJoints[ 4] = rSkeleton.getJoint( JOINT_LEFT_ELBOW );
						aJoints[ 5] = rSkeleton.getJoint( JOINT_RIGHT_ELBOW );
						aJoints[ 6] = rSkeleton.getJoint( JOINT_LEFT_HAND );
						aJoints[ 7] = rSkeleton.getJoint( JOINT_RIGHT_HAND );
						aJoints[ 8] = rSkeleton.getJoint( JOINT_TORSO );
						aJoints[ 9] = rSkeleton.getJoint( JOINT_LEFT_HIP );
						aJoints[10] = rSkeleton.getJoint( JOINT_RIGHT_HIP );
						aJoints[11] = rSkeleton.getJoint( JOINT_LEFT_KNEE );
						aJoints[12] = rSkeleton.getJoint( JOINT_RIGHT_KNEE );
						aJoints[13] = rSkeleton.getJoint( JOINT_LEFT_FOOT );
						aJoints[14] = rSkeleton.getJoint( JOINT_RIGHT_FOOT );

						// convert joint position to image
						cv::Point2f aPoint[15];
						for( int  s = 0; s < 15; ++ s )
						{
							const Point3f& rPos = aJoints[s].getPosition();
							mUserTracker.convertJointCoordinatesToDepth( rPos.x, rPos.y, rPos.z, &(aPoint[s].x), &(aPoint[s].y) );
						}

						// draw line
						cv::line( mImageBGR, aPoint[ 0], aPoint[ 1], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[ 1], aPoint[ 2], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[ 1], aPoint[ 3], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[ 2], aPoint[ 4], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[ 3], aPoint[ 5], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[ 4], aPoint[ 6], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[ 5], aPoint[ 7], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[ 1], aPoint[ 8], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[ 8], aPoint[ 9], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[ 8], aPoint[10], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[ 9], aPoint[11], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[10], aPoint[12], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[11], aPoint[13], cv::Scalar( 255, 0, 0 ), 3 );
						cv::line( mImageBGR, aPoint[12], aPoint[14], cv::Scalar( 255, 0, 0 ), 3 );

						// draw joint
						for( int  s = 0; s < 15; ++ s )
						{
							if( aJoints[s].getPositionConfidence() > 0.5 )
								cv::circle( mImageBGR, aPoint[s], 3, cv::Scalar( 0, 0, 255 ), 2 );
							else
								cv::circle( mImageBGR, aPoint[s], 3, cv::Scalar( 0, 255, 0 ), 2 );
						}
					}
				}
			}

			// show image
			cv::imshow( "User Image", mImageBGR );

			mUserFrame.release();
		}
		else
		{
			cerr << "Can't get user frame" << endl;
		}

		// check keyboard
		if( cv::waitKey( 1 ) == 'q' )
			break;
	}

	// stop
	mUserTracker.destroy();
	virDepth->destroy();
	vsDepth.destroy();
	mDevice.close();
	virDevice.close();

	NiTE::shutdown();
	openni::OpenNI::shutdown();

	return 0;
}

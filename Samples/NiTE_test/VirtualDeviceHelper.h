#pragma once

// STL Header
#include <functional>
#include <map>

// OpenNI Header
#include <OpenNI.h>
#include <PS1080.h>

// definition of customized property
#define GET_VIRTUAL_STREAM_IMAGE	100000
#define SET_VIRTUAL_STREAM_IMAGE	100001

class CFrameModifer : public openni::VideoStream::NewFrameListener
{
public:
	typedef std::function<void(const OniFrame&,OniFrame&)>	TCallback;

public:
	CFrameModifer( openni::VideoStream& rStream, TCallback func ) : m_rVStream( rStream )
	{
		m_funcProccess = func;
	}

	void onNewFrame( openni::VideoStream& rStream )
	{
		openni::VideoFrameRef mFrame;
		if( rStream.readFrame( &mFrame ) == openni::STATUS_OK )
		{
			OniFrame* pFrame = NULL;
			if( m_rVStream.invoke( GET_VIRTUAL_STREAM_IMAGE, pFrame ) == openni::STATUS_OK )
			{
				m_funcProccess( *(mFrame._getFrame()), *pFrame );
				m_rVStream.invoke( SET_VIRTUAL_STREAM_IMAGE, pFrame );
			}
		}
	}

protected:
	openni::VideoStream&	m_rVStream;
	TCallback				m_funcProccess;

	CFrameModifer& operator=(const CFrameModifer&);
};

openni::VideoStream* CreateVirtualStream( openni::Device& rVDevice, openni::VideoStream& rStream, CFrameModifer::TCallback func )
{
	openni::VideoStream* pStream = new openni::VideoStream();
	if( rStream.isValid() )
	{
		const openni::SensorInfo& rInfo = rStream.getSensorInfo();
		if( pStream->create( rVDevice, rInfo.getSensorType() ) == openni::STATUS_OK )
		{
			// Set configuration
			
			pStream->setProperty( ONI_STREAM_PROPERTY_VERTICAL_FOV,		rStream.getVerticalFieldOfView() );
			pStream->setProperty( ONI_STREAM_PROPERTY_HORIZONTAL_FOV,	rStream.getHorizontalFieldOfView() );
			pStream->setProperty( ONI_STREAM_PROPERTY_MIRRORING,			rStream.getMirroringEnabled() );
			
			if( rInfo.getSensorType() == openni::SENSOR_DEPTH )
			{
				pStream->setProperty( ONI_STREAM_PROPERTY_MIN_VALUE,		rStream.getMinPixelValue() );
				pStream->setProperty( ONI_STREAM_PROPERTY_MAX_VALUE,		rStream.getMaxPixelValue() );

				std::map<int,int>	mapProperties;
				mapProperties[XN_STREAM_PROPERTY_CONST_SHIFT]				= 8;
				mapProperties[XN_STREAM_PROPERTY_PARAM_COEFF]				= 8;
				mapProperties[XN_STREAM_PROPERTY_SHIFT_SCALE]				= 8;
				mapProperties[XN_STREAM_PROPERTY_MAX_SHIFT]					= 8;
				mapProperties[XN_STREAM_PROPERTY_S2D_TABLE]					= 4096;
				mapProperties[XN_STREAM_PROPERTY_D2S_TABLE]					= 20002;
				mapProperties[XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE]		= 8;
				mapProperties[XN_STREAM_PROPERTY_ZERO_PLANE_PIXEL_SIZE]		= 8;
				mapProperties[XN_STREAM_PROPERTY_EMITTER_DCMOS_DISTANCE]	= 8;

				for( auto itProp = mapProperties.begin(); itProp != mapProperties.end(); ++ itProp )
				{
					int iSize = itProp->second;
					char* pData = new char[itProp->second];
					if( rStream.getProperty( itProp->first, pData, &iSize ) == openni::STATUS_OK )
					{
						pStream->setProperty( itProp->first, pData, iSize );
					}
					else
					{
						std::cerr << "This VideoStream doesn't provide property [" << itProp->first << "], which is required for NiTE2" << std::endl;
					}
					delete [] pData;
				}
			}
			pStream->setVideoMode( rStream.getVideoMode() );

			// add listener
			rStream.addNewFrameListener( new CFrameModifer( *pStream, func ) );
		}
	}
	return pStream;
}

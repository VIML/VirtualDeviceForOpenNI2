VirtualDeviceForOpenNI2
=======================

This project provides a vitual device module for OpenNI 2, which aceept user feed any depth, color images.

With the virtual deivce, you can:

1. create a device in OpenNI, to get the data from sensor with their own SDK
2. modify the raw data from sensor, let middleware libraries use modified data

=======================

To use this virtual device, you just need to put the "VirtualDevice.dll" into the OpenNI driver directory.

The drivers usally at the path "OpenNI2\Drivers" relate to working directory.
(You should find files like PS1080.dll, Kinect.dll inside this directory).

After copy the file into drivers directory, you can crate a virtual device with URI:

	\\OpenNI2\\VirtualDevice\\<STRING>

Example:

	Device	devVirDevice;
	devVirDevice.open( "\\OpenNI2\\VirtualDevice\\TEST" );

After virtual device created successfully, you can crate the depth and color VideoStream of it.
You MUST set the VideoMode for each VideoStream you created.

If you are using a VideoStream for depth map, the properties ONI_STREAM_PROPERTY_VERTICAL_FOV and ONI_STREAM_PROPERTY_HORIZONTAL_FOV are required for coordinate convert (between depth and world).

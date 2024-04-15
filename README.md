Small project showing an approach for subpixel edge detection using OpenCV.

The application tries to load an image called TestImage.bmp. If this image cannot be found, an example image showing an ellipse is generated.

The edge detection runs on the loaded or created image and shows the found contours.

![This is an image](/readme/DetectedContour.PNG)

Pressing the key 'g' will show the direction of the gradient for each contour point.

![This is an image](/readme/DetectedContourWithGradient.PNG)


Note:
This is a two stage build process.
First the SuperBuild needs to run to download and build 3rd party dependencies.
The SuperBuild will create the final project that consumes the 3rd parties.
The SuperBuild project needs to run in debug and release to be able to provide the dependencies.
Build Scripts will be provided.

This is a face detection plugin for Nuke 5.1. It uses the OpenCV image
processing library. The plugin was written by Vilya Harvey (that's me!).

For more information about
- Nuke:   http://www.thefoundry.co.uk/
- OpenCV: http://opencv.willowgarage.com/

The face detection relies on training data redistributed as part of the OpenCV
project. Copies of this data are included in the data/haarcascades directory.
The data is subject to it's own license terms, as set out at the beginning of
each file.


Requirements
------------
- Nuke 5.1
- OpenCV 1.0.0


Compiling
---------
- Set your NDKDIR to wherever your copy of the Nuke Development Kit is
  installed.
- Set your OPENCVDIR to wherever your copy of OpenCV is installed.
- make
- make install (creates a link to the plugin in you ~/.nuke directory).


Use
---
The node is called VH_FaceDetect in Nuke. Connect an RGB image source to it's
input and choose one of the Haar cascade files from the data/haarcascade
directory and you're done. Advanced users may wish to change the colour of the
circles it draws around faces... :-)


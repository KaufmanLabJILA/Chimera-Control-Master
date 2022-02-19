

import cv2
from vimba import *

with Vimba.get_instance () as vimba:
	cams = vimba.get_all_cameras ()

	print(cams[0])

	with cams[0] as cam:
		frame = cam.get_frame ()
		frame.convert_pixel_format(PixelFormat.Mono8)
		cv2.imwrite('C:/Users/klab/Documents/Alec/3D_MOT/test_saveframe/frame.jpg', frame.as_opencv_image ())
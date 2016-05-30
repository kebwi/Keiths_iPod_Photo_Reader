Keith's iPod Photo Reader
v2.1

==========================================================
QUICK START

If you're the kind of person who never reads the directions, then go mess with the program to your heart's content and come back here if and when you get stuck.  If you're the kind of person who skims the README and docs looking for gold nuggets of useful information, then skip down to HOW TO USE IT, or even down to KNOWN ENCODINGS OF HIGHEST RESOLUTION IMAGES if you feel so inclined.

However, do not complain to me or anyone else that the program is difficult or confusing until you have resorted to the utter humility of actually reading this file in its entirety!...then you can complain about it.  :-)

==========================================================
INTRODUCTION

This program serves a very specific and minimally useful purpose, to provide access to the Apple iPod's elusive ithmb photo library.  These files can be found in the /Photos/Thumbs/ directory of an iPod that has been synced to contain a photo library.

The iPod stores images in two major formats.  The easiest to understand is the "full resolution" photo format.  Such files are direct transfers of original images to the iPod such that the image is preserved at its high resolution, probably many Megapixels.  This can be useful for transferring photos from one computer to another or simply for having constant access to a library of high-resolution images without the need to carry a computer or external hard-drive around with you all the time.  Full resolution photos cannot be viewed on the iPod's display or on a video device attached to the iPod.  They are intended solely for storing and transferring images on the iPod as an external storage device.

The other photo format used by the iPod is intended for presenting photos on the iPod's own screen or for presenting photos on a video device (TV) attached to the iPod.  These images are preprocessed by iTunes during syncing so that the iPod does not have to perform any resizing on the fly.  In fact, the iPod uses many different resolutions for various situations, and each resolution is stored independently on the iPod, ready for immediate use whenever the need arises.  For example, the resolutions that are preprocessed by iTunes for a 4th generation iPod are:

42x30: used in the five by five thumbnail display on the iPod's screen
130x88: not sure what this is used for, possibly on the iPod's screen during displaying on an external video device
220x176: used for displaying images on the iPod's screen
720x480: used for displaying images on a video device (TV)

Alternatively, a sixth generation iPod stores only three resolutions (I think): 64x64, 320x240, and 720x480.

Note that the resolutions that are preprocessed for one model of iPod tend to differ from those preprocessed for another, although the highest preprocessed resolution (720x480) is fairly consistent across iPod lines.  This is because the highest resolution images are preprocessed for display on NTSC video devices (a typical TV for example), on which 720x480 is an industry standard.

The ithmb file format requires converting each image to a particular encoding that is tailored to the display format (either the iPod's screen or an external TV) and then concatenating all of the images in the library into a single file that may span hundreds of Megabytes.  If a photo library is large enough it will spill over into sequentially numbered files.  The encoding for each image is relatively complicated and you don't need to understand it to use this program.  :-)

Note that your ithmb file collection probably consists of a set of multiple files, each starting with a four-digit prefix.  All files with a given four-digit prefix store images of the same resolution and encoding method (I'm *pretty* sure about this).  Therefore, you only need to determine the settings for a single file of a given four-digit prefix to find the settings for all files with that prefix.

==========================================================
SPEED, PERFORMANCE, ETC.

Opening a large library will take a long time.  Converting the images from the ithmb encoding (in most cases some version of YUV, although in some cases variations on RGB) to a more computer friendly encoding such as 32-bit RGB is a fairly slow process.  Be patient.

==========================================================
HOW TO USE IT

When the program is launched you are presented with the Work Bench, which is used to find the proper settings for your ithmb files.  Start by opening an ithmb file either from your iPod (which should appear as an external disk on your Desktop) or from an ithmb file on your mac if you have copied one from your iPod to your mac.

The next step is to figure out the settings that decode a correct image.  Notice that as soon as you open an ithmb file, a sample image appears.  This is the first image in the file.  It will very likely look like total garbage at first.  There are three settings which must be set:

  -- the image dimensions
  -- offset (to first image from beginning of file)
  -- the endian
  -- the encoding method.

Start with the dimensions.  If you are only interested in reading the highest resolution images (if you are attempting to recover a lost image library for example), then set the dimensions to 720x480.  The next step is to try every available decoding method combined with each of the two endian settings.  Simply click on the various methods and the 'Flip Endian' option in a methodical order to try every combination.  As you do this, the sample image will automatically update.  Trying every combination won't take very long and will unfailingly determine if the program can read the file at the indicated dimensions.

Note, if you want to change the dimensions, you will have to click the 'Test On First Image' button to get the sample image to update.

If you find a decoding method that seems to work with the only problem being that the image is offset by junk, solid black, or noise at the beginning, then try increasing the offset to see if that helps.  In most cases, the offset should always be 0, but there have been a few rare cases where an offset helped.  Using the setting properly is an artform.  It really should be left at 0 in almost all circumstances.

If no settings produce a good image, then either the image dimensions in the file are not 720x480 or the image is encoded in a way that the current version of the program doesn't decode.  Open a different ithmb file and try again.  There isn't much point in trying multiple files with the same four-digit prefix (see the discussion at the end of the introduction above).  Just try one file of a given prefix.  Once you find the 720x480 images, open and convert all images with that prefix to recover your library.

Once you find the correct settings for your file, click the 'Open All Image in File' button to read the entire file at once.  This may take a while.  When it is done, you will be presented with a library of thumbnails.  You can page through the library to view all the thumbnails using the page-up and page-down keys.  You can also click on any individual thumbnail to view it at full size.  Any full size image can be saved as a full size PICT file to disk.

Also note that you can automatically save all the images in the library to PICT files using the 'Save All Images' menu command.

==========================================================
FINAL IMAGE CONVERSION

You may want to convert the saved PICT files to a more common format, such as JPEG, PNG, or TIFF.  You can do this with Preview (in your mac's /Applications/ directory) or with any other common image conversion utility such as Graphic Converter, Photoshop, etc.

==========================================================
KNOWN ENCODINGS OF HIGHEST RESOLUTION IMAGES

This section last updated 080927.

The offset should almost always be left set to 0.  If you find a situation where this is not the case please contact me so I can record it.

The endian may depend on whether you are using a PPC or Intel mac (and *might* depend on the model of iPod, I'm not sure), so try both endian settings.  Note that the prefixes that correspond to particular image resolutions may or may not vary from one iPod to another, I simply don't know for certain.  The following data represent settings observed on some iPods.  iPod nanos probably don't store 720x480 images because, to my knowledge, they can't show images on external display devices.

If you discover the correct settings for a specific four-digit prefix on a particular model of iPod, please notify me so I can update this database for other users.  I will need to know is the file prefix, the image dimensions, the decoding method, and the iPod model.

Thank you very much for helping me build this database.

----------------------------------------
IPOD STANDARD

4th generation iPod (iPod Photo):
    Highest resolution ithmb image: 720x480
    File prefix for highest res images: 1019
    Encoding method: 16-bit YCbCr 4:2:2, Interlaced Shared Chrominance

5th generation iPod (iPod Video):
    Highest resolution ithmb image: 720x480
    File prefix for highest res images: 1019
    Encoding method: 16-bit YCbCr 4:2:2, Interlaced Shared Chrominance

6th generation iPod (iPod Classic):
    Highest resolution ithmb image: 720x480
    File prefix for highest res images: 1067
    Encoding method: 12-bit YCbCr 4:2:0, Blue first, half image padded

----------------------------------------
IPOD NANO

1th generation iPod nano (plastic body):
    Highest resolution ithmb image: 176x132
    File prefix for highest res images: 1023
    Encoding method: 16-bit RGB 6-bit green

2nd generation iPod nano (anodyzed aluminum body):
    Highest resolution ithmb image: 176x132
    File prefix for highest res images: 1023
    Encoding method: 16-bit RGB 6-bit green

3rd generation iPod nano (video, short & fat shape):
    Highest resolution ithmb image: 720x480
    File prefix for highest res images: 1067
    Encoding method: 12-bit YCbCr 4:2:0, Blue first, half image padded

4th generation iPod nano (video, original slim nano shape):
    Highest resolution ithmb image: 720x480
    File prefix for highest res images: 1067
    Encoding method: 12-bit YCbCr 4:2:0, Blue first, half image padded

----------------------------------------
IPOD TOUCH AND IPHONE

1st generation iPod touch:
    Highest resolution ithmb image: 640x480
    File prefix for highest res images: 3008
    Encoding method: 16-bit RGB

2nd generation iPod touch (tapered back):
    No data yet, but probably the same as the first gen and current iPhones.  Email me if you find out.

1st generation iPhone:
    Highest resolution ithmb image: 640x480
    File prefix for highest res images: 3008
    Encoding method: 16-bit RGB

2st generation iPhone (3G with GPS):
    Highest resolution ithmb image: 640x480
    File prefix for highest res images: 3008
    Encoding method: 16-bit RGB

==========================================================
CREDITS

080927
Keith Wiley
kwiley@keithwiley.com
http://keithwiley.com

==========================================================
VERSION HISTORY

080927
Version 2.1
Bug Fixes
-- None
Changes
-- Added 16 and 32 bit grayscale decoding methods.
-- Added a first-image-offset option (best left at 0 in most cases).

080604
Version 2.0
Bug Fixes
-- None
Changes
-- More precise YUV/RGB conversion matrices.
-- Complete overhaul of the user interface, primarily to generalize it for a wider range of image dimensions and encodings.
-- Numerous cosmetic changes.

060107
Version 1.1
Bug Fixes
-- None
Changes
-- There is a new menu option to automatically save all the images from the .ithmb file to your harddrive.  When you use this option, you will be prompted to select a folder into which to save all the images.  The files will be named "iPod Photo - #####" where ##### is the image index in the .ithmb file.

050801
Version 1.0
Bug Fixes
-- None
Changes
-- First public release

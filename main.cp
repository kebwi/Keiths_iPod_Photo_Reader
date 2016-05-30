//
//  main.cpp
//  KeithsIPodPhotoReader
//
//  Created by Keith Wiley on 080523.
//  Copyright __MyCompanyName__ 2008. All rights reserved.
//

#include <Carbon/Carbon.h>
#include "TApplication.h"
#include "TWindow.h"

#include "GWorldWrapper.h"
#include "WorkingDialog.h"
#include "PascalStringUtil.h"
#include "Endian.h"
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <time.h>

#include <iostream>	//debug

using namespace std;

//--------------------------------------------------------------------------------------------
// Our custom application class
class CarbonApp : public TApplication
{
	public:
							
							CarbonApp() {}
		virtual 			~CarbonApp() {}
		
	protected:
		virtual Boolean 	HandleCommand( const HICommandExtended& inCommand );
};

// Our main window class
class MainWindow : public TWindow
{
	public:
							MainWindow(CFStringRef name);
		virtual 			~MainWindow();
		
	protected:
		virtual Boolean 	HandleCommand( const HICommandExtended& inCommand );
		
		virtual OSStatus	HandleEvent(EventHandlerCallRef		inCallRef,
										TCarbonEvent&			inEvent );
	
	private:
		void				TestImage();
};

//--------------------------------------------------------------------------------------------

//******************************************************************************
//Extern Globals

//******************************************************************************
//Global Declarations
MainWindow *gAboutWindow = NULL, *gThumbsWindow = NULL, *gImageWindow = NULL, *gWorkBenchWindow = NULL;
bool gQuit = false;
FSSpec gITHMB_FSSpec;
WorkingDialog *gWorkingDialog = NULL;
int gThumbWindowPos = -1, gImageViewIndex = -1;
	//	F1009_1.ithmb	42x30		1260 pixels		2520 bytes
	//	F1015_1.ithmb	130x88		11440 pixels	22880 bytes
	//	F1019_1.ithmb	720x480		345600 pixels	691200 bytes
	//	F1020_1.ithmb	220x176		38720 pixels	77440 bytes
//int gImageWidth = 42, gImageHeight = 30;
//int gImageWidth = 130, gImageHeight = 88;
int gImageWidth = 720, gImageHeight = 480;
//int gImageWidth = 220, gImageHeight = 176;
int gFirstImageOffset = 0;
bool gFlipEndian = false;
int gBytesPerPixel = 2;
int gMethod = 1;
int gThumbSpacer = 4;
int gNumThumbsAcross = 10, gNumThumbsDown = 8;
int gThumbScalar = 8;
int gThumbWidth = gImageWidth / gThumbScalar, gThumbHeight = gImageHeight / gThumbScalar;
GWorldWrapper* gImage = NULL;
vector<GWorldWrapper*> gThumbs;

//******************************************************************************
//Function Prototypes
	//	In this file
	//	In other files
		//toolbox.cpp
void ToolBoxInit();
void WindowInit();
void MenuBarInit();
void DoEvent(EventRecord &eventPtr);
		//FileProcs.cpp
OSErr GetOneFileWithPreview(short numTypes, OSType openTypeList[], FSSpecPtr theFSSpecPtr, void *theFilterProc);
OSErr PutPictToFile(PicHandle thePicture, FSSpec& theFSSpec);
OSErr PutPictToFileAutomatically(PicHandle picH, FSSpec& theFSSpec, OSType creatorType, OSType fileType, bool usePassedInFSSpec);
OSErr ChooseFolder(FSSpec &myAppSpec, char* message);

#pragma mark -

void RedrawThumbsWindow()
{
	if (gThumbWindowPos == -1)
		return;
	
	gThumbsWindow->Show();
	
	SetPort(gThumbsWindow->GetPort());
	Rect bounds;
	//ClipRect(&bounds);
	GetPortBounds(gThumbsWindow->GetPort(), &bounds);
	bounds.top += 30;
	EraseRect(&bounds);	//Make sure to erase previous selection box (around currently viewed image)
	
	ForeColor(blackColor);
	for (int y = 0; y < gNumThumbsDown; y++)
		for (int x = 0; x < gNumThumbsAcross; x++)
		{
			int thumbIndex = gThumbWindowPos + y * gNumThumbsAcross + x;
			if (thumbIndex >= gThumbs.size())
				return;
			
			Rect loc = gThumbs[thumbIndex]->GetDim();
			OffsetRect(&loc, x * gThumbWidth + (x + 1) * gThumbSpacer, 30 + y * gThumbHeight + (y + 1) * gThumbSpacer);
			gThumbs[thumbIndex]->CopyWorldBits(gThumbs[0]->GetDim(), loc, gThumbsWindow->GetPort());
			
			if (thumbIndex == gImageViewIndex)
			{
				PenSize(4, 4);
				ForeColor(cyanColor);
				loc.left -= 4;
				loc.top -= 4;
				loc.right += 4;
				loc.bottom += 4;
				FrameRect(&loc);
				ForeColor(blackColor);
				PenSize(1, 1);
			}
		}
}

void RedrawImageWindow()
{
	if (gImageViewIndex == -1 || !gImage)
		return;
	
	gImageWindow->Show();
	
	gImage->CopyWorldBits(gImage->GetDim(), gImageWindow->GetPort());
	
	RedrawThumbsWindow();
	
	MenuHandle menuHandle = GetMenuHandle(129);
	EnableMenuItem(menuHandle, 2);
}

#pragma mark -

RGBColor ConvertTwoByteRGBToRGBColor(unsigned char highByte, unsigned char lowByte)
{
	RGBColor rgb;
	unsigned char red, green, blue;
	
	red =	(highByte >> 2) & 31/*0b00011111*/;
	green =	((highByte << 3) & 24/*0b00011000*/) | ((lowByte >> 5) & 7/*0b00000111*/);
	blue =	lowByte & 31/*0b00011111*/;
	
	rgb.red = red * 2048;
	rgb.green = green * 2048;
	rgb.blue = blue * 2048;
	
	return rgb;
}

RGBColor ConvertTwoByteRGBWith6BitGreenToRGBColor(unsigned char highByte, unsigned char lowByte)
{
	RGBColor rgb;
	unsigned char red, green, blue;
	
	red =	(highByte >> 3) & 31/*0b00011111*/;
	green =	((highByte << 3) & 56/*0b00111000*/) | ((lowByte >> 5) & 7/*0b00000111*/);
	blue =	lowByte & 31/*0b00011111*/;
	
	rgb.red = red * 2048;
	rgb.green = green * 1024;
	rgb.blue = blue * 2048;
	
	return rgb;
}

RGBColor ConvertFourByteRGBToRGBColor(unsigned long fourBytes, bool alphaFirst)
{
	RGBColor rgb;
	unsigned char red, green, blue;
	
	if (alphaFirst)
	{
		red = (fourBytes >> 16) & 255;
		green = (fourBytes >> 8) & 255;
		blue = fourBytes & 255;
	}
	else
	{
		red = (fourBytes >> 24) & 255;
		green = (fourBytes >> 16) & 255;
		blue = (fourBytes >> 8) & 255;
	}
	
	rgb.red = red * 256;
	rgb.green = green * 256;
	rgb.blue = blue * 256;
	
	return rgb;
}

RGBColor ConvertYUVtoRGBColor(float y, float u, float v)
{
	RGBColor rgb;
	
	y -= .5;
	u -= .5;
	v -= .5;
	
	y *= 2.0;
	u *= 2.0;
	v *= 2.0;
	
	float red =		(y +							1.13983 *		v);
	float green =	(y +		-0.39465 *	u +		-0.5806 *	v);
	float blue =	(y +		2.03211 *		u);
	
	red /= 2.0;
	green /= 2.0;
	blue /= 2.0;
	
	red += .5;
	green += .5;
	blue += .5;
	
	float cutoff = 1.0;
	
	if (red < 0)
		red = 0;
	else if (red > cutoff)
		red = cutoff;
	if (green < 0)
		green = 0;
	else if (green > cutoff)
		green = cutoff;
	if (blue < 0)
		blue = 0;
	else if (blue > cutoff)
		blue = cutoff;
	
	float scaler = 65535.0;// / cutoff;
	
	rgb.red = red * scaler;
	rgb.green = green * scaler;
	rgb.blue = blue * scaler;
	
	return rgb;
}

RGBColor ConvertCLToRGBColor(unsigned char chrominance, unsigned char luminance)
{
	float y = (float)luminance / 255.0;
	float u = (float)((chrominance >> 4) & 15/*0b00001111*/) / 16.0;
	float v = (float)(chrominance & 15/*0b00001111*/) / 16.0;
	
	return ConvertYUVtoRGBColor(y, u, v);
}

RGBColor ConvertCLCLToRGBColor(bool evenPixel, long fourBytes)//unsigned char evenC, unsigned char evenL, unsigned char oddC, unsigned char oddL)
{
	//unsigned char evenC = (fourBytes >> 24) & 255/*0b11111111*/;
	//unsigned char evenL = (fourBytes >> 16) & 255/*0b11111111*/;
	//unsigned char oddC = (fourBytes >> 8) & 255/*0b11111111*/;
	//unsigned char oddL = fourBytes & 255/*0b11111111*/;
	
	float y = evenPixel ? ((float)((fourBytes >> 16) & 255/*0b11111111*/) / 255.0) : ((float)(fourBytes & 255/*0b11111111*/) / 255.0);
	float u = (float)((fourBytes >> 24) & 255/*0b11111111*/) / 255.0;
	float v = (float)((fourBytes >> 8) & 255/*0b11111111*/) / 255.0;
	
	return ConvertYUVtoRGBColor(y, u, v);
}

RGBColor ConvertCLCLToRGBColorXeven(long fourBytes)//unsigned char evenC, unsigned char evenL, unsigned char oddC, unsigned char oddL)
{
	//unsigned char evenC = (fourBytes >> 24) & 255/*0b11111111*/;
	//unsigned char evenL = (fourBytes >> 16) & 255/*0b11111111*/;
	//unsigned char oddC = (fourBytes >> 8) & 255/*0b11111111*/;
	//unsigned char oddL = fourBytes & 255/*0b11111111*/;
	
	float y = (float)((fourBytes >> 16) & 255/*0b11111111*/) / 255.0;
	float u = (float)((fourBytes >> 24) & 255/*0b11111111*/) / 255.0;
	float v = (float)((fourBytes >> 8) & 255/*0b11111111*/) / 255.0;
	
	return ConvertYUVtoRGBColor(y, u, v);
}

RGBColor ConvertCLCLToRGBColorXodd(long fourBytes)//unsigned char evenC, unsigned char evenL, unsigned char oddC, unsigned char oddL)
{
	//unsigned char evenC = (fourBytes >> 24) & 255/*0b11111111*/;
	//unsigned char evenL = (fourBytes >> 16) & 255/*0b11111111*/;
	//unsigned char oddC = (fourBytes >> 8) & 255/*0b11111111*/;
	//unsigned char oddL = fourBytes & 255/*0b11111111*/;
	
	float y = (float)(fourBytes & 255/*0b11111111*/) / 255.0;
	float u = (float)((fourBytes >> 24) & 255/*0b11111111*/) / 255.0;
	float v = (float)((fourBytes >> 8) & 255/*0b11111111*/) / 255.0;
	
	return ConvertYUVtoRGBColor(y, u, v);
}

#pragma mark -

void Draw8BitGrayImage(unsigned char* buffer, GWorldWrapper* gww)
{
	gww->StartUsingGWorld();
	
	unsigned char byte;
	RGBColor c;
	for (int y = 0; y < gImageHeight; y++)
		for (int x = 0; x < gImageWidth; x++)
		{
			byte = buffer[y * gImageWidth + x];
			
			c.red = c.green = c.blue = byte * 256;
			
			SetCPixel(x, y, &c);
		}
	
	gww->FinishUsingGWorld();
}

void Draw16BitGrayImage(unsigned char* buffer, GWorldWrapper* gww)
{
	gww->StartUsingGWorld();
	
	unsigned short twoBytes;
	RGBColor c;
	int xTimes2;
	int imageWidthTimes2 = gImageWidth * 2;
	for (int y = 0; y < gImageHeight; y++)
		for (int x = 0; x < gImageWidth; x++)
		{
			xTimes2 = x * 2;
			
			twoBytes = *(unsigned short*)(buffer + y * imageWidthTimes2 + xTimes2);
			if (gFlipEndian)
				FlipEndian(twoBytes);
			
			c.red = c.green = c.blue = twoBytes * 16;
			
			SetCPixel(x, y, &c);
		}
	
	gww->FinishUsingGWorld();
}

void Draw32BitGrayImage(unsigned char* buffer, GWorldWrapper* gww)
{
	gww->StartUsingGWorld();
	
	unsigned long fourBytes;
	RGBColor c;
	int xTimes4;
	int imageWidthTimes4 = gImageWidth * 4;
	for (int y = 0; y < gImageHeight; y++)
		for (int x = 0; x < gImageWidth; x++)
		{
			xTimes4 = x * 4;
			
			fourBytes = *(unsigned long*)(buffer + y * imageWidthTimes4 + xTimes4);
			if (gFlipEndian)
				FlipEndian(fourBytes);
			
			c.red = c.green = c.blue = fourBytes;
			
			SetCPixel(x, y, &c);
		}
	
	gww->FinishUsingGWorld();
}

void Draw16BitRGBImage(unsigned char* buffer, GWorldWrapper* gww, bool sixBitGreen)
{
	gww->StartUsingGWorld();
	
	unsigned short twoBytes;
	unsigned char highByte, lowByte;
	RGBColor c;
	int xTimes2;
	int imageWidthTimes2 = gImageWidth * 2;
	for (int y = 0; y < gImageHeight; y++)
		for (int x = 0; x < gImageWidth; x++)
		{
			xTimes2 = x * 2;
			
			twoBytes = *(unsigned short*)(buffer + y * imageWidthTimes2 + xTimes2);
			if (gFlipEndian)
				FlipEndian(twoBytes);
			highByte = *(char*)&twoBytes;
			lowByte = *(((char*)&twoBytes) + 1);
			
			if (!sixBitGreen)
				c = ConvertTwoByteRGBToRGBColor(highByte, lowByte);
			else c = ConvertTwoByteRGBWith6BitGreenToRGBColor(highByte, lowByte);
			
			SetCPixel(x, y, &c);
		}
	
	gww->FinishUsingGWorld();
}

void Draw32BitRGBImage(unsigned char* buffer, GWorldWrapper* gww, bool alphaFirst)
{
	gww->StartUsingGWorld();
	
	unsigned long fourBytes;
	RGBColor c;
	int xTimes4;
	int imageWidthTimes4 = gImageWidth * 4;
	for (int y = 0; y < gImageHeight; y++)
		for (int x = 0; x < gImageWidth; x++)
		{
			xTimes4 = x * 4;
			
			fourBytes = *(unsigned long*)(buffer + y * imageWidthTimes4 + xTimes4);
			if (gFlipEndian)
				FlipEndian(fourBytes);
			
			c = ConvertFourByteRGBToRGBColor(fourBytes, alphaFirst);
			
			SetCPixel(x, y, &c);
		}
	
	gww->FinishUsingGWorld();
}

void DrawYUV422Image(unsigned char* buffer, GWorldWrapper* gww, bool interlaced, bool sharedChrominance)
{
	gww->StartUsingGWorld();
	
	int imagePixels = gImageWidth * gImageHeight;
	
	unsigned char evenC, evenL, oddC, oddL;
	RGBColor c;
	int xTimes2, xMinus1Times2, yOver2, yMinus1Over2;
	int imageWidthTimes2 = gImageWidth * 2;
	unsigned char* cPtr;
	unsigned short twoBytes;
	unsigned long fourBytes;
	unsigned char highByte, lowByte;
	for (int y = 0; y < gImageHeight; y++)
	{
		yOver2 = y / 2;
		yMinus1Over2 = (y - 1) / 2;
		
		for (int x = 0; x < gImageWidth; x++)
		{
			xTimes2 = x * 2;
			xMinus1Times2 = (x - 1) * 2;
			
			if (!interlaced)
			{
				if (!sharedChrominance)
				{
					twoBytes = *(unsigned short*)(buffer + y * imageWidthTimes2 + xTimes2);
					if (gFlipEndian)
						FlipEndian(twoBytes);
					highByte = *(char*)&twoBytes;
					lowByte = *(((char*)&twoBytes) + 1);
					
					c = ConvertCLToRGBColor(highByte, lowByte);
				}
				else
				{
					if (x % 2 == 0)
					{
						fourBytes = *(unsigned long*)(buffer + y * imageWidthTimes2 + xTimes2);
						if (gFlipEndian)
							FlipEndian(fourBytes);
						c = ConvertCLCLToRGBColorXeven(fourBytes);
					}
					else
					{
						fourBytes = *(unsigned long*)(buffer + y * imageWidthTimes2 + xMinus1Times2);
						if (gFlipEndian)
							FlipEndian(fourBytes);
						c = ConvertCLCLToRGBColorXodd(fourBytes);
					}
				}
			}
			else	//interlaced
			{
				if (y % 2 == 0)
				{
					if (!sharedChrominance)
					{
						twoBytes = *(unsigned short*)(buffer + yOver2 * imageWidthTimes2 + xTimes2);
						if (gFlipEndian)
							FlipEndian(twoBytes);
						highByte = *(char*)&twoBytes;
						lowByte = *(((char*)&twoBytes) + 1);
						
						c = ConvertCLToRGBColor(highByte, lowByte);
					}
					else
					{
						if (x % 2 == 0)
						{
							fourBytes = *(unsigned long*)(buffer + yOver2 * imageWidthTimes2 + xTimes2);
							if (gFlipEndian)
								FlipEndian(fourBytes);
							c = ConvertCLCLToRGBColorXeven(fourBytes);
						}
						else
						{
							fourBytes = *(unsigned long*)(buffer + yOver2 * imageWidthTimes2 + xMinus1Times2);
							if (gFlipEndian)
								FlipEndian(fourBytes);
							c = ConvertCLCLToRGBColorXodd(fourBytes);
						}
					}
				}
				else
				{
					if (!sharedChrominance)
					{
						twoBytes = *(unsigned short*)(buffer + yMinus1Over2 * imageWidthTimes2 + xTimes2);
						if (gFlipEndian)
							FlipEndian(twoBytes);
						highByte = *(char*)&twoBytes;
						lowByte = *(((char*)&twoBytes) + 1);
						
						c = ConvertCLToRGBColor(highByte, lowByte);
					}
					else
					{
						if (x % 2 == 0)
						{
							fourBytes = *(unsigned long*)(buffer + imagePixels + yMinus1Over2 * imageWidthTimes2 + xTimes2);
							if (gFlipEndian)
								FlipEndian(fourBytes);
							c = ConvertCLCLToRGBColorXeven(fourBytes);
						}
						else
						{
							fourBytes = *(unsigned long*)(buffer + imagePixels + yMinus1Over2 * imageWidthTimes2 + xMinus1Times2);
							if (gFlipEndian)
								FlipEndian(fourBytes);
							c = ConvertCLCLToRGBColorXodd(fourBytes);
						}
					}
				}
			}
			
			SetCPixel(x, y, &c);
		}
	}
	
	gww->FinishUsingGWorld();
}

void DrawYUV420PlanarImage(unsigned char* buffer, GWorldWrapper* gww, bool blueFirst)
{
	gww->StartUsingGWorld();
	
	int imagePixels = gImageWidth * gImageHeight;
	
	unsigned char lum, cb, cr;
	float lumF, cbF, crF;
	int xOver2, yOver2;
	int imageWidthOver2 = gImageWidth / 2;
	int imageQuarterPixels = imagePixels / 4;
	RGBColor c;
	for (int y = 0; y < gImageHeight; y++)
	{
		yOver2 = y / 2;
		for (int x = 0; x < gImageWidth; x++)
		{
			xOver2 = x / 2;
			
			lum = buffer[y * gImageWidth + x];
			cb = buffer[imagePixels + yOver2 * imageWidthOver2 + xOver2];
			cr = buffer[imagePixels + imageQuarterPixels + yOver2 * imageWidthOver2 + xOver2];
			
			lumF = (float)lum / 255.0;
			cbF = (float)cb / 255.0;
			crF = (float)cr / 255.0;
			
			if (blueFirst)
				c = ConvertYUVtoRGBColor(lumF, cbF, crF);
			else c = ConvertYUVtoRGBColor(lumF, crF, cbF);
			
			SetCPixel(x, y, &c);
		}
	}
	
	gww->FinishUsingGWorld();
}

#pragma mark -

void ReadITHMBBuffer(unsigned char *buffer)
{
	switch (gMethod)
	{
		case 1:
			DrawYUV422Image(buffer, gImage, true, true);
			break;
		case 2:
			DrawYUV422Image(buffer, gImage, false, true);
			break;
		case 3:
			DrawYUV422Image(buffer, gImage, true, false);
			break;
		case 4:
			DrawYUV422Image(buffer, gImage, false, false);
			break;
		case 5:
			DrawYUV420PlanarImage(buffer, gImage, true);
			break;
		case 6:
			DrawYUV420PlanarImage(buffer, gImage, false);
			break;
		case 7:
			Draw16BitRGBImage(buffer, gImage, true);
			break;
		case 8:
			Draw16BitRGBImage(buffer, gImage, false);
			break;
		case 9:
			Draw32BitRGBImage(buffer, gImage, false);
			break;
		case 10:
			Draw32BitRGBImage(buffer, gImage, true);
			break;
		case 11:
			Draw8BitGrayImage(buffer, gImage);
			break;
		case 12:
			Draw16BitGrayImage(buffer, gImage);
			break;
		case 13:
			Draw32BitGrayImage(buffer, gImage);
			break;
	}
}

void SetBytesPerPixel()
{
	
	switch (gMethod)
	{
		case 11:
			gBytesPerPixel = 1;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 7:
		case 8:
		case 12:
			gBytesPerPixel = 2;
			break;
		case 5:
		case 6:
			gBytesPerPixel = 2;	//1.5 seems more logical here, but the files are often padded (the pad includes a half-cropped image)
			break;
		case 9:
		case 10:
		case 13:
			gBytesPerPixel = 4;
			break;
	}
}

void SetThumbsWindowSize()
{
	int thumbWindowWidth = gThumbWidth * gNumThumbsAcross + gThumbSpacer * (gNumThumbsAcross + 1);
	int thumbWindowHeight = gThumbHeight * gNumThumbsDown + gThumbSpacer * (gNumThumbsDown + 1);
	Rect bounds;
	GetPortBounds(gThumbsWindow->GetPort(), &bounds);
	if (bounds.right != thumbWindowWidth || bounds.bottom != thumbWindowHeight + 30)
		SizeWindow(gThumbsWindow->GetWindowRef(), thumbWindowWidth, thumbWindowHeight + 30, true);
}

void ReadThumbsFromITHMBfile()
{
	OSErr err;
	
	//Open the file and get its size to calculate the number of images in the library
	SInt16 inRef = -1;
	err = FSpOpenDF(&gITHMB_FSSpec, fsRdPerm, &inRef);
	if (err != noErr)
		return;
	
	SInt32 dataLength = 0;
	err = GetEOF(inRef, &dataLength);
	if (err != noErr)
	{
		err = FSClose(inRef);
		return;
	}
	
	SetBytesPerPixel();
	
	int imagePixels = gImageWidth * gImageHeight;
	long imageBytes = imagePixels * gBytesPerPixel;
	int numImagesInLibrary = (dataLength - gFirstImageOffset) / imageBytes;
	
	Rect imageDim = { 0, 0, gImageHeight, gImageWidth };
	Rect thumbDim = { 0, 0, gThumbHeight, gThumbWidth };
	
	if (gImage)
	{
		delete gImage;
		gImage = NULL;
	}
	gImage = new GWorldWrapper(32, imageDim, -1);
	
	for (int i = 0; i < gThumbs.size(); i++)
		delete gThumbs[i];
	gThumbs.clear();
	gThumbsWindow->Hide();
	
	gImageWindow->Hide();
	MenuHandle menuHandle = GetMenuHandle(129);
	DisableMenuItem(menuHandle, 2);
	
	gThumbWindowPos = -1;
	gImageViewIndex = -1;
	
	//Read in all the images (creating their thumbs as we go)
	if (gWorkingDialog)
	{
		delete gWorkingDialog;
		gWorkingDialog = NULL;
	}
	gWorkingDialog = new WorkingDialog(numImagesInLibrary);
	
	unsigned char* buffer = new unsigned char[imageBytes];
	int numImagesRead = 0;
	for (numImagesRead = 0; numImagesRead < numImagesInLibrary; numImagesRead++)
	{
		EventRecord theEvent;
		WaitNextEvent((everyEvent), &theEvent, 0, NULL);
		if (WorkingDialog::GetLastDialogRunStatus() != WorkingDialog::RUNNING)
			break;
		
		const SInt32 imageOffset = gFirstImageOffset + numImagesRead * imageBytes;
		err = SetFPos(inRef, fsFromStart, imageOffset);
		if (err != noErr)
		{
			delete gWorkingDialog;
			gWorkingDialog = NULL;
			delete [] buffer;
			err = FSClose(inRef);
			return;
		}
		
		err = FSRead(inRef, &imageBytes, buffer);
		if (err != noErr)
		{
			delete gWorkingDialog;
			gWorkingDialog = NULL;
			delete [] buffer;
			err = FSClose(inRef);
			return;
		}
		
		GWorldWrapper* thumb = NULL;
		try
		{
			thumb = new GWorldWrapper(32, thumbDim, NULL);
		}
		catch (...)
		{
			if (thumb)
				delete thumb;
			break;
		}
		
		ReadITHMBBuffer(buffer);
		
		thumb->StartUsingGWorld();
		gImage->CopyWorldBits(gImage->GetDim(), thumb->GetDim());
		thumb->FinishUsingGWorld();
		
		gThumbs.push_back(thumb);
		
		gWorkingDialog->IncrementValueAndDrawWorkingDialogProgressBar();
	}
	delete gWorkingDialog;
	gWorkingDialog = NULL;
	delete [] buffer;
	
	if (inRef != -1)
		err = FSClose(inRef);
	if (err != noErr)
		return;
	
	SizeWindow(gImageWindow->GetWindowRef(), gImageWidth, gImageHeight, true);
	
	SetThumbsWindowSize();
	
	Str255 librarySizePascal;
	NumToString(numImagesRead, librarySizePascal);
	Str255 windowTitlePascal = "\p images in library";
	PascalAppend(librarySizePascal, windowTitlePascal);
	char windowTitle[128];
	PascalToC(librarySizePascal, windowTitle);
	gThumbsWindow->SetTitle(CFStringCreateWithCString(NULL, windowTitle, kCFStringEncodingASCII));
	
	gThumbWindowPos = 0;
	RedrawThumbsWindow();
	
	if (gThumbs.size() > 0)
	{
		MenuHandle menuHandle = GetMenuHandle(129);
		DisableMenuItem(menuHandle, 2);
		EnableMenuItem(menuHandle, 3);
	}
}

void ReadOneImageFromITHMBfile()
{
	OSErr err;
	
	//Open the file and get its size to calculate the number of images in the library
	SInt16 inRef = -1;
	err = FSpOpenDF(&gITHMB_FSSpec, fsRdPerm, &inRef);
	if (err != noErr)
		return;
	
	SInt32 dataLength = 0;
	err = GetEOF(inRef, &dataLength);
	if (err != noErr)
	{
		err = FSClose(inRef);
		return;
	}
	
	SetBytesPerPixel();
	
	int imagePixels = gImageWidth * gImageHeight;
	long imageBytes = imagePixels * gBytesPerPixel;
	int numImagesInLibrary = (dataLength - gFirstImageOffset) / imageBytes;
	
	if (gImageViewIndex > numImagesInLibrary)
	{
		err = FSClose(inRef);
		return;
	}
	
	Rect imageDim = { 0, 0, gImageHeight, gImageWidth };
	
	unsigned char* buffer = new unsigned char[imageBytes];
	
	const SInt32 imageOffset = gFirstImageOffset + gImageViewIndex * imageBytes;
	err = SetFPos(inRef, fsFromStart, imageOffset);
	if (err != noErr)
	{
		delete [] buffer;
		err = FSClose(inRef);
		return;
	}
	
	err = FSRead(inRef, &imageBytes, buffer);
	if (err != noErr)
	{
		delete [] buffer;
		err = FSClose(inRef);
		return;
	}
	
	if (inRef != -1)
		err = FSClose(inRef);
	if (err != noErr)
		return;
	
	ReadITHMBBuffer(buffer);
	
	delete [] buffer;
}

#pragma mark -

void OpenFromPhotosThumbsFolder()
{
	OSErr err = ChooseFolder(gITHMB_FSSpec, "Select a folder in which to look for .ithmb files.  For example, select the /Photos/Thumbs folder on your iPod.");
	if (err != noErr)
		return;
	
	//strcpy((char*)gITHMB_FSSpec.name, (char*)"\pF1009_1.ithmb");
	//strcpy((char*)gITHMB_FSSpec.name, (char*)"\pF1015_1.ithmb");
	strcpy((char*)gITHMB_FSSpec.name, (char*)"\pF1019_1.ithmb");
	//strcpy((char*)gITHMB_FSSpec.name, (char*)"\pF1020_1.ithmb");
	
	ReadThumbsFromITHMBfile();
}

void OpenFromITHMBfile()
{
	OSErr err = GetOneFileWithPreview(0, NULL, &gITHMB_FSSpec, NULL);
	if (err != noErr)
		return;
	
	ReadThumbsFromITHMBfile();
}

#pragma mark -

void SavePresentImage()
{
	if (!gImage)
		return;
	
	Rect r = gImage->GetDim();
	PicHandle picH = OpenPicture(&r);
	gImage->CopyWorldBits(gImage->GetDim());
	ClosePicture();
	
	FSSpec fsspec;	//Unused
	OSErr err = PutPictToFile(picH, fsspec);
}

void AutoSavePresentImage(FSSpec fsspec)
{
	if (!gImage)
		return;
	
	Str255 fileName;
	PascalCopy(gITHMB_FSSpec.name, fileName);
	for (int i = fileName[0]; i > 0; i--)
		if (fileName[i] == '.')
		{
			fileName[0] = i - 1;
			break;
		}
	PascalAppend(fileName, "\p_");
	
	if (gThumbs.size() >= 1000000 && gImageViewIndex < 1000000)
		PascalAppend(fileName, "\p0");
	if (gThumbs.size() >= 100000 && gImageViewIndex < 100000)
		PascalAppend(fileName, "\p0");
	if (gThumbs.size() >= 10000 && gImageViewIndex < 10000)
		PascalAppend(fileName, "\p0");
	if (gThumbs.size() >= 1000 && gImageViewIndex < 1000)
		PascalAppend(fileName, "\p0");
	if (gThumbs.size() >= 100 && gImageViewIndex < 100)
		PascalAppend(fileName, "\p0");
	if (gThumbs.size() >= 10 && gImageViewIndex < 10)
		PascalAppend(fileName, "\p0");
	
	Str255 str1;
	NumToString(gImageViewIndex, str1);
		PascalAppend(fileName, str1);
	
	PascalAppend(fileName, "\p.pct");
	
	PascalCopy(fileName, fsspec.name);
	
	Rect r = gImage->GetDim();
	PicHandle picH = OpenPicture(&r);
	gImage->CopyWorldBits(gImage->GetDim());
	ClosePicture();
	
	OSErr err = PutPictToFileAutomatically(picH, fsspec, 'ttxt', 'PICT', true);
}

void SaveAllImages()
{
	FSSpec fsspec;
	OSErr err = ChooseFolder(fsspec, "Select a folder in which to save the PICT files.");
	if (err != noErr)
		return;
	
	if (gWorkingDialog)
	{
		delete gWorkingDialog;
		gWorkingDialog = NULL;
	}
	gWorkingDialog = new WorkingDialog(gThumbs.size());
	
	for (int i = 0; i < gThumbs.size(); i++)
	{
		EventRecord theEvent;
		WaitNextEvent((everyEvent), &theEvent, 0, NULL);
		if (WorkingDialog::GetLastDialogRunStatus() != WorkingDialog::RUNNING)
			break;
		
		gImageViewIndex = i;
		ReadOneImageFromITHMBfile();
		//RedrawImageWindow();
		//QDFlushPortBuffer(gImageWindow->GetPort(), NULL);
		AutoSavePresentImage(fsspec);
		
		gWorkingDialog->IncrementValueAndDrawWorkingDialogProgressBar();
	}
	
	delete gWorkingDialog;
	gWorkingDialog = NULL;
}

#pragma mark -

void ScrollThumbsUp()
{
	if (gThumbs.size() == 0)
		return;
	
	int numThumbsInWindow = gNumThumbsAcross * gNumThumbsDown;
	
	gThumbWindowPos -= numThumbsInWindow;
	if (gThumbWindowPos < 0)
		gThumbWindowPos = 0;
	
	RedrawThumbsWindow();
}

void ScrollThumbsDown()
{
	if (gThumbs.size() == 0)
		return;
	
	int numThumbsInWindow = gNumThumbsAcross * gNumThumbsDown;
	
	gThumbWindowPos += numThumbsInWindow;
	if (gThumbWindowPos > (int)gThumbs.size() - numThumbsInWindow)
		gThumbWindowPos = (int)gThumbs.size() - numThumbsInWindow;
	if (gThumbWindowPos < 0)
		gThumbWindowPos = 0;
	
	RedrawThumbsWindow();
}

void HandleClickInThumbsWindow(Point where)
{
	if (gThumbs.size() == 0)
		return;
	
	SetPort(gThumbsWindow->GetPort());
	GlobalToLocal(&where);
	
	for (int y = 0; y < gNumThumbsDown; y++)
		for (int x = 0; x < gNumThumbsAcross; x++)
		{
			Rect loc = gThumbs[0]->GetDim();
			loc.left -= gThumbSpacer / 2;
			loc.top -= gThumbSpacer / 2;
			loc.right += gThumbSpacer / 2;
			loc.bottom += gThumbSpacer / 2;
			OffsetRect(&loc, x * gThumbWidth + (x + 1) * gThumbSpacer, 30 + y * gThumbHeight + (y + 1) * gThumbSpacer);
			if (PtInRect(where, &loc))
			{
				int imageViewIndex = gThumbWindowPos + y * gNumThumbsAcross + x;
				if (imageViewIndex < gThumbs.size())
				{
					gImageViewIndex = imageViewIndex;
					ReadOneImageFromITHMBfile();
					RedrawImageWindow();
				}
				return;
			}
		}
}

#pragma mark -

//--------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	gThumbsWindow = new MainWindow(CFSTR("Thumbs"));
	gImageWindow = new MainWindow(CFSTR("Image"));
	
	gWorkBenchWindow = new MainWindow(CFSTR("WorkBench"));
	
	gWorkBenchWindow->Show();
	
	CarbonApp app;
	
	app.Run();
	return 0;
}

#pragma mark -

//--------------------------------------------------------------------------------------------
Boolean
CarbonApp::HandleCommand( const HICommandExtended& inCommand )
{
	switch ( inCommand.commandID )
	{
		case kHICommandAbout:
			if (!gAboutWindow)
				gAboutWindow = new MainWindow(CFSTR("About"));
			gAboutWindow->Show();
			gAboutWindow->BringToFront();
			gWorkBenchWindow->Select();
			return true;
			break;
		case 'WkBh':
			gWorkBenchWindow->Show();
			gWorkBenchWindow->Select();
			return true;
			break;
		case 'OpTh':
			OpenFromPhotosThumbsFolder();
			return true;
			break;
		case kHICommandOpen:
			OpenFromITHMBfile();
			return true;
			break;
		case kHICommandSave:
			SavePresentImage();
			return true;
			break;
		case 'SvAl':
			SaveAllImages();
			return true;
			break;
		case 'PgBk':
			ScrollThumbsUp();
			break;
		case 'PgFd':
			ScrollThumbsDown();
			break;
		default:
			return false;
			break;
	}
}

#pragma mark -

//--------------------------------------------------------------------------------------------

MainWindow::MainWindow(CFStringRef name) :
	TWindow(name)
{
}

MainWindow::~MainWindow()
{
	if (this == gAboutWindow)
		gAboutWindow = NULL;
}

Boolean MainWindow::HandleCommand( const HICommandExtended& inCommand )
{
	switch ( inCommand.commandID )
	{
		case 'Load':
			{
				OSErr err = GetOneFileWithPreview(0, NULL, &gITHMB_FSSpec, NULL);
				if (err != noErr)
					break;
				
				ControlID controlID = { 'WkBh', 2 };
				ControlRef controlRef;
				GetControlByID(GetWindowRef(), &controlID, &controlRef);
				
				char filename[64];
				PascalToC(gITHMB_FSSpec.name, filename);
				SetControlData(controlRef, kControlNoPart, kControlStaticTextTextTag, strlen(filename), filename);
				
				controlID.id = 8;
				GetControlByID(GetWindowRef(), &controlID, &controlRef);
				EnableControl(controlRef);
				
				controlID.id = 9;
				GetControlByID(GetWindowRef(), &controlID, &controlRef);
				EnableControl(controlRef);
				
				TestImage();
			}
			return true;
			break;
		case 'OpAl':
			{
				Hide();
				if (gITHMB_FSSpec.name[0] > 0)
					ReadThumbsFromITHMBfile();
			}
			return true;
			break;
		case 'Endn':
		case 'Mthd':
		case 'Test':
			{
				if (gITHMB_FSSpec.name[0] > 0)
					TestImage();
			}
			return true;
			break;
		default:
			return false;
			break;
	}
}

OSStatus MainWindow::HandleEvent(
	EventHandlerCallRef		inRef,
	TCarbonEvent&			inEvent )
{
	OSStatus				result = eventNotHandledErr;
	UInt32					attributes;
	HICommandExtended		command;
	
	switch (inEvent.GetClass())
	{
		case kEventClassWindow:
			switch (inEvent.GetKind())
			{
				case kEventWindowBoundsChanged:
					{
						if (this == gThumbsWindow)
						{
							Rect bounds;
							GetPortBounds(GetPort(), &bounds);
							gNumThumbsAcross = round((float)bounds.right / (float)(gThumbWidth + gThumbSpacer));
							gNumThumbsDown = round((float)(bounds.bottom - 30) / (float)(gThumbHeight + gThumbSpacer));
							SetThumbsWindowSize();
							RedrawThumbsWindow();
						}
					}
					break;
				case kEventWindowGetClickActivation:
				case kEventWindowClickContentRgn:
				case kEventWindowHandleContentClick:
					{
						if (this == gThumbsWindow)
						{
							Point where;
							inEvent.GetParameter( kEventParamMouseLocation, &where );
							HandleClickInThumbsWindow(where);
						}
						result = noErr;
					}
					break;
			}
			break;
	}
	
	if (result == eventNotHandledErr)
		result = TWindow::HandleEvent(inRef, inEvent);
	
	return result;
}

void MainWindow::TestImage()
{
	ControlID controlID = { 'WkBh', 0 };
	ControlRef controlRef;
	char buffer[9];
	Size readSize;
	OSStatus status;
	
	controlID.id = 3;
	status = GetControlByID(GetWindowRef(), &controlID, &controlRef);
	status = GetControlData(controlRef, kControlEditTextPart, kControlEditTextTextTag, 8, buffer, &readSize);
	buffer[readSize] = '\0';
	stringstream ss1(buffer);
	ss1 >> gImageWidth;
	if (gImageWidth < 1)
		gImageWidth = 1;
	else if (gImageWidth > 999999)
		gImageWidth = 999999;
	
	controlID.id = 4;
	status = GetControlByID(GetWindowRef(), &controlID, &controlRef);
	status = GetControlData(controlRef, kControlEditTextPart, kControlEditTextTextTag, 8, buffer, &readSize);
	buffer[readSize] = '\0';
	stringstream ss2(buffer);
	ss2 >> gImageHeight;
	if (gImageHeight < 1)
		gImageHeight = 1;
	else if (gImageHeight > 999999)
		gImageHeight = 999999;
	
	controlID.id = 5;
	status = GetControlByID(GetWindowRef(), &controlID, &controlRef);
	status = GetControlData(controlRef, kControlEditTextPart, kControlEditTextTextTag, 8, buffer, &readSize);
	buffer[readSize] = '\0';
	stringstream ss3(buffer);
	ss3 >> gFirstImageOffset;
	if (gFirstImageOffset < 0)
		gFirstImageOffset = 0;
	
	controlID.id = 6;
	status = GetControlByID(GetWindowRef(), &controlID, &controlRef);
	gFlipEndian = (GetControl32BitValue(controlRef) != 0);
	
	controlID.id = 7;
	status = GetControlByID(GetWindowRef(), &controlID, &controlRef);
	gMethod = GetControl32BitValue(controlRef);
	
	gThumbScalar = 9;
	do
	{
		gThumbScalar--;
		gThumbWidth = gImageWidth / gThumbScalar;
		gThumbHeight = gImageHeight / gThumbScalar;
	} while (gThumbScalar > 0 && (gThumbWidth < 32 || gThumbHeight < 32));
	
	Rect bounds;
	GetPortBounds(gThumbsWindow->GetPort(), &bounds);
	gNumThumbsAcross = round((float)bounds.right / (float)(gThumbWidth + gThumbSpacer));
	gNumThumbsDown = round((float)(bounds.bottom - 30) / (float)(gThumbHeight + gThumbSpacer));
	
	SizeWindow(gImageWindow->GetWindowRef(), gImageWidth, gImageHeight, true);
	
	Rect imageDim = { 0, 0, gImageHeight, gImageWidth };
	
	delete gImage;
	gImage = new GWorldWrapper(32, imageDim, -1);
	
	gImageViewIndex = 0;
	ReadOneImageFromITHMBfile();
	RedrawImageWindow();
	gImageViewIndex = -1;
}

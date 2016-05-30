#include "GWorldWrapper.h"
#include "WorkingDialog.h"
#include "PascalStringUtil.h"
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <time.h>

using namespace std;

//******************************************************************************
//Extern Globals

//******************************************************************************
//Global Declarations
WindowRef gThumbsWindow = NULL, gImageWindow;
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
int gThumbWidth = 720 / 8, gThumbHeight = 480 / 8;
int gThumbSpacer = 5;
int gNumThumbsAcross = 8, gNumThumbsDown = 8;
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
	
	ShowWindow(gThumbsWindow);
	SetPort(GetWindowPort(gThumbsWindow));
	Rect bounds;
	//ClipRect(&bounds);
	GetPortBounds(GetWindowPort(gThumbsWindow), &bounds);
	//EraseRect(&bounds);
	
	ForeColor(blackColor);
	TextSize(9);
	MoveTo(10, 15);
	DrawString("\pUse the '[' and ']' keys to move forward and backward through the library one page at a time.");
	MoveTo(10, 25);
	DrawString("\pClick on a thumbnail to view it full size (720x480).  Full size images can be saved as PICT files.");
	
	for (int y = 0; y < gNumThumbsDown; y++)
		for (int x = 0; x < gNumThumbsAcross; x++)
		{
			int thumbIndex = gThumbWindowPos + y * gNumThumbsAcross + x;
			if (thumbIndex >= gThumbs.size())
				return;
			
			Rect loc = gThumbs[thumbIndex]->GetDim();
			OffsetRect(&loc, x * gThumbWidth + (x + 1) * gThumbSpacer, 30 + y * gThumbHeight + (y + 1) * gThumbSpacer);
			gThumbs[thumbIndex]->CopyWorldBits(gThumbs[0]->GetDim(), loc, GetWindowPort(gThumbsWindow));
		}
}

void RedrawImageWindow()
{
	if (gImageViewIndex == -1)
		return;
	
	ShowWindow(gImageWindow);
	SelectWindow(gImageWindow);
	
	gImage->CopyWorldBits(gImage->GetDim(), GetWindowPort(gImageWindow));
}

RGBColor ConvertTwoBytesToRGBColor(unsigned char highByte, unsigned char lowByte)
{
	RGBColor rgb;
	
	unsigned char red =		(highByte >> 2) & 0b00011111;
	unsigned char green =	((highByte << 3) & 0b00011000) | ((lowByte >> 5) & 0b00000111);
	unsigned char blue =	lowByte & 0b00011111;
	
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
	
	float red =		(1.0 *		y +							1.140 *		v);
	float green =	(1.0 *		y +		-0.394 *	u +		-0.581 *	v);
	float blue =	(1.0 *		y +		2.028 *		u);
	
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
	float u = (float)((chrominance >> 4) & 0b00001111) / 16.0;
	float v = (float)(chrominance & 0b00001111) / 16.0;
	
	return ConvertYUVtoRGBColor(y, u, v);
}

RGBColor ConvertCLCLToRGBColor(bool evenPixel, long fourBytes)//unsigned char evenC, unsigned char evenL, unsigned char oddC, unsigned char oddL)
{
	/*
	unsigned char evenC = (fourBytes >> 24) & 0b11111111;
	unsigned char evenL = (fourBytes >> 16) & 0b11111111;
	unsigned char oddC = (fourBytes >> 8) & 0b11111111;
	unsigned char oddL = fourBytes & 0b11111111;
	*/
	float y = evenPixel ? ((float)((fourBytes >> 16) & 0b11111111) / 255.0) : ((float)(fourBytes & 0b11111111) / 255.0);
	float u = (float)((fourBytes >> 24) & 0b11111111) / 255.0;
	float v = (float)((fourBytes >> 8) & 0b11111111) / 255.0;
	
	return ConvertYUVtoRGBColor(y, u, v);
}

RGBColor ConvertCLCLToRGBColorXeven(long fourBytes)//unsigned char evenC, unsigned char evenL, unsigned char oddC, unsigned char oddL)
{
	/*
	unsigned char evenC = (fourBytes >> 24) & 0b11111111;
	unsigned char evenL = (fourBytes >> 16) & 0b11111111;
	unsigned char oddC = (fourBytes >> 8) & 0b11111111;
	unsigned char oddL = fourBytes & 0b11111111;
	*/
	float y = (float)((fourBytes >> 16) & 0b11111111) / 255.0;
	float u = (float)((fourBytes >> 24) & 0b11111111) / 255.0;
	float v = (float)((fourBytes >> 8) & 0b11111111) / 255.0;
	
	return ConvertYUVtoRGBColor(y, u, v);
}

RGBColor ConvertCLCLToRGBColorXodd(long fourBytes)//unsigned char evenC, unsigned char evenL, unsigned char oddC, unsigned char oddL)
{
	/*
	unsigned char evenC = (fourBytes >> 24) & 0b11111111;
	unsigned char evenL = (fourBytes >> 16) & 0b11111111;
	unsigned char oddC = (fourBytes >> 8) & 0b11111111;
	unsigned char oddL = fourBytes & 0b11111111;
	*/
	float y = (float)(fourBytes & 0b11111111) / 255.0;
	float u = (float)((fourBytes >> 24) & 0b11111111) / 255.0;
	float v = (float)((fourBytes >> 8) & 0b11111111) / 255.0;
	
	return ConvertYUVtoRGBColor(y, u, v);
}

void Draw16BitRGBImage(unsigned char* buffer, GWorldWrapper* gww)
{
	gww->StartUsingGWorld();
	
	unsigned char highByte, lowByte;
	RGBColor c;
	int xTimes2;
	int imageWidthTimes2 = gImageWidth * 2;
	for (int y = 0; y < gImageHeight; y++)
		for (int x = 0; x < gImageWidth; x++)
		{
			xTimes2 = x * 2;
			
			highByte = buffer[y * imageWidthTimes2 + xTimes2];
			lowByte = buffer[y * imageWidthTimes2 + xTimes2 + 1];
			
			c = ConvertTwoBytesToRGBColor(highByte, lowByte);
			
			SetCPixel(x, y, &c);
		}
	
	gww->FinishUsingGWorld();
}

void DrawYUV422Image(unsigned char* buffer, GWorldWrapper* gww)
{
	gww->StartUsingGWorld();
	
	unsigned char highByte, lowByte;
	RGBColor c;
	int xTimes2;
	int imageWidthTimes2 = gImageWidth * 2;
	for (int y = 0; y < gImageHeight; y++)
		for (int x = 0; x < gImageWidth; x++)
		{
			xTimes2 = x * 2;
			
			highByte = buffer[y * imageWidthTimes2 + xTimes2];
			lowByte = buffer[y * imageWidthTimes2 + xTimes2 + 1];
			
			c = ConvertCLToRGBColor(highByte, lowByte);
			
			SetCPixel(x, y, &c);
		}
	
	gww->FinishUsingGWorld();
}

void DrawInterlacedYUV422SharedChrominanceImageIntoGWorld(unsigned char* buffer, GWorldWrapper* gww)
{
	gww->StartUsingGWorld();
	
	int imagePixels = gImageWidth * gImageHeight;
	
	unsigned char evenC, evenL, oddC, oddL;
	RGBColor c;
	int xTimes2, xMinus1Times2, yOver2, yMinus1Over2;
	int imageWidthTimes2 = gImageWidth * 2;
	unsigned char* cPtr;
	long fourBytes;
	for (int y = 0; y < gImageHeight; y++)
	{
		yOver2 = y / 2;
		yMinus1Over2 = (y - 1) / 2;
		
		for (int x = 0; x < gImageWidth; x++)
		{
			xTimes2 = x * 2;
			xMinus1Times2 = (x - 1) * 2;
			
			if (y % 2 == 0)
			{
				if (x % 2 == 0)
				{
					/*evenC = buffer[yOver2 * imageWidthTimes2 + xTimes2];
					evenL = buffer[yOver2 * imageWidthTimes2 + xTimes2 + 1];
					oddC = buffer[yOver2 * imageWidthTimes2 + xTimes2 + 2];
					oddL = buffer[yOver2 * imageWidthTimes2 + xTimes2 + 3];
					
					cPtr = buffer + yOver2 * imageWidthTimes2 + xTimes2;
					evenC = *cPtr++;
					evenL = *cPtr++;
					oddC = *cPtr++;
					oddL = *cPtr++;
					*/
					fourBytes = *(long*)(buffer + yOver2 * imageWidthTimes2 + xTimes2);
					c = ConvertCLCLToRGBColorXeven(fourBytes);
				}
				else
				{
					/*evenC = buffer[yOver2 * imageWidthTimes2 + xMinus1Times2];
					evenL = buffer[yOver2 * imageWidthTimes2 + xMinus1Times2 + 1];
					oddC = buffer[yOver2 * imageWidthTimes2 + xMinus1Times2 + 2];
					oddL = buffer[yOver2 * imageWidthTimes2 + xMinus1Times2 + 3];
					
					cPtr = buffer + yOver2 * imageWidthTimes2 + xMinus1Times2;
					evenC = *cPtr++;
					evenL = *cPtr++;
					oddC = *cPtr++;
					oddL = *cPtr++;
					*/
					fourBytes = *(long*)(buffer + yOver2 * imageWidthTimes2 + xMinus1Times2);
					c = ConvertCLCLToRGBColorXodd(fourBytes);
				}
			}
			else
			{
				if (x % 2 == 0)
				{
					/*evenC = buffer[imagePixels + yMinus1Over2 * imageWidthTimes2 + xTimes2];
					evenL = buffer[imagePixels + yMinus1Over2 * imageWidthTimes2 + xTimes2 + 1];
					oddC = buffer[imagePixels + yMinus1Over2 * imageWidthTimes2 + xTimes2 + 2];
					oddL = buffer[imagePixels + yMinus1Over2 * imageWidthTimes2 + xTimes2 + 3];
					
					cPtr = buffer + imagePixels + yMinus1Over2 * imageWidthTimes2 + xTimes2;
					evenC = *cPtr++;
					evenL = *cPtr++;
					oddC = *cPtr++;
					oddL = *cPtr++;
					*/
					fourBytes = *(long*)(buffer + imagePixels + yMinus1Over2 * imageWidthTimes2 + xTimes2);
					c = ConvertCLCLToRGBColorXeven(fourBytes);
				}
				else
				{
					/*evenC = buffer[imagePixels + yMinus1Over2 * imageWidthTimes2 + xMinus1Times2];
					evenL = buffer[imagePixels + yMinus1Over2 * imageWidthTimes2 + xMinus1Times2 + 1];
					oddC = buffer[imagePixels + yMinus1Over2 * imageWidthTimes2 + xMinus1Times2 + 2];
					oddL = buffer[imagePixels + yMinus1Over2 * imageWidthTimes2 + xMinus1Times2 + 3];
					
					cPtr = buffer + imagePixels + yMinus1Over2 * imageWidthTimes2 + xMinus1Times2;
					evenC = *cPtr++;
					evenL = *cPtr++;
					oddC = *cPtr++;
					oddL = *cPtr++;
					*/
					fourBytes = *(long*)(buffer + imagePixels + yMinus1Over2 * imageWidthTimes2 + xMinus1Times2);
					c = ConvertCLCLToRGBColorXodd(fourBytes);
				}
			}
			/*
			evenC = (fourBytes >> 24) & 0b11111111;
			evenL = (fourBytes >> 16) & 0b11111111;
			oddC = (fourBytes >> 8) & 0b11111111;
			oddL = fourBytes & 0b11111111;
			*/
			//c = ConvertCLCLToRGBColor(x % 2 == 0, fourBytes);//evenC, evenL, oddC, oddL);
			
			SetCPixel(x, y, &c);
		}
	}
	
	gww->FinishUsingGWorld();
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
	
	int imagePixels = gImageWidth * gImageHeight;
	long imageBytes = imagePixels * 2;
	int numImagesInLibrary = dataLength / imageBytes;
	
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
	HideWindow(gThumbsWindow);
	HideWindow(gImageWindow);
	
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
		if (WaitNextEvent(everyEvent, &theEvent, 0, NULL))
			DoEvent(theEvent);
		if (gWorkingDialog->GetDialogStopStatus() != WorkingDialog::NOT_STOPPED)
			break;
		
		const SInt32 imageOffset = numImagesRead * imageBytes;
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
		
		DrawInterlacedYUV422SharedChrominanceImageIntoGWorld(buffer, gImage);
		
		thumb->StartUsingGWorld();
		gImage->CopyWorldBits(gImage->GetDim(), thumb->GetDim());
		thumb->FinishUsingGWorld();
		
		gThumbs.push_back(thumb);
		
		gWorkingDialog->DrawWorkingDialogProgressBar(numImagesRead + 1);
	}
	delete gWorkingDialog;
	gWorkingDialog = NULL;
	delete [] buffer;
	
	if (inRef != -1)
		err = FSClose(inRef);
	if (err != noErr)
		return;
	
	SizeWindow(gImageWindow, gImageWidth, gImageHeight, true);
	
	int thumbWindowWidth = gThumbWidth * gNumThumbsAcross + gThumbSpacer * (gNumThumbsAcross + 1);
	int thumbWindowHeight = gThumbHeight * gNumThumbsDown + gThumbSpacer * (gNumThumbsDown + 1);
	SizeWindow(gThumbsWindow, thumbWindowWidth, thumbWindowHeight + 30, true);
	
	Str255 librarySizePascal;
	NumToString(numImagesRead, librarySizePascal);
	Str255 windowTitlePascal = "\p images in library";
	PascalAppend(librarySizePascal, windowTitlePascal);
	char windowTitle[128];
	PascalToC(librarySizePascal, windowTitle);
	SetWindowTitleWithCFString(gThumbsWindow, CFStringCreateWithCString(NULL, windowTitle, kCFStringEncodingASCII));
	
	gThumbWindowPos = 0;
	RedrawThumbsWindow();
	
	if (gThumbs.size() > 0)
	{
		MenuHandle menuHandle = GetMenuHandle(129);
		EnableMenuItem(menuHandle, 4);
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
	
	int imagePixels = gImageWidth * gImageHeight;
	long imageBytes = imagePixels * 2;
	int numImagesInLibrary = dataLength / imageBytes;
	
	if (gImageViewIndex > numImagesInLibrary)
	{
		err = FSClose(inRef);
		return;
	}
	
	Rect imageDim = { 0, 0, gImageHeight, gImageWidth };
	
	unsigned char* buffer = new unsigned char[imageBytes];
	
	const SInt32 imageOffset = gImageViewIndex * imageBytes;
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
	
	DrawInterlacedYUV422SharedChrominanceImageIntoGWorld(buffer, gImage);
	
	delete [] buffer;
	
	if (inRef != -1)
		err = FSClose(inRef);
	if (err != noErr)
		return;
}

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

void SavePresentImage()
{
	Rect r = gImage->GetDim();
	PicHandle picH = OpenPicture(&r);
	gImage->CopyWorldBits(gImage->GetDim());
	ClosePicture();
	
	FSSpec fsspec;	//Unused
	OSErr err = PutPictToFile(picH, fsspec);
}

void AutoSavePresentImage(FSSpec fsspec)
{
	Str255 fileName = "\piPod Photo - ";
	
	if (gImageViewIndex < 10000)
		PascalAppend(fileName, "\p0");
	if (gImageViewIndex < 1000)
		PascalAppend(fileName, "\p0");
	if (gImageViewIndex < 100)
		PascalAppend(fileName, "\p0");
	if (gImageViewIndex < 10)
		PascalAppend(fileName, "\p0");
	
	Str255 str1;
	NumToString(gImageViewIndex, str1);
		PascalAppend(fileName, str1);
	
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
		if (WaitNextEvent(everyEvent, &theEvent, 0, NULL))
			DoEvent(theEvent);
		if (gWorkingDialog->GetDialogStopStatus() != WorkingDialog::NOT_STOPPED)
			break;
		
		gImageViewIndex = i;
		ReadOneImageFromITHMBfile();
		//RedrawImageWindow();
		//QDFlushPortBuffer(GetWindowPort(gImageWindow), NULL);
		AutoSavePresentImage(fsspec);
		
		gWorkingDialog->DrawWorkingDialogProgressBar(i + 1);
	}
	
	delete gWorkingDialog;
	gWorkingDialog = NULL;
}

void ScrollThumbsUp()
{
	int numThumbsInWindow = gNumThumbsAcross * gNumThumbsDown;
	
	gThumbWindowPos -= numThumbsInWindow;
	if (gThumbWindowPos < 0)
		gThumbWindowPos = 0;
	
	RedrawThumbsWindow();
}

void ScrollThumbsDown()
{
	int numThumbsInWindow = gNumThumbsAcross * gNumThumbsDown;
	
	gThumbWindowPos += numThumbsInWindow;
	if (gThumbWindowPos > gThumbs.size() - numThumbsInWindow)
		gThumbWindowPos = gThumbs.size() - numThumbsInWindow;
	if (gThumbWindowPos < 0)
		gThumbWindowPos = 0;
	
	RedrawThumbsWindow();
}

void HandleClickInThumbsWindow(Point where)
{
	SetPort(GetWindowPort(gThumbsWindow));
	GlobalToLocal(&where);
	
	for (int y = 0; y < gNumThumbsDown; y++)
		for (int x = 0; x < gNumThumbsAcross; x++)
		{
			Rect loc = gThumbs[0]->GetDim();
			OffsetRect(&loc, x * gThumbWidth + (x + 1) * gThumbSpacer, 30 + y * gThumbHeight + (y + 1) * gThumbSpacer);
			if (PtInRect(where, &loc))
			{
				int imageViewIndex = gThumbWindowPos + y * gNumThumbsAcross + x;
				if (imageViewIndex < gThumbs.size())
				{
					gImageViewIndex = imageViewIndex;
					ReadOneImageFromITHMBfile();
					RedrawImageWindow();
					
					MenuHandle menuHandle = GetMenuHandle(129);
					EnableMenuItem(menuHandle, 3);
				}
				return;
			}
		}
}

int main(void)
{
	ToolBoxInit();
	WindowInit();
	MenuBarInit();
	
	EventRecord theEvent;
	while (!gQuit)
	{
		if (WaitNextEvent(everyEvent, &theEvent, 4, NULL))
			DoEvent(theEvent);
	}
	
	return 0;
}
#include "GWorldWrapper.h"
#include "PascalStringUtil.h"
#include <vector.h>

//******************************************************************************
//Extern Globals

//******************************************************************************
//Global Declarations
const OSType kApplicationSignature  = FOUR_CHAR_CODE('PhDc');
const ResType kOpenResourceType = FOUR_CHAR_CODE('open');
const unsigned char* kApplicationName = "\piPod .ithmb file decoder";

//******************************************************************************
//Function Prototypes
	//	In this file
Handle CreateOpenHandle (OSType theApplicationSignature, short theNumTypes, const OSTypePtr theTypeList);
pascal void HandleNavEvent(NavEventCallbackMessage theCallBackSelector, NavCBRecPtr theCallBackParms, void *theCallBackUD);
OSErr GetPictureFromFile(FSSpecPtr theFSSpecPtr, PicHandle& thePic);
OSErr GetAppFSSpec(FSSpec *myAppSpec);
	//	In external files
		//toolbox.cpp
void DoEvent(EventRecord &eventPtr);
		//ImageProcs.cpp
void CopyPixelValuesFromImage(GWorldWrapper& image, unsigned char* pixelValues, Rect bounds);

#pragma mark -

OSErr GetOneFileWithPreview (short numTypes, OSType openTypeList[], FSSpecPtr theFSSpecPtr, void *theFilterProc)
{
	NavReplyRecord		myReply;
	NavDialogOptions	myDialogOptions;
	NavTypeListHandle	myOpenList = NULL;
	NavEventUPP			myEventUPP = NewNavEventUPP(HandleNavEvent);
	OSErr				myErr = noErr;
	
	if (theFSSpecPtr == NULL)
		return(paramErr);
	// specify the options for the dialog box
	NavGetDefaultDialogOptions(&myDialogOptions);
	myDialogOptions.dialogOptionFlags -= kNavNoTypePopup;
	myDialogOptions.dialogOptionFlags -= kNavAllowMultipleFiles;
	BlockMoveData(kApplicationName, myDialogOptions.clientName, kApplicationName[0] + 1);
	
	// create a handle to an 'open' resource
	myOpenList = (NavTypeListHandle)CreateOpenHandle(kApplicationSignature, numTypes, openTypeList);
	if (myOpenList != NULL)
		HLock((Handle)myOpenList);
	
	// prompt the user for a file
	myErr = NavGetFile(NULL, &myReply, &myDialogOptions, myEventUPP, NULL, (NavObjectFilterUPP)theFilterProc, myOpenList, NULL);
	
	if ((myErr == noErr) && myReply.validRecord)
	{
		AEKeyword		myKeyword;
		DescType		myActualType;
		Size			myActualSize = 0;
		
		// get the FSSpec for the selected file
		if (theFSSpecPtr != NULL)
			myErr = AEGetNthPtr(&(myReply.selection), 1, typeFSS, &myKeyword, &myActualType, theFSSpecPtr, sizeof(FSSpec), &myActualSize);

		NavDisposeReply(&myReply);
	}
	
	if (myOpenList != NULL) {
		HUnlock((Handle)myOpenList);
		DisposeHandle((Handle)myOpenList);
	}
	
	DisposeNavEventUPP(myEventUPP);
 
	return(myErr);
}

Handle CreateOpenHandle (OSType theApplicationSignature, short theNumTypes, const OSTypePtr theTypeList)
{
	Handle myHandle = NULL;
	
	// see if we have an 'open' resource...
	myHandle = Get1Resource('open', 128);
	if ( myHandle != NULL && ResError() == noErr )
	{
		DetachResource( myHandle );
		return myHandle;
	} else {
		myHandle = NULL;
	}
	
	// nope, use the passed in types and dynamically create the NavTypeList
	if (theTypeList == NULL)
		return myHandle;
	
	if (theNumTypes > 0)
	{
		myHandle = NewHandle(sizeof(NavTypeList) + (theNumTypes * sizeof(OSType)));
		if (myHandle != NULL)
		{
			NavTypeListHandle 	myOpenResHandle	= (NavTypeListHandle)myHandle;
			
			(*myOpenResHandle)->componentSignature = theApplicationSignature;
			(*myOpenResHandle)->osTypeCount = theNumTypes;
			BlockMoveData(theTypeList, (*myOpenResHandle)->osType, theNumTypes * sizeof(OSType));
		}
	}
	
	return myHandle;
}

pascal void HandleNavEvent(NavEventCallbackMessage theCallBackSelector, NavCBRecPtr theCallBackParms, void *theCallBackUD)
{
#pragma unused(theCallBackUD)
	WindowPtr		myWindow = NULL;	
	
	if (theCallBackSelector == kNavCBEvent) {
		switch (theCallBackParms->eventData.eventDataParms.event->what) {
			case updateEvt:
#if TARGET_OS_MAC
				// Handle Update Event
#endif
				break;
			case nullEvent:
				// Handle Null Event
				break;
		}
	}
}

OSErr PutPictToFile(PicHandle thePicture, FSSpec& theFSSpec)
{
	OSErr				anErr = noErr;
	NavEventUPP			myEventUPP = NewNavEventUPP(HandleNavEvent);
	NavReplyRecord		reply;
	NavDialogOptions	dialogOptions;
	OSType				fileTypeToSave = 'PICT';
	AEKeyword			theKeyword;
	DescType			actualType;
	Size				actualSize;
	FSSpec				documentFSSpec;
	long				inOutCount;
	short				refNum, count;
	unsigned char 		header[512];

	for (count = 0; count < 512; count++)
		header[count] = 0x00;

	anErr = NavGetDefaultDialogOptions(&dialogOptions); 
	dialogOptions.dialogOptionFlags |= kNavSelectDefaultLocation;

	anErr = NavPutFile( nil, &reply, &dialogOptions, myEventUPP, fileTypeToSave, 'ttxt', nil );

	if (anErr == noErr && reply.validRecord)
	{	
		anErr = AEGetNthPtr(&(reply.selection), 1, typeFSS, &theKeyword, &actualType,
							&documentFSSpec, sizeof(documentFSSpec), &actualSize );

		if (anErr == noErr)
		{
			anErr = FSpCreate(&documentFSSpec, 'ttxt', fileTypeToSave, smSystemScript);
			if (anErr == dupFNErr)
			{
				anErr = FSpDelete(&documentFSSpec);
				anErr = FSpCreate(&documentFSSpec, 'ttxt', fileTypeToSave, smSystemScript);
			}		// this is quick 'n' dirty or there'd be more robust handling here

			// write the file
			FSpOpenDF(&documentFSSpec, fsRdWrPerm, &refNum );
			inOutCount = 512;
			anErr = FSWrite(refNum, &inOutCount, header);		// write the header
			if (anErr == noErr)
			{
				inOutCount = GetHandleSize((Handle)thePicture);
				anErr = FSWrite(refNum, &inOutCount, *thePicture);
			}
			FSClose(refNum);
		}
		
		reply.translationNeeded = false;
		anErr = NavCompleteSave(&reply, kNavTranslateInPlace);

		NavDisposeReply(&reply);
	}
	
	theFSSpec = documentFSSpec;
	return anErr;
}

OSErr PutPictToFileAutomatically(PicHandle picH, FSSpec& theFSSpec, OSType creatorType, OSType fileType, bool usePassedInFSSpec)
{
	FSSpec	myAppSpec;
	OSErr	err, ignore;
	short	pictFileRefNum;
	long	dataLength, zeroData;
	
	Str255 s;
	if (!usePassedInFSSpec)
	{
		err = GetAppFSSpec(&myAppSpec);
		if (err != noErr)
			return err;
		
		NumToString(0, s);
		PascalCopy(s, myAppSpec.name);
	}
	
	if (!usePassedInFSSpec)
		err = FSpCreate(&myAppSpec, creatorType, fileType, smSystemScript);
	else err = FSpCreate(&theFSSpec, creatorType, fileType, smSystemScript);
	
	//if (err == -48)
	//	SysBeep(1);
	
	if (err != noErr && err != -48)		//-48 is a "replacing-file" error
		return err;
	
	if (!usePassedInFSSpec)
		err = FSpOpenDF(&myAppSpec, fsRdWrPerm, &pictFileRefNum);	//Open the file
	else err = FSpOpenDF(&theFSSpec, fsRdWrPerm, &pictFileRefNum);	//Open the file
	
	if (err != noErr)
		return err;
	
	zeroData = 0;
	dataLength = 4;
	
	for (int i = 1; i <= 512 / 4; i++)
		err = FSWrite(pictFileRefNum, &dataLength, &zeroData);	//Fill header with zeroes
	
	if (err != noErr)
		return err;
	
	dataLength = GetHandleSize(Handle(picH));
	
	HLock(Handle(picH));
	err = FSWrite(pictFileRefNum, &dataLength, Ptr(*picH));
	HUnlock(Handle(picH));
	
	ignore = FSClose(pictFileRefNum);	//Close the file
	
	//KillPicture(picH);
	
	if (!usePassedInFSSpec)
	{
		theFSSpec = myAppSpec;
		PascalCopy(s, theFSSpec.name);
	}
	
	return err;
}

OSErr PutRawToFile(char* dataBlock, long dataLength)
{
	OSErr				anErr = noErr;
	NavEventUPP			myEventUPP = NewNavEventUPP(HandleNavEvent);
	NavReplyRecord		reply;
	NavDialogOptions	dialogOptions;
	OSType				fileTypeToSave = '????';
	AEKeyword			theKeyword;
	DescType			actualType;
	Size				actualSize;
	FSSpec				documentFSSpec;
	short				refNum;

	anErr = NavGetDefaultDialogOptions(&dialogOptions); 
	dialogOptions.dialogOptionFlags |= kNavSelectDefaultLocation;
	
	anErr = NavPutFile( nil, &reply, &dialogOptions, myEventUPP, fileTypeToSave, '????', nil );

	if (anErr == noErr && reply.validRecord)
	{	
		anErr = AEGetNthPtr(&(reply.selection), 1, typeFSS, &theKeyword, &actualType,
							&documentFSSpec, sizeof(documentFSSpec), &actualSize );

		if (anErr == noErr)
		{
			anErr = FSpCreate(&documentFSSpec, '????', fileTypeToSave, smSystemScript);
			if (anErr == dupFNErr)
			{
				anErr = FSpDelete(&documentFSSpec);
				anErr = FSpCreate(&documentFSSpec, '????', fileTypeToSave, smSystemScript);
			}		// this is quick 'n' dirty or there'd be more robust handling here

			// write the file
			FSpOpenDF(&documentFSSpec, fsRdWrPerm, &refNum );
			anErr = FSWrite(refNum, &dataLength, dataBlock);
			
			FSClose(refNum);
		}
		
		reply.translationNeeded = false;
		anErr = NavCompleteSave(&reply, kNavTranslateInPlace);

		NavDisposeReply(&reply);
	}
	
	return anErr;
}

OSErr GetAppFSSpec(FSSpec *myAppSpec)
{
	ProcessSerialNumber	serial;
	ProcessInfoRec		info;
	OSErr				err = -1;
	
	serial.highLongOfPSN	= 0;
	serial.lowLongOfPSN	= kCurrentProcess;

	info.processInfoLength	= sizeof(ProcessInfoRec);
	info.processName		= NULL;
	info.processSignature	= 'KhIS';
	info.processType		= 'APPL';
	info.processAppSpec		= myAppSpec; //Points to app on exit

	err = GetProcessInformation(&serial, &info);

	return err;
}

//Runs the Choose Folder dialog and returns an FSSpec for a file
//that would/could be in that folder, requiring only a new name
//to be added to the FSSpec so that it can then be passed to FSpCreate.
OSErr ChooseFolder(FSSpec &myAppSpec, char* message)
{
	NavDialogCreationOptions inOptions;
	NavEventUPP myEventUPP = NewNavEventUPP(HandleNavEvent);
	NavDialogRef outDialog;
	NavReplyRecord outReply;
	OSStatus status;
	OSErr err;
	
	//Run the Choose Folder dialog
	status = NavGetDefaultDialogCreationOptions(&inOptions);
	inOptions.message = CFStringCreateWithCString(NULL, message, kCFStringEncodingASCII);
	status = NavCreateChooseFolderDialog(&inOptions, myEventUPP, NULL, NULL, &outDialog);
	status = NavDialogRun(outDialog);
	status = NavDialogGetReply(outDialog, &outReply);
	NavDialogDispose(outDialog);
	
	//Get the AEDesc from the reply
	AEDesc actualDesc;
	err = AECoerceDesc(&outReply.selection, typeFSRef, &actualDesc);
	
	//Get the parent of the chosen folder from the AEdesc
	FSRef fileRefParent;
	err = AEGetDescData(&actualDesc, &fileRefParent, sizeof(FSRef));
	
	AEDisposeDesc(&actualDesc);
	
	//Fill in the FSSpec using the parent of the chosen folder
	//This is basically my way of filling in the vRefNum of the FSSpec
	//After this is done, I just need the parID for the FSSPec which will
	//the dir id of the chosen folder, not its parent
	err = FSGetCatalogInfo(&fileRefParent, kFSCatInfoNone, NULL, NULL, &myAppSpec, NULL);
	
	//Create a CInfoPBRec and get the id of the chosen folder
	//According to http://developer.apple.com/documentation/Carbon/Reference/File_Manager/file_manager/function_group_1.html#//apple_ref/c/func/PBGetCatInfoSync
	//I need to set:
	//	ioFDirIndex = 0
	//	ioNamePtr = folder name
	//	ioDrDirID = folder's parent id
	//	ioVRefNum = volume ref
	CInfoPBRec paramBlock;
	paramBlock.dirInfo.ioFDirIndex = 0;
	paramBlock.dirInfo.ioNamePtr = myAppSpec.name;
	paramBlock.dirInfo.ioDrDirID = myAppSpec.parID;
	paramBlock.dirInfo.ioVRefNum = myAppSpec.vRefNum;
	err = PBGetCatInfoSync(&paramBlock);
	
	//Fill in the dir id of the chosen folder as the par id of the FSSpec.
	myAppSpec.parID = paramBlock.dirInfo.ioDrDirID;
	
	//Now we're done.  The name of the FSSpec can be set to that of a file
	//that is to be generated and can be passed to FSpCreate
	
	return err;
}
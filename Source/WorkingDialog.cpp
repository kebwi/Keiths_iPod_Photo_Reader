#include "WorkingDialog.h"
#include "PascalStringUtil.h"
#include <string>
#include <sstream>

#include <iostream>
using namespace std;

//******************************************************************************
//Extern Globals
extern WorkingDialog *gWorkingDialog;

//******************************************************************************
//Global Declarations
static WorkingDialog::RunStatus sLastRunStatus = WorkingDialog::DONE_WITHOUT_CANCEL;

//******************************************************************************
//Function Prototypes
	//	In this file
	//	In external files

WorkingDialog::WorkingDialog(int theMaxValue, bool theButtonStoppable, bool theArrowStoppable, bool drawImmediately) :
	maxValue(theMaxValue), value(0), buttonStoppable(theButtonStoppable), arrowStoppable(theArrowStoppable),
	runStatus(RUNNING)
{
	if (maxValue < 1)
		maxValue = 1;	//Avoid divide by zero error in the draw routine
	
	sLastRunStatus = runStatus;
	
	IBNibRef theNib;
	OSStatus err = CreateNibReference(CFSTR("main"), &theNib);
	
	string instanceName;
	if (buttonStoppable)
		instanceName = "WorkingWithButton";
	else instanceName = "WorkingWithoutButton";
	
	CFStringRef windowInstanceNameCFStrRef = CFStringCreateWithCString(NULL, instanceName.c_str(), kCFStringEncodingASCII);
	CreateWindowFromNib(theNib, windowInstanceNameCFStrRef, &window);
	InstallStandardEventHandler(GetWindowEventTarget(window));
	
	eventHandlerUPP = NewEventHandlerUPP(EventHandler);
	EventTypeSpec dialogEvents[] = {{kEventClassControl, kEventControlHit},
									{kEventClassWindow, kEventWindowDrawContent},
									{kEventClassWindow, kEventWindowActivated},
									{kEventClassKeyboard, kEventRawKeyDown},
									{kEventClassKeyboard, kEventRawKeyRepeat},
									{kEventClassKeyboard, kEventRawKeyUp}};
	OSStatus status = InstallWindowEventHandler(window, eventHandlerUPP, GetEventTypeCount(dialogEvents),
												dialogEvents, (void*)this, NULL);
	
	ControlRef controlRef;
	ControlID controlID = { '????', 4 };
	GetControlByID(window, &controlID, &controlRef);
	stringstream ss;
	ss << maxValue;
	SetControlData(controlRef, kControlNoPart, kControlStaticTextTextTag, ss.str().length(), ss.str().c_str());
	
	if (true)//drawImmediately)
		ShowWindow(window);
}

WorkingDialog::~WorkingDialog()
{
	if (runStatus == RUNNING)
		runStatus = sLastRunStatus = DONE_WITHOUT_CANCEL;
	
	DisposeWindow(window);
	
	assert(gWorkingDialog == this);
	gWorkingDialog = NULL;
	
	DisposeEventHandlerUPP(eventHandlerUPP);
}

//static
WorkingDialog::RunStatus WorkingDialog::GetLastDialogRunStatus()
{
	return sLastRunStatus;
}

//static
void WorkingDialog::Reset()
{
	sLastRunStatus = DONE_WITHOUT_CANCEL;
}

void WorkingDialog::IncrementValueAndDrawWorkingDialogProgressBar()
{
	value++;
	DrawWorkingDialogProgressBar();
}

void WorkingDialog::DrawWorkingDialogProgressBar(int presValue)
{
	value = presValue;
	DrawWorkingDialogProgressBar();
}

void WorkingDialog::DrawWorkingDialogProgressBar()
{
	ControlRef controlRef;
	ControlID controlID = { '????', 0 };
	
	controlID.id = 2;
	GetControlByID(window, &controlID, &controlRef);
	SetControl32BitValue(controlRef, ((value * 200) / maxValue));
	
	controlID.id = 3;
	GetControlByID(window, &controlID, &controlRef);
	stringstream ss;
	ss << value;
	SetControlData(controlRef, kControlNoPart, kControlStaticTextTextTag, ss.str().length(), ss.str().c_str());
	
	HIWindowFlush(window);
}

//static
pascal OSStatus WorkingDialog::EventHandler(EventHandlerCallRef nextHandler, EventRef event, void *workingDialog)
{
	WorkingDialog *wd = (WorkingDialog*)workingDialog;
	
	bool closeDialog = false;
	OSStatus result = wd->HandleEvent(event, closeDialog);
	if (closeDialog)
		delete wd;
	
	return result;
}

OSStatus WorkingDialog::HandleEvent(EventRef event, bool &closeDialog)
{
	OSStatus result = eventNotHandledErr;
	UInt32 eventClass = GetEventClass(event);
	UInt32 eventKind = GetEventKind(event);
	
	switch (eventClass)
	{
		case kEventClassControl:
			switch (eventKind)
			{
				case kEventControlHit:
					ControlRef controlHit = NULL;
					ControlID controlID = { '????', 0 };
					GetEventParameter(event, kEventParamDirectObject, typeControlRef, NULL, 
										sizeof(ControlRef), NULL, &controlHit);
					
					OSStatus status = GetControlID(controlHit, &controlID);
					if (HandleDialogItemHit(controlID.id, closeDialog))
						result = noErr;
					break;
			}
			break;
		case kEventClassWindow:
			switch (eventKind)
			{
				case kEventWindowDrawContent:	//Contents need to be redrawn
				case kEventWindowActivated:
					break;
			}
			break;
		case kEventClassKeyboard:
			switch (eventKind)
			{
				case kEventRawKeyDown:
				case kEventRawKeyRepeat:
					{
						char theChar;
						OSErr err = GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar,
													NULL, sizeof(char), NULL, &theChar);
						
						UInt32 modifierKeys;
						err = GetEventParameter(event, kEventParamKeyModifiers, typeUInt32,
												NULL, sizeof(UInt32), NULL, &modifierKeys);
						
						bool largeArrow = ((modifierKeys & shiftKey) != 0);
						
						switch (theChar)
						{
							case 28:
								if (arrowStoppable)
								{
									runStatus = sLastRunStatus = (largeArrow ? LARGE_ARROW_LEFT : SMALL_ARROW_LEFT);
									closeDialog = true;
								}
								result = noErr;
								break;
							case 29:
								if (arrowStoppable)
								{
									runStatus = sLastRunStatus = (largeArrow ? LARGE_ARROW_RIGHT : SMALL_ARROW_RIGHT);
									closeDialog = true;
								}
								result = noErr;
								break;
							case 30:
								if (arrowStoppable)
								{
									runStatus = sLastRunStatus = (largeArrow ? LARGE_ARROW_UP : SMALL_ARROW_UP);
									closeDialog = true;
								}
								result = noErr;
								break;
							case 31:
								if (arrowStoppable)
								{
									runStatus = sLastRunStatus = (largeArrow ? LARGE_ARROW_DOWN : SMALL_ARROW_DOWN);
									closeDialog = true;
								}
								result = noErr;
								break;
						}
					}
					break;
				case kEventRawKeyUp:
					{
						char theChar;
						OSErr err = GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar,
													NULL, sizeof(char), NULL, &theChar);
						
						UInt32 modifierKeys;
						err = GetEventParameter(event, kEventParamKeyModifiers, typeUInt32,
												NULL, sizeof(UInt32), NULL, &modifierKeys);
					}
					break;
			}
			break;
	}
	
	return result;
}

bool WorkingDialog::HandleDialogItemHit(int itemHit, bool &closeDialog)
{
	ControlRef controlRef;
	ControlID controlID = { '????', 0 };
	OSStatus status;
	
	controlID.id = itemHit;
	status = GetControlByID(window, &controlID, &controlRef);
	
	if (itemHit == 1000)	//OK button
	{
		runStatus = sLastRunStatus = STOP_BUTTON;
		closeDialog = true;
		return true;
	}
	
	return false;
}

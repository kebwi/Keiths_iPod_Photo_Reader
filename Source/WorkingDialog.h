#ifndef __WORKING_DIALOG__
#define __WORKING_DIALOG__

#define kMoveToFront (WindowPtr) -1L

class WorkingDialog
{
	public:
		enum RunStatus { RUNNING,
						DONE_WITHOUT_CANCEL,
						STOP_BUTTON,
						SMALL_ARROW_LEFT, SMALL_ARROW_RIGHT, SMALL_ARROW_UP, SMALL_ARROW_DOWN,
						LARGE_ARROW_LEFT, LARGE_ARROW_RIGHT, LARGE_ARROW_UP, LARGE_ARROW_DOWN };
		
	private:
		EventHandlerUPP eventHandlerUPP;
		
		DialogPtr theDialog;
		WindowRef window;
		int maxValue, value;
		bool buttonStoppable;
		bool arrowStoppable;
		RunStatus runStatus;
		
		WorkingDialog();	//Declare away
		
		virtual OSStatus HandleEvent(EventRef event, bool &closeDialog);
		bool HandleDialogItemHit(int itemHit, bool &closeDialog);
		
	public:
		WorkingDialog(int theMaxValue, bool theButtonStoppable = true, bool theArrowStoppable = false, bool drawImmediately = true);
		~WorkingDialog();
		
		DialogPtr GetDialog()				{ return theDialog; }
		RunStatus GetDialogRunStatus()		{ return runStatus; }
		static RunStatus GetLastDialogRunStatus();
		static void Reset();
		
		void IncrementValueAndDrawWorkingDialogProgressBar();
		void DrawWorkingDialogProgressBar(int presValue);
		void DrawWorkingDialogProgressBar();
		
		void HandleDialogEvent(EventRecord &eventPtr, short itemHit);
		
		static pascal OSStatus EventHandler(EventHandlerCallRef nextHandler, EventRef event, void *workingDialog);
};

#endif


/* Copyright (c) Dietmar Planitzer, 1998. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */


#import <Cocoa/Cocoa.h>


@interface GLUTClipboardController : NSWindowController
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
   <NSWindowDelegate>
#endif
{
   IBOutlet NSScrollView *		_scrollView;
   IBOutlet NSTextField *		_infoText;
            NSTimer *			_updateTimer;
            int					_lastChangeCount;		/* PBoards change count when we created the last view */
            BOOL					_firstTime;
}

- (IBAction)toggleWindow:(id)sender;

- (BOOL)isClipboardWindowVisible;
- (void)updateClipboardWindow;

@end

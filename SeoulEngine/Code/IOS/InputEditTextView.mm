//
//  InputEditTextView.m
//  IOS
//
//  Copyright (c) Demiurge Studios, Inc.
//  
//  This source code is licensed under the MIT license.
//  Full license details can be found in the LICENSE file
//  in the root directory of this source tree.
//

#include "Core.h"
#include "IOSAppDelegate.h"
#include "InputManager.h"
#include "IOSEngine.h"
#include "ITextEditable.h"
#include "LocManager.h"

#import "InputEditTextView.h"
#import <UIKit/UIKit.h>
#import <UIKit/UITextField.h>

using namespace Seoul;

static const double kWaitToDisplayView = 0.1;
static const double kButtonCornerRadius = 5;

/**
 * Delegate, used to capture change events from the keyboard text box
 * and handle filtering and length restrictions.
 */
@interface IOSKeyboardTextFieldDelegate : NSObject < UITextFieldDelegate >
-(BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string;
-(BOOL)textFieldShouldReturn:(UITextField*)textField;
@end

@implementation IOSKeyboardTextFieldDelegate
-(BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
	if (IOSEngine::Get())
	{
		// Apply most filtering constraints.
		IOSEditTextSettings const settings = IOSEngine::Get()->GetEditTextSettings();
		String const sOriginalText(string);
		String sFilteredString(sOriginalText);
		ITextEditable::TextEditableApplyConstraints(settings.m_Constraints, sFilteredString);
		if (sFilteredString != sOriginalText)
		{
			return NO;
		}

		// Specially handle max characters constraint, if specified.
		if (settings.m_Constraints.m_iMaxCharacters > 0)
		{
			// Explicitly handle max length constraints.
			Int const iOldLength = (Int)[textField.text length];
			Int const iReplacementLength = (Int)[string length];
			Int const iRangeLength = (Int)range.length;
			Int const iNewLength = (iOldLength - iRangeLength + iReplacementLength);

			return (iNewLength >= 0 && iNewLength <= settings.m_Constraints.m_iMaxCharacters);
		}
	}

	return YES;
}

-(BOOL)textFieldShouldReturn:(UITextField*)textField
{
	if([[[UIDevice currentDevice] systemVersion] floatValue] >= 8.0)
	{
		[((InputEditTextView*)[textField superview]) textActionFinished:true];
		return YES;
	}
	else
	{
		return NO;
	}
}
@end

@implementation InputEditTextView

- (IBAction)onInputButtonSubmit:(UIButton *)sender {
	[self textActionFinished:true];
}

-(void) textActionFinished:(BOOL) applyText
{
	if (IOSEngine::Get())
	{
		ITextEditable* pTextEditable = IOSEngine::Get()->GetTextEditable();
		if (pTextEditable)
		{
			// Ok button, submit text.
			if (applyText)
			{
				String const sText([self.inputText text]);

				// Apply the text and dismiss.
				pTextEditable->TextEditableApplyText(sText);
			}

			// Tell the text editing API that we're done editing.
			pTextEditable->TextEditableStopEditing();
		}
	}
	// Run on main thread
	dispatch_async(dispatch_get_main_queue(), ^{
		IOSAppDelegate *pAppDelegate = (IOSAppDelegate*)[UIApplication sharedApplication].delegate;
		[pAppDelegate hideInputEditTextView];
	});
}

- (void)initView
{
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(adjustForKeyboard:) name:UIKeyboardWillShowNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onKeyboardDismissed:) name:UIKeyboardWillHideNotification object:nil];

	// some keyboards, like international keyboards, or ipads with a keyboard attached, can change frame once up.
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(adjustForKeyboard:) name:UIKeyboardWillChangeFrameNotification object:nil];

	self.inputButton.accessibilityIdentifier = @"InputEditTextView_InputButton";

	// Hookup a delegate to the text box, and tweak its configuration.
	[self.inputText setDelegate:[[IOSKeyboardTextFieldDelegate alloc] init]];
	if([[[UIDevice currentDevice] systemVersion] floatValue] >= 8.0)
	{
		self.inputText.returnKeyType = UIReturnKeyDone;
		[self.inputText setEnablesReturnKeyAutomatically:YES];
	}

	[self.inputText setAutocapitalizationType:UITextAutocapitalizationTypeSentences];
	[self.inputText setAutocorrectionType:UITextAutocorrectionTypeDefault];
	[self.inputText setSpellCheckingType:UITextSpellCheckingTypeDefault];

	IOSEditTextSettings const settings = IOSEngine::Get()->GetEditTextSettings();

	if(settings.m_bAllowNonLatinKeyboard)
	{
		[self.inputText setKeyboardType:UIKeyboardTypeDefault];
	}
	else
	{
		[self.inputText setKeyboardType:UIKeyboardTypeASCIICapable];
	}

	[self.inputText becomeFirstResponder];

	[self.inputText setText: settings.m_sText.ToNSString()];

	self.inputButton.layer.cornerRadius = kButtonCornerRadius;

	// view starts invisible.
	// Wait some short period of time before displaying to avoid Mr. Poppins.
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{

		[NSThread sleepForTimeInterval:kWaitToDisplayView];
		dispatch_async(dispatch_get_main_queue(), ^{
			// make visible.
			if (self != nil)
			{
				self.alpha = 1;
			}
		});
	});

#if SEOUL_AUTO_TESTS
	if (g_bRunningAutomatedTests)
	{
		// Automatically dismiss the input view if automated tests are running.
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(Int64)(0.25 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
			[self.inputButton sendActionsForControlEvents:UIControlEventTouchUpInside];
		});
	}
#endif // /SEOUL_AUTO_TESTS
}

- (void)onKeyboardDismissed:(NSNotification *)notification
{
	[self textActionFinished:false];
}

- (void)adjustForKeyboard:(NSNotification *)notification
{
	NSDictionary* keyboardInfo = [notification userInfo];
	NSValue* keyboardFrameBegin = [keyboardInfo valueForKey:UIKeyboardFrameEndUserInfoKey];
	CGRect keyboardFrameBeginRect = [keyboardFrameBegin CGRectValue];

	NSNumber *duration = [notification.userInfo objectForKey:UIKeyboardAnimationDurationUserInfoKey];
	UIViewAnimationCurve keyboardTransitionAnimationCurve;
	[[notification.userInfo valueForKey:UIKeyboardAnimationCurveUserInfoKey]
	 getValue:&keyboardTransitionAnimationCurve];

	double doubleDuration = [duration doubleValue];
	[UIView beginAnimations:nil context:nil];

	[UIView setAnimationDuration:doubleDuration];
	[UIView setAnimationCurve:keyboardTransitionAnimationCurve];
	[UIView setAnimationBeginsFromCurrentState:YES];

	// slide UIView containing textbox up equal to the height of the keyboard.
	CGRect rect = self.frame;
	rect.origin.y = -keyboardFrameBeginRect.size.height;
	self.frame = rect;

	[UIView commitAnimations];
}

- (IBAction)onBackgroundClicked:(UIButton *)sender
{
	[self textActionFinished:false];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];

	[_inputText setDelegate:nil];
    [_inputText release];
    [_inputButton release];
    [super dealloc];
}
@end

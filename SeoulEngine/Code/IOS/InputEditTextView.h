//
//  InputEditTextView.h
//  IOS
//
//  Copyright (c) Demiurge Studios, Inc.
//  
//  This source code is licensed under the MIT license.
//  Full license details can be found in the LICENSE file
//  in the root directory of this source tree.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface InputEditTextView : UIView
@property (retain, nonatomic) IBOutlet UITextField *inputText;
@property (retain, nonatomic) IBOutlet UIButton *inputButton;

-(void)initView;
-(void)textActionFinished:(BOOL)applyText;
-(void)adjustForKeyboard:(NSNotification *)notification;
-(void)onKeyboardDismissed:(NSNotification *)notification;
@end

NS_ASSUME_NONNULL_END

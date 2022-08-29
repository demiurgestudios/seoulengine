//
//  InputEditTextViewController.m
//  IOS
//
//  Copyright (c) Demiurge Studios, Inc.
//  
//  This source code is licensed under the MIT license.
//  Full license details can be found in the LICENSE file
//  in the root directory of this source tree.
//

#import "InputEditTextView.h"
#import "InputEditTextViewController.h"

@interface InputEditTextViewController ()

@property (nonatomic, strong) InputEditTextView * viewRef;

@end

@implementation InputEditTextViewController
- (id)init {
	[super init];
	
	self.viewRef = [[[NSBundle mainBundle] loadNibNamed:@"InputEditTextView" owner:nil options:nil] objectAtIndex:0];
	
	self.viewRef.frame = self.view.bounds;
	[self.view addSubview:self.viewRef];
	
	return self;
}

-(void)viewDidAppear:(BOOL)bAnimated
{
	[super viewDidAppear:bAnimated];
	[self.viewRef initView];
}

- (void)loadView
{
	[super loadView];

    self.definesPresentationContext = YES; //self is presenting view controller
	[self.view setBackgroundColor:[UIColor clearColor]];
    if (@available(iOS 8.0, *))
    {
        self.modalPresentationStyle = UIModalPresentationOverCurrentContext;
    }
    else
    {
        self.modalPresentationStyle = UIModalPresentationCurrentContext;
    }
}

@end

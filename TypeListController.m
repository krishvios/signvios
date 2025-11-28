/*

File: TypeListController.m
Abstract: 
 View controller for changing the type of a specific item. Displays a list of
types with a checkmark accessory
 view indicating the designated row. Selection of a row changes the type and
navigates back to the previous view.
 

Version: 1.0

Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple Inc.
("Apple") in consideration of your agreement to the following terms, and your
use, installation, modification or redistribution of this Apple software
constitutes acceptance of these terms.  If you do not agree with these terms,
please do not use, install, modify or redistribute this Apple software.

In consideration of your agreement to abide by the following terms, and subject
to these terms, Apple grants you a personal, non-exclusive license, under
Apple's copyrights in this original Apple software (the "Apple Software"), to
use, reproduce, modify and redistribute the Apple Software, with or without
modifications, in source and/or binary forms; provided that if you redistribute
the Apple Software in its entirety and without modifications, you must retain
this notice and the following text and disclaimers in all such redistributions
of the Apple Software.
Neither the name, trademarks, service marks or logos of Apple Inc. may be used
to endorse or promote products derived from the Apple Software without specific
prior written permission from Apple.  Except as expressly stated in this notice,
no other rights or licenses, express or implied, are granted by Apple herein,
including but not limited to any patent rights that may be infringed by your
derivative works or by other works in which the Apple Software may be
incorporated.

The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
COMBINATION WITH YOUR PRODUCTS.

IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR
DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF
APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Copyright (C) 2008 Apple Inc. All Rights Reserved.

*/

#import "TypeListController.h"
#import "Utilities.h"
#import "AppDelegate.h"
#import "ntouch-Swift.h"

@implementation TypeListController

@synthesize types, editingItem;

- (void)viewDidLoad {
	[super viewDidLoad];
//	self.tableView.contentInset = (self.navigationController.navigationBar.translucent) ? UIEdgeInsetsMake(64.0, 0, 0, 0) : UIEdgeInsetsMake(0, 0, 0, 0);
	
	self.tableView.rowHeight = UITableViewAutomaticDimension;
	self.tableView.estimatedRowHeight = 44;
}

- (UIStatusBarStyle)preferredStatusBarStyle {
	return Theme.statusBarStyle;
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	
	// Remove any previous selection.
	if ([editingItem objectForKey:kTypeListTitleKey])
		self.title = Localize([editingItem objectForKey:kTypeListTitleKey]);
	else
		self.title = Localize(@"Types");
	NSIndexPath *tableSelection = [self.tableView indexPathForSelectedRow];
	[self.tableView deselectRowAtIndexPath:tableSelection animated:NO];
	[self.tableView reloadData];

	NSString *text = [editingItem valueForKey:kTypeListTextKey];
	if (text.length) {
		NSInteger row;
		NSIndexPath *selection = nil;
		if ([editingItem objectForKey:kTypeListFieldKey]) {
			for (row = 0; row < [types count]; row++)
				if ([text isEqualToString:[[types objectAtIndex:row] valueForKey:[editingItem objectForKey:kTypeListFieldKey]]]) {
					selection = [NSIndexPath indexPathForRow:row inSection:0];
					break;
				}
		}
		else {
			for (row = 0; row < [types count]; row++)
				if ([text isEqualToString:[types objectAtIndex:row]]) {
					selection = [NSIndexPath indexPathForRow:row inSection:0];
					break;
				}
		}
		if (!selection)
			selection = [NSIndexPath indexPathForRow:0 inSection:0];
		[editingItem setValue:[NSNumber numberWithInteger:selection.row] forKey:kTypeListIndexKey];
		// this is to workaround a problem with translucent navigation bars
		if (!self.navigationController.navigationBar.translucent)
			[self.tableView scrollToRowAtIndexPath:selection atScrollPosition:UITableViewScrollPositionMiddle animated:YES];
	}
}

// Selection updates the editing item's type and navigates to the previous view controller.
- (NSIndexPath *)tableView:(UITableView *)aTableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	if ([editingItem objectForKey:kTypeListFieldKey])
		[editingItem setValue:[[types objectAtIndex:indexPath.row] valueForKey:[editingItem objectForKey:kTypeListFieldKey]] forKey:kTypeListTextKey];
	else
		[editingItem setValue:[types objectAtIndex:indexPath.row] forKey:kTypeListTextKey];
	[editingItem setValue:[NSNumber numberWithInteger:indexPath.row] forKey:kTypeListIndexKey];
	
	[self.tableView reloadData];
	return indexPath;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[self.tableView deselectRowAtIndexPath:indexPath animated:YES];

	if (self.selectionBlock) {
		self.selectionBlock(self.editingItem);
	}

	[self popViewController];
}

- (void)popViewController {
	[self.navigationController popViewControllerAnimated:YES];
}

// The table uses standard UITableViewCells. The text for a cell is simply the string value of the matching type.
- (UITableViewCell *)tableView:(UITableView *)aTableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	ThemedTableViewCell *cell = [self.tableView dequeueReusableCellWithIdentifier:@"TypeCell"];
	if (!cell)
		cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"TypeCell"];
	cell.textLabel.lineBreakMode = NSLineBreakByWordWrapping;
    cell.textLabel.numberOfLines = 0;
	
	NSString *text;
	
	if ([editingItem objectForKey:kTypeListFieldKey])
		text = [[types objectAtIndex:indexPath.row] valueForKey:[editingItem objectForKey:kTypeListFieldKey]];
	else
		text = [types objectAtIndex:indexPath.row];
	
	cell.textLabel.text = Localize(text);
	
	if ([cell.textLabel.text isEqualToString:[editingItem valueForKey:kTypeListTextKey]]) {
		cell.accessoryView = nil;
		cell.accessoryType = UITableViewCellAccessoryCheckmark;
	}
	else {
		cell.accessoryType = UITableViewCellAccessoryNone;
		cell.accessoryView = [[UIView alloc] initWithFrame:CGRectMake(0,0, 20, 20)];
	}
	return cell;
}

// The table has one row for each possible type.
- (NSInteger)tableView:(UITableView *)aTableView numberOfRowsInSection:(NSInteger)section {
	return [types count];
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	return [self tableView:tableView viewForHeaderInSection:section].frame.size.height;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	return [self viewController:self viewForHeaderInSection:section];
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    
	if ([editingItem objectForKey:kTypeListHeaderKey])
		return [editingItem objectForKey:kTypeListHeaderKey];
    else
		return nil;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
	if ([editingItem objectForKey:kTypeListFooterKey])
		return [editingItem objectForKey:kTypeListFooterKey];
	else
		return nil;
}

-(void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
	 {
		 [self.tableView reloadData];
	 } completion:nil];
	
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

@end

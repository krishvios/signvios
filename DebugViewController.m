//
//  DebugViewController.m
//  ntouch
//
//  Created by DBaker on 1/21/14.
//  Copyright (c) 2014 Sorenson Communications. All rights reserved.
//

#import "DebugViewController.h"
#import "ntouch-Swift.h"

@interface DebugViewController ()

@end

enum {
	debugTools,
	debugToolsSection
};

@implementation DebugViewController

@synthesize debugArry;
@synthesize editingItem;
@synthesize searchingArry;
@synthesize searchBar;

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return debugToolsSection;
}

- (NSInteger)countLines
{
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	[debugArry removeAllObjects];
	// Reads the trace.conf file then counts the number of lines in it and adds the lines to the debugArray.
	NSString *documentsDirectory = [paths objectAtIndex:0];
	NSString *filePath = [documentsDirectory stringByAppendingPathComponent:@"trace.conf"];
	NSString *content = [NSString stringWithContentsOfFile:filePath encoding:NSUTF8StringEncoding error:NULL];
	
	unsigned numberOfLines, index, stringLength = [content length];
	
	for (index = 0, numberOfLines = 0; index < stringLength; numberOfLines++) {
		NSMutableString *debugString = [[NSMutableString alloc] init];
		[debugString setString:[content substringFromIndex:index]];
		NSRange debugStringRange = [debugString rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]];
		[debugArry addObject:[debugString substringWithRange:NSMakeRange(0, debugStringRange.location)]];
		index = NSMaxRange([content lineRangeForRange:NSMakeRange(index, 0)]);
	}
	
	return numberOfLines;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if (searchingArry) {
		return debugArry.count;
	}
	return [self countLines];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *CellIdentifier = @"Cell";
	ThemedTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
	if (cell == nil) {
		cell = [[ThemedTableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier];
	}
	// Populates the table with entries from the debugArray
	cell.textLabel.text = [NSString stringWithString: [debugArry objectAtIndex:indexPath.row]];

	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	
	// Displays a list 0-10 for the user to selest a debug setting for the selected item.
	NSString *cellLabelText = [NSString stringWithString: [debugArry objectAtIndex:indexPath.row]];
	NSRange debugRange = [cellLabelText rangeOfCharacterFromSet:[NSCharacterSet whitespaceCharacterSet]];
	
	TypeListController *controller = [[TypeListController alloc] initWithStyle:UITableViewStyleGrouped];
	editingItem = [[NSMutableDictionary alloc] init];
	[editingItem setValue:[cellLabelText substringToIndex:debugRange.location] forKey:kTypeListTitleKey];
	[editingItem setValue:@"Value" forKey:kTypeListTextKey];

	controller.types = [NSArray arrayWithObjects:
							  @"0",
							  @"1",
							  @"2",
							  @"3",
							  @"4",
							  @"5",
							  @"6",
							  @"7",
							  @"8",
							  @"9",
							  @"10",
							   nil];
	controller.editingItem = editingItem;
	[self.navigationController pushViewController:controller animated:YES];
	
}

- (void)search
{
	NSString *searchString = searchBar.text;
	
	NSString *element;
	NSMutableArray *searchArray = [[NSMutableArray alloc] init];
		
	for (element in debugArry)
	{
		NSRange range = [element rangeOfString:searchString
                                     options:NSCaseInsensitiveSearch];
		
		if (range.length > 0) {
			[searchArray addObject:element];
		}
	}
	[debugArry removeAllObjects];
	if ([searchArray count] > 0) {
		for (id object in searchArray)
		{
			[debugArry addObject:object];
		}
		searchingArry = YES;
	}
}

-(void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
	[self search];
	[self.tableView reloadData];
}

- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar
{
	self.searchBar.showsCancelButton = NO;
	[self.searchBar resignFirstResponder];
}

- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar
{
	self.searchBar.showsCancelButton = YES;
	[self search];
	[self.tableView reloadData];
}

- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar
{
	[self search];
	[self.tableView reloadData];
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
	self.searchBar.text = nil;
	self.searchBar.showsCancelButton = NO;
	searchingArry = NO;
	[self.tableView reloadData];
	[self.searchBar resignFirstResponder];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	self.searchBar.barStyle = Theme.searchBarStyle;
	self.searchBar.keyboardAppearance = Theme.keyboardAppearance;
	
	if (editingItem) {
		// the TypeListViewController just returned
		NSString *name = [editingItem valueForKey:kTypeListTextKey];
		NSString *title = [editingItem valueForKey:kTypeListTitleKey];
		if(![name isEqualToString:@"Value"]) { // Only set the trace flag if a user selected a new value
			[[SCIVideophoneEngine sharedEngine] setTraceFlagWithName:title value:[name intValue]];
			[[SCIVideophoneEngine sharedEngine] saveFile];
		}
		searchingArry = NO;
		[self.tableView reloadData];
	}
}

- (IBAction)Back
{
	[self dismissViewControllerAnimated:YES completion:nil];
}

- (void)viewDidLoad {
	[super viewDidLoad];
	self.tableView.allowsSelection = YES;
	self.navigationItem.title = [NSString stringWithFormat:@"Debug Flags"];
	
	if ([self respondsToSelector:@selector(edgesForExtendedLayout)])
		self.edgesForExtendedLayout = UIRectEdgeNone;
	
	debugArry = [[NSMutableArray alloc] init];

	// Add Search bar to table
	self.searchBar = [[UISearchBar alloc] initWithFrame:CGRectMake(0, 0, 320, 44)];
	self.searchBar.delegate = self;
	self.tableView.tableHeaderView = self.searchBar;
	
	self.view.backgroundColor = Theme.backgroundColor;
	
	UIBarButtonItem *backButton = [[UIBarButtonItem alloc] initWithTitle:@"Back" style: UIBarButtonItemStylePlain target:self action:@selector(Back)];
	self.navigationItem.leftBarButtonItem = backButton;
	self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
}

@end

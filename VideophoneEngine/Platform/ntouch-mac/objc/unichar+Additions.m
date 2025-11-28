//
//  unichar+Additions.m
//  ntouchMac
//
//  Created by Nate Chandler on 7/24/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "unichar+Additions.h"

size_t unistrlen(const unichar *unichars)
{
	size_t length = 0;
	
	while (0 != unichars[length]) {
		length++;
	}
	
	return length;
}
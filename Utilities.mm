//
//  Utilities.m
//  Common Utilities
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2009 - 2011 Sorenson Communications, Inc. -- All rights reserved
//

#import "Utilities.h"
#import <CoreGraphics/CoreGraphics.h>
#import "AppDelegate.h"
#import "PhoneNumberFormatter.h"
#import "SCIVideophoneEngine.h"
#import <mach/mach.h>

@implementation Utilities

@end

#pragma mark Randomization

NSInteger RandomInteger(NSInteger max) {
	return ((float)random() / (float)INT32_MAX) *max;
}

void RandomizeList(NSMutableArray *list) {
	NSInteger i, iSwap;
	for (i = 0; i < [list count]; i++) {
		iSwap = RandomInteger([list count]);
		[list exchangeObjectAtIndex:i withObjectAtIndex:iSwap];
	}
}

#pragma mark Alerts

void Alert(NSString *sTitle, NSString *sMessage) {
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:sTitle message:sMessage preferredStyle:UIAlertControllerStyleAlert];
	[alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:nil]];
	[appDelegate.topViewController presentViewController:alert animated:YES completion:nil];
}

void AlertNotYetImplemented() {
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:[appDelegate applicationName] message:@"This feature is not yet implemented." preferredStyle:UIAlertControllerStyleAlert];
	[alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:nil]];
	[appDelegate.topViewController presentViewController:alert animated:YES completion:nil];
}

Boolean Confirm(NSString *sMessage) {
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:[appDelegate applicationName] message:sMessage preferredStyle:UIAlertControllerStyleAlert];
	[alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
	[alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
	[appDelegate.topViewController presentViewController:alert animated:YES completion:nil];
	return YES;
}

NSString *IntegerWithCommas(NSInteger i) {
	NSNumber *n = [NSNumber numberWithInteger:i];
	static NSNumberFormatter *numberFormatter = nil;
	if (!numberFormatter) {
		numberFormatter = [[NSNumberFormatter alloc] init];
		[numberFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
		[numberFormatter setUsesGroupingSeparator:YES];
	}
	return [numberFormatter stringFromNumber:n];
}

NSString *IntegerWithoutCommas(NSInteger i) {
	NSNumber *n = [NSNumber numberWithInteger:i];
	static NSNumberFormatter *numberFormatter = nil;
	if (!numberFormatter) {
		numberFormatter = [[NSNumberFormatter alloc] init];
		[numberFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
		[numberFormatter setUsesGroupingSeparator:NO];
	}
	return [numberFormatter stringFromNumber:n];
}

NSString *FloatWithCommas(float f) {
	NSNumber *n = [NSNumber numberWithFloat:f];
	static NSNumberFormatter *numberFormatter = nil;
	if (!numberFormatter) {
		numberFormatter = [[NSNumberFormatter alloc] init];
		[numberFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
		[numberFormatter setMinimumFractionDigits:0];
		[numberFormatter setMaximumFractionDigits:2];
		[numberFormatter setUsesGroupingSeparator:YES];
	}
	return [numberFormatter stringFromNumber:n];
}

NSString *FloatWithoutCommas(float f) {
	NSNumber *n = [NSNumber numberWithFloat:f];
	static NSNumberFormatter *numberFormatter = nil;
	if (!numberFormatter) {
		numberFormatter = [[NSNumberFormatter alloc] init];
		[numberFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
		[numberFormatter setMinimumFractionDigits:0];
		[numberFormatter setMaximumFractionDigits:2];
		[numberFormatter setUsesGroupingSeparator:NO];
	}
	return [numberFormatter stringFromNumber:n];
}

NSString *FloatToString(float f) {
	NSNumber *n = [NSNumber numberWithFloat:f];
	return [n stringValue];
}

NSString *FloatToChips(float f) {
	NSMutableString *s;
	NSNumber *n;
	NSInteger i = round(f);
	if (f >= 1000000) {
		n = [NSNumber numberWithFloat:f / 1000000];
		s = [NSMutableString stringWithFormat:@"%@M", [n stringValue]];
	}
	else if (f >= 10000 && i % 1000 == 0) {
		n = [NSNumber numberWithFloat:f / 1000];
		s = [NSMutableString stringWithFormat:@"%@K", [n stringValue]];
	}
	else {
		n = [NSNumber numberWithFloat:f];
		s = [NSMutableString stringWithString:[n stringValue]];
		if (f >= 1000) 
			[s insertString:@"," atIndex:[s length] - 3];
	}
	[s replaceOccurrencesOfString:@"-" withString:@"–" options:NSLiteralSearch range:NSMakeRange(0, [s length])];
	return s;
}

NSString *SecondsToTime(NSUInteger time) {
	NSUInteger hours = time / 3600;
	NSUInteger minutes = (time - (hours *3600)) / 60;
	NSUInteger seconds = time % 60;
/*
	NSString *sHours = [[NSString alloc] initWithFormat:@"%d", (seconds / 3600)];
	NSString *sMinutes = [[NSString alloc] initWithFormat:@"%d", (seconds / 60)];
	NSString *sSeconds = [[NSString alloc] initWithFormat:@"%d", (seconds % 60)];
*/
	NSString *sTime;
	if (hours > 0)
		if (minutes > 9)
			if (seconds > 9)
				sTime = [NSString stringWithFormat:@"%lu:%lu:%lu", (unsigned long)hours, (unsigned long)minutes, (unsigned long)seconds];
			else
				sTime = [NSString stringWithFormat:@"%lu:%lu:0%lu", (unsigned long)hours, (unsigned long)minutes, (unsigned long)seconds];
		else
			if (seconds > 9)
				sTime = [NSString stringWithFormat:@"%lu:0%lu:%lu", (unsigned long)hours, (unsigned long)minutes, (unsigned long)seconds];
			else
				sTime = [NSString stringWithFormat:@"%lu:0%lu:0%lu", (unsigned long)hours, (unsigned long)minutes, (unsigned long)seconds];
	else
		if (seconds > 9)
			sTime = [NSString stringWithFormat:@"%lu:%lu", (unsigned long)minutes, (unsigned long)seconds];
		else
			sTime = [NSString stringWithFormat:@"%lu:0%lu", (unsigned long)minutes, (unsigned long)seconds];
	return sTime;
}

CGContextRef MyCreateBitmapContext(NSInteger pixelsWide, NSInteger pixelsHigh) {
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef context = CGBitmapContextCreate(NULL,
									pixelsWide,
									pixelsHigh,
									8, // bits per component
									pixelsWide * 4, // bytes per row
									colorSpace,
									kCGImageAlphaPremultipliedLast);
	CGColorSpaceRelease(colorSpace);
	return context;
}

UIImage *scaleAndRotateImage(UIImage *image, float size) {
    CGImageRef imgRef = image.CGImage;
    CGFloat width = CGImageGetWidth(imgRef);
    CGFloat height = CGImageGetHeight(imgRef);
    CGAffineTransform transform = CGAffineTransformIdentity;
    CGRect bounds = CGRectMake(0, 0, width, height);
    if (width > size || height > size) {
        CGFloat ratio = width / height;
        if (ratio > 1) {
            bounds.size.width = size;
            bounds.size.height = bounds.size.width / ratio;
        }
        else {
            bounds.size.height = size;
            bounds.size.width = bounds.size.height * ratio;
        }
    }
	CGFloat scaleRatio = bounds.size.width / width;
    CGSize imageSize = CGSizeMake(width, height);
    CGFloat boundHeight = 0;
    UIImageOrientation orientation = image.imageOrientation;
    switch(orientation) {
        case UIImageOrientationUp: //EXIF = 1
            transform = CGAffineTransformIdentity;
            break;
        case UIImageOrientationUpMirrored: //EXIF = 2
            transform = CGAffineTransformMakeTranslation(imageSize.width, 0.0);
            transform = CGAffineTransformScale(transform, -1.0, 1.0);
            break;
        case UIImageOrientationDown: //EXIF = 3
            transform = CGAffineTransformMakeTranslation(imageSize.width, imageSize.height);
            transform = CGAffineTransformRotate(transform, M_PI);
            break;
        case UIImageOrientationDownMirrored: //EXIF = 4
            transform = CGAffineTransformMakeTranslation(0.0, imageSize.height);
            transform = CGAffineTransformScale(transform, 1.0, -1.0);
            break;
        case UIImageOrientationLeftMirrored: //EXIF = 5
            boundHeight = bounds.size.height;
            bounds.size.height = bounds.size.width;
            bounds.size.width = boundHeight;
            transform = CGAffineTransformMakeTranslation(imageSize.height, imageSize.width);
            transform = CGAffineTransformScale(transform, -1.0, 1.0);
            transform = CGAffineTransformRotate(transform, 3.0 * M_PI / 2.0);
            break;
        case UIImageOrientationLeft: //EXIF = 6
            boundHeight = bounds.size.height;
            bounds.size.height = bounds.size.width;
            bounds.size.width = boundHeight;
            transform = CGAffineTransformMakeTranslation(0.0, imageSize.width);
            transform = CGAffineTransformRotate(transform, 3.0 * M_PI / 2.0);
            break;
        case UIImageOrientationRightMirrored: //EXIF = 7
            boundHeight = bounds.size.height;
            bounds.size.height = bounds.size.width;
            bounds.size.width = boundHeight;
            transform = CGAffineTransformMakeScale(-1.0, 1.0);
            transform = CGAffineTransformRotate(transform, M_PI / 2.0);
            break;
        case UIImageOrientationRight: //EXIF = 8
            boundHeight = bounds.size.height;
            bounds.size.height = bounds.size.width;
            bounds.size.width = boundHeight;
            transform = CGAffineTransformMakeTranslation(imageSize.height, 0.0);
            transform = CGAffineTransformRotate(transform, M_PI / 2.0);
            break;
        default:
            [NSException raise:NSInternalInconsistencyException format:@"Invalid image orientation"];
    }
    UIGraphicsBeginImageContext(bounds.size);
    CGContextRef context = UIGraphicsGetCurrentContext();
    if (orientation == UIImageOrientationRight || orientation == UIImageOrientationLeft) {
        CGContextScaleCTM(context, -scaleRatio, scaleRatio);
        CGContextTranslateCTM(context, -height, 0);
    }
    else {
        CGContextScaleCTM(context, scaleRatio, -scaleRatio);
        CGContextTranslateCTM(context, 0, -height);
    }
    CGContextConcatCTM(context, transform);
    CGContextDrawImage(UIGraphicsGetCurrentContext(), CGRectMake(0, 0, width, height), imgRef);
    UIImage *imageCopy = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return imageCopy;
}

UIImage *MakeIcon(UIImage *picture, float size) {
	if (/* DISABLES CODE */ (YES))
		return scaleAndRotateImage(picture, size);
	else {
		CGImageRef original = [picture CGImage];
		CGContextRef bitmap = MyCreateBitmapContext(size, size);
		CGContextDrawImage(bitmap, CGRectMake(0, 0, size, size), original);
		CGImageRef resized = CGBitmapContextCreateImage(bitmap);
		CGContextRelease(bitmap);
		UIImage *result = [UIImage imageWithCGImage:resized];
		CGImageRelease(resized); // TODO: make sure this doesn't cause a crash versus a leak
		return result;
	}
}

// date/time utilities

NSString *DateToJustDate(NSDate *date) {
	if (date == nil)
		return nil;
	return [[date description] substringToIndex:10];
}

NSString *DateToString(NSDate *date, NSDateFormatterStyle dateStyle, NSDateFormatterStyle timeStyle) {
	if (date == nil)
		return nil;
	NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
	[dateFormatter setDateStyle:dateStyle];
	[dateFormatter setTimeStyle:timeStyle];
	return [dateFormatter stringFromDate:date];
}

NSString *DateToDateString(NSDate *date) {
	return DateToString(date, NSDateFormatterShortStyle, NSDateFormatterNoStyle);
}

NSString *DateToTimeString(NSDate *date) {
	return DateToString(date, NSDateFormatterNoStyle, NSDateFormatterMediumStyle);
}

NSString *DateToTimeStamp(NSDate *date) {
	if (date == nil)
		return nil;
	NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
	[dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
	return [dateFormatter stringFromDate:date];
}

NSString *DateToJustTimeStamp(NSDate *date) {
	if (date == nil)
		return nil;
	NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
	[dateFormatter setDateFormat:@"HH:mm:ss"];
	return [dateFormatter stringFromDate:date];
}

NSString *FractionDigits(float f, NSInteger digits) {
	NSNumber *n = [NSNumber numberWithFloat:f];
	NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
	[numberFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
	[numberFormatter setMinimumFractionDigits:digits];
	[numberFormatter setMaximumFractionDigits:digits];
	[numberFormatter setUsesGroupingSeparator:YES];
	return [numberFormatter stringFromNumber:n];
}

NSString *PercentDigits(float f, NSInteger digits) {
	NSNumber *n = [NSNumber numberWithFloat:f];
	NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
	[numberFormatter setNumberStyle:NSNumberFormatterPercentStyle];
	[numberFormatter setMinimumFractionDigits:digits];
	[numberFormatter setMaximumFractionDigits:digits];
	return [numberFormatter stringFromNumber:n];
}

NSString *FloatToCurrency(float f) {
	NSNumber *n = [NSNumber numberWithFloat:f];
	NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
	[numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
	[numberFormatter setMinimumFractionDigits:0];
	[numberFormatter setMaximumFractionDigits:2];
	return [numberFormatter stringFromNumber:n];
}

NSString *FloatToCurrencyRounded(float f) {
	NSNumber *n = [NSNumber numberWithFloat:f];
	NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
	[numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
	[numberFormatter setMaximumFractionDigits:0];
	return [numberFormatter stringFromNumber:n];
}

NSString *FloatToPercent(float f) {
	NSNumber *n = [NSNumber numberWithFloat:f];
	NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
	[numberFormatter setNumberStyle:NSNumberFormatterPercentStyle];
	[numberFormatter setMinimumFractionDigits:0];
	[numberFormatter setMaximumFractionDigits:2];
	return [numberFormatter stringFromNumber:n];
}

NSString *IntegerToNth(NSInteger i) {
	switch (i) {
		case 1: return [NSString stringWithFormat:@"%ldst", (long)i];
		case 2: return [NSString stringWithFormat:@"%ldnd", (long)i];
		case 3: return [NSString stringWithFormat:@"%ldrd", (long)i];
		default: return [NSString stringWithFormat:@"%ldth", (long)i];
	}
}

NSString *UnformatPhoneNumber(NSString *input) {
	// Return blank string if nil passed in to prevent crash
	if (!input) {
		return @"";
	}
	// Dont UNformat string if it matches a name in our ring group
	if([[SCIVideophoneEngine sharedEngine] isNameRingGroupMember:input])
		return input;
	
	input = [input uppercaseString];
	
	// Create Dictionary to store previously processed strings.
	static NSMutableDictionary *phoneNumberCache;
	if(!phoneNumberCache)
		phoneNumberCache = [[NSMutableDictionary alloc] init];
	
	// attempt to get unformated phonenumber from dictionary cache
	NSString *cachedPhoneNumber = [phoneNumberCache objectForKey:input];
	
	if(cachedPhoneNumber != nil)
		return cachedPhoneNumber;
	
	// dont try to unformat domains.
	if(ValidIPAddressOrDomain(input)) {
		[phoneNumberCache setObject:input forKey:input];
		return input;
	}
	
	NSMutableString *output = [NSMutableString string];
	for (int i = 0; i < [input length]; i++) {
		char c = [input characterAtIndex:i];
		if (c >= '0' && c <= '9')
			[output appendFormat:@"%c", c];
		else switch (c) {
			case 'A':
			case 'B':
			case 'C':
				[output appendString:@"2"];
				break;
			case 'D':
			case 'E':
			case 'F':
				[output appendString:@"3"];
				break;
			case 'G':
			case 'H':
			case 'I':
				[output appendString:@"4"];
				break;
			case 'J':
			case 'K':
			case 'L':
				[output appendString:@"5"];
				break;
			case 'M':
			case 'N':
			case 'O':
				[output appendString:@"6"];
				break;
			case 'P':
			case 'Q':
			case 'R':
			case 'S':
				[output appendString:@"7"];
				break;
			case 'T':
			case 'U':
			case 'V':
				[output appendString:@"8"];
				break;
			case 'W':
			case 'X':
			case 'Y':
			case 'Z':
				[output appendString:@"9"];
				break;
		}
	}
	if (input) {
		[phoneNumberCache setObject:output forKey:input];
	}
	return output;
}


NSString *FormatAsPhoneNumber(NSString *input) {
	if(!input)
		return nil;
	
	// Dont format string if it matches a name in our ring group
	if([[SCIVideophoneEngine sharedEngine] isNameRingGroupMember:input])
		return input;
	
	if(ValidIPAddressOrDomain(input))
		return input;
	
	if ([input rangeOfCharacterFromSet:[NSCharacterSet letterCharacterSet]].location != NSNotFound) {
		return input;
	}
	
	static PhoneNumberFormatter *formatter = nil;
	if (!formatter) {
		formatter = [[PhoneNumberFormatter alloc] init];
	}
	NSString *output = [formatter format:input withLocale:@"us"];
	return output;
}

NSString *AddPhoneNumberTrunkCode(NSString *phoneNumber) {
	NSString *temp = UnformatPhoneNumber(phoneNumber);
	// make sure the phone number starts with a 1
	if (temp.length == 10 && ![[temp substringToIndex:1] isEqualToString:@"1"]) {
		temp = [@"1" stringByAppendingString:temp];
	}
	return temp;
}

NSDate *DateWithGMTString(NSString *string) {
	static NSDateFormatter *dateFormatter = nil;
	if (!dateFormatter) {
		dateFormatter = [[NSDateFormatter alloc] init];
		[dateFormatter setDateFormat:@"MM/dd/yyyy HH:mm:ss"];
	}
	NSDate *date = [dateFormatter dateFromString:string];
	if (!date) return nil;
	
	static NSCalendar *calendar = nil;
	if (!calendar) {
		calendar = [[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
		[calendar setTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0]];
	}
	NSDateComponents *components = [calendar components:(
														 NSCalendarUnitYear |
														 NSCalendarUnitMonth |
														 NSCalendarUnitDay |
														 NSCalendarUnitHour |
														 NSCalendarUnitMinute |
														 NSCalendarUnitSecond)
											   fromDate:date];
	date = [calendar dateFromComponents:components];
	return date;
}

BOOL ValidDomain(NSString *dialString) {
	NSCharacterSet *dotSet = [NSCharacterSet characterSetWithCharactersInString:@"."];
	NSCharacterSet *charSet = [NSCharacterSet characterSetWithCharactersInString:@"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLKMNOPQRSTUVWXYZ"];
	if ([dialString rangeOfCharacterFromSet:dotSet].location != NSNotFound) {
		if ([dialString rangeOfCharacterFromSet:charSet].location != NSNotFound) {
			return YES;  //if the string contains a dot and at least one character assume it's a valid domain (need better check)
		}
	}
	return NO;
}

BOOL ValidIPAddressOrDomain(NSString *string) {
	if (!string)
		return NO;
	if (ValidDomain(string))
		return YES;
	NSUInteger count = 0;
	for (NSUInteger index = 0; index < string.length; index++)
		if ([string characterAtIndex:index] == '.')
			count++;
	return (count == 3 && [string rangeOfString:@".."].location == NSNotFound);
}

// Check string for invalid characters.  See defs for expressions.
BOOL ValidateInputField(NSString *inputString, NSString *regexp) {
	NSError *error = nil;
	NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:regexp options:NSRegularExpressionCaseInsensitive error:&error];
	return [regex numberOfMatchesInString:inputString options:0 range:NSMakeRange(0, [inputString length])] == 0;
}

NSString *relativeDate(NSDate *date) {
	NSCalendar *calendar = [NSCalendar currentCalendar];
	NSDate *now = [NSDate date];
	NSDateComponents *components = [calendar components:(NSCalendarUnitHour | NSCalendarUnitMinute | NSCalendarUnitSecond) fromDate:now];
	
	[components setHour:-[components hour]];
	[components setMinute:-[components minute]];
	[components setSecond:-[components second]];
	NSDate *today = [calendar dateByAddingComponents:components toDate:now options:0]; //This variable should now be pointing at a date object that is the start of today (midnight);
	
	[components setHour:-24];
	[components setMinute:0];
	[components setSecond:0];
	NSDate *yesterday = [calendar dateByAddingComponents:components toDate:today options:0];
	
	components = [calendar components:(NSCalendarUnitWeekday | NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay) fromDate:now];
	
	[components setDay:([components day] - 7)];
	NSDate *lastWeek  = [calendar dateFromComponents:components];
	
	NSString *time = nil;
	
	if ([date compare:today] == NSOrderedDescending) {
		static NSDateFormatter *dayFormatter = nil;
		if (!dayFormatter ) {
			dayFormatter = [[NSDateFormatter alloc] init];
			[dayFormatter setDateStyle:NSDateFormatterNoStyle];
			[dayFormatter setTimeStyle:NSDateFormatterShortStyle];
		}
		time = [dayFormatter stringFromDate:date];
	}
	else if ([date compare:yesterday] == NSOrderedDescending)
		time = Localize(@"Yesterday");
		else if ([date compare:lastWeek] == NSOrderedDescending) {
			components = [calendar components:NSCalendarUnitWeekday fromDate:date];
			switch ([components weekday]) {
				case 1:
					time = Localize(@"Sunday");
					break;
				case 2:
					time = Localize(@"Monday");
					break;
				case 3:
					time = Localize(@"Tuesday");
					break;
				case 4:
					time = Localize(@"Wednesday");
					break;
				case 5:
					time = Localize(@"Thursday");
					break;
				case 6:
					time = Localize(@"Friday");
					break;
				case 7:
					time = Localize(@"Saturday");
					break;
			}
		}
		else {
			static NSDateFormatter *formatter = nil;
			if (!formatter) {
				formatter = [[NSDateFormatter alloc] init];
				[formatter setDateStyle:NSDateFormatterShortStyle];
				[formatter setTimeStyle:NSDateFormatterNoStyle];
			}
			time = [formatter stringFromDate:date];
		}
	return time;
}

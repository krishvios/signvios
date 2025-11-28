#import "SCIDebug.h"
#import "NSString+SCIAdditions.h"

extern int g_stiTraceDebug;

void SCIDebug(BOOL emphasize, const char *fileName, int lineNumber, const char* method, NSString *format, ...)
{
    if (g_stiTraceDebug) {
        @autoreleasepool {
            va_list args;
            va_start(args, format);
			
            static NSDateFormatter *debugFormatter = [[NSDateFormatter alloc] init];
            static bool formatSet=false;
            @synchronized (debugFormatter) {
				if (!formatSet) {
					[debugFormatter setDateFormat:@"HH:mm:ss"];
					formatSet=true;
				}
				
				NSString *logmsg = [[NSString alloc] initWithFormat:format arguments:args];
				NSString *filePath = [[NSString alloc] initWithUTF8String:fileName];
				NSString *methodName = [[NSString alloc] initWithUTF8String:method];
				NSString *timestamp = [debugFormatter stringFromDate:[NSDate date]];
//				NSDictionary *infoDict = [[NSBundle mainBundle] infoDictionary];
				
				if (emphasize) {
					NSString *emphasisCharacterString = @"#";
					unichar emphasisCharacter = [emphasisCharacterString characterAtIndex:0];
					NSString *emphasisString = [NSString sci_stringWithCharacter:emphasisCharacter ofLength:50];
					const char *s_emphasis = [emphasisString UTF8String];
					fprintf(stdout, "%s\n%s\n%s\n%s\n%s\nSCILog: %s [%s:%d][%s] %s\n%s\n%s\n%s\n%s\n%s\n",
							s_emphasis, s_emphasis, s_emphasis, s_emphasis, s_emphasis,
							[timestamp UTF8String],
							/*[[infoDict objectForKey:(NSString *)kCFBundleNameKey] UTF8String],*/
							[[filePath lastPathComponent] UTF8String],
							lineNumber,
							[methodName UTF8String],
							[logmsg UTF8String],
							s_emphasis, s_emphasis, s_emphasis, s_emphasis, s_emphasis);
				} else {
					fprintf(stdout, "SCILog: %s [%s:%d][%s] %s\n",
							[timestamp UTF8String],
							/*[[infoDict objectForKey:(NSString *)kCFBundleNameKey] UTF8String],*/
							[[filePath lastPathComponent] UTF8String],
							lineNumber,
							[methodName UTF8String],
							[logmsg UTF8String]);
				}
            }
			
        }
    }
}

NSString *SCIStringFromBOOL(BOOL boolean)
{
	return boolean ? @"YES" : @"NO";
}

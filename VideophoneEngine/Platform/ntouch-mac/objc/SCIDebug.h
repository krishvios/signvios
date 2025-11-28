#import <Foundation/Foundation.h>
	
#define SCILog(format...) SCIDebug(NO, __FILE__, __LINE__, __func__, format)
#define SCILogEmphatic(format...) SCIDebug(YES, __FILE__, __LINE__, __func__, format)
	
#ifdef stiDEBUG
#define SCIRaise(title, formatString...) [NSException raise:title format:formatString];
#else
#define SCIRaise(title, format...) SCILog(format);
#endif


#ifdef __cplusplus
extern "C" {
#endif

void SCIDebug(BOOL emphasize, const char *fileName, int lineNumber, const char* method, NSString *format, ...);
NSString *SCIStringFromBOOL(BOOL boolean);


#ifdef __cplusplus
}
#endif

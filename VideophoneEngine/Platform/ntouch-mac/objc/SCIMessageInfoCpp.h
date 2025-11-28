
#import "SCIMessageInfo.h"

class CstiMessageInfo;

@interface SCIMessageInfo (Cpp)

+ (SCIMessageInfo *)infoWithCstiMessageInfo:(CstiMessageInfo &)info category:(CstiItemId &)category;

@end

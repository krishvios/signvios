#import "SCICallListItem.h"
#import "CstiCallList.h"
#import "CstiCallListItem.h"
#import "CstiCallHistoryItem.h"

@interface SCICallListItem (Cpp)

+ (id)itemWithCstiCallListItem:(CstiCallListItem *)item callListType:(CstiCallList::EType)callListType;
+ (id)itemWithCstiCallHistoryItem:(const CstiCallHistoryItemSharedPtr &)item callListType:(CstiCallList::EType)callListType;

@property (nonatomic, assign) CstiCallList::EType typeAsCallListType;
@property (nonatomic, assign) CstiCallListItem *cstiCallListItem;
@property (nonatomic, assign) CstiCallHistoryItemSharedPtr cstiCallHistoryItem;

CstiCallList::EType CstiCallListTypeFromSCICallType(SCICallType ct);
SCICallType SCICallTypeFromCstiCallListType(CstiCallList::EType clt);

@end

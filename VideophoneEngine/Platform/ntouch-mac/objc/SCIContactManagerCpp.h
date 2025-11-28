#import "SCIContactManager.h"
#import "IstiContact.h"

@interface SCIContactManager (Cpp)
- (void)refreshWithAddedIstiContact:(const IstiContactSharedPtr &)istiContact;
- (void)refreshWithEditedIstiContact:(const IstiContactSharedPtr &)istiContact;
- (void)refreshWithRemoveItemId:(const CstiItemId &)itemId;
@end

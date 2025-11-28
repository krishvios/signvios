#import "SCIContact.h"
#import "IstiContact.h"
#import "CstiFavorite.h"

@interface SCIContact (Cpp)

- (id)initWithIstiContact:(const IstiContactSharedPtr &)contact;
- (id)initWithCstiFavorite:(const CstiFavoriteSharedPtr &)favorite;
- (id)initWithIstiContactFromLDAPDirectory:(const IstiContactSharedPtr &)contact;

@property (nonatomic, readonly) IstiContactSharedPtr istiContact;
- (IstiContactSharedPtr)generateIstiContact;

@property (nonatomic, readonly) CstiFavoriteSharedPtr cstiFavorite;

@end

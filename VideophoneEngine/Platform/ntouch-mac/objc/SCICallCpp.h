#import "SCICall.h"
#import "IstiCall.h"
#import "stiDefs.h"
#import "stiSVX.h"

@interface SCICall (Cpp)

+ (id)callWithIstiCall:(IstiCall*)call;

@property (nonatomic, readonly) IstiCall *istiCall;

- (void)didReceiveText:(NSString *)text;
- (SstiCallStatistics *)getStatistics;

- (BOOL)getGeolocationRequested;
- (void)setGeolocationRequestedWithAvailability:(GeolocationAvailable)geolocationAvailable;

@end


extern SCICallProtocol SCICallProtocolFromEstiProtocol(EstiProtocol protocol);

extern NSString *NSStringFromEsmiCallState(EsmiCallState state);
extern SCICallState SCICallStateFromEsmiCallState(EsmiCallState state);

#import "SCIVideophoneEngine.h"
#import "SCIPropertyManager.h"
#import "IstiVideophoneEngine.h"
#import "CstiCallList.h"
#import "CstiCoreResponse.h"
#import "CstiMessageResponse.h"
#import "IstiVideoOutput.h"
#import "IstiPlatform.h"
#import "IstiNetwork.h"
#import "CstiCoreService.h"

class IstiRingGroupMgrResponse;
class CstiCoreRequest;
class CstiMessageRequest;
class CstiMessageResponse;

@class SCISettingItem;

@interface SCIVideophoneEngine (Cpp)

@property (nonatomic, readonly) IstiVideophoneEngine *videophoneEngine;

- (void)clearCallListWithType:(CstiCallList::EType)callListType completionHandler:(void (^)(NSError *error))handler;

- (void)handleCoreRequestId:(unsigned int)requestId withBlock:(void (^)(CstiCoreResponse *, NSError *))handler;

- (void)sendBasicCoreRequest:(CstiCoreRequest *)req completion:(void (^)(NSError *error))block;
- (void)sendComplexCoreRequest:(CstiCoreRequest *)req completion:(void (^)(CstiCoreResponse *response, NSError *sendError))block;
- (void)sendCoreRequest:(CstiCoreRequest *)req completion:(void (^)(CstiCoreResponse *response, NSError *sendError))block;

- (void)fetchSettings:(NSArray *)settingsToFetch completion:(void (^)(NSArray *settingList, NSError *error))block;

- (void)fetchUserSetting:(const char *)userSettingName completion:(void (^)(SCISettingItem *setting, NSError *error))block;
- (void)fetchUserSetting:(const char *)userSettingName defaultSetting:(SCISettingItem *)defaultSetting completion:(void (^)(SCISettingItem *, NSError *))block;
- (void)setUserSetting:(SCISettingItem *)userSetting completion:(void (^)(NSError *error))block;
- (void)setUserSetting:(SCISettingItem *)userSettingItem
		 preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
		postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
			completion:(void (^)(NSError *error))completionBlock;
- (void)setUserSetting:(SCISettingItem *)userSettingItem
		 preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
		postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
			   timeout:(NSTimeInterval)timeout
			completion:(void (^)(NSError *error))completionBlock;


- (void)fetchPhoneSetting:(const char *)phoneSettingName completion:(void (^)(SCISettingItem *setting, NSError *error))block;
- (void)fetchPhoneSetting:(const char *)phoneSettingName defaultSetting:(SCISettingItem *)defaultSetting completion:(void (^)(SCISettingItem *setting, NSError *error))block;
- (void)setPhoneSetting:(SCISettingItem *)phoneSetting completion:(void (^)(NSError *error))block;
- (void)setPhoneSetting:(SCISettingItem *)phoneSettingItem
		  preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
		 postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
			 completion:(void (^)(NSError *error))completionBlock;
- (void)setPhoneSetting:(SCISettingItem *)phoneSettingItem
		  preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
		 postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
				timeout:(NSTimeInterval)timeout
			 completion:(void (^)(NSError *error))completionBlock;

- (void)fetchSetting:(const char *)setting ofScope:(SCIPropertyScope)scope completion:(void (^)(SCISettingItem *setting, NSError *error))block;
- (void)fetchSetting:(const char *)setting ofScope:(SCIPropertyScope)scope defaultSetting:(SCISettingItem *)defaultSetting completion:(void (^)(SCISettingItem *setting, NSError *error))block;
- (void)setSetting:(SCISettingItem *)settingItem ofScope:(SCIPropertyScope)scope completion:(void (^)(NSError *error))block;
- (void)setSetting:(SCISettingItem *)settingItem
		   ofScope:(SCIPropertyScope)scope
	 preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
	postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
		completion:(void (^)(NSError *error))completionBlock;
- (void)setSetting:(SCISettingItem *)settingItem
		   ofScope:(SCIPropertyScope)scope
	 preStoreBlock:(void (^)(id oldValue, id newValue))preStoreBlock
	postStoreBlock:(void (^)(id oldValue, id newValue))postStoreBlock
		   timeout:(NSTimeInterval)timeout
		completion:(void (^)(NSError *error))completionBlock;

- (void)sendBasicMessageRequest:(CstiMessageRequest *)req completion:(void (^)(NSError *error))block;
- (void)sendMessageRequest:(CstiMessageRequest *)req completion:(void (^)(CstiMessageResponse *response, NSError *sendError))block;
- (void)sendComplexMessageRequest:(CstiMessageRequest *)req completion:(void (^)(CstiMessageResponse *response, NSError *sendError))block;

- (void)requestPropertyChangedNotificationsForProperty:(const char *)propertyName;

- (void)updateNetworkSettings;
- (void)updateNetworkSettingsWithCompletion:(void (^)(NSError *))block;

@end

extern NSError *NSErrorFromCstiCoreResponse(CstiCoreResponse *resp);
template<typename T>
extern NSError *NSErrorFromServiceResponse(const ServiceResponseSP<T>& result)
{
	NSError *err = nil;
	if (result) {
		if (result->coreErrorNumber != CstiCoreResponse::eNO_ERROR)
		{
			auto cstrError = result->coreErrorMessage;
			NSDictionary *userInfo;
			if (!cstrError.empty()) {
				userInfo = [NSDictionary dictionaryWithObjectsAndKeys:
							[[NSString alloc] initWithUTF8String:cstrError.c_str()], NSLocalizedDescriptionKey,
							nil];
			}
			err = [NSError errorWithDomain:SCIErrorDomainCoreResponse
									  code:SCICoreResponseErrorCodeFromECoreError(result->coreErrorNumber)
								  userInfo:userInfo];
		}
	} else {
		err = [NSError errorWithDomain:SCIErrorDomainCoreRequestSend
								  code:CstiCoreResponse::eUNKNOWN_ERROR
							  userInfo:nil];
	}
	return err;
}
extern NSError *NSErrorFromCstiMessageResponse(CstiMessageResponse *resp);
extern NSError *NSErrorFromIstiRingGroupMgrResponse(IstiRingGroupMgrResponse *resp);
extern NSString *NSStringFromFilePlayState(uint32_t param);
extern NSString *NSStringFromFilePlayError(uint32_t param);
extern SCIVoiceCarryOverType SCIVoiceCarryOverTypeFromEstiVcoType(EstiVcoType vcoType);
extern EstiVcoType EstiVcoTypeFromSCIVoiceCarryOverType(SCIVoiceCarryOverType voiceCarryOverType);
extern EstiInterfaceMode EstiInterfaceModeForSCIInterfaceMode(SCIInterfaceMode mode);
extern SCIInterfaceMode SCIInterfaceModeFromEstiInterfaceMode(EstiInterfaceMode mode);
extern SCICoreResponseErrorCode SCICoreResponseErrorCodeFromECoreError(CstiCoreResponse::ECoreError coreError);
extern NSString *NSStringFromSCICoreResponseErrorCode(SCICoreResponseErrorCode error);
extern IstiPlatform::EstiRestartReason IstiPlatformEstiRestartReasonFromSCIRestartReason(SCIRestartReason reason);
extern EstiDialSource EstiDialSourceForSCIDialSource(SCIDialSource dialSource);
extern EstiDialMethod EstiDialMethodForSCIDialMethod(SCIDialMethod dialMethod);
extern SCIDialMethod SCIDialMethodForEstiDialMethod(EstiDialMethod dialMethod);




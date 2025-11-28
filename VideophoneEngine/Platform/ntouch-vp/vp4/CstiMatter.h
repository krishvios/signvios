/*!
*  \file CstiMatter.h
*  \brief The Matter Interface
*
*  Class declaration for the Matter Interface Class.  
*   This class is responsible for communicating with Smart Home devices
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
*
*/
#pragma once

//
// Includes
//
#include "stiSVX.h"
#include "IMatter.h"
#include "CstiSignal.h"
#include "CstiEventQueue.h"

//#include <controller/BleConfig.h>
//#include <ble/ExamplePersistentStorage.h>
#ifdef LENGTH
#define PREVOUS_LENGTH LENGTH
#undef LENGTH
#endif 

#include <controller/ExamplePersistentStorage.h>
#include <crypto/PersistentStorageOperationalKeystore.h>
#include <crypto/RawKeySessionKeystore.h>
#include <credentials/PersistentStorageOpCertStore.h>
#include <app/icd/client/DefaultICDClientStorage.h>
#include <credentials/GroupDataProviderImpl.h>
#include <app/icd/client/CheckInHandler.h>
#include <commands/common/CredentialIssuerCommands.h>
#include <commands/common/CHIPCommand.h>

#ifdef PREVOUS_LENGTH
#define LENGTH PREVOUS_LENGTH
#undef PREVIOUS_LENGTH
#endif

//
// Constants
//

//
// Forward Declarations
//

//
// Globals
//

/*!
 * \brief 
 *  
 *  Class declaration for the Matter Class.  
 *	This class is responsible for interfacing with platfroms and devices that support matter.
 *  
 */
class CstiMatter : public IMatter, public CstiEventQueue 
{
public:
	
	/*!
	 * \brief CstiMatterVP constructor
	 */
	CstiMatter ();
	
	/*!
	 * \brief Initialize
	 * 
	 * \return stiHResult 
	 */
	stiHResult Initialize ();
	
	CstiMatter (const CstiMatter &other) = delete;
	CstiMatter (CstiMatter &&other) = delete;
	CstiMatter &operator= (const CstiMatter &other) = delete;
	CstiMatter &operator= (CstiMatter &&other) = delete;

	/*!
	 * \brief destructor
	 */
	~CstiMatter () override;

	/*!
	 * \brief Get instance 
	 */
	static IMatter *InstanceGet ();

	/*!
	* \brief Start up the task
	*/
	virtual stiHResult Startup ();

	/*!
	 * \brief Parses the devices QR code which is returned through a signal
	 */
	void qrCodeParse (const std::string qrCode) override;

	/*!
	 * \brief Commissions the device to the network. The devices assigned ID will be returned through a signal.
	 */
	void deviceCommission (
		const uint16_t nodeId, 
		const uint16_t pinCode,
		const uint16_t discriminator,
		const std::string ssId,
		const std::string networkPassWord) override;

	/*!
	 * \brief Turns the light on or off based on the state of the lightOn bool
	 */
	void lightOnSet (
		const uint16_t nodeId,
		const uint16_t endPointId,
		const bool lightOn) override;

	/*!
	 * \brief Sets the light's color
	 */
	void lightColorSet (
		const uint16_t nodeId,
		const uint16_t endPointId,
		const uint16_t r,
		const uint16_t g,
  		const uint16_t b) override;


public:
	ISignal<IMatter::parsedQrCode>& qrCodeParsedSignalGet () override;
	ISignal<>& qrCodeParsedFailedSignalGet () override;
	ISignal<uint16_t>& commissionedEndPointIdSignalGet () override; 

	CstiSignal<IMatter::parsedQrCode> qrCodeParsedSignal;
	CstiSignal<> qrCodeParsedFailedSignal;
	CstiSignal<uint16_t> commissionedEndPointIdSignal;

private:

    PersistentStorage m_storage;
    // TODO: It's pretty weird that we re-init mCommissionerStorage for every
    // identity without shutting it down or something in between...
    PersistentStorage m_commissionerStorage;

    chip::PersistentStorageOperationalKeystore m_keystore;
    chip::Credentials::PersistentStorageOpCertStore m_certStore;
    static chip::Crypto::RawKeySessionKeystore m_sessionKeystore;

    static chip::app::DefaultICDClientStorage m_ICDClientStorage;
    static chip::Credentials::GroupDataProviderImpl m_groupDataProvider;
    static const chip::Credentials::AttestationTrustStore *m_trustStore;
    static chip::app::CheckInHandler m_checkInHandler;
    static chip::Credentials::DeviceAttestationRevocationDelegate *m_revocationDelegate;

    CredentialIssuerCommands *m_credIssuerCmds;

    // TODO: Commissioner code should be moved to its own class.
    stiHResult ensureCommissionerForIdentity(
	std::string identity);

    // Commissioners are keyed by name and local node id.
    struct CommissionerIdentity
    {
        bool operator<(const CommissionerIdentity & other) const
        {
            return m_name < other.m_name || (m_name == other.m_name && m_localNodeId < other.m_localNodeId);
        }
        std::string m_name;
        chip::NodeId m_localNodeId;
        uint8_t m_RCAC[chip::Controller::kMaxCHIPDERCertLength] = {};
        uint8_t m_ICAC[chip::Controller::kMaxCHIPDERCertLength] = {};
        uint8_t m_NOC[chip::Controller::kMaxCHIPDERCertLength]  = {};

        size_t m_RCACLen;
        size_t m_ICACLen;
        size_t m_NOCLen;
    };

    // InitializeCommissioner uses various members, so can't be static.  This is
    // obviously a little odd, since the commissioners are then shared across
    // multiple commands in interactive mode...
    stiHResult initializeCommissioner(
	CommissionerIdentity & identity, 
	chip::FabricId fabricId);

    void shutdownCommissioner(
	const CommissionerIdentity & key);

    chip::FabricId currentCommissionerId();
    stiHResult identityNodeIdGet(
	std::string identity, 
	chip::NodeId * nodeId);

    static std::map<CommissionerIdentity, std::unique_ptr<chip::Controller::DeviceCommissioner>> m_commissioners;
    static std::set<CstiMatter *> m_deferredCleanups;

    chip::Optional<char *> m_commissionerName;
    chip::Optional<chip::NodeId> m_commissionerNodeId;
    chip::Optional<chip::VendorId> m_commissionerVendorId;
    chip::Optional<uint16_t> m_bleAdapterId;
    chip::Optional<char *> m_matterRootPath;
    chip::Optional<char *> m_paaTrustStorePath;
    chip::Optional<char *> m_CDTrustStorePath;
    chip::Optional<bool> m_useMaxSizedCerts;
    chip::Optional<bool> m_onlyAllowTrustedCdKeys;
    chip::Optional<char *> m_dacRevocationSetPath;

    void parse(
	std::string qrCode);

    bool isQRCode(
	std::string qrCode);

    stiHResult trustStoreGet(
	const char *paaTrustStorePath, 
	const chip::Credentials::AttestationTrustStore **trustStore);

    stiHResult revocationDelegateGet(
    	const char * revocationSetPath,
    	chip::Credentials::DeviceAttestationRevocationDelegate ** revocationDelegate);

};


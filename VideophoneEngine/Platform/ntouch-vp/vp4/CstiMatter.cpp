/*!
*  \file CstiMatter.cpp
*  \brief Matter Interface
*
*  Class declaration for the Matter Class.  
*   This class is responsible for communicating with Matter and smart home devices.
*   
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
*
*/
//
// Includes
//
#include "CstiMatter.h"
#include "stiTools.h"
#include <filesystem>
#include <setup_payload/ManualSetupPayloadParser.h>
#include <setup_payload/QRCodeSetupPayloadParser.h>
#include <setup_payload/SetupPayload.h>
#include <commands/icd/ICDCommand.h>
#include <credentials/attestation_verifier/FileAttestationTrustStore.h>
#include <credentials/attestation_verifier/TestDACRevocationDelegateImpl.h>


//
// Constants
//

//
// Typedefs
//
#define MATTER_DIR "Matter"
#define MATTER_CERT_DIR "Matter/certs"
#define MATTER_TRUST_DIR "Matter/TrustStore"
#define MATTER_CONFIG_FILE "matterConfig"

#define MAX_GROUPS_PER_FABRIC 5
#define MAX_GROUP_KEYS_PER_FABRIC 8

//
// Globals
//


// All fabrics share the same ICD client storage.
chip::app::DefaultICDClientStorage CstiMatter::m_ICDClientStorage;
chip::Crypto::RawKeySessionKeystore CstiMatter::m_sessionKeystore;
chip::app::CheckInHandler CstiMatter::m_checkInHandler;
chip::Credentials::GroupDataProviderImpl CstiMatter::m_groupDataProvider { MAX_GROUPS_PER_FABRIC, MAX_GROUP_KEYS_PER_FABRIC };
const chip::Credentials::AttestationTrustStore *CstiMatter::m_trustStore = nullptr;
chip::Credentials::DeviceAttestationRevocationDelegate *CstiMatter::m_revocationDelegate = nullptr;
std::map<CstiMatter::CommissionerIdentity, std::unique_ptr<chip::Controller::DeviceCommissioner>> CstiMatter::m_commissioners;

//
// Locals
//

const std::string identityVpCommissioner = "VP-fabric-commissioner";
#if 0
constexpr chip::FabricId identityVpFabricId  = chip::kUndefinedFabricId;
#endif
constexpr chip::FabricId kIdentityOtherFabricId  = 1;



//
// Forward Declarations
//

//
// Constructor
//
/*!
 * \brief  Constructor
 * 
 */
CstiMatter::CstiMatter ()
:
	CstiEventQueue ("CstiMatterQueue")
{

}

//
// Destructor
//
/*!
 * \brief Destructor
 */
CstiMatter::~CstiMatter ()
{

}

/*!
 * \brief Initializes the object  
 * 
 * \return stiHResult 
 * stiRESULT_SUCCESS, or on error 
 *  
 */
stiHResult CstiMatter::Initialize ()
{
	auto hResult = stiRESULT_SUCCESS;
#if 0
	CHIP_ERROR chipError = CHIP_NO_ERROR;
	auto allowTestCdSigningKey = false;
	auto funcSuccess = true;

	uint16_t port = 0;
	const chip::Controller::DeviceControllerSystemState *systemState = nullptr;
	chip::bdx::BDXTransferServer *server = nullptr;
	chip::app::InteractionModelEngine *matterEngine = nullptr;
	CommissionerIdentity vpIdentity{ identityVpCommissioner, chip::kUndefinedNodeId };
    chip::Controller::FactoryInitParams factoryInitParams;
    ExampleCredentialIssuerCommands credIssuerCommands;
    Commands commands;

	// Create the Matter storage dir.
	std::string matterDir;

    stiOSDynamicDataFolderGet(&matterDir);
    matterDir.append (MATTER_TRUST_DIR);
    std::filesystem::path matterPath(matterDir);

	stiOSDynamicDataFolderGet(&matterDir);
	matterDir.append (MATTER_DIR);
	std::filesystem::path storagePath(matterDir);
    if (!std::filesystem::is_directory(storagePath))
    {
        funcSuccess = std::filesystem::create_directory(storagePath);
    }
	stiTESTCOND (funcSuccess, stiRESULT_ERROR);

	m_matterRootPath.SetValue (const_cast<char *>(storagePath.string ().c_str ()));

    chipError = chip::Platform::MemoryInit();
	stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

    registerCommandsICD(commands, &credIssuerCommands);
	m_credIssuerCmds = &credIssuerCommands;

    chipError = chip::DeviceLayer::Internal::BLEMgrImpl().ConfigureBle(m_bleAdapterId.ValueOr(0), true);
	stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

	m_storage.Init(MATTER_CONFIG_FILE, storagePath.string().c_str());
    m_keystore.Init(&m_storage);
    m_certStore.Init(&m_storage);

    // chip-tool uses a non-persistent keystore.
    // ICD storage lifetime is currently tied to the chip-tool's lifetime. Since chip-tool interactive mode is currently used for
    // ICD commissioning and check-in validation, this temporary storage meets the test requirements.
    // TODO: Implement persistent ICD storage for the chip-tool.
    chipError = m_ICDClientStorage.Init(&m_storage, &m_sessionKeystore);
	stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

    factoryInitParams.fabricIndependentStorage = &m_storage;
    factoryInitParams.operationalKeystore = &m_keystore;
    factoryInitParams.opCertStore = &m_certStore;
    factoryInitParams.enableServerInteractions = false; //NeedsOperationalAdvertising();
    factoryInitParams.sessionKeystore = &m_sessionKeystore;

    // Init group data provider that will be used for all group keys and IPKs for the
    // chip-tool-configured fabrics. This is OK to do once since the fabric tables
    // and the DeviceControllerFactory all "share" in the same underlying data.
    // Different commissioner implementations may want to use alternate implementations
    // of GroupDataProvider for injection through factoryInitParams.
    m_groupDataProvider.SetStorageDelegate (&m_storage);
    m_groupDataProvider.SetSessionKeystore (factoryInitParams.sessionKeystore);
    chipError = m_groupDataProvider.Init ();
    stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

    chip::Credentials::SetGroupDataProvider (&m_groupDataProvider);
    factoryInitParams.groupDataProvider = &m_groupDataProvider;

    port = m_storage.GetListenPort ();
    factoryInitParams.listenPort = port;
    chipError = chip::Controller::DeviceControllerFactory::GetInstance ().Init (factoryInitParams);
    stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

    systemState = chip::Controller::DeviceControllerFactory::GetInstance ().GetSystemState ();
    stiTESTCOND (nullptr != systemState, stiRESULT_ERROR);

	server = systemState->BDXTransferServer ();
    stiTESTCOND (nullptr != server, stiRESULT_ERROR);

    server->SetDelegate(&BDXDiagnosticLogsServerDelegate::GetInstance());

    if (!std::filesystem::is_directory(matterPath))
    {
        funcSuccess = std::filesystem::create_directory(matterPath);
    }
    stiTESTCOND (funcSuccess, stiRESULT_ERROR);

	m_paaTrustStorePath.SetValue (const_cast<char *>(matterPath.string ().c_str ()));
	hResult = trustStoreGet(m_paaTrustStorePath.ValueOr(nullptr), &m_trustStore);
	stiTESTRESULT();

	hResult = revocationDelegateGet(m_dacRevocationSetPath.ValueOr(nullptr), &m_revocationDelegate);
	stiTESTRESULT();

    matterEngine = chip::app::InteractionModelEngine::GetInstance();
    stiTESTCOND (matterEngine != nullptr, stiRESULT_ERROR);

    chipError = ChipToolCheckInDelegate()->Init(&m_ICDClientStorage, matterEngine);
    stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

    chipError = m_checkInHandler.Init(chip::Controller::DeviceControllerFactory::GetInstance().GetSystemState()->ExchangeMgr(),
                                                            &m_ICDClientStorage, ChipToolCheckInDelegate(), matterEngine);
    stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

    hResult = initializeCommissioner(vpIdentity, identityVpFabricId);
	stiTESTRESULT();

    // After initializing first commissioner, add the additional CD certs once
    {
	    stiOSDynamicDataFolderGet(&matterDir);
	    matterDir.append (MATTER_CERT_DIR);
	    std::filesystem::path storagePath(matterDir);
        if (!std::filesystem::is_directory(storagePath))
        {
            funcSuccess = std::filesystem::create_directory(storagePath);
        }
	    stiTESTCOND (funcSuccess, stiRESULT_ERROR);

        const char *cdTrustStorePath = matterDir.c_str();

        auto additionalCdCerts = chip::Credentials::LoadAllX509DerCerts(cdTrustStorePath, 
                                                                                                                  chip::Credentials::CertificateValidationMode::kPublicKeyOnly);
        chipError = m_credIssuerCmds->AddAdditionalCDVerifyingCerts(additionalCdCerts);
        stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);
    }
    allowTestCdSigningKey = !m_onlyAllowTrustedCdKeys.ValueOr(false);
    m_credIssuerCmds->SetCredentialIssuerOption(CredentialIssuerCommands::CredentialIssuerOptions::kAllowTestCdSigningKey,
                                                                                     allowTestCdSigningKey);

STI_BAIL:
#endif
	return hResult;
} 

/*!
* \retval None
*/
stiHResult CstiMatter::Startup ()
{
	CstiEventQueue::StartEventLoop();

	return stiRESULT_SUCCESS;
}

/*!
 * \brief Parses the devices QR code which is returned through a signal
 */
void CstiMatter::qrCodeParse (
	const std::string qrCode)
{
	PostEvent([this, qrCode]
	{
		parse (qrCode);
	});
}


void CstiMatter::parse(std::string qrCode)
{
	auto hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG(hResult);

	IMatter::parsedQrCode parsedCode;

	CHIP_ERROR chipError = CHIP_NO_ERROR;
	chip::SetupPayload payload;

	bool realQRCode = isQRCode(qrCode);

    chipError =  realQRCode ? chip::QRCodeSetupPayloadParser(qrCode).populatePayload(payload) : 
										   chip::ManualSetupPayloadParser(qrCode).populatePayload(payload);
	stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

	parsedCode.version = payload.version;
	parsedCode.vendorId = payload.vendorID;
	parsedCode.productId = payload.productID;
	switch (payload.commissioningFlow)
	{
	case chip::CommissioningFlow::kCustom:
		parsedCode.commissionFlow = 2;
		break;

	case chip::CommissioningFlow::kUserActionRequired:
		parsedCode.commissionFlow = 1;
		break;

	case chip::CommissioningFlow::kStandard:
	default:
		parsedCode.commissionFlow = 0;
		break;
	}

	parsedCode.shortDiscriminator = payload.discriminator.IsShortDiscriminator();
	parsedCode.discriminator =  parsedCode.shortDiscriminator ? payload.discriminator.GetShortValue () : payload.discriminator.GetLongValue ();
	parsedCode.setupPinCode =  payload.setUpPINCode;

	qrCodeParsedSignal.Emit (parsedCode);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		qrCodeParsedFailedSignal.Emit ();
	}

}


bool CstiMatter::isQRCode(std::string qrCode)
{
    return qrCode.rfind(chip::kQRCodePrefix) == 0;
}


stiHResult CstiMatter::trustStoreGet(
	const char *paaTrustStorePath, 
	const chip::Credentials::AttestationTrustStore **trustStore)
{
	auto hResult = stiRESULT_SUCCESS;  

	stiTESTCOND (paaTrustStorePath != nullptr, stiRESULT_ERROR);

    static chip::Credentials::FileAttestationTrustStore attestationTrustStore{ paaTrustStorePath };

	stiTESTCOND (paaTrustStorePath != nullptr && attestationTrustStore.paaCount() == 0, stiRESULT_ERROR);

    *trustStore = &attestationTrustStore;

STI_BAIL:
    return hResult;
}


stiHResult CstiMatter::revocationDelegateGet(
	const char * revocationSetPath,
    chip::Credentials::DeviceAttestationRevocationDelegate ** revocationDelegate)
{
	auto hResult = stiRESULT_SUCCESS;  
	CHIP_ERROR chipError = CHIP_NO_ERROR;
    static chip::Credentials::TestDACRevocationDelegateImpl testDacRevocationDelegate;

    if (revocationSetPath != nullptr)
    {
        chipError = testDacRevocationDelegate.SetDeviceAttestationRevocationSetPath(revocationSetPath);
        stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

        *revocationDelegate = &testDacRevocationDelegate;
    }

STI_BAIL:
    return hResult;
}


stiHResult CstiMatter::ensureCommissionerForIdentity(
	std::string identity)
{
	auto hResult = stiRESULT_SUCCESS;  
    chip::FabricId fabricId;
    chip::NodeId nodeId;

	hResult = identityNodeIdGet(identity, &nodeId);
    stiTESTRESULT ();

    {
        CommissionerIdentity lookupKey{ identity, nodeId };
        if (m_commissioners.find(lookupKey) != m_commissioners.end())
        {
            stiTHROW(stiRESULT_ERROR);
        }

        // Need to initialize the commissioner.
        fabricId = strtoull(identity.c_str(), nullptr, 0);
        stiTESTCONDMSG (fabricId >= kIdentityOtherFabricId, stiRESULT_ERROR, "Invalid identity: %s", identity.c_str());

        return initializeCommissioner(lookupKey, fabricId);
    }

STI_BAIL:
	return hResult;
}


stiHResult CstiMatter::identityNodeIdGet(
	std::string identity, 
	chip::NodeId * nodeId)
{
	auto hResult = stiRESULT_SUCCESS;  
   
	if (m_commissionerNodeId.HasValue())
    {
        *nodeId = m_commissionerNodeId.Value();
    }
    else if (identity == kIdentityNull)
    {
        *nodeId = chip::kUndefinedNodeId;
    }
    else if (m_commissionerStorage.Init(identity.c_str()) != CHIP_NO_ERROR)
    {
        stiTHROW (stiRESULT_ERROR); 
    }
    else
    {
        *nodeId = m_commissionerStorage.GetLocalNodeId();
    }

STI_BAIL:
    return hResult;
}


void CstiMatter::shutdownCommissioner(
	const CommissionerIdentity &key)
{
    m_commissioners[key].get()->Shutdown();
}


stiHResult CstiMatter::initializeCommissioner(
	CommissionerIdentity &identity, 
	chip::FabricId fabricId)
{
	auto hResult = stiRESULT_SUCCESS;
	CHIP_ERROR chipError = CHIP_NO_ERROR;
    std::unique_ptr<::chip::Controller::DeviceCommissioner> commissioner = std::make_unique<::chip::Controller::DeviceCommissioner>();
    chip::Controller::SetupParams commissionerParams;
    chip::Crypto::P256Keypair ephemeralKey;

    chipError = m_credIssuerCmds->SetupDeviceAttestation(commissionerParams, m_trustStore, m_revocationDelegate);
	stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

    if (fabricId != chip::kUndefinedFabricId)
    {
        // TODO - OpCreds should only be generated for pairing command
        //        store the credentials in persistent storage, and
        //        generate when not available in the storage.
        chipError = m_commissionerStorage.Init(identity.m_name.c_str(), m_matterRootPath.ValueOr(nullptr));
        if (m_useMaxSizedCerts.HasValue())
        {
            auto option = CredentialIssuerCommands::CredentialIssuerOptions::kMaximizeCertificateSizes;
            m_credIssuerCmds->SetCredentialIssuerOption(option, m_useMaxSizedCerts.Value());
        }

        chipError  = m_credIssuerCmds->InitializeCredentialsIssuer(m_commissionerStorage);
		stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

        chip::MutableByteSpan nocSpan(identity.m_NOC);
        chip::MutableByteSpan icacSpan(identity.m_ICAC);
        chip::MutableByteSpan rcacSpan(identity.m_RCAC);

        chipError = ephemeralKey.Initialize(chip::Crypto::ECPKeyTarget::ECDSA);
		stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

        chipError = m_credIssuerCmds->GenerateControllerNOCChain(identity.m_localNodeId, fabricId,
																											m_commissionerStorage.GetCommissionerCATs(),
																											ephemeralKey, rcacSpan, icacSpan, nocSpan);
		stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

        identity.m_RCACLen = rcacSpan.size();
        identity.m_ICACLen = icacSpan.size();
        identity.m_NOCLen  = nocSpan.size();

        commissionerParams.operationalKeypair = &ephemeralKey;
        commissionerParams.controllerRCAC = rcacSpan;
        commissionerParams.controllerICAC = icacSpan;
        commissionerParams.controllerNOC = nocSpan;
        commissionerParams.permitMultiControllerFabrics = true;
        commissionerParams.enableServerInteractions = false;
    }

    // TODO: Initialize IPK epoch key in ExampleOperationalCredentials issuer rather than relying on DefaultIpkValue
    commissionerParams.operationalCredentialsDelegate = m_credIssuerCmds->GetCredentialIssuer();
    commissionerParams.controllerVendorId             = m_commissionerVendorId.ValueOr(chip::VendorId::TestVendor1);

    chipError =  chip::Controller::DeviceControllerFactory::GetInstance().SetupCommissioner(commissionerParams, *(commissioner.get()));
	stiTESTCOND (chipError == CHIP_NO_ERROR, stiRESULT_ERROR);

    CstiMatter::m_ICDClientStorage.UpdateFabricList(commissioner->GetFabricIndex());

    m_commissioners[identity] = std::move(commissioner);

STI_BAIL:
    return hResult;
}


	/*!
	 * \brief Commissions the device to the network. The devices assigned ID will be returned through a signal.
	 */
void CstiMatter::deviceCommission (
	const uint16_t nodeId, 
	const uint16_t pinCode,
	const uint16_t discriminator,
	const std::string ssId,
	const std::string networkPassWord)
{

}

    /*!
	 * \brief Turns the light on or off based on the state of the lightOn bool
	 */
void CstiMatter::lightOnSet (
	const uint16_t nodeId,
	const uint16_t endPointId,
	const bool lightOn)
{

}

	/*!
	 * \brief Sets the light's color
	 */
void CstiMatter::lightColorSet (
	const uint16_t nodeId,
	const uint16_t endPointId,
	const uint16_t r,
	const uint16_t g,
  	const uint16_t b)
{

}


ISignal<IMatter::parsedQrCode>& CstiMatter::qrCodeParsedSignalGet ()
{
	return qrCodeParsedSignal;
}

ISignal<>& CstiMatter::qrCodeParsedFailedSignalGet ()
{
	return qrCodeParsedFailedSignal;
}

ISignal<uint16_t>& CstiMatter::commissionedEndPointIdSignalGet () 
{
	return commissionedEndPointIdSignal;
}




///
/// \file CstiLightRingBase.h
/// \brief Declaration of the LightRing class
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
///

#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include "CstiEventQueue.h"
#include "CstiSignal.h"

#include "LedApi.h"

#include "ILightRingVP.h"
#include "CstiHelios.h"
#include "stiTaskInfo.h"
#include "stiOSLinuxWd.h"
#include "stiTrace.h"

//
// Constants
//
const int FLASHER_MIN = 0;
const int FLASHER_MAX = 1000;

//
// Typedefs
//

//
// Structs
//
struct delayRGBIntensity {
    int duration;
    uint8_t intensity;
    lr_led_rgb lr_vals[NUM_LR_LEDS];
};

//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//

/*! \class IstiLightRing 
 *  
 * \brief Controls Led ringer on ntouch video phone. 
 *  
 * \note Functions are pure virutal and       
 * each function placed will need to be instantiated  
 * in CstiLightRingBase or derived class
 *  
 * \li Initialize
 * \li PatternSet
 * \li Start 
 * \li Stop 
 * \li notificationsStart
 * \li notificationsStop
 * \li lightRingStoppedSignalGet
 * \li notificationStoppedSignalGet
 * \li lightRingTurnOn
 * \li lightRingTurnOff
 * \li lightRingTestPattern
 * \li alertLedsTurnOn
 * \li alertLedsTurnOff
 * \li alertLedsTestPattern
*/
class CstiLightRingBase : public ILightRingVP, public CstiEventQueue
{
public:

	// flasher bank mask values
	enum
	{
		eFLASHER_OFF = 0,
		eFLASHER_RIGHT = 1,
		eFLASHER_LEFT = 2,
		eFLASHER_ALL = 3
	};

	CstiLightRingBase (const std::map<IstiLightRing::EstiLedColor, lr_led_rgb> &colorMap);
	virtual ~CstiLightRingBase ();
	
	/*!
	 * \brief Initialization routine called by 
	 * GetInstance if necessary 
	 * 
	 * \return stiHResult 
	 */
	stiHResult Initialize (
		std::shared_ptr<CstiHelios> helios);

	/*!
	 * \brief This takes a SstiLightRingSample as input data as well as a ring count
	 * 
	 * \param const SstiLightRingSample* poLightRingSample 
	 * \param uint32_t unCount 
	 */
	void PatternSet (
		Pattern patternId,
		EstiLedColor color,
		int brightness,
		int flasherBrightness) override;

	/*!
	 * \brief This takes an enum to set the notification pattern
	 * 
	 * \param pattern enum
	 */
	void notificationPatternSet (
		NotificationPattern pattern) override;

	/*!
	 * \brief Starts light ring for nDuration seconds
	 * 
	 * \param int nDuration 
	 * 	Number of seconds to run light ring 
	 */
	void Start (
		int nDuration) override;
		
	/*!
	 * \brief Stops the light ringer
	 */
	void Stop () override;

	/*!
	 * \brief Starts light ring notifications
	 * 
	 * \param int nDuration 
	 * \param int nBrightness 
	 * 	Number of seconds to run light ring 
	 */
	void notificationsStart (
		int nDuration,
		int nBrightness) override;
		
	/*!
	 * \brief Stops light ring notifications
	 */
	void notificationsStop () override;

	virtual stiHResult HeliosFramesSend ();

	virtual stiHResult LightRingFramesSend ();

	stiHResult Startup ();

	stiHResult Shutdown ();
	
	ISignal<>& lightRingStoppedSignalGet () override;

	ISignal<>& notificationStoppedSignalGet () override;

	std::shared_ptr<CstiHelios> m_helios;

protected:

	lr_led_rgb colorGet (
		const std::map<IstiLightRing::EstiLedColor,
		lr_led_rgb> &colorMap,
		IstiLightRing::EstiLedColor ledColor);

	stiHResult FlasherStop ();

	std::vector<lightring_frame_rgb> m_lightringFrames;

	std::vector<delayRGBIntensity> m_lightringNotificationFrames;

	const std::map<IstiLightRing::EstiLedColor, lr_led_rgb> m_colorMap;

	Pattern m_patternId = IstiLightRing::Pattern::Flash;

	uint8_t m_brightness = 0xff;

	uint16_t m_flasherBrightness = FLASHER_MAX;

	EstiLedColor m_color = IstiLightRing::estiLED_USE_PATTERN_COLORS;



	bool m_sendFrames = true;

	bool m_bPlaying = false;

	CstiSignalConnection::Collection m_signalConnections;

private:
	
	enum EEvent
	{
		estiEVENT_STATUSLED_MONITOR_TASK_MSG = estiEVENT_NEXT
	};


	bool m_includeFlasher {true};

	vpe::EventTimer m_lightRingTimer;
	vpe::EventTimer m_flasherTimer;
	vpe::EventTimer m_lightRingNotificationsTimer;

	//
	// Event Handlers
	//
	void eventTimer ();
	void eventFlasherTimer ();
	void eventLightRingNotificationsTimer ();

protected:

	virtual void eventPatternSet (
		Pattern patternId,
		EstiLedColor color,
		int brightness,
		int flasherBrightness);

	virtual void eventNotificationPatternSet (
		NotificationPattern pattern);

	virtual void eventStart (
		int duration,
		bool includeFlasher);

	virtual void eventStop ();

	virtual void eventNotificationsStart (
		int duration,
		int brightness);

	virtual void eventNotificationsStop ();

	// Helper function to set up frames in derived classes
	inline static std::string makeFrame (
		uint8_t duration,
		uint8_t intensity,
		const std::vector<const char*>& colors)
	{
		std::ostringstream oss;

		oss << "0x" << std::hex << std::setw (2) << std::setfill ('0') << (int)duration << " ";
		oss << "0x" << std::hex << std::setw (2) << std::setfill ('0') << (int)intensity << " ";

		for (const auto& color : colors)
		{
			oss << color << " ";
		}

		std::string result = oss.str ();

		// Remove trailing space
		if (!result.empty ()) result.pop_back ();
		return result;
	}

	// Helper function to set up repeated frames in derived classes
	inline static std::vector<std::string> makeRepeatedFrames (
		uint8_t duration,
		uint8_t intensity,
		const std::vector<const char*>& colors,
		int count)
	{
		std::vector<std::string> patterns;
		patterns.reserve (count);
		for (int i = 0; i < count; ++i)
		{
			patterns.push_back (makeFrame (duration, intensity, colors));
		}
		return patterns;
	}

	virtual const std::vector<std::string>& signMailPatternGet () const = 0;
	virtual const std::vector<std::string>& missedCallPatternGet () const = 0;
	virtual const std::vector<std::string>& missedCallAndSignMailPatternGet () const = 0;

private:

	stiHResult FlasherStart ();
	
	std::vector<delayRGBIntensity> parseTestVectors(std::vector<std::string> strings);

	CstiSignal<> lightRingStoppedSignal;
	CstiSignal<> notificationStoppedSignal;

};

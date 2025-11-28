///
/// \file CstiLightRing.h
/// \brief Declaration of the LightRing class
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015-2025 Sorenson Communications, Inc. -- All rights reserved
///

#pragma once

#include "CstiLightRingBase.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

class CstiMonitorTask;

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
 * in CstiLightRing.cpp                               
 *  
 * \li Initialize
 * \li PatternSet
 * \li Start 
 * \li Stop 
 * 
 */
class CstiLightRing : public CstiLightRingBase
{
public:

	CstiLightRing ();
	~CstiLightRing () override = default;
	
	/*!
	 * \brief Initialization routine called by 
	 * GetInstance if necessary 
	 * 
	 * \return stiHResult 
	 */
	stiHResult Initialize (
		CstiMonitorTask *monitorTask,
		std::shared_ptr<CstiHelios> helios);

	void lightRingTurnOn (uint8_t red, uint8_t green, uint8_t blue, uint8_t intensity) override;
	void lightRingTurnOff () override;
	void lightRingTestPattern (bool on) override;
	void alertLedsTurnOn (uint16_t intensity) override;
	void alertLedsTurnOff () override;
	void alertLedsTestPattern (int intensity) override;

protected:

	const std::vector<std::string>& signMailPatternGet () const override { return m_signMail; }
	const std::vector<std::string>& missedCallPatternGet () const override { return m_missedCall; }
	const std::vector<std::string>& missedCallAndSignMailPatternGet () const override { return m_missedCallAndSignMail; }

private:
	void eventMfddrvdStatusChanged ();
	stiHResult RcuHotplug ();
	CstiMonitorTask *m_monitorTask = nullptr;

	/* Light Ring Notification Vectors
	* ------------
	*    LED8     LED1    LED2
	*         -----------
	*        /           \
	*       /             \
	* LED7  |             |  LED3
	*       \             /
	*        \-----------/
	*   LED6      LED5    LED4       
	*
	* Vector Structure
	* ----------------
	* Duration Intensity   LED1        LED2      LED3   ...LED8
	*    |       |         r  g  b     r g b     r g b
	*    |       |         |  |  |     | | |     | | |
	*    |       |         -- -- --    ------    ------
	*{0x00    0x00       0x00 00 00  0x000000  0x000000 .......}
	*/

	// Define colors for light ring notifications
	static constexpr const char* OFF = "0x000000";
	static constexpr const char* BLU = "0x00a5ff";
	static constexpr const char* AMB = "0xff6500";
	static constexpr uint8_t FRAME_DURATION_MS = 0;
	static constexpr uint8_t INTENSITY = 64;
	static constexpr int NUM_FRAMES = 1; // Was 6?
	// Note: in makeRepeatedFrames () below, duration needs
	// to be set to non-zero if NUM_FRAMES > 1


	const std::vector<std::string> m_signMail =
		makeRepeatedFrames (FRAME_DURATION_MS, INTENSITY,
			{
				OFF, OFF, OFF, OFF, OFF, BLU, BLU, BLU
			}, NUM_FRAMES);

	const std::vector<std::string> m_missedCall =
		makeRepeatedFrames (FRAME_DURATION_MS, INTENSITY,
			{
				OFF, AMB, AMB, AMB, OFF, OFF, OFF, OFF
			}, NUM_FRAMES);

	const std::vector<std::string> m_missedCallAndSignMail =
		makeRepeatedFrames (FRAME_DURATION_MS, INTENSITY,
			{
				OFF, AMB, AMB, AMB, OFF, BLU, BLU, BLU
			}, NUM_FRAMES);
};

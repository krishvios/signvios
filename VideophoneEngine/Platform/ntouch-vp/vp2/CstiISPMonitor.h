#ifndef CSTIISPMONITOR_H
#define CSTIISPMONITOR_H

#include "CstiEventQueue.h"
#include "IVideoInputVP2.h"
#include "CstiSignal.h"
#include <vector>


class CstiMonitorTask;


class CstiISPMonitor : public CstiEventQueue
{
public:

	CstiISPMonitor ();

	~CstiISPMonitor () override;

	void Startup ();

	void Shutdown ();

	stiHResult Initialize (
		CstiMonitorTask *pMonitorTask);

	stiHResult HdmiPassthroughSet (
		bool bHdmiPassthrough);
	
	stiHResult ambientLightGet (
		IVideoInputVP2::AmbientLight *ambiance,
		float *rawVal);

private:

	void eventRcuConnectionStatusChanged ();
	void eventRcuReset ();
	void eventReadPipe ();
	void eventMonitorTimer ();

	vpe::EventTimer m_monitorTimer;
	void monitorTimerStart ();

	CstiSignalConnection::Collection m_signalConnections;

	struct SstiCurveEntry
	{
		SstiCurveEntry () = default;

		SstiCurveEntry (
			float fValue,
			float fPosition)
		:
			fValue (fValue),
			fPosition (fPosition)
		{
		}

		float fValue {0};
		float fPosition {0};
	};

	enum ECurveVariable
	{
		eG = 0,
		eJ,
		eY
	};

	struct SstiRegisterEntry
	{
		uint32_t un32Address {0};
		uint8_t un8Value {0};
	};


	struct SstiConfiguration
	{
		bool bValid {false};
		int nMaxGain {8};
		float fGainWeight {1.0f};
		float fLumaWeight {1.0f};
		float fExposureWeight {1.0f};
		int nSampleRate {1000};

		std::vector<SstiCurveEntry> SaturationCurve;
		ECurveVariable eSaturationVariable {eG};

		std::vector<SstiCurveEntry> YOffsetCurve;
		ECurveVariable eYOffsetVariable {eG};

		std::vector<SstiCurveEntry> GDenoiseCurve;
		ECurveVariable eGDenoiseVariable {eG};

		std::vector<SstiCurveEntry> BRDenoiseCurve;
		ECurveVariable eBRDenoiseVariable {eG};

		std::vector<SstiCurveEntry> UVDenoiseCurve;
		ECurveVariable eUVDenoiseVariable {eG};

		std::vector<SstiCurveEntry> BrightnessCurve;
		ECurveVariable eBrightnessVariable;

		std::vector<uint8_t> ContrastTable;

		std::vector<SstiRegisterEntry> RegisterList;
	};
	
	struct AmbientLimits
	{
		float low = 0;
		float midlow = 0;
		float midhigh = 0;
		float high = 0;
	};
	
	stiHResult ConfigurationRead (
		const std::string &FileName,
		CstiISPMonitor::SstiConfiguration *pCfg);

	void ReadCurve (
		char *pCurrent,
		char *pEnd,
		std::vector<CstiISPMonitor::SstiCurveEntry> *pCurve,
		CstiISPMonitor::ECurveVariable *peCurveVariable);

	void ReadTable (
		char *pCurrent,
		char *pEnd,
		std::vector<uint8_t> *pTable);

	void PrintCurve (
		const std::vector<CstiISPMonitor::SstiCurveEntry> &Curve);

	stiHResult StaticSettingsSet ();

	stiHResult DynamicSettingsSet ();

	float FindCurveValue (
		float fInputValue,
		const std::vector<CstiISPMonitor::SstiCurveEntry> &Curve);

	void SaturationSet (
		float fInputValue,
		const std::vector<CstiISPMonitor::SstiCurveEntry> &SaturationCurve,
		int *pnCurrentSaturation);

	void YOffsetSet (
		float fInputValue,
		const std::vector<CstiISPMonitor::SstiCurveEntry> &YOffsetCurve,
		int *pnCurrentYOffset);

	void GDenoiseSet (
		float fInputValue,
		const std::vector<CstiISPMonitor::SstiCurveEntry> &GDenoiseCurve,
		int *pnCurrentGDenoise);

	void BRDenoiseSet (
		float fInputValue,
		const std::vector<CstiISPMonitor::SstiCurveEntry> &BRDenoiseCurve,
		int *pnCurrentBRDenoise);

	void UVDenoiseSet (
		float fInputValue,
		const std::vector<CstiISPMonitor::SstiCurveEntry> &UVDenoiseCurve,
		int *pnCurrentUVDenoise);

	void BrightnessSet (
		float fInputValue,
		const std::vector<CstiISPMonitor::SstiCurveEntry> &BrightnessCurve,
		int *pnCurrentBrightness);

	int m_PipeFd {-1};

	std::string m_ConfigurationFilename;
	SstiConfiguration *m_pConfiguration {nullptr};

	int m_nSaturation {-1};
	int m_nYOffset {0};
	int m_nGDenoise {0};
	int m_nBRDenoise {0};
	int m_nUVDenoise {0};
	int m_nBrightness {0};
	
	float m_jValue = 0.0;

	bool m_bRcuConnected {false};
	bool m_bHdmiPassthrough {false};
	
	std::map<int, CstiISPMonitor::AmbientLimits> m_ambientLimits;
	
	IVideoInputVP2::AmbientLight m_prevAmbiance = IVideoInputVP2::AmbientLight::UNKNOWN;

	CstiMonitorTask *m_pMonitorTask {nullptr};
};

#endif // CSTIISPMONITOR_H

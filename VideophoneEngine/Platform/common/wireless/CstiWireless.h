/////////////////////////////////////////////////////////////////////////////////
///
///  \brief	class CstiWireless :public IstiWireless
///
///
///
#ifndef CSTIWIRELESS_H
#define CSTIWIRELESS_H

#include <unistd.h>
#include <sys/socket.h> /* for connect and socket*/
#include <sys/ioctl.h>
#include <asm/types.h>
#include "rtmp_type.h"
#include "IstiWireless.h"
#include </usr/include/linux/wireless.h>



int SciExtract_event_stream(struct stream_descr *   stream,	    // Stream of events
							struct iw_event *   	iwe,	    // Extracted event
							int    					we_version);

class CstiWireless : public IstiWireless
{
public:

	virtual ~CstiWireless () {}
	
	virtual  stiHResult WirelessAvailable();
	virtual  stiHResult WAPIsConnected(SstiSSIDListEntry *);
    virtual  stiHResult WAPListGet( WAPListInfo *);
    virtual  stiHResult WirelessSignalStrengthGet(int *);
    virtual  stiHResult WirelessDeviceCarrierGet(bool *);
    virtual  stiHResult WirelessParamCmdSet(char *);
	virtual  stiHResult Connect(const char *,int);
	virtual  stiHResult Connect(const WEP *,int);
	virtual  stiHResult Connect(const WpaPsk *,int);
	virtual  stiHResult Connect(const WpaEap *,int);
};

#endif //CSTIWIRELESS_H

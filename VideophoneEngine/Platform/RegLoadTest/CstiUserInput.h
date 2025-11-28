// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIUSERINPUT_H
#define CSTIUSERINPUT_H

#include "IstiUserInput.h"
#include "CstiOsTaskMQ.h"
#include "CstiSignal.h"
#include <list>

//
// Constants
//

//
// Typedefs
//
	
//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//

class CstiUserInput : public CstiOsTaskMQ, public IstiUserInput
{
public:

	CstiUserInput ();
	
	virtual ~CstiUserInput ();
	
	stiHResult Initialize ();
	
	static IstiUserInput *InstanceGet ();

	ISignal<int> &inputSignalGet () override
	{
		return inputSignal;
	};

	ISignal<int> &remoteInputSignalGet () override
	{
		return remoteInputSignal;
	};

	ISignal<int,bool> &buttonStateSignalGet () override
	{
		return buttonStateSignal;
	};

protected:
	CstiSignal<int> inputSignal;
	CstiSignal<int> remoteInputSignal;
	CstiSignal<int,bool> buttonStateSignal;

private:

	int Task () override;
};
#endif // CSTIUSERINPUT_H

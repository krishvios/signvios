#include "CstiAudioInput.h"
#include "stiTrace.h"

#ifndef WIN32
	#include <err.h>
#endif
//#include <unistd.h>
#include <fcntl.h>

CstiAudioInput *CstiAudioInput::gpCstiAudioInput = NULL;

static const short pcm_silence[] = { // one packet of silence in uLaw
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000 };

/* u-LAW Encode code from 
        http://www.codeproject.com/KB/security/g711audio.aspx
        and ported to C++.
*/
byte encode(short pcm) //16-bit

{

    const int BIAS = 0x84; //132, or 1000 0100
    const int MAX = 32635; //32767 (max 15-bit integer) minus BIAS

    //Get the sign bit. Shift it for later 

    //use without further modification

    int sign = (pcm & 0x8000) >> 8;
    //If the number is negative, make it 

    //positive (now it's a magnitude)

    if (sign != 0)
        pcm = -pcm;
    //The magnitude must be less than 32635 to avoid overflow

    if (pcm > MAX) pcm = MAX;
    //Add 132 to guarantee a 1 in 

    //the eight bits after the sign bit

    pcm += BIAS;

    /* Finding the "exponent"
    * Bits:
    * 1 2 3 4 5 6 7 8 9 A B C D E F G
    * S 7 6 5 4 3 2 1 0 . . . . . . .
    * We want to find where the first 1 after the sign bit is.
    * We take the corresponding value from
    * the second row as the exponent value.
    * (i.e. if first 1 at position 7 -> exponent = 2) */
    int exponent = 7;
    //Move to the right and decrement exponent until we hit the 1

    for (int expMask = 0x4000; (pcm & expMask) == 0; 
         exponent--, expMask >>= 1) { }

    /* The last part - the "mantissa"
    * We need to take the four bits after the 1 we just found.
    * To get it, we shift 0x0f :
    * 1 2 3 4 5 6 7 8 9 A B C D E F G
    * S 0 0 0 0 0 1 . . . . . . . . . (meaning exponent is 2)
    * . . . . . . . . . . . . 1 1 1 1
    * We shift it 5 times for an exponent of two, meaning
    * we will shift our four bits (exponent + 3) bits.
    * For convenience, we will actually just shift
    * the number, then and with 0x0f. */
    int mantissa = (pcm >> (exponent + 3)) & 0x0f;

    //The mu-law byte bit arrangement 

    //is SEEEMMMM (Sign, Exponent, and Mantissa.)

    uint8_t mulaw = (sign | exponent << 4 | mantissa);

    //Last is to flip the bits

    return ~mulaw;
}



//==================================================================================//
CstiAudioInput::CstiAudioInput ()
	:
	m_bPrivacy (estiFALSE),
	m_bRunning (false)
{
	gpCstiAudioInput = this;
}

stiHResult CstiAudioInput::AudioRecordStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	while (!m_qAudioPackets.empty())
	{
		m_qAudioPackets.pop();
	}
	//Send Start Event
	ntouchPC::CstiNativeAudioLink::StartAudioRecording();
	m_bRunning = true;
	return hResult;
}


stiHResult CstiAudioInput::AudioRecordStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_bRunning = false;
	//Send Stop Event
	ntouchPC::CstiNativeAudioLink::StopAudioRecording();
	return hResult;
}

stiHResult CstiAudioInput::AudioRecordCodecSet (EstiAudioCodec eAudioCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

	
stiHResult CstiAudioInput::AudioRecordSettingsSet (const SstiAudioRecordSettings * pAudioRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiAudioInput::PrivacySet (EstiBool bEnable)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (bEnable != m_bPrivacy)
	{
		m_bPrivacy = bEnable;

		audioPrivacySignal.Emit (bEnable);
		
		stiTESTRESULT ();
	}

STI_BAIL:

	return hResult;
}

stiHResult CstiAudioInput::PrivacyGet (EstiBool *pbEnabled) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	*pbEnabled = m_bPrivacy;
	
	return hResult;
}


stiHResult CstiAudioInput::EchoModeSet(EsmdEchoMode eEchoMode)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

int CstiAudioInput::GetDeviceFD () const
{
	return 0;
}

void CstiAudioInput::PacketDeliver(short *ipBuffer, unsigned int inLen)
{
	assert(inLen == 480);
	std::lock_guard<std::recursive_mutex> threadSafe(m_LockMutex);
	if (!m_bPrivacy && m_bRunning) //We have audio privacy on so we need to mute or not send data.
	{
		m_qAudioPackets.push(ipBuffer);
		if (m_qAudioPackets.size () == 1)
		{
			packetReadyStateSignal.Emit (true);
		}
	}
}

stiHResult CstiAudioInput::AudioRecordPacketGet (SsmdAudioFrame * pAudioFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	short * rdr_buffer;
	std::lock_guard<std::recursive_mutex> threadSafe ( m_LockMutex );

	stiTESTCOND(!m_qAudioPackets.empty(), stiRESULT_ERROR);

	rdr_buffer = m_qAudioPackets.front();
	m_qAudioPackets.pop();

	//
	// Should just be the ONE byte that triggered this buffer read, but empty the rdr_buffer queue anyway.
	// If there are more bytes that we can read here, do another sendto().
	//
	stiTESTCOND(pAudioFrame, stiRESULT_ERROR);
	stiTESTCOND(pAudioFrame->pun8Buffer, stiRESULT_ERROR);
	stiTESTCOND(pAudioFrame->unBufferMaxSize >= 240, stiRESULT_ERROR);

	pAudioFrame->unFrameSizeInBytes = 240;			//Always writing out 240 bytes...

	for (int i = 0; i < 240; i++)
	{
		pAudioFrame->pun8Buffer[i] = encode(rdr_buffer[i]);
	}

STI_BAIL:
	if (m_qAudioPackets.size () == 0)
	{
		packetReadyStateSignal.Emit (false);
	}
	return hResult;
}

/*************************************************
* GetInstance - STATIC Singleton.
* This method gets the instance of this class.
*************************************************/
CstiAudioInput *CstiAudioInput::GetInstance()
{
	return gpCstiAudioInput;
}

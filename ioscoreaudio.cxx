/*
 * maccoreaudio.cxx
 *
 * Copyright (c) 2004 Network for Educational Technology ETH
 *
 * Written by Hannes Friederich, Andreas Fenkart.
 * Based on work of Shawn Pai-Hsiang Hsiao
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *          
 */
 
#pragma implementation "ioscoreaudio.h" 

#include "ioscoreaudio.h"
 


PCREATE_SOUND_PLUGIN(iOSAudio, PSoundChannelIOSAudio);
 
namespace PWLibStupidLinkerHacks
{
	int loadCoreAudioStuff;
}

/***** PSoundChannel implementation *****/

PSoundChannelIOSAudio::PSoundChannelIOSAudio() 
   : PSoundChannel()
{
}

PSoundChannelIOSAudio::PSoundChannelIOSAudio(const PString & device,
                Directions dir,
                unsigned numChannels,
                unsigned sampleRate,
                unsigned bitsPerSample)
   : PSoundChannel(device, dir, numChannels, sampleRate, bitsPerSample)
{
}

PSoundChannelIOSAudio::~PSoundChannelIOSAudio()
{
}


PBoolean PSoundChannelIOSAudio::Open(const PString & deviceName,
    Directions dir,
    unsigned numChannels,
    unsigned sampleRate,
    unsigned bitsPerSample)
{
    return PTrue;
}

PString PSoundChannelIOSAudio::GetDefaultDevice(Directions dir)
{
     return CA_DUMMY_DEVICE_NAME;
}

PStringList PSoundChannelIOSAudio::GetDeviceNames(PSoundChannel::Directions dir, PPluginManager * pluginMgr)
{
  PStringList devices;
  devices.AppendString(CA_DUMMY_DEVICE_NAME);
  return devices;
}

// End of file
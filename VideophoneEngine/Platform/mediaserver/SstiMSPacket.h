#pragma once
struct SstiMSPacket
{
	unsigned char* buffer; //Buffer Data
	int length;			   //Length of Data
	bool keyFrame;         //Is a Keyframe
	long clockTime;
};


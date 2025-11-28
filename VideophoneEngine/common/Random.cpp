
#include "Random.h"

namespace vpe
{

RandomBytesEngine::RandomBytesEngine()
	: m_engine(m_random()) // seed engine
{
}

std::string RandomBytesEngine::bytesGet(size_t numBytes)
{
	// for testing
	//return std::string(numBytes, '0');

	// Define a distribution of random chars
	std::uniform_int_distribution<unsigned int> uniform_bytes_dist(
			std::numeric_limits<unsigned char>::min(),
			std::numeric_limits<unsigned char>::max()
			);

	std::string ret;

	ret.reserve(numBytes);

	for(size_t i = 0; i < numBytes; ++i)
	{
		ret += static_cast<unsigned char>(uniform_bytes_dist(m_engine));
	}

	return ret;
}

} // vpe namespace

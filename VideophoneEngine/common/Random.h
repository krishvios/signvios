#pragma once

#include <random>
#include <string>

namespace vpe
{

/*!
 * \brief seeded engine to get random bytes
 */
class RandomBytesEngine
{
public:
	RandomBytesEngine();

	/*!
	* \brief Return a std::string of specified number of bytes
	* \param size of resultant string
	*/
	std::string bytesGet(size_t numBytes);

private:

	std::random_device m_random;

	// probably mersenne twister...
	// TODO: make this static to share amongst instantiations?
	// TODO: If that were the case, it would probably need to be synchronized
	std::default_random_engine m_engine;
};

} // vpe namespace

/*
 * CCRC  crc32 class - Copyright (c) 2004, mypapit (papit58@yahoo.com)
 * zlib/libpng License
 *  
 * 2004-07-30
 */

#include <cstdio>
/*!
 * \brief Useful and simple CRC32 class.
 * 
 */

class CCRC {

	public :
		/**file-base crc*/

		
		/*!
		 
		* \brief Calculates crc32 checksums, by file pointer
		 * \param f
		 * The input file pointer to be checked.
		 * \return
		 * The file calculated CRC32 checksums
		 *
		 */
		unsigned long getFileCRC(FILE *f);
		
		
		/*!
		 
		 * \brief Calculates crc32 checksums, by file pointer and size
		 * The following function was added by Sorenson Media, Inc. to be able to 
		 * calculate a crc on the first un32Bytes of a given file.
		 *
		 * \param f
		 * The input file pointer to be checked.
		 * \param nBytes
		 * The number of bytes to read and check within the file
		 * \return
		 * The file calculated CRC32 checksums
		 *
		 */
		unsigned long getFileCRC(FILE *f, unsigned long nBytes);
		
		
		/*!
	 	  * \brief  Calculates crc32 checksums, by file pointer
		 
		 
		 * \param f
		 * The input file pointer to be checked.
		 *  \param closeable
		 *  The <i>closeable</i> parameter is not implemented
		 *
		 * \return
		 * The file calculated CRC32 checksums
		 * */
		unsigned long getFileCRC(FILE *f, bool closeable);

		/**
		 * \brief Calculates min
		*
		 * \param x
		 * The input file name to be checked
		 * \param y
		 * Not implemented
		 * \return min of x and y
		 * The file calculated CRC32 checksums
		 */
		unsigned long min (unsigned long x, unsigned long y);
		
		/**
		 * \brief Calculates crc32 checksums, using the filename
		*
		 * \param filename
		 * The input file name to be checked
		 * \param closeable
		 * Not implemented
		 * \return 
		 * The file calculated CRC32 checksums
		 */
		unsigned long getFileCRC(char *filename, bool closeable);

		//********************************
		//string, block byte crc function
		//*******************************
		
		/*! 
		 * \brief Calculate  crc checksum by a given string
		 * \param string
		 * The string to be calculated
		 * \param len
		 * The length of the string to be calculated
		 * \return
		 * The calculated CRC32 checksum of the string
		 */
		unsigned long getStringCRC( const char *string, size_t len);

		
		/**
		 * \brief  Calculate crc checksum from a block of data 
		 * \param buf
		 * Data block  to be calculated.
		 * \param size
		 * The size of individual data in the buffer, i.e : sizeof(long), sizeof(int)
		 * \param len
		 * Length of buffer 
		 * \return
		 * The calculated CRC32 checksum of the data block
		 */
		
		unsigned long getBlockCRC( void *buf,  int size, size_t len);

		//******************************************
		//misc function
		//****************************************
		
		/** 
		 * \brief Get the latest (recent) crc32 checksum results
		 * \return
		 * The most recent CRC32 checksum value
		 */
		unsigned long getCurrentCRC ();

		/** Default constructor */
		CCRC() = default;
		~CCRC() = default;

		CCRC (const CCRC &other) = default;
		CCRC (CCRC &&other) = default;
		CCRC &operator=(const CCRC &other) = default;
		CCRC &operator=(CCRC &&other) = default;
		
		/** \brief  Update the crc
		*\param buf
		*\param n
		 * */
		void accumulateCRC (
			const char *buf,
			int n);
		
		void prepareCRC ();
		
	//private data, dont thinker...
	//
	private :
		unsigned long m_crc{0};
		unsigned long updateCRC(const char *buf, size_t n, unsigned long crc);
		char m_buffer[4096]{};
	

};

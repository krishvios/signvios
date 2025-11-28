#pragma once
#include "stiError.h"
#include "stiDefs.h"
#include <cinttypes>

class IstiVideoPlaybackFrame
{
public:

	IstiVideoPlaybackFrame (const IstiVideoPlaybackFrame &other) = delete;
	IstiVideoPlaybackFrame (IstiVideoPlaybackFrame &&other) = delete;
	IstiVideoPlaybackFrame &operator= (const IstiVideoPlaybackFrame &other) = delete;
	IstiVideoPlaybackFrame &operator= (IstiVideoPlaybackFrame &&other) = delete;

	/*!
	 * \brief Get buffer
	 *
	 * \return uint8_t*
	 */
	virtual uint8_t *BufferGet () = 0;		// pointer to the video packet data

	/*!
	 * \brief Get buffer size
	 *
	 * \return uint32_t
	 */
	virtual uint32_t BufferSizeGet () const = 0;	// size of this buffer in bytes

	/*!
	 * \brief BufferSizeSet
	 *
	 * \param un32BufferSize
	 *
	 * \return stiHResult
	 */
	virtual stiHResult BufferSizeSet (uint32_t ) = 0;

	/*!
	 * \brief Get data size
	 *
	 * \return uint32_t
	 */
	virtual uint32_t DataSizeGet () const = 0;		// Number of bytes in the buffer.

	/*!
	 * \brief Get data size
	 *
	 * \param un32DataSize
	 *
	 * \return stiHResult
	 */
	virtual stiHResult DataSizeSet (
		uint32_t un32DataSize) = 0;

	virtual bool FrameIsCompleteGet () = 0;
	virtual void FrameIsCompleteSet (bool bFrameIsComplete) = 0;
	virtual void FrameIsKeyframeSet (bool bFrameIsKeyframe) = 0;
	virtual bool FrameIsKeyframeGet () = 0;

protected:

	IstiVideoPlaybackFrame () = default;
	virtual ~IstiVideoPlaybackFrame() = default;
};

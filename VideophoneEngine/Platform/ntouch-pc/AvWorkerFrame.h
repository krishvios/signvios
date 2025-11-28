#pragma once
#include "IVideoDisplayFrame.h"
extern "C"
{
#include "libavcodec/avcodec.h"
}

class AvWorkerFrame :
    public IVideoDisplayFrame
{
public:
    AvWorkerFrame ();
    ~AvWorkerFrame ();
    // Inherited via IVideoDisplayFrame
    virtual std::tuple<uint8_t *, uint8_t *, uint8_t *> data () override;
    virtual std::tuple<int, int, int> stride () override;
    virtual int widthGet () const override;
    virtual int heightGet () const override;
    virtual EstiColorSpace colorspaceGet () override;

    void frameReset();

    AVFrame *m_frame = nullptr;
};


#include "GStreamerPipeline.h"
#include "stiDefs.h"

class DisplaySinkBin : public GStreamerBin
{
public:
	DisplaySinkBin(void *qquickItem, bool mirrored);
	~DisplaySinkBin () override = default;

private:
};

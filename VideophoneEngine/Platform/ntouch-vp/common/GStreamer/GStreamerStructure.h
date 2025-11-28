#include <gst/gst.h>


class GStreamerStructure
{
public:

	GStreamerStructure (GstStructure *structure)
	:
		m_structure (structure)
	{
	}

	bool intGet (const std::string &name, int *value)
	{
		return gst_structure_get_int (m_structure, name.c_str (), value);
	}

private:

	GstStructure *m_structure {nullptr};
};

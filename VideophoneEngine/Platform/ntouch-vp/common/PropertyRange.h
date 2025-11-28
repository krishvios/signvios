#pragma once

struct PropertyRange
{
	PropertyRange () = default;

	PropertyRange (
		int minValue,
		int maxValue,
		int defaultValue,
		double stepValue = 0.0)
	:
		min (minValue),
		max (maxValue),
		nDefault (defaultValue),
		step (stepValue)
	{}

	int min {0};
	int max {0};
	int nDefault {0};
	double step {0.0};
};

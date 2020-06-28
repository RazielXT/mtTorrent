#pragma once

class SpeedChart
{
public:
	SpeedChart();

	void update(float dlSpeed, float upSpeed);

	void resetChart();

private:

	int chartTime = 0;

};

#pragma once
#ifndef CLUBDATA
#define CLUBDATA

#define VBINS 150
#define HBINS 15
#define CLUBS 22

#define THRESHOLD 240

#define MINCLUBCHANGE 750

class data {
	public:
		const static int club_image_data_1080p[CLUBS][HBINS][VBINS];
};

#endif
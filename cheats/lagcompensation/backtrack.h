#pragma once

#include "lagcompensation.h"
#include "..\misc\logs.h"

class backtrack : public singleton <backtrack>
{
	std::deque <adjust_data> records[65];

	bool should_backup[65];
	adjust_data backup_data[65];
public:
	std::deque <adjust_data> get_records(int i);

	void store_record(player_t* e);
	void clear_records(player_t* e);

	void adjust_player(player_t* e, adjust_data record);
	void backup_player(player_t* e);

	bool is_valid(float simulation_time, float curtime = FLT_MIN);
};
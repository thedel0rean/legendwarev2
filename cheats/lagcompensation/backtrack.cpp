// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "backtrack.h"
#include "..\misc\misc.h"

std::deque <adjust_data> backtrack::get_records(int i)
{
	return records[i];
}

void backtrack::store_record(player_t* e)
{
	if (e->m_flSimulationTime() < e->m_flOldSimulationTime())
		return;
	
	if (!records[e->EntIndex()].empty() && (e->GetAbsOrigin() - records[e->EntIndex()].back().origin).LengthSqr() > 4096.0f)
		records[e->EntIndex()].clear();

	records[e->EntIndex()].emplace_back(adjust_data(e));

	while (records[e->EntIndex()].size() > 28)
		records[e->EntIndex()].pop_front();
}

void backtrack::clear_records(player_t* e)
{
	records[e->EntIndex()].clear();
	should_backup[e->EntIndex()] = false;
}

void backtrack::adjust_player(player_t* e, adjust_data record)
{
	if (!should_backup[e->EntIndex()])
		backup_data[e->EntIndex()].store_data(e);

	record.adjust_player();
	should_backup[e->EntIndex()] = true;
}

void backtrack::backup_player(player_t* e)
{
	if (!should_backup[e->EntIndex()])
		return;

	backup_data[e->EntIndex()].adjust_player();
	should_backup[e->EntIndex()] = false;
}

bool backtrack::is_valid(float simulation_time, float curtime)
{
	auto net_channel_info = m_engine()->GetNetChannelInfo();

	if (!net_channel_info)
		return false;

	if (curtime == FLT_MIN)
		curtime = util::server_time();

	auto correct = net_channel_info->GetLatency(FLOW_OUTGOING) + net_channel_info->GetLatency(FLOW_INCOMING) + lagcompensation::get().get_interpolation();

	static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));
	correct = math::clamp(correct, 0.0f, sv_maxunlag->GetFloat());

	auto delta_time = fabs(correct - (curtime - simulation_time));
	return delta_time <= sv_maxunlag->GetFloat();
}
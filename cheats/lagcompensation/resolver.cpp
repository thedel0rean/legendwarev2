// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "lagcompensation.h"
#include "..\ragebot\aimbot.h"
#include "..\autowall\autowall.h"
#include "..\misc\logs.h"

void c_player_resolver::run()
{
	auto e = m_e;

	if (e->m_flSimulationTime() != e->m_flOldSimulationTime() && g_ctx.globals.check_aa_enable[e->EntIndex()]) //-V550
		check_aa();

	if (g_ctx.globals.check_aa[e->EntIndex()])
	{
		resolve_yaw = 0.0f;
		resolve();

		if (e->m_flSimulationTime() != e->m_flOldSimulationTime()) //-V550
			lagcompensation::get().previous_yaw[e->EntIndex()] = math::normalize_yaw(e->m_angEyeAngles().y);
	}

	if (g_cfg.ragebot.pitch_antiaim_correction)
		pitch_resolve();
}

void c_player_resolver::resolve()
{
	auto e = m_e;
	auto resolve_index = g_ctx.globals.missed_shots[e->EntIndex()] % 2;

	if (g_ctx.globals.lby[e->EntIndex()])
		resolve_index = g_ctx.globals.missed_shots[e->EntIndex()] % 3;

	if (!resolve_index)
		check_lby();

	auto current_side = SIDE_RESOLVER_FIRST;

	if (g_ctx.globals.history_data[e->EntIndex()].type_normal > 1)
		current_side = SIDE_RESOLVER_SECOND;

	check_side(resolve_index);

	if (g_ctx.globals.lby[e->EntIndex()] && g_ctx.globals.history_data[e->EntIndex()].type_low_delta)
	{
		if (g_ctx.globals.check_side[e->EntIndex()] == current_side)
		{
			switch (resolve_index)
			{
			case 1:
				resolve_yaw = 90.0f;
				break;
			case 2:
				resolve_yaw = -90.0f;
				break;
			}
		}
		else
		{
			switch (resolve_index)
			{
			case 1:
				resolve_yaw = -90.0f;
				break;
			case 2:
				resolve_yaw = 90.0f;
				break;
			}
		}
	}
	else
	{
		if (g_ctx.globals.check_side[e->EntIndex()] == current_side)
		{
			switch (resolve_index)
			{
			case 0:
				resolve_yaw = 90.0f;
				break;
			case 1:
				resolve_yaw = -90.0f;
				break;
			}
		}
		else
		{
			switch (resolve_index)
			{
			case 0:
				resolve_yaw = -90.0f;
				break;
			case 1:
				resolve_yaw = 90.0f;
				break;
			}
		}
	}

	auto weapon = e->m_hActiveWeapon().Get();

	if (weapon && fabs(m_globals()->m_curtime - weapon->m_fLastShotTime()) <= TICKS_TO_TIME(1))
		resolve_yaw = -resolve_yaw;

	yaw = math::normalize_yaw(e->m_angEyeAngles().y + resolve_yaw);
}

void c_player_resolver::check_aa()
{
	auto e = m_e;

	if (fabs(math::normalize_pitch(e->m_angEyeAngles().x)) > 45.0f)
	{
		g_ctx.globals.check_aa[e->EntIndex()] = true;
		return;
	}

	auto weapon = e->m_hActiveWeapon().Get();

	if (weapon && fabs(m_globals()->m_curtime - weapon->m_fLastShotTime()) <= TICKS_TO_TIME(1))
	{
		g_ctx.globals.check_aa[e->EntIndex()] = true;
		return;
	}
	
	g_ctx.globals.check_aa[e->EntIndex()] = false;
}

void c_player_resolver::check_side(int resolve_index)
{
	auto e = m_e;
	auto jitter = false;

	static int jitter_ticks[65];
	static int none_jitter_ticks[65];

	auto eye_angles = math::normalize_yaw(e->m_angEyeAngles().y);
	auto is_jittering = fabs(eye_angles - lagcompensation::get().previous_yaw[e->EntIndex()]) > e->get_max_desync_delta();

	if (is_jittering)
	{
		jitter_ticks[e->EntIndex()]++;
		none_jitter_ticks[e->EntIndex()] = 0;
	}
	else
		none_jitter_ticks[e->EntIndex()]++;

	if (none_jitter_ticks[e->EntIndex()] > 16)
	{
		none_jitter_ticks[e->EntIndex()] = 17;
		jitter_ticks[e->EntIndex()] = 0;
	}

	if (jitter_ticks[e->EntIndex()] > 16)
	{
		jitter_ticks[e->EntIndex()] = 17;
		jitter = true;
	}

	if (resolve_index && !jitter)
		return;

	if (jitter)
	{
		auto jitter_resolve_index = g_ctx.globals.missed_shots[e->EntIndex()] % 4;

		if (g_ctx.globals.lby[e->EntIndex()])
			jitter_resolve_index = g_ctx.globals.missed_shots[e->EntIndex()] % 6;

		if (jitter_resolve_index > 1)
			return;

		static auto jitter_check_angle = math::normalize_yaw(eye_angles + lagcompensation::get().previous_yaw[e->EntIndex()]) * 0.5f;

		if (is_jittering)
			jitter_check_angle = math::normalize_yaw(eye_angles + lagcompensation::get().previous_yaw[e->EntIndex()]) * 0.5f;

		if (g_ctx.globals.check_side_type[e->EntIndex()])
		{
			if (eye_angles < jitter_check_angle)
			{
				g_ctx.globals.check_side[e->EntIndex()] = SIDE_RESOLVER_FIRST;
				return;
			}
			else
			{ 
				g_ctx.globals.check_side[e->EntIndex()] = SIDE_RESOLVER_SECOND;
				return;
			}
		}
		else
		{
			if (eye_angles > jitter_check_angle)
			{
				g_ctx.globals.check_side[e->EntIndex()] = SIDE_RESOLVER_SECOND;
				return;
			}
			else
			{
				g_ctx.globals.check_side[e->EntIndex()] = SIDE_RESOLVER_FIRST;
				return;
			}
		}
	}
	else
	{
		auto at_target_angle = math::normalize_yaw(math::calculate_angle(e->get_shoot_position(), g_ctx.local()->GetAbsOrigin()).y + 180.0f);
		auto angle_difference = math::normalize_yaw(eye_angles - at_target_angle);

		if (fabs(angle_difference) > 80.0f)
		{ 
			if (angle_difference < 0.0f)
			{
				if (g_ctx.globals.check_side_type[e->EntIndex()])
				{
					lagcompensation::get().first_ticks[e->EntIndex()]++;
					lagcompensation::get().second_ticks[e->EntIndex()] = 0;
				}
				else
				{
					lagcompensation::get().second_ticks[e->EntIndex()]++;
					lagcompensation::get().first_ticks[e->EntIndex()] = 0;
				}
			}
			else if (angle_difference > 0.0f)
			{
				if (g_ctx.globals.check_side_type[e->EntIndex()])
				{
					lagcompensation::get().second_ticks[e->EntIndex()]++;
					lagcompensation::get().first_ticks[e->EntIndex()] = 0;
				}
				else
				{
					lagcompensation::get().first_ticks[e->EntIndex()]++;
					lagcompensation::get().second_ticks[e->EntIndex()] = 0;
				}
			}
		}
		else
		{
			if (angle_difference > 0.0f)
			{
				if (g_ctx.globals.check_side_type[e->EntIndex()])
				{
					lagcompensation::get().first_ticks[e->EntIndex()]++;
					lagcompensation::get().second_ticks[e->EntIndex()] = 0;
				}
				else
				{
					lagcompensation::get().second_ticks[e->EntIndex()]++;
					lagcompensation::get().first_ticks[e->EntIndex()] = 0;
				}
			}
			else if (angle_difference < 0.0f)
			{
				if (g_ctx.globals.check_side_type[e->EntIndex()])
				{
					lagcompensation::get().second_ticks[e->EntIndex()]++;
					lagcompensation::get().first_ticks[e->EntIndex()] = 0;
				}
				else
				{
					lagcompensation::get().first_ticks[e->EntIndex()]++;
					lagcompensation::get().second_ticks[e->EntIndex()] = 0;
				}
			}
		}
	}
	
	if (lagcompensation::get().first_ticks[e->EntIndex()] > 16)
	{
		lagcompensation::get().first_ticks[e->EntIndex()] = 17;
		lagcompensation::get().second_ticks[e->EntIndex()] = 0;

		g_ctx.globals.check_side[e->EntIndex()] = SIDE_RESOLVER_FIRST;
	}
	else if (lagcompensation::get().second_ticks[e->EntIndex()] > 16)
	{
		lagcompensation::get().second_ticks[e->EntIndex()] = 17;
		lagcompensation::get().first_ticks[e->EntIndex()] = 0;

		g_ctx.globals.check_side[e->EntIndex()] = SIDE_RESOLVER_SECOND;
	}
}

void c_player_resolver::check_lby()
{
	auto e = m_e;

	if (e->m_flSimulationTime() == e->m_flOldSimulationTime()) //-V550
		return;

	auto is_lby = false;

	for (auto i = 0; i < e->animlayer_count(); i++)
	{
		auto layer = e->get_animlayers()[i];

		if (e->sequence_activity(layer.m_nSequence) == ACT_CSGO_IDLE_TURN_BALANCEADJUST && layer.m_flWeight && layer.m_flCycle < 0.9f)
			is_lby = true;
	}

	static int lby_ticks[65];

	if (is_lby)
		lby_ticks[e->EntIndex()]++;
	else
		lby_ticks[e->EntIndex()] = 0;

	if (lby_ticks[e->EntIndex()] > 16)
	{
		lby_ticks[e->EntIndex()] = 17;
		g_ctx.globals.lby[e->EntIndex()] = true;
		return;
	}

	g_ctx.globals.lby[e->EntIndex()] = false;
}

void c_player_resolver::pitch_resolve()
{
	auto e = m_e;

	if (!(e->m_fFlags() & FL_ONGROUND) && e->m_angEyeAngles().x >= 178.36304f)
		pitch = -89.0f;
	else
	{
		if (fabs(e->m_angEyeAngles().x) > 89.0f)
			pitch = g_ctx.globals.fired_shots[e->EntIndex()] % 4 != 3 ? 89.0f : -89.0f;
		else if (g_ctx.globals.fired_shots[e->EntIndex()] % 4 != 3)
			pitch = 89.0f;
	}
}
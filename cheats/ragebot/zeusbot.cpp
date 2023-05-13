// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "zeusbot.h"

void zeusbot::zeus_run(CUserCmd* cmd)
{
	best_index = -1;

	if (g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_TASER)
		return;

	if (!best_target_zeus())
		return;

	if (!g_ctx.globals.weapon->can_fire(false))
		return;

	auto aim_point = best_entity->hitbox_position(HITBOX_PELVIS);
	auto aim_angle = math::calculate_angle(g_ctx.globals.eye_pos, aim_point);

	if (autowall::get().wall_penetration(g_ctx.globals.eye_pos, aim_point, best_entity).damage >= best_entity->m_iHealth() + 75 && aimbot::get().hitchance(best_entity, aim_angle, aim_point, HITBOX_PELVIS, 80.0f, false, false))
	{
		cmd->m_viewangles = aim_angle;
		cmd->m_buttons |= IN_ATTACK;
		cmd->m_tickcount = TIME_TO_TICKS(best_entity->m_flSimulationTime() + lagcompensation::get().get_interpolation());

		g_ctx.globals.aimbot_working = true;
	}
}

bool zeusbot::best_target_zeus()
{
	auto good_distance = FLT_MAX;

	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{
		auto e = (player_t*)m_entitylist()->GetClientEntity(i);

		if (!e->valid(true))
			continue;

		if (e->m_bGunGameImmunity() || e->m_fFlags() & FL_FROZEN)
			continue;

		if (g_cfg.player_list.white_list[i])
			continue;
		
		auto local_position = g_ctx.globals.eye_pos;
		auto entity_position = e->hitbox_position(HITBOX_PELVIS);
		auto current_distance = local_position.DistTo(entity_position);

		if (current_distance < good_distance)
		{
			auto entity_angle = math::calculate_angle(g_ctx.globals.eye_pos, entity_position);

			Vector forward;
			math::angle_vectors(entity_angle, forward);
			forward *= 170.0f;
			
			if (util::visible(local_position, local_position + forward, e, g_ctx.local()))
			{
				good_distance = current_distance;
				best_index = i;
				best_entity = e;
			}
		}
	}

	best_distance = good_distance;
	return best_index != -1;
}
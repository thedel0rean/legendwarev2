// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <random>
#include "aimbot.h"
#include "..\autowall\autowall.h"
#include "..\misc\misc.h"
#include "..\misc\prediction_system.h"
#include "..\misc\logs.h"

void aimbot::create_move(CUserCmd* m_pcmd)
{
	if (g_ctx.globals.weapon->is_non_aim())
		return;

	if (g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_TASER)
		return;

	static auto r8cock_flag = true;

	if (m_engine()->IsActiveApp() && g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !(m_pcmd->m_buttons & IN_ATTACK))
	{
		auto server_time = util::server_time();

		g_ctx.globals.revolver_working = true;
		r8cock_flag = true;

		if (r8cock_flag && g_ctx.globals.weapon->can_fire(false))
		{
			if (g_ctx.globals.r8cock_time <= server_time)
			{
				if (g_ctx.globals.weapon->m_flNextSecondaryAttack() <= server_time)
					g_ctx.globals.r8cock_time = server_time + 0.234375f;
				else
					m_pcmd->m_buttons |= IN_ATTACK2;
			}
			else
				m_pcmd->m_buttons |= IN_ATTACK;

			r8cock_flag = server_time > g_ctx.globals.r8cock_time;
		}
		else
		{
			r8cock_flag = false;
			g_ctx.globals.r8cock_time = server_time + 0.234375f;
			m_pcmd->m_buttons &= ~IN_ATTACK;
		}
	}

	iterate_players(m_pcmd);
}

bool compare_records(const adjust_data& first, const adjust_data& second)
{
	auto first_pitch = math::normalize_pitch(first.angles.x);
	auto second_pitch = math::normalize_pitch(second.angles.x);

	if (fabs(first_pitch - second_pitch) > 15.0f) //-V550
		return fabs(first_pitch) < fabs(second_pitch);
	else if (first.duck_amount != second.duck_amount) //-V550
		return first.duck_amount < second.duck_amount;
	else if (first.origin != second.origin) //-V550
		return first.origin.DistTo(g_ctx.local()->GetAbsOrigin()) < second.origin.DistTo(g_ctx.local()->GetAbsOrigin());
	else
		return first.simulation_time > second.simulation_time;
}

void aimbot::iterate_players(CUserCmd* m_pcmd)
{
	std::vector <int> enemies;

	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (!e->valid(true, false))
		{
			target[i].reset();
			continue;
		}

		if (e->m_bGunGameImmunity() || e->m_fFlags() & FL_FROZEN)
		{
			target[i].reset();
			continue;
		}

		if (g_cfg.player_list.white_list[i])
		{
			target[i].reset();
			continue;
		}

		if (m_clientstate()->m_iChockedCommands != 6 && m_clientstate()->m_iChockedCommands != 7 && !fakelag::get().condition && key_binds::get().get_key_bind_state(20))
		{
			target[i].reset();
			continue;
		}

		enemies.emplace_back(i);
	}

	std::shuffle(enemies.begin(), enemies.end(), std::default_random_engine(m_pcmd->m_command_number));
	auto scanned_targets = 0;

	for (auto i : enemies)
	{
		if (scanned_targets >= 2)
			break;

		auto e = static_cast<player_t *>(m_entitylist()->GetClientEntity(i));
		auto records = backtrack::get().get_records(i);

		auto expoits = false;
		auto instant_dobule_tap = g_cfg.ragebot.double_tap && g_cfg.ragebot.instant && misc::get().double_tap_enabled && !g_ctx.globals.double_tap_aim && g_ctx.globals.weapon->can_double_tap();

		if (!instant_dobule_tap)
			expoits = (g_cfg.antiaim.hide_shots && misc::get().hide_shots_enabled || g_cfg.ragebot.double_tap && misc::get().double_tap_enabled && !g_cfg.ragebot.instant && g_ctx.globals.weapon->can_double_tap()) && (g_ctx.globals.tickbase_shift || g_ctx.globals.next_tickbase_shift);

		if (expoits || instant_dobule_tap)
		{
			if (records.empty())
			{
				target[i].reset();
				continue;
			}

			auto scanned = false;
			scan_data history_data;

			std::sort(records.begin(), records.end(), compare_records);
			auto history_record = -1;

			for (auto i = 0; i < records.size(); i++)
			{
				auto record = records.at(i);

				if (record.dormant)
					continue;

				if (instant_dobule_tap && g_ctx.globals.fixed_tickbase < TIME_TO_TICKS(record.simulation_time))
					continue;

				if (!backtrack::get().is_valid(record.simulation_time, TICKS_TO_TIME(g_ctx.globals.fixed_tickbase)))
					continue;
				
				backtrack::get().adjust_player(e, record);
				best_point(e, history_data, false);

				scanned = true;

				if (!history_data.aim_point.IsZero())
					history_record = i;

				break;
			}

			if (scanned)
				++scanned_targets;

			if (history_record != -1)
			{
				target[i].aim_point = history_data.aim_point;
				target[i].hitbox = history_data.hitbox;
				target[i].damage = history_data.damage;

				target[i].best_record = records.at(history_record);
				backtrack::get().adjust_player(e, target[i].best_record);
			}
			else
			{
				target[i].reset();
				continue;
			}
		}
		else if (records.size() <= 1)
		{
			if (e->IsDormant())
			{
				target[i].reset();
				continue;
			}

			scan_data last_data;
			best_point(e, last_data, false);

			++scanned_targets;

			if (last_data.aim_point.IsZero())
			{
				target[i].reset();
				continue;
			}
			else
			{
				target[i].aim_point = last_data.aim_point;
				target[i].hitbox = last_data.hitbox;
				target[i].damage = last_data.damage;

				target[i].best_record.reset();
			}
		}
		else
		{
			auto scanned = false;
			scan_data last_data, history_data;

			if (!e->IsDormant())
			{
				best_point(e, last_data, false);
				scanned = true;
			}

			std::sort(records.begin(), records.end(), compare_records);	
			auto history_record = -1;

			for (auto i = 0; i < records.size(); i++)
			{
				auto record = records.at(i);

				if (record.dormant)
					continue;

				if (!backtrack::get().is_valid(record.simulation_time))
					continue;

				backtrack::get().adjust_player(e, record);
				best_point(e, history_data, false);

				scanned = true;

				if (!history_data.aim_point.IsZero())
					history_record = i;

				break;
			}

			if (scanned)
				++scanned_targets;

			if (history_record != -1 && history_data.damage >= last_data.damage)
			{
				target[i].aim_point = history_data.aim_point;
				target[i].hitbox = history_data.hitbox;
				target[i].damage = history_data.damage;

				target[i].best_record = records.at(history_record);
				backtrack::get().adjust_player(e, target[i].best_record);
			}
			else if (!last_data.aim_point.IsZero())
			{
				target[i].aim_point = last_data.aim_point;
				target[i].hitbox = last_data.hitbox;
				target[i].damage = last_data.damage;

				target[i].best_record.reset();
				backtrack::get().backup_player(e);
			}
			else
			{
				target[i].reset();
				continue;
			}
		}

		auto fov = math::get_fov(engine_angles, math::calculate_angle(g_ctx.globals.eye_pos, target[i].aim_point));

		if (fov > (float)config.fov)
		{
			target[i].reset();
			continue;
		}

		target[i].init
		(
			fov,
			g_ctx.globals.eye_pos.DistTo(target[i].aim_point),
			e->m_iHealth()
		);

		target[i].can_hurt_ticks++;
	}

	if (g_ctx.globals.weapon->can_fire(true))
	{
		target_id = -1;

		for (auto i = 1; i < m_globals()->m_maxclients; i++)
			target_side[i] = NONE;
	}

	if (config.autostop && config.autostop_modifiers[AUTOSTOP_FORCE_ACCURACY])
	{
		auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

		if (!weapon_info)
		{
			for (auto i = 1; i < m_globals()->m_maxclients; i++)
				target[i].reset();

			return;
		}

		auto velocity = engineprediction::get().backup_data.velocity;
		auto max_speed = 0.34f * (g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed);

		if (velocity.Length2D() >= max_speed)
		{
			for (auto i = 1; i < m_globals()->m_maxclients; i++)
				target[i].reset();

			return;
		}
	}

	get_target(m_pcmd);
}

void aimbot::get_target(CUserCmd* m_pcmd)
{
	auto found_high_priority = false;

	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{ 
		if (g_cfg.player_list.high_priority[i] && target[i].can_hurt_ticks)
		{
			found_high_priority = true;
			break;
		}
	}

	if (found_high_priority)
		for (auto i = 1; i < m_globals()->m_maxclients; i++)
			if (!g_cfg.player_list.high_priority[i])
				target[i].can_hurt_ticks = 0;
	
	auto aim_point = ZERO;

	auto best_index = INT_MAX;
	auto best_fov = FLT_MAX;
	auto best_distance = FLT_MAX;
	auto best_health = INT_MAX;
	auto best_damage = INT_MIN;

	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{
		if (!target[i].can_hurt_ticks)
			continue;
		
		switch (config.selection_type)
		{
		case 0:
			if (i < best_index)
			{
				best_index = i;
				target_id = i;
				aim_point = target[i].aim_point;
			}
			break;
		case 1:
			if (target[i].fov < best_fov)
			{
				best_fov = target[i].fov;
				target_id = i;
				aim_point = target[i].aim_point;
			}
			break;
		case 2:
			if (target[i].distance < best_distance)
			{
				best_distance = target[i].distance;
				target_id = i;
				aim_point = target[i].aim_point;
			}
			break;
		case 3:
			if (target[i].health < best_health)
			{
				best_health = target[i].health;
				target_id = i;
				aim_point = target[i].aim_point;
			}
			break;
		case 4:
			if (target[i].damage > best_damage)
			{
				best_damage = target[i].damage;
				target_id = i;
				aim_point = target[i].aim_point;
			}
			break;
		}
	}

	aim(m_pcmd, aim_point);
}

void aimbot::aim(CUserCmd* m_pcmd, const Vector& aim_point)
{
	if (target_id == -1)
		return;

	auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

	if (!weapon_info)
		return;

	if (!g_ctx.globals.weapon->can_fire(true))
		return;

	auto is_zoomable_weapon =
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 ||
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1 ||
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08 ||
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP ||
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AUG ||
		g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SG553;

	if (config.autoscope && is_zoomable_weapon && !g_ctx.globals.weapon->m_zoomLevel())
	{
		m_pcmd->m_buttons |= IN_ATTACK2;
		return;
	}

	auto e = static_cast<player_t *>(m_entitylist()->GetClientEntity(target_id));
	auto next_angle = math::calculate_angle(g_ctx.globals.eye_pos, aim_point);

	auto maximum_accuracy = false;

	if (config.skip_hitchance_if_low_inaccuracy && !g_ctx.globals.double_tap_aim && g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND) //-V807
	{
		auto weapon_info_inaccuracy = g_ctx.globals.weapon->m_zoomLevel() ? weapon_info->flInaccuracyStandAlt : weapon_info->flInaccuracyStand;

		if (engineprediction::get().backup_data.flags & FL_DUCKING)
			weapon_info_inaccuracy = g_ctx.globals.weapon->m_zoomLevel() ? weapon_info->flInaccuracyCrouchAlt : weapon_info->flInaccuracyCrouch;

		if (g_ctx.globals.inaccuracy - weapon_info_inaccuracy < 0.0001f)
			maximum_accuracy = true;
	}

	if (!maximum_accuracy)
	{
		auto hitchanced = true;

		if (g_ctx.globals.double_tap_aim ? config.double_tap_hitchance : config.hitchance)
		{
			if (g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_SSG08 || g_ctx.local()->m_fFlags() & FL_ONGROUND || engineprediction::get().backup_data.flags & FL_ONGROUND || g_ctx.local()->m_vecVelocity().Length2D() >= 5.0f || fabs(g_ctx.local()->m_vecVelocity().z) >= 5.0f)
				hitchanced = false;

			if (!hitchanced)
				hitchanced = hitchance(e, next_angle, target[target_id].aim_point, target[target_id].hitbox, (float)(g_ctx.globals.double_tap_aim ? config.double_tap_hitchance_amount : config.hitchance_amount), !g_ctx.globals.double_tap_aim, true);
		}

		if (!hitchanced)
			return;
	}

	if (!config.silent_aim)
		m_engine()->SetViewAngles(next_angle);

	if (config.autoshoot || m_pcmd->m_buttons & IN_ATTACK)
	{
		static auto get_hitbox_name = [](int hitbox, bool shot_info = false) -> std::string
		{
			switch (hitbox)
			{
			case HITBOX_HEAD:
				return shot_info ? crypt_str("Head") : crypt_str("head");
			case HITBOX_CHEST:
				return shot_info ? crypt_str("Chest") : crypt_str("chest");
			case HITBOX_STOMACH:
				return shot_info ? crypt_str("Stomach") : crypt_str("stomach");
			case HITBOX_PELVIS:
				return shot_info ? crypt_str("Pelvis") : crypt_str("pelvis");
			case HITBOX_RIGHT_UPPER_ARM:
			case HITBOX_RIGHT_FOREARM:
			case HITBOX_RIGHT_HAND:
				return shot_info ? crypt_str("Left arm") : crypt_str("left arm");
			case HITBOX_LEFT_UPPER_ARM:
			case HITBOX_LEFT_FOREARM:
			case HITBOX_LEFT_HAND:
				return shot_info ? crypt_str("Right arm") : crypt_str("right arm");
			case HITBOX_RIGHT_THIGH:
			case HITBOX_RIGHT_CALF:
				return shot_info ? crypt_str("Left leg") : crypt_str("left leg");
			case HITBOX_LEFT_THIGH:
			case HITBOX_LEFT_CALF:
				return shot_info ? crypt_str("Right leg") : crypt_str("right leg");
			case HITBOX_RIGHT_FOOT:
				return shot_info ? crypt_str("Left foot") : crypt_str("left foot");
			case HITBOX_LEFT_FOOT:
				return shot_info ? crypt_str("Right foot") : crypt_str("right foot");
			}
		};

		player_info_t player_info;
		m_engine()->GetPlayerInfo(target_id, &player_info);

		std::string hitchance = crypt_str("0");

		if (g_ctx.globals.double_tap_aim ? config.double_tap_hitchance : config.hitchance)
			hitchance = g_ctx.globals.double_tap_aim ? std::to_string(config.double_tap_hitchance_amount) : std::to_string(config.hitchance_amount);

		if (maximum_accuracy)
			hitchance = crypt_str("MA");

		auto backtrack_ticks = 0;

		if (target[target_id].best_record.valid())
		{
			auto net_channel_info = m_engine()->GetNetChannelInfo();

			if (net_channel_info)
			{
				auto correct = net_channel_info->GetLatency(FLOW_OUTGOING) + net_channel_info->GetLatency(FLOW_INCOMING) + lagcompensation::get().get_interpolation();

				static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));
				correct = math::clamp(correct, 0.0f, sv_maxunlag->GetFloat());

				auto delta_time = fabs(correct - (util::server_time() - target[target_id].best_record.simulation_time));
				backtrack_ticks = math::clamp(TIME_TO_TICKS(delta_time), 0, 32);
			}
		}

		g_ctx.last_shot_info.target_name = player_info.szName;
		g_ctx.last_shot_info.client_hitbox = get_hitbox_name(target[target_id].hitbox, true);
		g_ctx.last_shot_info.client_damage = target[target_id].damage;
		g_ctx.last_shot_info.hitchance = g_ctx.globals.double_tap_aim ? config.double_tap_hitchance_amount : config.hitchance_amount;
		g_ctx.last_shot_info.backtrack_ticks = backtrack_ticks;
		g_ctx.last_shot_info.aim_point = aim_point;
		
#if BETA
		auto angle = math::calculate_angle(e->get_shoot_position(), g_ctx.local()->GetAbsOrigin()).y;
		auto delta = math::normalize_yaw(e->m_angEyeAngles().y - angle);

		std::stringstream log;

		log << crypt_str("Fired shot at ") + (std::string)player_info.szName + crypt_str(": ");
		log << crypt_str("hitchance: ") + hitchance + crypt_str(", ");
		log << crypt_str("hitbox: ") + get_hitbox_name(target[target_id].hitbox) + crypt_str(", ");
		log << crypt_str("damage: ") + std::to_string(target[target_id].damage) + crypt_str(", ");
		log << crypt_str("backtrack: ") + std::to_string(backtrack_ticks) + crypt_str(", ");
		log << crypt_str("side: ") + std::to_string(g_ctx.globals.side[e->EntIndex()]) + crypt_str(", ");
		log << crypt_str("angle: ") + std::to_string(delta);

		if (g_cfg.misc.events_to_log[EVENTLOG_HIT])
			eventlogs::get().add(log.str());
#endif

		m_pcmd->m_viewangles = next_angle;
		m_pcmd->m_buttons |= IN_ATTACK;

		if (target[target_id].best_record.valid())
		{
			m_pcmd->m_tickcount = TIME_TO_TICKS(target[target_id].best_record.simulation_time + lagcompensation::get().get_interpolation()); //-V807
			lagcompensation::get().player_data[target_id] = target[target_id].best_record;

			target_side[target_id] = target[target_id].best_record.side;
		}
		else
		{
			m_pcmd->m_tickcount = TIME_TO_TICKS(e->m_flSimulationTime() + lagcompensation::get().get_interpolation());
			lagcompensation::get().player_data[target_id].store_data(e);

			target_side[target_id] = (resolver_side)g_ctx.globals.side[target_id];
		}

		for (auto i = 1; i < m_globals()->m_maxclients; i++)
			target[i].reset();

		g_ctx.globals.aimbot_working = true;
		g_ctx.globals.revolver_working = false;
		g_ctx.globals.weapon_fired = true;
		g_ctx.globals.last_aimbot_shot = m_globals()->m_tickcount;
		g_ctx.shot.weapon_fired = false;
	}
}

bool aimbot::hitchance(player_t* e, Vector angles, const Vector& point, int hitbox, float chance, bool accuracy_boost_amount, bool intersection)
{
	angles.Clamp();

	auto forward = ZERO;
	auto right = ZERO;
	auto up = ZERO;

	math::angle_vectors(angles, &forward, &right, &up);

	math::fast_vec_normalize(forward);
	math::fast_vec_normalize(right);
	math::fast_vec_normalize(up);

	auto needed = static_cast <int> (chance / 100.0f * 256.0f);
	auto allowed_misses = 256 - needed;

	auto hits = 0;
	auto i = 0;

	while (i < 256)
	{
		math::random_seed((i & 255) + 1);

		auto b = math::random_float(0.0f, 2.0f * M_PI);
		auto c = math::random_float(0.0f, 1.0f);
		auto d = math::random_float(0.0f, 2.0f * M_PI);

		auto spread_val = c * g_ctx.globals.spread;
		auto inaccuracy_val = c * g_ctx.globals.inaccuracy;

		auto v_spread = Vector(cos(b) * spread_val + cos(d) * inaccuracy_val, sin(b) * spread_val + sin(d) * inaccuracy_val, 0.0f);
		auto direction = ZERO;

		direction.x = forward.x + right.x * v_spread.x + up.x * v_spread.y;
		direction.y = forward.y + right.y * v_spread.x + up.y * v_spread.y;
		direction.z = forward.z + right.z * v_spread.x + up.z * v_spread.y; //-V778

		math::fast_vec_normalize(direction);

		auto spread_view = ZERO;

		math::vector_angles(direction, spread_view);
		spread_view.NormalizeNoClamp();

		auto end = ZERO;

		math::angle_vectors(angles - (spread_view - angles), end);
		math::fast_vec_normalize(end);

		auto trace_end = g_ctx.globals.eye_pos + end * point.DistTo(g_ctx.globals.eye_pos);
		auto intersected = true;

		if (intersection && !can_hit_hitgroup(e, g_ctx.globals.eye_pos, trace_end, hitbox))
			intersected = false;

		if (intersected)
		{
			auto fire_data = autowall::get().wall_penetration(g_ctx.globals.eye_pos, trace_end, e);

			if (fire_data.damage >= 1 && get_hitgroup(hitbox) == get_hitgroup(fire_data.hitbox))
			{
				auto accuracy_boost = true;

				if (accuracy_boost_amount && (float)fire_data.damage / (float)target[target_id].damage < (float)config.accuracy_boost_amount / 100.0f)
					accuracy_boost = false;

				if (accuracy_boost)
					hits++;
			}
		}

		auto hitchance = static_cast <float> (hits) / 255.0f * 100.f;

		if (hitchance >= chance)
			return true;

		if (i - hits > allowed_misses)
			return false;

		i++;
	}

	return false;
}

bool aimbot::can_hit_hitgroup(player_t* e, const Vector& start, const Vector& end, int hitbox)
{
	auto model = e->GetModel();

	if (!model)
		return false;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return false;

	auto studio_set = studio_model->pHitboxSet(e->m_nHitboxSet());

	if (!studio_set)
		return false;

	auto hitboxes = get_hitboxes(get_hitgroup(hitbox));

	for (auto hitbox : hitboxes)
	{
		auto studio_hitbox = studio_set->pHitbox(hitbox);

		if (!studio_hitbox)
			continue;

		auto matrix = e->m_CachedBoneData().Base();
		Vector min, max;

		if (studio_hitbox->radius != -1.0f) //-V550
		{
			math::vector_transform(studio_hitbox->bbmin, matrix[studio_hitbox->bone], min);
			math::vector_transform(studio_hitbox->bbmax, matrix[studio_hitbox->bone], max);

			auto dist = math::segment_to_segment(start, end, min, max);

			if (dist < studio_hitbox->radius)
				return true;
		}
		else
		{
			math::vector_transform(math::vector_rotate(studio_hitbox->bbmin, studio_hitbox->rotation), matrix[studio_hitbox->bone], min);
			math::vector_transform(math::vector_rotate(studio_hitbox->bbmax, studio_hitbox->rotation), matrix[studio_hitbox->bone], max);

			math::vector_transform(start, matrix[studio_hitbox->bone], min);
			math::vector_rotate(end, matrix[studio_hitbox->bone], max);

			if (math::intersect_line_with_bb(min, max, studio_hitbox->bbmin, studio_hitbox->bbmax))
				return true;
		}
	}

	return false;
}

hitgroup aimbot::get_hitgroup(int hitbox)
{
	if (hitbox == HITBOX_HEAD)
		return HEAD;
	else if (hitbox >= HITBOX_NECK && hitbox <= HITBOX_UPPER_CHEST)
		return BODY;
	else if (hitbox >= HITBOX_RIGHT_THIGH && hitbox <= HITBOX_LEFT_FOOT)
		return LEGS;
	else if (hitbox >= HITBOX_RIGHT_HAND && hitbox <= HITBOX_LEFT_FOREARM)
		return ARMS;

	return UNKNOWN;
}

std::vector <int> aimbot::get_hitboxes(hitgroup hitgroup)
{
	std::vector <int> hitboxes;

	switch (hitgroup)
	{
	case HEAD:
		hitboxes.emplace_back(HITBOX_HEAD);
		break;
	case BODY:
		hitboxes.emplace_back(HITBOX_NECK);
		hitboxes.emplace_back(HITBOX_PELVIS);
		hitboxes.emplace_back(HITBOX_STOMACH);
		hitboxes.emplace_back(HITBOX_LOWER_CHEST);
		hitboxes.emplace_back(HITBOX_CHEST);
		hitboxes.emplace_back(HITBOX_UPPER_CHEST);
		break;
	case LEGS:
		hitboxes.emplace_back(HITBOX_RIGHT_THIGH);
		hitboxes.emplace_back(HITBOX_LEFT_THIGH);
		hitboxes.emplace_back(HITBOX_RIGHT_CALF);
		hitboxes.emplace_back(HITBOX_LEFT_CALF);
		hitboxes.emplace_back(HITBOX_RIGHT_FOOT);
		hitboxes.emplace_back(HITBOX_LEFT_FOOT);
		break;
	case ARMS:
		hitboxes.emplace_back(HITBOX_RIGHT_HAND);
		hitboxes.emplace_back(HITBOX_LEFT_HAND);
		hitboxes.emplace_back(HITBOX_RIGHT_UPPER_ARM);
		hitboxes.emplace_back(HITBOX_RIGHT_FOREARM);
		hitboxes.emplace_back(HITBOX_LEFT_UPPER_ARM);
		hitboxes.emplace_back(HITBOX_LEFT_FOREARM);
		break;
	}

	return hitboxes;
}

bool compare_points(const aimbot::point& first, const aimbot::point& second)
{
	return !first.center;
}

bool compare_hitboxes(const int& first, const int& second) //-V751
{
	return first == HITBOX_CHEST || first == HITBOX_STOMACH || first == HITBOX_PELVIS;
}

void aimbot::best_point(player_t* e, scan_data& data, bool optimize_points, Vector eye_pos)
{
	auto output = ZERO;
	auto best_damage = -1;

	auto minimum_visible_damage = 1;
	auto minimum_damage = 1;

	if (config.minimum_visible_damage > 100)
		minimum_visible_damage = e->m_iHealth() + config.minimum_visible_damage - 100;
	else
		minimum_visible_damage = math::clamp(config.minimum_visible_damage, 1, e->m_iHealth());

	if (config.minimum_damage > 100)
		minimum_damage = e->m_iHealth() + config.minimum_damage - 100;
	else
		minimum_damage = math::clamp(config.minimum_damage, 1, e->m_iHealth());

	if (key_binds::get().get_key_bind_state(4 + g_ctx.globals.current_weapon))
	{
		if (config.minimum_override_damage > 100)
			minimum_visible_damage = e->m_iHealth() + config.minimum_override_damage - 100;
		else
			minimum_visible_damage = math::clamp(config.minimum_override_damage, 1, e->m_iHealth());

		if (config.minimum_override_damage > 100)
			minimum_damage = e->m_iHealth() + config.minimum_override_damage - 100;
		else
			minimum_damage = math::clamp(config.minimum_override_damage, 1, e->m_iHealth());
	}

	auto hitboxes = hitboxes_from_vector(e, config.hitscan);

	if (hitboxes.size() >= 2)
		std::sort(hitboxes.begin(), hitboxes.end(), compare_hitboxes);

	auto best_body_damage_autowall = false;
	auto best_body_damage = -1;
	
	for (auto hitbox : hitboxes)
	{
		if (hitbox != HITBOX_CHEST && hitbox != HITBOX_STOMACH && hitbox != HITBOX_PELVIS)
		{
			if (g_cfg.player_list.force_body_aim[e->EntIndex()])
				continue;

			if (key_binds::get().get_key_bind_state(3))
				continue;

			if (config.baim[BAIM_UNRESOLVED] && g_ctx.globals.missed_shots[e->EntIndex()] >= 2 || g_ctx.globals.missed_shots[e->EntIndex()] >= 6)
				continue;

			auto can_hit_body = true;
			auto minimum_body_damage = best_body_damage_autowall ? minimum_damage : minimum_visible_damage;

			switch (config.baim_level)
			{
			case 0:
				if (best_body_damage < minimum_body_damage)
					can_hit_body = false;

				break;
			case 1:
				if (best_body_damage < 1)
					can_hit_body = false;

				break;
			}

			if (can_hit_body)
			{
				if (g_ctx.globals.weapon->can_double_tap() && (misc::get().double_tap_enabled || g_ctx.globals.double_tap_aim))
					continue;

				if (config.baim[BAIM_PREFER])
					continue;

				if (config.baim[BAIM_AIR] && !(e->m_fFlags() & FL_ONGROUND))
					continue;

				if (config.baim[BAIM_HIGH_VELOCITY] && e->m_vecVelocity().Length() > 400.0f)
					continue;

				if (config.baim[BAIM_LETHAL])
				{
					auto minimum_lethal_damage = max(minimum_body_damage, e->m_iHealth());

					if (best_body_damage >= minimum_lethal_damage)
						continue;
					else if (config.baim_level == 2)
					{
						auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

						if (!weapon_info)
							continue;

						if (weapon_info->iDamage >= minimum_lethal_damage)
							continue;
					}
				}
			}
		}

		std::vector <point> points;
		getpoints(e, hitbox, points, optimize_points);

		if (points.size() >= 2)
			std::sort(points.begin(), points.end(), compare_points);

		for (auto point : points)
		{
			auto fire_data = autowall::get().wall_penetration(eye_pos, point.position, e);

			if (get_hitgroup(hitbox) != get_hitgroup(fire_data.hitbox))			
				continue;

			auto can_stop = config.autostop;

			if (can_stop && !point.center && config.autostop_modifiers[AUTOSTOP_CENTER])
				can_stop = false;

			if (fire_data.visible)
			{
				if (fire_data.damage > best_body_damage && (hitbox == HITBOX_CHEST || hitbox == HITBOX_STOMACH || hitbox == HITBOX_PELVIS))
				{
					best_body_damage_autowall = false;
					best_body_damage = fire_data.damage;
				}

				if (fire_data.damage > best_damage && fire_data.damage >= minimum_visible_damage || point.center && best_damage - fire_data.damage < 10 && fire_data.damage >= minimum_visible_damage)
				{
					if (can_stop && config.autostop_modifiers[AUTOSTOP_LETHAL] && fire_data.damage < e->m_iHealth())
						can_stop = false;

					best_damage = fire_data.damage;
					output = point.position;

					data.hitbox = hitbox;
					data.damage = fire_data.damage;

					if (can_stop)
						should_stop = true;
				}
			}
			else if (config.autowall)
			{
				if (fire_data.damage > best_body_damage && (hitbox == HITBOX_CHEST || hitbox == HITBOX_STOMACH || hitbox == HITBOX_PELVIS))
				{
					best_body_damage_autowall = true;
					best_body_damage = fire_data.damage;
				}

				if (fire_data.damage > best_damage && fire_data.damage >= minimum_damage || point.center && best_damage - fire_data.damage < 10 && fire_data.damage >= minimum_damage)
				{
					if (can_stop && (config.autostop_modifiers[AUTOSTOP_VISIBLE] || config.autostop_modifiers[AUTOSTOP_LETHAL] && fire_data.damage < e->m_iHealth())) //-V648
						can_stop = false;

					best_damage = fire_data.damage;
					output = point.position;

					data.hitbox = hitbox;
					data.damage = fire_data.damage;

					if (can_stop)
						should_stop = true;
				}
			}
		}
	}

	data.aim_point = output;
}

void aimbot::getpoints(player_t* e, int hitbox_id, std::vector <point>& points, bool optimize_points)
{
	auto model = e->GetModel();

	if (!model)
		return;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return;

	auto set = studio_model->pHitboxSet(e->m_nHitboxSet());

	if (!set)
		return;

	auto hitbox = set->pHitbox(hitbox_id);

	if (!hitbox)
		return;

	auto max = ZERO;
	auto min = ZERO;

	math::vector_transform(hitbox->bbmin, e->m_CachedBoneData().Base()[hitbox->bone], min);
	math::vector_transform(hitbox->bbmax, e->m_CachedBoneData().Base()[hitbox->bone], max);

	auto center = (min + max) * 0.5f;
	auto current_angles = math::calculate_angle(center, g_ctx.globals.eye_pos);

	auto forward = ZERO;
	math::angle_vectors(current_angles, forward);

	auto top = Vector(0.0f, 0.0f, 1.0f);
	auto bottom = Vector(0.0f, 0.0f, -1.0f);

	auto right = forward.Cross(Vector(0.0f, 0.0f, 1.0f));
	auto left = Vector(-right.x, -right.y, right.z);

	auto top_right = top + right;
	auto top_left = top + left;

	auto bottom_right = bottom + right;
	auto bottom_left = bottom + left;
	
	auto angle = math::calculate_angle(e->get_shoot_position(), g_ctx.local()->GetAbsOrigin()).y;
	auto delta = math::normalize_yaw(e->m_angEyeAngles().y - angle);

	auto backup_pointscale_head = config.pointscale_head;
	auto backup_pointscale_chest = config.pointscale_chest;
	auto backup_pointscale_stomach = config.pointscale_stomach;
	auto backup_pointscale_pelvis = config.pointscale_pelvis;
	auto backup_pointscale_legs = config.pointscale_legs;

	if (optimize_points)
	{
		config.pointscale_head = 0.0f;
		config.pointscale_chest = 0.0f;
		config.pointscale_stomach = 0.0f;
		config.pointscale_pelvis = 0.0f;
		config.pointscale_legs = 0.0f;
	}

	switch (hitbox_id)
	{
	case HITBOX_HEAD:
		if (math::normalize_pitch(e->m_angEyeAngles().x) >= 45.0f)
		{
			if (fabs(delta) > 120.0f)
			{
				auto multi_points_scale = 0.6f + config.pointscale_head * 0.4f;
				auto edge_multi_points_scale = 0.4f + config.pointscale_head * 0.2f;
				
				if (g_ctx.globals.missed_shots[e->EntIndex()] >= 2 && g_ctx.globals.side[e->EntIndex()] == RIGHT)
					points.push_back(point{ center + top_left * hitbox->radius * edge_multi_points_scale, true });
				else if (g_ctx.globals.missed_shots[e->EntIndex()] >= 2 && g_ctx.globals.side[e->EntIndex()] == LEFT)
					points.push_back(point{ center + top_right * hitbox->radius * edge_multi_points_scale, true });
				else
				{
					points.push_back(point{ center + top * hitbox->radius * multi_points_scale, true });
					points.push_back(point{ center + top_right * hitbox->radius * edge_multi_points_scale, false });
					points.push_back(point{ center + top_left * hitbox->radius * edge_multi_points_scale, false });
				}
			}
			else if (fabs(delta) > 80.0f)
			{
				auto multi_points_scale = 0.4f + config.pointscale_head * 0.4f;
				
				if (delta < 0.0f)
					points.push_back(point{ center + right * hitbox->radius * multi_points_scale, false });
				else if (delta > 0.0f)
					points.push_back(point{ center + left * hitbox->radius * multi_points_scale, false });

				points.push_back(point{ center + top * hitbox->radius * multi_points_scale, true });

				if (delta < 0.0f)
					points.push_back(point{ center + top_right * hitbox->radius * multi_points_scale, false });
				else if (delta > 0.0f)
					points.push_back(point{ center + top_left * hitbox->radius * multi_points_scale, false });
			}
			else
			{
				if (config.pointscale_head) //-V550
				{
					auto multi_points_scale = 0.2f + config.pointscale_head * 0.6f;

					points.push_back(point{ center, true });
					points.push_back(point{ center + right * hitbox->radius * multi_points_scale, false });
					points.push_back(point{ center + left * hitbox->radius * multi_points_scale, false });
				}
				else
					points.push_back(point{ center, true });
			}
		}
		else
		{
			if (config.pointscale_head) //-V550
			{
				auto multi_points_scale = 0.2f + config.pointscale_head * 0.6f;

				points.push_back(point{ center, true });
				points.push_back(point{ center + right * hitbox->radius * multi_points_scale, false });
				points.push_back(point{ center + left * hitbox->radius * multi_points_scale, false });
			}
			else
				points.push_back(point{ center, true });
		}

		break;
	case HITBOX_CHEST:
		top.z = 0.7f;

		if (!config.pointscale_chest) //-V550
			points.push_back(point{ center + top * hitbox->radius, true });
		else if (!config.safe_body_points)
		{
			auto multi_points_scale = 0.2f + config.pointscale_chest * 0.6f;

			top_right = top + right * multi_points_scale;
			top_left = top + left * multi_points_scale;

			points.push_back(point{ center + top * hitbox->radius, true });
			points.push_back(point{ center + top_right * hitbox->radius, false });
			points.push_back(point{ center + top_left * hitbox->radius, false });
		}
		else if (fabs(delta) > 120.0f)
		{
			if (g_ctx.globals.side[e->EntIndex()] == NONE || e->m_flDuckAmount() > 0.5f)
				points.push_back(point{ center + top * hitbox->radius, true });
			else
			{
				auto multi_points_scale = 0.6f + config.pointscale_chest * 0.2f;

				top_right = top + right * multi_points_scale;
				top_left = top + left * multi_points_scale;

				if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
					points.push_back(point{ center + top_right * hitbox->radius, true });

				if (g_ctx.globals.side[e->EntIndex()] != LEFT)
					points.push_back(point{ center + top_left * hitbox->radius, true });
			}
		}
		else if (fabs(delta) > 60.0f)
		{
			if (g_ctx.globals.side[e->EntIndex()] == NONE)
				points.push_back(point{ center + top * hitbox->radius, true });
			else
			{
				auto multi_points_scale = 0.6f + config.pointscale_chest * 0.2f;

				top_right = top + right * multi_points_scale;
				top_left = top + left * multi_points_scale;

				if (delta > 0.0f)
				{
					if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
						points.push_back(point{ center + top_right * hitbox->radius, true });

					if (g_ctx.globals.side[e->EntIndex()] != LEFT)
						points.push_back(point{ center + top_left * hitbox->radius, true });
				}
				else
				{
					if (g_ctx.globals.side[e->EntIndex()] != LEFT)
						points.push_back(point{ center + top_right * hitbox->radius, true });

					if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
						points.push_back(point{ center + top_left * hitbox->radius, true });
				}
			}
		}
		else
		{
			if (g_ctx.globals.side[e->EntIndex()] == NONE || e->m_flDuckAmount() > 0.5f)
				points.push_back(point{ center + top * hitbox->radius, true });
			else
			{
				auto multi_points_scale = 0.6f + config.pointscale_chest * 0.2f;

				top_right = top + right * multi_points_scale;
				top_left = top + left * multi_points_scale;

				if (g_ctx.globals.side[e->EntIndex()] != LEFT)
					points.push_back(point{ center + top_right * hitbox->radius, true });

				if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
					points.push_back(point{ center + top_left * hitbox->radius, true });
			}
		}

		break;
	case HITBOX_STOMACH:
		top.z = 0.4f;

		if (!config.pointscale_stomach) //-V550
			points.push_back(point{ center + top * hitbox->radius, true });
		else if (!config.safe_body_points)
		{
			auto multi_points_scale = 0.2f + config.pointscale_stomach * 0.6f;

			top_right = top + right * multi_points_scale;
			top_left = top + left * multi_points_scale;

			points.push_back(point{ center + top * hitbox->radius, true });
			points.push_back(point{ center + top_right * hitbox->radius, false });
			points.push_back(point{ center + top_left * hitbox->radius, false });
		}
		else if (fabs(delta) > 120.0f)
		{
			if (g_ctx.globals.side[e->EntIndex()] == NONE)
				points.push_back(point{ center + top * hitbox->radius, true });
			else if (e->m_flDuckAmount() > 0.5f)
			{
				auto multi_points_scale = 0.6f + config.pointscale_stomach * 0.2f;

				top_right = top + right * multi_points_scale;
				top_left = top + left * multi_points_scale;

				if (g_ctx.globals.side[e->EntIndex()] != LEFT)
					points.push_back(point{ center + top_right * hitbox->radius, true });

				if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
					points.push_back(point{ center + top_left * hitbox->radius, true });
			}
			else
			{
				auto multi_points_scale = 0.6f + config.pointscale_stomach * 0.2f;

				top_right = top + right * multi_points_scale;
				top_left = top + left * multi_points_scale;

				if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
					points.push_back(point{ center + top_right * hitbox->radius, true });

				if (g_ctx.globals.side[e->EntIndex()] != LEFT)
					points.push_back(point{ center + top_left * hitbox->radius, true });
			}
		}
		else if (fabs(delta) > 60.0f)
		{
			if (g_ctx.globals.side[e->EntIndex()] == NONE)
				points.push_back(point{ center + top * hitbox->radius, true });
			else
			{
				auto multi_points_scale = 0.6f + config.pointscale_stomach * 0.2f;

				top_right = top + right * multi_points_scale;
				top_left = top + left * multi_points_scale;

				if (delta > 0.0f)
				{
					if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
						points.push_back(point{ center + top_right * hitbox->radius, true });

					if (g_ctx.globals.side[e->EntIndex()] != LEFT)
						points.push_back(point{ center + top_left * hitbox->radius, true });
				}
				else
				{
					if (g_ctx.globals.side[e->EntIndex()] != LEFT)
						points.push_back(point{ center + top_right * hitbox->radius, true });

					if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
						points.push_back(point{ center + top_left * hitbox->radius, true });
				}
			}
		}
		else
		{
			if (g_ctx.globals.side[e->EntIndex()] == NONE)
				points.push_back(point{ center + top * hitbox->radius, true });
			else if (e->m_flDuckAmount() > 0.5f)
			{
				auto multi_points_scale = 0.6f + config.pointscale_stomach * 0.2f;

				top_right = top + right * multi_points_scale;
				top_left = top + left * multi_points_scale;

				if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
					points.push_back(point{ center + top_right * hitbox->radius, true });

				if (g_ctx.globals.side[e->EntIndex()] != LEFT)
					points.push_back(point{ center + top_left * hitbox->radius, true });
			}
			else
			{
				auto multi_points_scale = 0.6f + config.pointscale_stomach * 0.2f;

				top_right = top + right * multi_points_scale;
				top_left = top + left * multi_points_scale;

				if (g_ctx.globals.side[e->EntIndex()] != LEFT)
					points.push_back(point{ center + top_right * hitbox->radius, true });

				if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
					points.push_back(point{ center + top_left * hitbox->radius, true });
			}
		}

		break;
	case HITBOX_PELVIS:
		if (!config.pointscale_pelvis) //-V550
			points.push_back(point{ center, true });
		else if (!config.safe_body_points)
		{
			auto multi_points_scale = 0.2f + config.pointscale_pelvis * 0.6f;

			points.push_back(point{ center, true });
			points.push_back(point{ center + right * hitbox->radius * multi_points_scale, false });
			points.push_back(point{ center + left * hitbox->radius * multi_points_scale, false });
		}
		else if (fabs(delta) > 120.0f)
		{
			if (g_ctx.globals.side[e->EntIndex()] == NONE)
				points.push_back(point{ center, true });
			else if (e->m_flDuckAmount() > 0.5f)
			{
				auto multi_points_scale = 0.6f + config.pointscale_pelvis * 0.2f;

				if (g_ctx.globals.side[e->EntIndex()] != LEFT)
					points.push_back(point{ center + right * hitbox->radius * multi_points_scale, true });

				if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
					points.push_back(point{ center + left * hitbox->radius * multi_points_scale, true });
			}
			else
			{
				auto multi_points_scale = 0.6f + config.pointscale_pelvis * 0.2f;

				if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
					points.push_back(point{ center + right * hitbox->radius * multi_points_scale, true });

				if (g_ctx.globals.side[e->EntIndex()] != LEFT)
					points.push_back(point{ center + left * hitbox->radius * multi_points_scale, true });
			}
		}
		else if (fabs(delta) > 60.0f)
		{
			if (g_ctx.globals.side[e->EntIndex()] == NONE)
				points.push_back(point{ center, true });
			else
			{
				auto multi_points_scale = 0.6f + config.pointscale_pelvis * 0.2f;

				if (delta > 0.0f)
				{
					if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
						points.push_back(point{ center + right * hitbox->radius * multi_points_scale, true });

					if (g_ctx.globals.side[e->EntIndex()] != LEFT)
						points.push_back(point{ center + left * hitbox->radius * multi_points_scale, true });
				}
				else
				{
					if (g_ctx.globals.side[e->EntIndex()] != LEFT)
						points.push_back(point{ center + right * hitbox->radius * multi_points_scale, true });

					if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
						points.push_back(point{ center + left * hitbox->radius * multi_points_scale, true });
				}
			}
		}
		else
		{
			if (g_ctx.globals.side[e->EntIndex()] == NONE)
				points.push_back(point{ center, true });
			else if (e->m_flDuckAmount() > 0.5f)
			{
				auto multi_points_scale = 0.6f + config.pointscale_pelvis * 0.2f;

				if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
					points.push_back(point{ center + right * hitbox->radius * multi_points_scale, true });

				if (g_ctx.globals.side[e->EntIndex()] != LEFT)
					points.push_back(point{ center + left * hitbox->radius * multi_points_scale, true });
			}
			else
			{
				auto multi_points_scale = 0.6f + config.pointscale_pelvis * 0.2f;

				if (g_ctx.globals.side[e->EntIndex()] != LEFT)
					points.push_back(point{ center + right * hitbox->radius * multi_points_scale, true });

				if (g_ctx.globals.side[e->EntIndex()] != RIGHT)
					points.push_back(point{ center + left * hitbox->radius * multi_points_scale, true });
			}
		}

		break;
	case HITBOX_RIGHT_THIGH:
	case HITBOX_LEFT_THIGH:
	case HITBOX_RIGHT_CALF:
	case HITBOX_LEFT_CALF:
		if (!config.pointscale_legs) //-V550
			points.push_back(point{ center, true });
		else
		{
			auto multi_points_scale = 0.2f + config.pointscale_legs * 0.6f;

			points.push_back(point{ center + right * hitbox->radius * multi_points_scale, true });
			points.push_back(point{ center + left * hitbox->radius * multi_points_scale, true });
		}

		break;
	default:
		points.push_back(point{ center, true });
		break;
	}

	if (optimize_points)
	{
		config.pointscale_head = backup_pointscale_head;
		config.pointscale_chest = backup_pointscale_chest;
		config.pointscale_stomach = backup_pointscale_stomach;
		config.pointscale_pelvis = backup_pointscale_pelvis;
		config.pointscale_legs = backup_pointscale_legs;
	}
}

std::vector <int> aimbot::hitboxes_from_vector(player_t* e, const std::vector <int>& arr, bool esp)
{
	std::vector <int> hitboxes;

	if (esp)
	{
		if (arr[0])
			hitboxes.emplace_back(HITBOX_HEAD);

		if (arr[1])
			hitboxes.emplace_back(HITBOX_CHEST);

		if (arr[2])
			hitboxes.emplace_back(HITBOX_STOMACH);

		if (arr[3])
			hitboxes.emplace_back(HITBOX_PELVIS);

		if (arr[4])
		{
			hitboxes.emplace_back(HITBOX_RIGHT_UPPER_ARM);
			hitboxes.emplace_back(HITBOX_LEFT_UPPER_ARM);

			hitboxes.emplace_back(HITBOX_RIGHT_FOREARM);
			hitboxes.emplace_back(HITBOX_LEFT_FOREARM);

			hitboxes.emplace_back(HITBOX_RIGHT_HAND);
			hitboxes.emplace_back(HITBOX_LEFT_HAND);
		}

		if (arr[5])
		{
			hitboxes.emplace_back(HITBOX_RIGHT_THIGH);
			hitboxes.emplace_back(HITBOX_LEFT_THIGH);

			hitboxes.emplace_back(HITBOX_RIGHT_CALF);
			hitboxes.emplace_back(HITBOX_LEFT_CALF);
		}

		if (arr[6])
		{
			hitboxes.emplace_back(HITBOX_RIGHT_FOOT);
			hitboxes.emplace_back(HITBOX_LEFT_FOOT);
		}

		return hitboxes;
	}
	else
	{
		static auto mp_damage_headshot_only = m_cvar()->FindVar(crypt_str("mp_damage_headshot_only"));

		auto only_head = mp_damage_headshot_only->GetBool();
		auto ignore_limbs = config.ignore_limbs && e->m_vecVelocity().Length2D() > 20.0f;

		if (arr[0])
			hitboxes.emplace_back(HITBOX_HEAD);

		if (arr[1] && !only_head)
			hitboxes.emplace_back(HITBOX_CHEST);

		if (arr[2] && !only_head)
			hitboxes.emplace_back(HITBOX_STOMACH);

		if (arr[3] && !only_head)
			hitboxes.emplace_back(HITBOX_PELVIS);

		if (arr[4] && !only_head && !ignore_limbs)
		{
			hitboxes.emplace_back(HITBOX_RIGHT_UPPER_ARM);
			hitboxes.emplace_back(HITBOX_LEFT_UPPER_ARM);

			hitboxes.emplace_back(HITBOX_RIGHT_FOREARM);
			hitboxes.emplace_back(HITBOX_LEFT_FOREARM);

			hitboxes.emplace_back(HITBOX_RIGHT_HAND);
			hitboxes.emplace_back(HITBOX_LEFT_HAND);
		}

		if (arr[5] && !only_head && !ignore_limbs)
		{
			hitboxes.emplace_back(HITBOX_RIGHT_THIGH);
			hitboxes.emplace_back(HITBOX_LEFT_THIGH);

			hitboxes.emplace_back(HITBOX_RIGHT_CALF);
			hitboxes.emplace_back(HITBOX_LEFT_CALF);
		}

		if (arr[6] && !only_head && !ignore_limbs)
		{
			hitboxes.emplace_back(HITBOX_RIGHT_FOOT);
			hitboxes.emplace_back(HITBOX_LEFT_FOOT);
		}

		if (!only_head && g_ctx.globals.missed_shots[e->EntIndex()] >= 6 && !hitboxes.empty() && !arr[1] && !arr[2] && !arr[3])
			hitboxes.emplace_back(HITBOX_CHEST);

		return hitboxes;
	}
}

void aimbot::autostop(CUserCmd* m_pcmd)
{
	if (!should_stop)
		return;

	if (g_ctx.globals.slowwalking)
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	if (g_ctx.globals.weapon->is_empty())
		return;

	if (!config.autostop_modifiers[AUTOSTOP_BETWEEN_SHOTS] && !g_ctx.globals.weapon->can_fire(false))
		return;

	auto animlayer = g_ctx.local()->get_animlayers()[1];

	if (animlayer.m_nSequence)
	{
		auto activity = g_ctx.local()->sequence_activity(animlayer.m_nSequence);

		if (activity == ACT_CSGO_RELOAD && animlayer.m_flWeight)
			return;
	}

	auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

	if (!weapon_info)
		return;

	auto velocity = engineprediction::get().backup_data.velocity;
	auto max_speed = 0.34f * (g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed);

	if (velocity.Length2D() < max_speed)
		slowwalk::get().create_move(m_pcmd);
	else
	{
		Vector direction;
		Vector real_view;

		math::vector_angles(velocity, direction);
		m_engine()->GetViewAngles(real_view);

		direction.y = real_view.y - direction.y;

		Vector forward;
		math::angle_vectors(direction, forward);

		static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

		auto negative_side_speed = -cl_sidespeed->GetFloat();
		auto negative_direction = forward * negative_side_speed;

		m_pcmd->m_forwardmove = negative_direction.x;
		m_pcmd->m_sidemove = negative_direction.y;
	}
}

void aimbot::update_config()
{
	config.silent_aim = g_cfg.ragebot.silent_aim;
	config.selection_type = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].selection_type;
	config.fov = g_cfg.ragebot.field_of_view;
	config.autoshoot = g_cfg.ragebot.autoshoot;
	config.autowall = g_cfg.ragebot.autowall;
	config.autoscope = g_cfg.ragebot.autoscope;
	config.ignore_limbs = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].ignore_limbs;
	config.double_tap_hitchance = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].double_tap_hitchance;
	config.double_tap_hitchance_amount = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].double_tap_hitchance_amount;
	config.skip_hitchance_if_low_inaccuracy = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].skip_hitchance_if_low_inaccuracy;
	config.hitchance = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance;
	config.hitchance_amount = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance_amount;
	config.accuracy_boost = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].accuracy_boost;
	config.accuracy_boost_amount = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].accuracy_boost_amount;
	config.minimum_visible_damage = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_visible_damage;
	config.minimum_damage = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_damage;
	config.minimum_override_damage = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage;
	config.hitscan = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitscan;
	config.safe_body_points = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].safe_body_points;

	if (config.hitscan[0])
		config.pointscale_head = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].pointscale_head;
	else
		config.pointscale_head = 0.0f;

	if (config.hitscan[1])
		config.pointscale_chest = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].pointscale_chest;
	else
		config.pointscale_chest = 0.0f;

	if (config.hitscan[2])
		config.pointscale_stomach = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].pointscale_stomach;
	else
		config.pointscale_stomach = 0.0f;

	if (config.hitscan[3])
		config.pointscale_pelvis = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].pointscale_pelvis;
	else
		config.pointscale_pelvis = 0.0f;

	if (config.hitscan[5])
		config.pointscale_legs = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].pointscale_legs;
	else
		config.pointscale_legs = 0.0f;

	config.autostop = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop;
	config.autostop_modifiers = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers;
	config.baim_level = g_ctx.globals.weapon->can_double_tap() && (misc::get().double_tap_enabled || misc::get().recharging_double_tap) ? 1 : g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].baim_level;
	config.baim = g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].baim_settings;
}
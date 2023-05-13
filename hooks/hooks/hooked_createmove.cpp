// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\cheats\ragebot\antiaim.h"
#include "..\..\cheats\visuals\other_esp.h"
#include "..\..\cheats\misc\fakelag.h"
#include "..\..\cheats\misc\prediction_system.h"
#include "..\..\cheats\ragebot\aimbot.h"
#include "..\..\cheats\legitbot\legitbot.h"
#include "..\..\cheats\misc\bunnyhop.h"
#include "..\..\cheats\misc\airstrafe.h"
#include "..\..\cheats\lagcompensation\lagcompensation.h"
#include "..\..\cheats\misc\spammers.h"
#include "..\..\cheats\fakewalk\slowwalk.h"
#include "..\..\cheats\misc\misc.h"
#include "..\..\cheats\visuals\GrenadePrediction.h"
#include "..\..\cheats\ragebot\knifebot.h"
#include "..\..\cheats\ragebot\zeusbot.h"
#include "..\..\cheats\lagcompensation\local_animations.h"

using CreateMove_t = bool(__thiscall*)(IClientMode*, float, CUserCmd*);

int spectate_teleport(CUserCmd* cmd);
void adjust_tickbase(int shift_amount);

bool __stdcall hooks::hooked_createmove(float smt, CUserCmd* m_pcmd)
{
	static auto original_fn = clientmode_hook->get_func_address <CreateMove_t>(24);
	g_ctx.globals.should_modify_eye_position = false;

	if (!m_pcmd)
		return original_fn(m_clientmode(), smt, m_pcmd);

	if (!m_pcmd->m_command_number)
		return original_fn(m_clientmode(), smt, m_pcmd);

	if (key_binds::get().get_key_bind_state(22))
		g_ctx.send_packet = spectate_teleport(m_pcmd);

	if (!g_ctx.available())
		return original_fn(m_clientmode(), smt, m_pcmd);

	misc::get().rank_reveal();
	spammers::get().clan_tag();

	if (!g_ctx.local()->is_alive()) //-V807
		return original_fn(m_clientmode(), smt, m_pcmd);

	g_ctx.globals.weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!g_ctx.globals.weapon)
		return original_fn(m_clientmode(), smt, m_pcmd);

	g_ctx.globals.should_modify_eye_position = true;

	g_ctx.set_command(m_pcmd);
	util::server_time(m_pcmd);

	if (menu_open && g_ctx.globals.focused_on_input)
	{
		m_pcmd->m_buttons = 0;
		m_pcmd->m_forwardmove = 0.0f;
		m_pcmd->m_sidemove = 0.0f;
		m_pcmd->m_upmove = 0.0f;
	}

	if (g_ctx.globals.ticks_allowed < 15 && (misc::get().double_tap_enabled || misc::get().hide_shots_enabled))
	{
		g_ctx.globals.ticks_allowed++;
		m_pcmd->m_tickcount = INT_MAX;
		return false;
	}

	if (menu_open)
	{
		m_pcmd->m_buttons &= ~IN_ATTACK;
		m_pcmd->m_buttons &= ~IN_ATTACK2;
	}

	if (m_pcmd->m_buttons & IN_ATTACK2 && g_cfg.ragebot.enable && g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER)
		m_pcmd->m_buttons &= ~IN_ATTACK2;

	if (g_cfg.ragebot.enable && !g_ctx.globals.weapon->can_fire(true))
	{
		if (m_pcmd->m_buttons & IN_ATTACK && !g_ctx.globals.weapon->is_non_aim() && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
			m_pcmd->m_buttons &= ~IN_ATTACK;
		else if ((m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2) && (g_ctx.globals.weapon->is_knife() || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER) && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_HEALTHSHOT)
		{
			if (m_pcmd->m_buttons & IN_ATTACK)
				m_pcmd->m_buttons &= ~IN_ATTACK;

			if (m_pcmd->m_buttons & IN_ATTACK2)
				m_pcmd->m_buttons &= ~IN_ATTACK2;
		}
	}

	if (g_ctx.globals.weapon->can_fire(true))
		aimbot::get().shoot_poisiton.Zero();

	if (m_pcmd->m_buttons & IN_FORWARD && m_pcmd->m_buttons & IN_BACK)
	{
		m_pcmd->m_buttons &= ~IN_FORWARD;
		m_pcmd->m_buttons &= ~IN_BACK;
	}

	if (m_pcmd->m_buttons & IN_MOVELEFT && m_pcmd->m_buttons & IN_MOVERIGHT)
	{
		m_pcmd->m_buttons &= ~IN_MOVELEFT;
		m_pcmd->m_buttons &= ~IN_MOVERIGHT;
	}

	g_ctx.send_packet = true;

	g_ctx.globals.tickbase_shift = 0;
	g_ctx.globals.double_tap_fire = false;
	g_ctx.globals.exploits = g_cfg.ragebot.double_tap && g_cfg.ragebot.double_tap_key.key > KEY_NONE && g_cfg.ragebot.double_tap_key.key < KEY_MAX && misc::get().double_tap_key || g_cfg.antiaim.hide_shots && g_cfg.antiaim.hide_shots_key.key > KEY_NONE && g_cfg.antiaim.hide_shots_key.key < KEY_MAX && misc::get().hide_shots_key;
	g_ctx.globals.next_tick_lby_update = false;
	g_ctx.globals.stand_eye_pos = g_ctx.local()->get_shoot_position(m_gamemovement()->GetPlayerViewOffset(false).z);
	g_ctx.globals.eye_pos = g_ctx.local()->get_shoot_position();
	g_ctx.globals.current_weapon = g_ctx.globals.weapon->get_weapon_group(g_cfg.ragebot.enable);
	g_ctx.globals.slowwalking = false;

	auto wish_angle = m_pcmd->m_viewangles;

	misc::get().fast_stop(m_pcmd);

	if (g_cfg.misc.bunnyhop)
		bunnyhop::get().create_move();

	misc::get().SlideWalk(m_pcmd);

	misc::get().NoDuck(m_pcmd);

	misc::get().AutoCrouch(m_pcmd);

	GrenadePrediction::get().Tick(m_pcmd->m_buttons);

	if (g_cfg.misc.crouch_in_air && !(g_ctx.local()->m_fFlags() & FL_ONGROUND))
		m_pcmd->m_buttons |= IN_DUCK;

	engineprediction::get().prediction_data.reset(); //-V807
	engineprediction::get().setup();

	engineprediction::get().predict(m_pcmd);
	{
		if (g_cfg.misc.airstrafe)
			airstrafe::get().create_move(m_pcmd);

		if (key_binds::get().get_key_bind_state(19) && engineprediction::get().backup_data.flags & FL_ONGROUND && !(g_ctx.local()->m_fFlags() & FL_ONGROUND)) //-V807
			m_pcmd->m_buttons |= IN_JUMP;

		if (key_binds::get().get_key_bind_state(21))
			slowwalk::get().create_move(m_pcmd);

		if (g_cfg.ragebot.enable && g_ctx.globals.current_weapon != -1)
			aimbot::get().update_config();

		fakelag::get().Createmove();

		g_ctx.globals.aimbot_working = false;
		g_ctx.globals.revolver_working = false;

		auto backup_velocity = g_ctx.local()->m_vecVelocity();
		auto backup_abs_velocity = g_ctx.local()->m_vecAbsVelocity();

		g_ctx.local()->m_vecVelocity() = engineprediction::get().backup_data.velocity;
		g_ctx.local()->m_vecAbsVelocity() = engineprediction::get().backup_data.velocity;

		g_ctx.globals.weapon->update_accuracy_penality();

		g_ctx.local()->m_vecVelocity() = backup_velocity;
		g_ctx.local()->m_vecAbsVelocity() = backup_abs_velocity;

		g_ctx.globals.inaccuracy = g_ctx.globals.weapon->get_inaccuracy();
		g_ctx.globals.spread = g_ctx.globals.weapon->get_spread();

		if (g_cfg.ragebot.enable && g_cfg.ragebot.zeus_bot)
			zeusbot::get().zeus_run(m_pcmd);

		if (g_cfg.ragebot.enable && g_cfg.ragebot.knife_bot)
			knifebot::get().knife_run(m_pcmd);

		if (g_cfg.legitbot.enabled)
			g_Aimbot.OnMove(m_pcmd);

		if (misc::get().double_tap_key && g_ctx.globals.weapon->can_double_tap())
		{
			if (g_cfg.ragebot.instant && g_ctx.globals.weapon->can_shift_tickbase(14))
				adjust_tickbase(14);
			else if (!g_cfg.ragebot.instant)
				adjust_tickbase(11);
		}
		else if (misc::get().hide_shots_key && g_ctx.globals.weapon->can_shift_tickbase(9))
			adjust_tickbase(9);

		if (g_cfg.ragebot.enable && g_ctx.globals.current_weapon != -1)
		{
			m_engine()->GetViewAngles(aimbot::get().engine_angles);

			aimbot::get().should_stop = false;
			aimbot::get().create_move(m_pcmd);

			if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop)
				aimbot::get().autostop(m_pcmd);
		}

		if (g_cfg.ragebot.enable && !g_ctx.globals.aimbot_working && !g_ctx.globals.weapon->is_non_aim() && engineprediction::get().backup_data.flags & FL_ONGROUND && g_ctx.local()->m_fFlags() & FL_ONGROUND)
		{
			static auto twisted_speed = 0.0f;
			static auto twist_switch = false;

			if (twisted_speed <= 0.0f)
				twist_switch = true;
			else if (twisted_speed >= 0.5f)
				twist_switch = false;

			twist_switch ? twisted_speed += 0.1f : twisted_speed -= 0.1f;
			slowwalk::get().create_move(m_pcmd, 0.95f + twist_switch);
		}

		misc::get().automatic_peek(m_pcmd, wish_angle.y);

		antiaim::get().desync_angle = 0.0f;
		antiaim::get().create_move(m_pcmd);

		if (m_gamerules()->m_bIsValveDS() && (m_clientstate()->m_iChockedCommands >= 6 || g_ctx.globals.next_tick_lby_update && m_clientstate()->m_iChockedCommands >= 4)) //-V648
			g_ctx.send_packet = true;
		else if (!m_gamerules()->m_bIsValveDS() && (m_clientstate()->m_iChockedCommands >= 16 || g_ctx.globals.next_tick_lby_update && m_clientstate()->m_iChockedCommands >= 14)) //-V648
			g_ctx.send_packet = true;

		if (g_ctx.globals.should_send_packet)
			g_ctx.send_packet = true;

		if (g_ctx.globals.should_choke_packet)
		{
			g_ctx.globals.should_choke_packet = false;
			g_ctx.globals.should_send_packet = true;
			g_ctx.send_packet = false;
		}

		if (!g_ctx.globals.weapon->is_non_aim())
		{
			auto double_tap_aim_check = false;

			if (m_pcmd->m_buttons & IN_ATTACK && g_ctx.globals.double_tap_aim_check)
			{
				double_tap_aim_check = true;
				g_ctx.globals.double_tap_aim_check = false;
			}

			auto revolver_shoot = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !g_ctx.globals.revolver_working && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2);

			if (m_pcmd->m_buttons & IN_ATTACK && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || revolver_shoot)
			{
				for (auto i = 1; i < m_globals()->m_maxclients; i++)
				{
					auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

					if (e == g_ctx.local())
						continue;

					if (!e->valid(false))
						continue;

					memcpy(g_ctx.globals.aimbot_matrix[e->EntIndex()], e->m_CachedBoneData().Base(), e->get_bone_count() * sizeof(matrix3x4_t));
				}

				static auto weapon_recoil_scale = m_cvar()->FindVar(crypt_str("weapon_recoil_scale"));

				if (g_cfg.ragebot.enable)
					m_pcmd->m_viewangles -= g_ctx.local()->m_aimPunchAngle() * weapon_recoil_scale->GetFloat();

				auto fakeducking = false;
				
				if (!fakelag::get().condition && key_binds::get().get_key_bind_state(20))
					fakeducking = true;

				if (!fakeducking)
				{
					g_ctx.globals.should_choke_packet = true;
					g_ctx.send_packet = true;
				}

				aimbot::get().shoot_poisiton = g_ctx.globals.eye_pos;

				if (!double_tap_aim_check)
					g_ctx.globals.double_tap_aim = false;
			}
		}
		else if (g_ctx.globals.weapon->is_knife() && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2))
		{
			for (auto i = 1; i < m_globals()->m_maxclients; i++)
			{
				auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

				if (e == g_ctx.local())
					continue;

				if (!e->valid(false))
					continue;

				memcpy(g_ctx.globals.aimbot_matrix[e->EntIndex()], e->m_CachedBoneData().Base(), e->get_bone_count() * sizeof(matrix3x4_t));
			}

			auto fakeducking = false;

			if (!fakelag::get().condition && key_binds::get().get_key_bind_state(20))
				fakeducking = true;

			if (!fakeducking)
			{
				g_ctx.globals.should_choke_packet = true;
				g_ctx.send_packet = true;
			}
		}
		else if (g_ctx.globals.weapon->is_grenade() && g_ctx.globals.weapon->m_fThrowTime())
		{
			for (auto i = 1; i < m_globals()->m_maxclients; i++)
			{
				auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

				if (e == g_ctx.local())
					continue;

				if (!e->valid(false))
					continue;

				memcpy(g_ctx.globals.aimbot_matrix[e->EntIndex()], e->m_CachedBoneData().Base(), e->get_bone_count() * sizeof(matrix3x4_t));
			}

			auto fakeducking = false;

			if (!fakelag::get().condition && key_binds::get().get_key_bind_state(20))
				fakeducking = true;

			if (!fakeducking)
			{
				g_ctx.globals.should_choke_packet = true;
				g_ctx.send_packet = true;
			}
		}

		for (auto i = 1; i < m_globals()->m_maxclients; i++)
		{
			auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

			if (!e->valid(true, false))
				continue;

			backtrack::get().backup_player(e);
		}

		auto backup_ticks_allowed = g_ctx.globals.ticks_allowed;

		if (misc::get().double_tap(m_pcmd))
			misc::get().hide_shots(m_pcmd, false);
		else
		{
			g_ctx.globals.ticks_allowed = backup_ticks_allowed;
			misc::get().hide_shots(m_pcmd, true);
		}

		if (!g_ctx.globals.weapon->is_non_aim())
		{
			auto double_tap_aim_check = false;

			if (m_pcmd->m_buttons & IN_ATTACK && g_ctx.globals.double_tap_aim_check)
			{
				double_tap_aim_check = true;
				g_ctx.globals.double_tap_aim_check = false;
			}

			auto revolver_shoot = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !g_ctx.globals.revolver_working && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2);

			if (!double_tap_aim_check && m_pcmd->m_buttons & IN_ATTACK && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || revolver_shoot)
				g_ctx.globals.double_tap_aim = false;
		}

		if (m_globals()->m_tickcount - g_ctx.globals.last_aimbot_shot > 16)
		{
			g_ctx.globals.double_tap_aim = false;
			g_ctx.globals.double_tap_aim_check = false;
		}
	}
	engineprediction::get().finish();

	if (g_ctx.globals.loaded_script)
		for (auto current : c_lua::get().hooks.getHooks("on_createmove"))
			current.func();

	util::movement_fix(wish_angle, m_pcmd);

	if (g_cfg.misc.anti_untrusted)
		math::normalize_angles(m_pcmd->m_viewangles);
	else
	{
		m_pcmd->m_viewangles.y = math::normalize_yaw(m_pcmd->m_viewangles.y);
		m_pcmd->m_viewangles.z = 0.0f;
	}

	auto& out = g_ctx.globals.commands.emplace_back();

	out.is_outgoing = g_ctx.send_packet;
	out.is_used = false;
	out.command_number = m_pcmd->m_command_number;
	out.previous_command_number = 0;
	
	while (g_ctx.globals.commands.size() > (int)(1.0f / m_globals()->m_intervalpertick))
		g_ctx.globals.commands.pop_front();

	if (g_cfg.antiaim.fakelag && !g_ctx.send_packet && !m_gamerules()->m_bIsValveDS())
	{
		auto net_channel = m_clientstate()->m_ptrNetChannel;

		if (net_channel->m_nChokedPackets > 0 && !(net_channel->m_nChokedPackets % 4))
		{
			auto backup_choke = net_channel->m_nChokedPackets;
			net_channel->m_nChokedPackets = 0;

			net_channel->send_datagram();
			--net_channel->m_nOutSequenceNr;

			net_channel->m_nChokedPackets = backup_choke;
		}
	}

	if (g_ctx.send_packet && (!g_ctx.globals.should_send_packet || !g_cfg.misc.hold_firing_animation) && (!g_ctx.globals.should_choke_packet || (!misc::get().hide_shots_enabled && !g_ctx.globals.double_tap_fire)))
	{
		local_animations::get().local_data.fake_angles = m_pcmd->m_viewangles;
		local_animations::get().local_data.real_angles = local_animations::get().local_data.stored_real_angles;
	}

	local_animations::get().local_data.stored_real_angles = m_pcmd->m_viewangles;

	if (g_ctx.send_packet && g_ctx.globals.should_send_packet)
		g_ctx.globals.should_send_packet = false;

	if (misc::get().double_tap_enabled || misc::get().hide_shots_enabled)
	{
		if (g_ctx.send_packet)
			g_ctx.globals.ticks_allowed--;
		else
			g_ctx.globals.ticks_allowed++;
	}
	
	g_ctx.globals.ticks_allowed = math::clamp(g_ctx.globals.ticks_allowed, 0, 16);
	
	if (g_cfg.misc.buybot_enable && g_ctx.globals.should_buy)
	{
		--g_ctx.globals.should_buy;

		if (!g_ctx.globals.should_buy)
		{
			std::string buy;

			switch (g_cfg.misc.buybot1)
			{
			case 1:
				buy += crypt_str("buy g3sg1; ");
				break;
			case 2:
				buy += crypt_str("buy awp; ");
				break;
			case 3:
				buy += crypt_str("buy ssg08; ");
				break;
			}

			switch (g_cfg.misc.buybot2)
			{
			case 1:
				buy += crypt_str("buy elite; ");
				break;
			case 2:
				buy += crypt_str("buy deagle; buy revolver; ");
				break;
			}

			if (g_cfg.misc.buybot3[BUY_ARMOR])
				buy += crypt_str("buy vesthelm; buy vest; ");

			if (g_cfg.misc.buybot3[BUY_TASER])
				buy += crypt_str("buy taser; ");

			if (g_cfg.misc.buybot3[BUY_GRENADES])
				buy += crypt_str("buy molotov; buy hegrenade; buy smokegrenade; buy flashbang; buy flashbang; buy decoy; ");

			if (g_cfg.misc.buybot3[BUY_DEFUSER])
				buy += crypt_str("buy defuser; ");

			m_engine()->ExecuteClientCmd(buy.c_str());
		}
	}

	uintptr_t* frame_ptr;
	__asm mov frame_ptr, ebp;

	*(bool*)(*frame_ptr - 0x1C) = g_ctx.send_packet;

	g_ctx.globals.should_modify_eye_position = false;
	return false;
}

int GetEstimatedServerTickCount(CUserCmd* cmd)
{
	auto net_channel_info = m_engine()->GetNetChannelInfo();

	if (!net_channel_info)
		return 0;

	return cmd->m_tickcount + TIME_TO_TICKS(net_channel_info->GetLatency(FLOW_OUTGOING) + net_channel_info->GetLatency(FLOW_INCOMING)) + 1;
}

int spectate_teleport(CUserCmd* cmd)
{
	auto ticks_to_send_in_batch = g_ctx.globals.tickbase_shift + 2;

	auto lastcommand = m_clientstate()->m_iLastOutgoingCommand;
	auto chokedcount = m_clientstate()->m_iChockedCommands;
	
	auto nextcommandnr = lastcommand + chokedcount + 1;
	auto dwServerTickCount = GetEstimatedServerTickCount(cmd);

	for (auto i = 0; i < ticks_to_send_in_batch; i++)
	{
		cmd->m_command_number = nextcommandnr++;
		cmd->m_tickcount = (int)m_clientstate() + dwServerTickCount + TIME_TO_TICKS(0.2f) + i;

		if (ticks_to_send_in_batch > 1 && i != ticks_to_send_in_batch - 1)
			chokedcount++;
	}

	return chokedcount;
}

void adjust_tickbase(int shift_amount)
{
	auto simulation_ticks = min(m_clientstate()->m_iChockedCommands + shift_amount + 1, 16);

	if (simulation_ticks < 0)
		return;

	auto nci = m_engine()->GetNetChannelInfo();

	if (!nci)
		return;

	auto serverTickcount = m_globals()->m_tickcount;
	auto outgoing = nci->GetLatency(FLOW_OUTGOING);

	serverTickcount += (int)(outgoing / m_globals()->m_intervalpertick) + 2;

	static auto sv_clockcorrection_msecs = m_cvar()->FindVar(crypt_str("sv_clockcorrection_msecs"));
	auto nCorrectionTicks = TIME_TO_TICKS(math::clamp(sv_clockcorrection_msecs->GetFloat() / 1000.0f, 0.0f, 1.0f));

	auto nIdealFinalTick = serverTickcount + nCorrectionTicks;
	auto nEstimatedFinalTick = engineprediction::get().netvars_data[m_clientstate()->m_iCommandAck % MULTIPLAYER_BACKUP].tickbase + simulation_ticks;

	auto too_fast_limit = nIdealFinalTick + nCorrectionTicks;
	auto too_slow_limit = nIdealFinalTick - nCorrectionTicks;

	if (nEstimatedFinalTick > too_fast_limit || nEstimatedFinalTick < too_slow_limit)
		g_ctx.globals.fixed_tickbase = nIdealFinalTick - simulation_ticks + m_globals()->m_simticksthisframe;
}
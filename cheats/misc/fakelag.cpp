// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "fakelag.h"
#include "misc.h"
#include "prediction_system.h"
#include "logs.h"

void fakelag::Fakelag(CUserCmd* m_pcmd)
{
	static auto started_peeking = false;
	static auto fluctuate_ticks = 0;
	static auto switch_ticks = false;
	static auto random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);

	auto choked = m_clientstate()->m_iChockedCommands; //-V807
	auto flags = engineprediction::get().backup_data.flags; //-V807
	auto velocity = engineprediction::get().backup_data.velocity.Length(); //-V807
	auto velocity2d = engineprediction::get().backup_data.velocity.Length2D();

	auto max_speed = 260.0f;
	auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

	if (weapon_info)
		max_speed = g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;

	switch (g_cfg.antiaim.fakelag_type)
	{
	case 0:
		max_choke = g_cfg.antiaim.triggers_fakelag_amount;
		break;
	case 1:
		max_choke = random_factor;
		break;
	case 2:
		if (velocity2d >= 5.0f)
		{
			auto dynamic_factor = std::ceilf(64.0f / (velocity2d * m_globals()->m_intervalpertick));

			if (dynamic_factor > 16)
				dynamic_factor = g_cfg.antiaim.triggers_fakelag_amount;

			max_choke = dynamic_factor;
		}
		else
			max_choke = g_cfg.antiaim.triggers_fakelag_amount;
		break;
	case 3:
		max_choke = fluctuate_ticks;
		break;
	}

	if (choked != m_clientstate()->m_iChockedCommands)
	{
		choked = m_clientstate()->m_iChockedCommands;
		max_choke = 1;
	}
	else if (m_gamerules()->m_bIsValveDS())
		max_choke = min(max_choke, 6);

	if (g_cfg.ragebot.enable && g_ctx.globals.current_weapon != -1 && !g_ctx.globals.exploits && g_cfg.antiaim.fakelag && g_cfg.antiaim.fakelag_enablers[FAKELAG_PEEK] && g_cfg.antiaim.triggers_fakelag_amount > 6 && !started_peeking && velocity >= 5.0f)
	{
		auto predicted_eye_pos = g_ctx.globals.eye_pos + engineprediction::get().backup_data.velocity * m_globals()->m_intervalpertick * (float)(g_cfg.antiaim.triggers_fakelag_amount) * 0.5f;

		for (auto i = 1; i < m_globals()->m_maxclients; i++)
		{
			auto e = static_cast<player_t *>(m_entitylist()->GetClientEntity(i));

			if (!e->valid(true))
				continue;

			aimbot::scan_data predicted_data;
			aimbot::get().best_point(e, predicted_data, true, predicted_eye_pos);

			if (!predicted_data.aim_point.IsZero())
			{
				aimbot::scan_data data;
				aimbot::get().best_point(e, data, true);

				if (data.aim_point.IsZero())
				{
					random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);
					switch_ticks = !switch_ticks;
					fluctuate_ticks = switch_ticks ? g_cfg.antiaim.triggers_fakelag_amount : max(g_cfg.antiaim.triggers_fakelag_amount - 2, 1);

					g_ctx.send_packet = true;
					started_peeking = true;

					return;
				}
			}
		}
	}

	if (g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND && !m_gamerules()->m_bIsValveDS() && key_binds::get().get_key_bind_state(20)) //-V807
	{
		max_choke = 14;

		if (choked < max_choke)
			g_ctx.send_packet = false;
		else
			g_ctx.send_packet = true;
	}
	else if (!g_ctx.globals.exploits && g_cfg.antiaim.fakelag && g_cfg.antiaim.fakelag_enablers[FAKELAG_PEEK] && started_peeking)
	{
		if (choked < max_choke)
			g_ctx.send_packet = false;
		else
		{
			started_peeking = false;

			random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);
			switch_ticks = !switch_ticks;
			fluctuate_ticks = switch_ticks ? g_cfg.antiaim.triggers_fakelag_amount : max(g_cfg.antiaim.triggers_fakelag_amount - 2, 1);

			g_ctx.send_packet = true;
		}
	}
	else if (!g_ctx.globals.exploits && g_cfg.antiaim.fakelag && velocity >= 5.0f && g_ctx.globals.slowwalking && g_cfg.antiaim.fakelag_enablers[FAKELAG_SLOW_WALK])
	{
		if (choked < max_choke)
			g_ctx.send_packet = false;
		else
		{
			started_peeking = false;

			random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);
			switch_ticks = !switch_ticks;
			fluctuate_ticks = switch_ticks ? g_cfg.antiaim.triggers_fakelag_amount : max(g_cfg.antiaim.triggers_fakelag_amount - 2, 1);

			g_ctx.send_packet = true;
		}
	}
	else if (!g_ctx.globals.exploits && g_cfg.antiaim.fakelag && velocity >= 5.0f && !g_ctx.globals.slowwalking && g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND && g_cfg.antiaim.fakelag_enablers[FAKELAG_MOVE])
	{
		if (choked < max_choke)
			g_ctx.send_packet = false;
		else
		{
			started_peeking = false;

			random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);
			switch_ticks = !switch_ticks;
			fluctuate_ticks = switch_ticks ? g_cfg.antiaim.triggers_fakelag_amount : max(g_cfg.antiaim.triggers_fakelag_amount - 2, 1);

			g_ctx.send_packet = true;
		}
	}
	else if (!g_ctx.globals.exploits && g_cfg.antiaim.fakelag && !g_ctx.globals.slowwalking && !(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND) && g_cfg.antiaim.fakelag_enablers[FAKELAG_AIR])
	{
		static auto sv_gravity = m_cvar()->FindVar(crypt_str("sv_gravity"));
		auto start = g_ctx.local()->GetAbsOrigin();

		auto previous_prediction_ticks = (int)(g_cfg.antiaim.triggers_fakelag_amount * 0.5f) + 1;
		auto prediction_ticks = (int)(g_cfg.antiaim.triggers_fakelag_amount * 0.5f);

		auto previous_landed = false;
		auto landed = false;

		auto previous_end = start;
		auto end = start;

		auto previous_local_velocity = g_ctx.local()->m_vecVelocity();
		auto local_velocity = g_ctx.local()->m_vecVelocity(); //-V656

		previous_local_velocity.z -= sv_gravity->GetFloat() * m_globals()->m_intervalpertick * (float)previous_prediction_ticks;
		local_velocity.z -= sv_gravity->GetFloat() * m_globals()->m_intervalpertick * (float)prediction_ticks;

		previous_end += local_velocity * m_globals()->m_intervalpertick * (float)previous_prediction_ticks;
		end += local_velocity * m_globals()->m_intervalpertick * (float)prediction_ticks;

		previous_end.z -= 2.0f;
		end.z -= 2.0f;

		CTraceFilterWorldOnly filter;
		trace_t trace;

		Ray_t previous_ray;
		previous_ray.Init(start, previous_end);

		m_trace()->TraceRay(previous_ray, MASK_SOLID, &filter, &trace);

		if (trace.fraction != 1.0f && trace.plane.normal.z > 0.7f)
			previous_landed = true;

		Ray_t ray;
		ray.Init(start, end);

		m_trace()->TraceRay(ray, MASK_SOLID, &filter, &trace);

		if (trace.fraction != 1.0f && trace.plane.normal.z > 0.7f)
			landed = true;

		if (previous_landed && !landed)
		{
			random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);
			switch_ticks = !switch_ticks;
			fluctuate_ticks = switch_ticks ? g_cfg.antiaim.triggers_fakelag_amount : max(g_cfg.antiaim.triggers_fakelag_amount - 2, 1);

			g_ctx.send_packet = true;
			return;
		}

		if (choked < max_choke)
			g_ctx.send_packet = false;
		else
		{
			started_peeking = false;

			random_factor = min(rand() % 16 + 1, g_cfg.antiaim.triggers_fakelag_amount);
			switch_ticks = !switch_ticks;
			fluctuate_ticks = switch_ticks ? g_cfg.antiaim.triggers_fakelag_amount : max(g_cfg.antiaim.triggers_fakelag_amount - 2, 1);

			g_ctx.send_packet = true;
		}
	}
	else if (!g_ctx.globals.exploits && g_cfg.antiaim.fakelag)
	{
		max_choke = g_cfg.antiaim.fakelag_amount;

		if (m_gamerules()->m_bIsValveDS())
			max_choke = min(max_choke, 6);

		if (choked < max_choke)
			g_ctx.send_packet = false;
		else
		{
			started_peeking = false;

			random_factor = min(rand() % 16 + 1, g_cfg.antiaim.fakelag_amount);
			switch_ticks = !switch_ticks;
			fluctuate_ticks = switch_ticks ? g_cfg.antiaim.fakelag_amount : max(g_cfg.antiaim.fakelag_amount - 2, 1);

			g_ctx.send_packet = true;
		}
	}
	else if (g_ctx.globals.exploits || !antiaim::get().condition(m_pcmd, false) && (antiaim::get().type == ANTIAIM_LEGIT || g_cfg.antiaim.type[antiaim::get().type].desync)) //-V648
	{
		condition = true;
		started_peeking = false;

		if (choked < 1)
			g_ctx.send_packet = false;
		else
			g_ctx.send_packet = true;
	}
	else
		condition = true;
}

void fakelag::Createmove()
{
	if (FakelagCondition(g_ctx.get_command()))
		return;

	Fakelag(g_ctx.get_command());
	
	if (!m_gamerules()->m_bIsValveDS() && m_clientstate()->m_iChockedCommands <= 16)
	{
		static auto Fn = util::FindSignature(crypt_str("engine.dll"), crypt_str("B8 ? ? ? ? 3B F0 0F 4F F0 89 5D FC")) + 0x1;
		DWORD old = 0;

		VirtualProtect((void*)Fn, sizeof(uint32_t), PAGE_EXECUTE_READWRITE, &old);
		*(uint32_t*)Fn = 17;
		VirtualProtect((void*)Fn, sizeof(uint32_t), old, &old);
	}
}

bool fakelag::FakelagCondition(CUserCmd* m_pcmd)
{
	condition = false;

	if (g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN)
		condition = true;

	if (antiaim::get().freeze_check && !misc::get().double_tap_enabled && !misc::get().hide_shots_enabled)
		condition = true;

	return condition;
}
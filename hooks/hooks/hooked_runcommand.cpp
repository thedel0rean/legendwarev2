// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\cheats\misc\prediction_system.h"
#include "..\..\cheats\lagcompensation\local_animations.h"
#include "..\..\cheats\misc\misc.h"

float fix_velocity_modifier(player_t* e, int command_number, bool before)
{
	auto velocity_modifier = g_ctx.globals.last_velocity_modifier;
	auto delta = command_number - g_ctx.globals.last_velocity_modifier_tick;

	if (before)
		--delta;

	if (delta < 0 || g_ctx.globals.last_velocity_modifier == 1.0f)
		velocity_modifier = e->m_flVelocityModifier();
	else if (delta)
	{
		velocity_modifier = g_ctx.globals.last_velocity_modifier + m_globals()->m_intervalpertick * (float)delta * 0.4f;
		velocity_modifier = math::clamp(velocity_modifier, 0.0f, 1.0f);
	}

	return velocity_modifier;
}

using RunCommand_t = void(__thiscall*)(void*, player_t*, CUserCmd*, IMoveHelper*);

void __fastcall hooks::hooked_runcommand(void* ecx, void* edx, player_t* player, CUserCmd* m_pcmd, IMoveHelper* move_helper)
{
	static auto original_fn = prediction_hook->get_func_address <RunCommand_t> (19);

	if (!player)
		return original_fn(ecx, player, m_pcmd, move_helper);

	if (!m_pcmd)
		return original_fn(ecx, player, m_pcmd, move_helper);

	if (m_pcmd->m_tickcount > m_globals()->m_tickcount * 2)
	{
		m_pcmd->m_predicted = true;
		return;
	}

	if (g_cfg.ragebot.enable && g_ctx.local()->is_alive())
	{
		auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

		if (weapon && weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER)
			weapon->m_flPostponeFireReadyTime() = g_ctx.globals.r8cock_time;
	}

	player->m_flVelocityModifier() = fix_velocity_modifier(player, m_pcmd->m_command_number, true);
	original_fn(ecx, player, m_pcmd, move_helper);
	player->m_flVelocityModifier() = fix_velocity_modifier(player, m_pcmd->m_command_number, false);

	player->m_vphysicsCollisionState() = 0;
}

using InPrediction_t = bool(__thiscall*)(void*);

bool __stdcall hooks::hooked_inprediction()
{
	static auto original_fn = prediction_hook->get_func_address <InPrediction_t> (14);
	static auto maintain_sequence_transitions = util::FindSignature(crypt_str("client.dll"), crypt_str("84 C0 74 17 8B 87"));

	if (g_cfg.ragebot.enable && (uintptr_t)_ReturnAddress() == maintain_sequence_transitions)
		return true;

	return original_fn(m_prediction());
}

using WriteUsercmdDeltaToBuffer_t = bool(__thiscall*)(void*, int, void*, int, int, bool);
void WriteUser—md(void* buf, CUserCmd* incmd, CUserCmd* outcmd);

bool __fastcall hooks::hooked_writeusercmddeltatobuffer(void* ecx, void* edx, int slot, bf_write* buf, int from, int to, bool is_new_command)
{
	static auto original_fn = client_hook->get_func_address <WriteUsercmdDeltaToBuffer_t>(24);

	if (!g_ctx.globals.tickbase_shift)
		return original_fn(ecx, slot, buf, from, to, is_new_command);

	if (from != -1)
		return true;

	auto final_from = -1;

	uintptr_t frame_ptr;
	__asm mov frame_ptr, ebp;

	auto backup_commands = reinterpret_cast <int*> (frame_ptr + 0xFD8);
	auto new_commands = reinterpret_cast <int*> (frame_ptr + 0xFDC);

	auto newcmds = *new_commands;
	auto shift = g_ctx.globals.tickbase_shift;

	g_ctx.globals.tickbase_shift = 0;
	*backup_commands = 0;

	auto choked_modifier = newcmds + shift;

	if (choked_modifier > 62)
		choked_modifier = 62;

	*new_commands = choked_modifier;

	auto next_cmdnr = m_clientstate()->m_iChockedCommands + m_clientstate()->m_iLastOutgoingCommand + 1;
	auto final_to = next_cmdnr - newcmds + 1;

	if (final_to <= next_cmdnr)
	{
		while (original_fn(ecx, slot, buf, final_from, final_to, true))
		{
			final_from = final_to++;

			if (final_to > next_cmdnr)
				goto next_cmd;
		}

		return false;
	}
next_cmd:

	auto user_cmd = m_input()->GetUserCmd(final_from);

	if (!user_cmd)
		return true;

	CUserCmd to_cmd;
	CUserCmd from_cmd;

	from_cmd = *user_cmd;
	to_cmd = from_cmd;

	to_cmd.m_command_number++;
	to_cmd.m_tickcount += m_globals()->m_tickcount * 3;

	if (newcmds > choked_modifier)
		return true;

	for (auto i = choked_modifier - newcmds + 1; i > 0; --i)
	{
		WriteUser—md(buf, &to_cmd, &from_cmd);

		from_cmd = to_cmd;
		to_cmd.m_command_number++;
		to_cmd.m_tickcount++;
	}

	return true;
}

void WriteUser—md(void* buf, CUserCmd* incmd, CUserCmd* outcmd) 
{
	using WriteUserCmd_t = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
	static auto Fn = (WriteUserCmd_t)util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 51 53 56 8B D9"));

	__asm
	{
		mov     ecx, buf
		mov     edx, incmd
		push    outcmd
		call    Fn
		add     esp, 4
	}
}
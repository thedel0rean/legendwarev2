// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "legitbot.h"
#include "..\autowall\autowall.h"

bool IsNotTarget(player_t* e)
{
	if (!e)
		return true;

	if (e == g_ctx.local())
		return true;

	if (e->m_iHealth() <= 0)
		return true;

	if (e->m_bGunGameImmunity())
		return true;

	if (e->m_fFlags() & FL_FROZEN)
		return true;

	int entIndex = e->EntIndex();
	return entIndex >= m_globals()->m_maxclients;
}
//--------------------------------------------------------------------------------
bool Aimbot::IsRcs() {
	return g_ctx.local()->m_iShotsFired() >= g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_start;
}
//--------------------------------------------------------------------------------
float GetRealDistanceFOV(float distance, const Vector& angle, const Vector& viewangles) {
	Vector aimingAt;
	math::angle_vectors(viewangles, aimingAt);
	aimingAt *= distance;
	Vector aimAt;
	math::angle_vectors(angle, aimAt);
	aimAt *= distance;
	return aimingAt.DistTo(aimAt) / 5;
}
//--------------------------------------------------------------------------------
float Aimbot::GetFovToPlayer(const Vector& viewAngle, const Vector& aimAngle) {
	Vector delta = aimAngle - viewAngle;
	math::normalize_angles(delta);
	return sqrtf(powf(delta.x, 2.0f) + powf(delta.y, 2.0f));
}
//--------------------------------------------------------------------------------
bool Aimbot::IsLineGoesThroughSmoke(const Vector& vStartPos, const Vector& vEndPos) {
	static auto LineGoesThroughSmokeFn = (bool(*)(Vector vStartPos, Vector vEndPos))util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0"));
	return LineGoesThroughSmokeFn(vStartPos, vEndPos);
}
//--------------------------------------------------------------------------------
bool Aimbot::IsEnabled(CUserCmd* m_pcmd) {
	if (g_ctx.globals.current_weapon == -1)
		return false;

	auto pWeapon = g_ctx.globals.weapon;
	if (!pWeapon || !(pWeapon->is_sniper() || pWeapon->is_pistol() || pWeapon->is_rifle())) {
		return false;
	}

	auto weaponData = pWeapon->get_csweapon_info();
	auto weapontype = weaponData->WeaponType;

	if ((pWeapon->m_iItemDefinitionIndex() == WEAPON_AWP || pWeapon->m_iItemDefinitionIndex() == WEAPON_SSG08) && g_cfg.legitbot.only_in_zoom && !g_ctx.globals.scoped) {
		return false;
	}

	if (!(pWeapon->m_iClip1() > 0)) {
		return false;
	}

	return !g_cfg.legitbot.on_key || key_binds::get().get_key_bind_state(1) || (g_cfg.legitbot.autofire && key_binds::get().get_key_bind_state(0));
}
//--------------------------------------------------------------------------------
float Aimbot::GetSmooth() {
	float smooth = IsRcs() && g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_smooth_enabled ? g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_smooth : g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].smooth;
	return smooth;
}
//--------------------------------------------------------------------------------
void Aimbot::Smooth(const Vector& currentAngle, const Vector& aimAngle, Vector& angle) {
	auto smooth_value = GetSmooth();
	if (smooth_value <= 1) {
		return;
	}

	Vector delta = aimAngle - currentAngle;
	math::normalize_angles(delta);

	if (g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].smooth_type == 1) {
		float deltaLength = fmaxf(sqrtf((delta.x * delta.x) + (delta.y * delta.y)), 0.01f);
		delta *= (1.0f / deltaLength);

		math::random_seed(m_globals()->m_tickcount);
		float randomize = math::random_float(-0.1f, 0.1f);
		smooth_value = fminf((m_globals()->m_intervalpertick * 64.0f) / (randomize + smooth_value * 0.15f), deltaLength);
	}
	else {
		smooth_value = (m_globals()->m_intervalpertick * 64.0f) / smooth_value;
	}

	delta *= smooth_value;
	angle = currentAngle + delta;
	math::normalize_angles(angle);
}
//--------------------------------------------------------------------------------
void Aimbot::RCS(Vector& angle, player_t* target, bool should_run) {
	if (!g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs) {
		RCSLastPunch.Init();
		return;
	}

	if (g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_x == 0 && g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_y == 0) {
		RCSLastPunch.Init();
		return;
	}

	Vector punch = g_ctx.local()->m_aimPunchAngle() * m_cvar()->FindVar(crypt_str("weapon_recoil_scale"))->GetFloat();

	auto globals = m_globals();
	auto weapon = g_ctx.globals.weapon;

	if (weapon && weapon->m_flNextPrimaryAttack() > globals->m_curtime) {
		auto delta_angles = punch - RCSLastPunch;
		auto delta = weapon->m_flNextPrimaryAttack() - globals->m_curtime;
		if (delta >= globals->m_intervalpertick)
			punch = RCSLastPunch + delta_angles / static_cast<float>(TIME_TO_TICKS(delta));
	}

	CurrentPunch = punch;
	if (g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_type == 0 && !should_run)
		punch -= { RCSLastPunch.x, RCSLastPunch.y, 0.f };

	RCSLastPunch = CurrentPunch;
	if (!IsRcs()) {
		return;
	}

	float pithcmult = g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_x;

	if (g_ctx.local()->m_iShotsFired() < 4 + g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_start)
		pithcmult += 10;
	punch.x *= (pithcmult / 100.0f);
	punch.y *= (g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_y / 100.0f);

	angle -= punch;

	math::normalize_angles(angle);
}
//--------------------------------------------------------------------------------
float Aimbot::GetFov() {
	if (IsRcs() && g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs && g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_fov_enabled) return g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_fov;
	if (!silent_enabled) return g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].fov;
	return g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].silent_fov > g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].fov ? g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].silent_fov : g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].fov;
}
//--------------------------------------------------------------------------------
player_t* Aimbot::GetClosestPlayer(CUserCmd* cmd, int& bestBone) {
	Vector ang;
	Vector eVecTarget;
	Vector pVecTarget = g_ctx.globals.eye_pos;
	if (target && !kill_delay && g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].kill_delay > 0 && IsNotTarget(target)) {
		target = NULL;
		shot_delay = false;
		kill_delay = true;
		kill_delay_time = (int)GetTickCount() + g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].kill_delay;
	}
	if (kill_delay) {
		if (kill_delay_time <= (int)GetTickCount()) kill_delay = false;
		else return NULL;
	}

	player_t* player;
	target = NULL;
	int bestHealth = 100.f;
	float bestFov = 9999.f;
	float bestDamage = 0.f;
	float bestBoneFov = 9999.f;
	float bestDistance = 9999.f;
	int health;
	float fov;
	float damage;
	float distance;
	int fromBone = g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].aim_type == 1 ? 0 : g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].hitbox;
	int toBone = g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].aim_type == 1 ? 7 : g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].hitbox;
	for (int i = 1; i < m_globals()->m_maxclients; ++i) {
		damage = 0.f;
		player = (player_t*)m_entitylist()->GetClientEntity(i);
		if (IsNotTarget(player)) {
			continue;
		}
		if (!g_cfg.legitbot.deathmatch && player->m_iTeamNum() == g_ctx.local()->m_iTeamNum()) {
			continue;
		}
		for (int bone = fromBone; bone <= toBone; bone++) {
			eVecTarget = player->hitbox_position(bone);
			math::vector_angles(eVecTarget - pVecTarget, ang);
			math::normalize_angles(ang);
			distance = pVecTarget.DistTo(eVecTarget);
			if (g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].fov_type == 1)
				fov = GetRealDistanceFOV(distance, ang, cmd->m_viewangles + RCSLastPunch);
			else
				fov = GetFovToPlayer(cmd->m_viewangles + RCSLastPunch, ang);

			if (fov > GetFov())
				continue;

			if (!util::visible(g_ctx.globals.eye_pos, eVecTarget, player, g_ctx.local()))
			{
				if (!g_cfg.legitbot.autowall)
					continue;

				damage = autowall::get().wall_penetration(g_ctx.globals.eye_pos, eVecTarget, player).damage;

				if (damage < g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].min_damage)
					continue;

			}

			if ((g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].priority == 1 || g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].priority == 2) && damage == 0.f) //-V550
				damage = autowall::get().wall_penetration(g_ctx.globals.eye_pos, eVecTarget, player).damage;

			health = player->m_iHealth() - damage;
			if (g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].smoke_check && IsLineGoesThroughSmoke(pVecTarget, eVecTarget))
				continue;

			bool OnGround = (g_ctx.local()->m_fFlags() & FL_ONGROUND);
			if (g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].jump_check && !OnGround)
				continue;

			if (g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].aim_type == 1 && bestBoneFov < fov) {
				continue;
			}
			bestBoneFov = fov;
			if (
				(g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].priority == 0 && bestFov > fov) ||
				(g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].priority == 1 && bestHealth > health) ||
				(g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].priority == 2 && bestDamage < damage) ||
				(g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].priority == 3 && distance < bestDistance)
				) {
				bestBone = bone;
				target = player;
				bestFov = fov;
				bestHealth = health;
				bestDamage = damage;
				bestDistance = distance;
			}
		}
	}
	return target;
}
//--------------------------------------------------------------------------------
bool Aimbot::IsNotSilent(float fov) {
	return IsRcs() || !silent_enabled || fov > g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].silent_fov;
}
//--------------------------------------------------------------------------------
void Aimbot::OnMove(CUserCmd* m_pcmd) {
	if (!IsEnabled(m_pcmd)) {
		if (g_ctx.local() && m_engine()->IsInGame() && g_ctx.local()->is_alive() && g_cfg.legitbot.enabled && g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_type == 0) {
			auto pWeapon = g_ctx.globals.weapon;
			if (pWeapon && (pWeapon->is_sniper() || pWeapon->is_pistol() || pWeapon->is_rifle())) {
				RCS(m_pcmd->m_viewangles, target, false);
				math::normalize_angles(m_pcmd->m_viewangles);
				m_engine()->SetViewAngles(m_pcmd->m_viewangles);
			}
		}
		else {
			RCSLastPunch = { 0, 0, 0 };
		}

		is_delayed = false;
		shot_delay = false;
		kill_delay = false;
		silent_enabled = g_cfg.legitbot.silent && g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].silent_fov > 0;
		target = NULL;
		return;
	}

	math::random_seed(m_pcmd->m_command_number);

	auto weapon = g_ctx.globals.weapon;
	if (!weapon)
		return;

	auto weapon_data = weapon->get_csweapon_info();
	if (!weapon_data)
		return;

	bool should_do_rcs = false;
	Vector angles = m_pcmd->m_viewangles;
	Vector current = angles;
	float fov = 180.f;
	if (!(g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].flash_check && g_ctx.local()->m_flFlashDuration() > 0)) {
		int bestBone = -1;
		if (GetClosestPlayer(m_pcmd, bestBone)) {
			math::vector_angles(target->hitbox_position(bestBone) - g_ctx.globals.eye_pos, angles);
			math::normalize_angles(angles);
			if (g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].fov_type == 1)
				fov = GetRealDistanceFOV(g_ctx.globals.eye_pos.DistTo(target->hitbox_position(bestBone)), angles, m_pcmd->m_viewangles);
			else
				fov = GetFovToPlayer(m_pcmd->m_viewangles, angles);

			should_do_rcs = true;

			if (!g_cfg.legitbot.silent && !is_delayed && !shot_delay && g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].shot_delay > 0) {
				is_delayed = true;
				shot_delay = true;
				shot_delay_time = GetTickCount() + g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].shot_delay;
			}

			if (shot_delay && shot_delay_time <= GetTickCount()) {
				shot_delay = false;
			}

			if (shot_delay) {
				m_pcmd->m_buttons &= ~IN_ATTACK;
			}

			if (g_cfg.legitbot.autofire) {
				if (key_binds::get().get_key_bind_state(0)) {
					if (m_pcmd->m_command_number % 2 == 0) {
						m_pcmd->m_buttons &= ~IN_ATTACK;
					}
					else {
						m_pcmd->m_buttons |= IN_ATTACK;
					}
				}
			}

			if (g_cfg.legitbot.autostop) {
				m_pcmd->m_forwardmove = m_pcmd->m_sidemove = 0;
			}
		}
	}

	if (IsNotSilent(fov) && (should_do_rcs || g_cfg.legitbot.weapon[g_ctx.globals.current_weapon].rcs_type == 0)) {
		RCS(angles, target, should_do_rcs);
	}

	if (target && IsNotSilent(fov)) {
		Smooth(current, angles, angles);
	}

	math::normalize_angles(angles);
	m_pcmd->m_viewangles = angles;
	if (IsNotSilent(fov)) {
		m_engine()->SetViewAngles(angles);
	}

	silent_enabled = false;
	if (g_ctx.globals.weapon->is_pistol() && g_cfg.legitbot.autopistol) {
		float server_time = g_ctx.local()->m_nTickBase() * m_globals()->m_intervalpertick;
		float next_shot = g_ctx.globals.weapon->m_flNextPrimaryAttack() - server_time;
		if (next_shot > 0) {
			m_pcmd->m_buttons &= ~IN_ATTACK;
		}
	}
}

Aimbot g_Aimbot;
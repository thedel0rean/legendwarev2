#include "autowall.h"

bool autowall::is_breakable_entity(IClientEntity* e)
{
	if (!e || !e->EntIndex())
		return false;

	using Fn = bool(__fastcall*)(IClientEntity*);
	static auto is_breakable = reinterpret_cast <Fn> (util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 51 56 8B F1 85 F6 74 68 83 BE")));

	auto take_damage = (char*)((uintptr_t)e + *(size_t*)((uintptr_t)is_breakable + 0x26));
	auto take_damage_backup = *take_damage;

	auto client_class = e->GetClientClass();

	if ((client_class->m_pNetworkName[1] == 'B' && client_class->m_pNetworkName[9] == 'e' && client_class->m_pNetworkName[10] == 'S' && client_class->m_pNetworkName[16] == 'e') || (client_class->m_pNetworkName[1] != 'B' || client_class->m_pNetworkName[5] != 'D'))
		*take_damage = DAMAGE_YES;

	auto breakable = is_breakable(e);
	*take_damage = take_damage_backup;

	return breakable;
}

void autowall::scale_damage(player_t* e, CGameTrace &enterTrace, weapon_info_t *weaponData, float& currentDamage)
{
	if (!e->is_player())
		return;

	auto IsArmored = [&]()->bool
	{
		switch (enterTrace.hitgroup)
		{
		case HITGROUP_HEAD:
			return e->m_bHasHelmet();
		case HITGROUP_GENERIC:
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			return true;
		default:
			return false;
		}
	};

	auto HasHeavyArmor = e->m_bHasHeavyArmor();

	switch (enterTrace.hitgroup)
	{
	case HITGROUP_HEAD:
		currentDamage *= HasHeavyArmor ? 2.0f : 4.0f;
		break;
	case HITGROUP_STOMACH:
		currentDamage *= 1.25f;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		currentDamage *= 0.75f;
		break;
	}

	auto ArmorValue = e->m_ArmorValue();

	if (ArmorValue > 0 && IsArmored())
	{
		auto armorBonusRatio = 0.5f;
		auto armorRatio = weaponData->flArmorRatio / 2.0f;
		auto bonusValue = 1.0f;

		if (HasHeavyArmor)
		{
			armorBonusRatio = 0.33f;
			armorRatio *= 0.5f;
			bonusValue = 0.33f;
		}

		auto NewDamage = currentDamage * armorRatio;

		if (HasHeavyArmor)
			NewDamage *= 0.85f;

		if ((currentDamage - currentDamage * armorRatio) * bonusValue * armorBonusRatio > ArmorValue)
			NewDamage = currentDamage - ArmorValue / armorBonusRatio;

		currentDamage = NewDamage;
	}
}

bool autowall::trace_to_exit(CGameTrace& enterTrace, CGameTrace& exitTrace, Vector startPosition, const Vector& direction)
{
	auto start = ZERO;
	auto end = ZERO;

	auto currentDistance = 0.0f;
	auto firstContents = 0;

	while (currentDistance <= 90.0f)
	{
		currentDistance += 4.0f;
		start = startPosition + direction * currentDistance;

		if (!firstContents)
			firstContents = m_trace()->GetPointContents(start, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr);

		auto pointContents = m_trace()->GetPointContents(start, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr);

		if (!(pointContents & MASK_SHOT_HULL) || pointContents & CONTENTS_HITBOX && pointContents != firstContents) //-V648
		{
			end = start - direction * 4.0f;
			util::trace_line(start, end, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr, &exitTrace);

			if (exitTrace.startsolid && exitTrace.surface.flags & SURF_HITBOX)
			{
				util::trace_line(start, startPosition, MASK_SHOT_HULL, exitTrace.hit_entity, &exitTrace);

				if (exitTrace.DidHit() && !exitTrace.startsolid)
					return true;

				continue;
			}

			if (exitTrace.DidHit() && !exitTrace.startsolid)
			{
				if (is_breakable_entity(enterTrace.hit_entity) && is_breakable_entity(exitTrace.hit_entity))
					return true;

				if (enterTrace.surface.flags & SURF_NODRAW || !(exitTrace.surface.flags & SURF_NODRAW) && exitTrace.plane.normal.Dot(direction) <= 1.0f) //-V648
					return true;

				continue;
			}
		}
	}

	return false;
}

bool autowall::handle_bullet_penetration(weapon_info_t* weaponData, CGameTrace& enterTrace, Vector& eyePosition, const Vector& direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration, bool draw_impact)
{
	CGameTrace exitTrace;

	auto enterSurfaceData = m_physsurface()->GetSurfaceData(enterTrace.surface.surfaceProps);
	auto enterMaterial = enterSurfaceData->game.material;

	auto isSolidSurf = (enterTrace.contents >> 3) & CONTENTS_SOLID;
	auto isLightSurf = (enterTrace.surface.flags >> 7) & SURF_LIGHT;

	if (!weaponData->flPenetration || !trace_to_exit(enterTrace, exitTrace, enterTrace.endpos, direction) && !(m_trace()->GetPointContents(enterTrace.endpos, MASK_SHOT_HULL, nullptr) & MASK_SHOT_HULL)) //-V648
		return false;

	auto combinedPenetrationModifier = 0.0f;
	auto finalDamageModifier = 0.0f;

	auto exitSurfaceData = m_physsurface()->GetSurfaceData(exitTrace.surface.surfaceProps);

	if (enterMaterial == CHAR_TEX_GRATE || enterMaterial == CHAR_TEX_GLASS)
	{
		combinedPenetrationModifier = 3.0f;
		finalDamageModifier = 0.05f;
	}
	else if (isSolidSurf || isLightSurf)
	{
		combinedPenetrationModifier = 1.0f;
		finalDamageModifier = 0.16f;
	}
	else if (enterMaterial == CHAR_TEX_FLESH && ((player_t*)enterTrace.hit_entity)->m_iTeamNum() == g_ctx.local()->m_iTeamNum() && !ff_damage_reduction_bullets)
	{
		if (!ff_damage_bullet_penetration) //-V550
			return false;

		combinedPenetrationModifier = ff_damage_bullet_penetration;
		finalDamageModifier = 0.16f;
	}
	else
	{
		combinedPenetrationModifier = (enterSurfaceData->game.flPenetrationModifier + exitSurfaceData->game.flPenetrationModifier) / 2.0f;
		finalDamageModifier = 0.16f;
	}

	auto exitMaterial = exitSurfaceData->game.material;

	if (enterMaterial == exitMaterial)
	{
		if (exitMaterial == CHAR_TEX_CARDBOARD || exitMaterial == CHAR_TEX_WOOD)
			combinedPenetrationModifier = 3.0f;
		else if (exitMaterial == CHAR_TEX_PLASTIC)
			combinedPenetrationModifier = 2.0f;
	}

	auto thickness = (exitTrace.endpos - enterTrace.endpos).LengthSqr();
	auto modifier = fmax(1.0f / combinedPenetrationModifier, 0.0f);

	auto lostDamage = fmax((modifier * thickness / 24.0f) + currentDamage * finalDamageModifier + fmax(3.75f / penetrationPower, 0.0f) * 3.0f * modifier, 0.0f);

	if (lostDamage > currentDamage)
		return false;

	if (lostDamage > 0.0f)
		currentDamage -= lostDamage;

	if (currentDamage < 1.0f)
		return false;

	eyePosition = exitTrace.endpos;
	--possibleHitsRemaining;

	if (draw_impact)
		m_debugoverlay()->BoxOverlay(enterTrace.endpos, Vector(-2.0f, -2.0f, -2.0f), Vector(2.0f, 2.0f, 2.0f), QAngle(0.0f, 0.0f, 0.0f), g_cfg.esp.client_bullet_impacts_color.r(), g_cfg.esp.client_bullet_impacts_color.g(), g_cfg.esp.client_bullet_impacts_color.b(), g_cfg.esp.client_bullet_impacts_color.a(), 4.0f);

	return true;
}

bool autowall::fire_bullet(weapon_t* pWeapon, Vector& direction, bool& visible, float& currentDamage, int& hitbox, IClientEntity* e, float length, const Vector& pos)
{
	if (!pWeapon)
		return false;

	auto weaponData = pWeapon->get_csweapon_info();

	if (!weaponData)
		return false;

	CGameTrace enterTrace;
	CTraceFilter filter;

	filter.pSkip = g_ctx.local();
	currentDamage = weaponData->iDamage;

	auto eyePosition = pos;
	auto currentDistance = 0.0f;
	auto maxRange = weaponData->flRange;
	auto penetrationDistance = 3000.0f;
	auto penetrationPower = weaponData->flPenetration;
	auto possibleHitsRemaining = 4;

	while (currentDamage >= 1.0f)
	{
		maxRange -= currentDistance;
		auto end = eyePosition + direction * maxRange;

		util::trace_line(eyePosition, end, MASK_SHOT_HULL | CONTENTS_HITBOX, g_ctx.local(), &enterTrace);

		if (e)
			util::clip_trace_to_players(e, eyePosition, end + direction * 40.0f, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &enterTrace);

		auto enterSurfaceData = m_physsurface()->GetSurfaceData(enterTrace.surface.surfaceProps);
		auto enterSurfPenetrationModifier = enterSurfaceData->game.flPenetrationModifier;
		auto enterMaterial = enterSurfaceData->game.material;

		if (enterTrace.fraction == 1.0f)  //-V550
		{
			if (!e)
				m_debugoverlay()->BoxOverlay(enterTrace.endpos, Vector(-2.0f, -2.0f, -2.0f), Vector(2.0f, 2.0f, 2.0f), QAngle(0.0f, 0.0f, 0.0f), g_cfg.esp.client_bullet_impacts_color.r(), g_cfg.esp.client_bullet_impacts_color.g(), g_cfg.esp.client_bullet_impacts_color.b(), g_cfg.esp.client_bullet_impacts_color.a(), 4.0f);

			break;
		}

		currentDistance += enterTrace.fraction * maxRange;
		currentDamage *= pow(weaponData->flRangeModifier, currentDistance / 500.0f);

		if (currentDistance > penetrationDistance && weaponData->flPenetration || enterSurfPenetrationModifier < 0.1f)  //-V1051
		{
			if (!e)
				m_debugoverlay()->BoxOverlay(enterTrace.endpos, Vector(-2.0f, -2.0f, -2.0f), Vector(2.0f, 2.0f, 2.0f), QAngle(0.0f, 0.0f, 0.0f), g_cfg.esp.client_bullet_impacts_color.r(), g_cfg.esp.client_bullet_impacts_color.g(), g_cfg.esp.client_bullet_impacts_color.b(), g_cfg.esp.client_bullet_impacts_color.a(), 4.0f);

			break;
		}

		auto canDoDamage = enterTrace.hitgroup != HITGROUP_GEAR && enterTrace.hitgroup != HITGROUP_GENERIC;
		auto isPlayer = ((player_t*)enterTrace.hit_entity)->is_player();
		auto isEnemy = ((player_t*)enterTrace.hit_entity)->m_iTeamNum() != g_ctx.local()->m_iTeamNum();

		if (canDoDamage && isPlayer && isEnemy)
		{
			scale_damage((player_t*)enterTrace.hit_entity, enterTrace, weaponData, currentDamage);
			hitbox = enterTrace.hitbox;

			if (!e)
				m_debugoverlay()->BoxOverlay(enterTrace.endpos, Vector(-2.0f, -2.0f, -2.0f), Vector(2.0f, 2.0f, 2.0f), QAngle(0.0f, 0.0f, 0.0f), g_cfg.esp.client_bullet_impacts_color.r(), g_cfg.esp.client_bullet_impacts_color.g(), g_cfg.esp.client_bullet_impacts_color.b(), g_cfg.esp.client_bullet_impacts_color.a(), 4.0f);

			return true;
		}

		if (!possibleHitsRemaining)
		{
			if (!e)
				m_debugoverlay()->BoxOverlay(enterTrace.endpos, Vector(-2.0f, -2.0f, -2.0f), Vector(2.0f, 2.0f, 2.0f), QAngle(0.0f, 0.0f, 0.0f), g_cfg.esp.client_bullet_impacts_color.r(), g_cfg.esp.client_bullet_impacts_color.g(), g_cfg.esp.client_bullet_impacts_color.b(), g_cfg.esp.client_bullet_impacts_color.a(), 4.0f);

			break;
		}

		static auto damageReductionBullets = m_cvar()->FindVar(crypt_str("ff_damage_reduction_bullets"));
		static auto damageBulletPenetration = m_cvar()->FindVar(crypt_str("ff_damage_bullet_penetration"));

		if (!handle_bullet_penetration(weaponData, enterTrace, eyePosition, direction, possibleHitsRemaining, currentDamage, penetrationPower, damageReductionBullets->GetFloat(), damageBulletPenetration->GetFloat(), !e))
		{
			if (!e)
				m_debugoverlay()->BoxOverlay(enterTrace.endpos, Vector(-2.0f, -2.0f, -2.0f), Vector(2.0f, 2.0f, 2.0f), QAngle(0.0f, 0.0f, 0.0f), g_cfg.esp.client_bullet_impacts_color.r(), g_cfg.esp.client_bullet_impacts_color.g(), g_cfg.esp.client_bullet_impacts_color.b(), g_cfg.esp.client_bullet_impacts_color.a(), 4.0f);

			break;
		}

		visible = false;
	}

	return false;
}

autowall::returninfo_t autowall::wall_penetration(const Vector& eye_pos, Vector& point, IClientEntity* e)
{
	g_ctx.globals.autowalling = true;

	auto tmp = point - eye_pos;

	auto angles = ZERO;
	math::vector_angles(tmp, angles);

	auto direction = ZERO;
	math::angle_vectors(angles, direction);

	direction.NormalizeInPlace();

	auto visible = true;
	auto damage = -1.0f;
	auto hitbox = -1;

	if (fire_bullet(g_ctx.globals.weapon, direction, visible, damage, hitbox, e, 0.0f, eye_pos))
	{
		g_ctx.globals.autowalling = false;
		return returninfo_t(visible, (int)damage, hitbox); //-V2003
	}
	else
	{
		g_ctx.globals.autowalling = false;
		return returninfo_t(false, -1, -1);
	}
}
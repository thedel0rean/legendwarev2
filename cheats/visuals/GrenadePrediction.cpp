// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "GrenadePrediction.h"

void GrenadePrediction::Tick(int buttons)
{
	act = ACT_NONE;

	bool in_attack = buttons & IN_ATTACK;
	bool in_attack2 = buttons & IN_ATTACK2;

	if (in_attack || in_attack2)
	{
		if (in_attack && in_attack2)
			act = ACT_LOB;
		else if (!in_attack)
			act = ACT_DROP;
		else
			act = ACT_THROW;
	}
}

void GrenadePrediction::View(CViewSetup* setup, weapon_t* weapon)
{
	if (g_ctx.local()->is_alive() && g_ctx.get_command())
	{
		if (!antiaim::get().freeze_check && (g_ctx.get_command()->m_buttons & IN_ATTACK || g_ctx.get_command()->m_buttons & IN_ATTACK2))
		{
			type = weapon->m_iItemDefinitionIndex();
			Simulate(setup);
		}
		else
			type = 0;
	}
}

void GrenadePrediction::Paint()
{
	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	auto lifetime = path.size() > 1 ? 9999.0f : 0.0f;

	if (type && path.size() > 1)
	{
		Vector nadeStart, nadeEnd;
		Vector prev = path[0];

		for (auto it = path.begin(), end = path.end(); it != end; ++it)
		{
			if (math::world_to_screen(prev, nadeStart) && math::world_to_screen(*it, nadeEnd))
			{
				render::get().line((int)nadeStart.x, (int)nadeStart.y, (int)nadeEnd.x, (int)nadeEnd.y, g_cfg.esp.grenade_prediction_tracer_color);
				render::get().line((int)nadeStart.x - 1, (int)nadeStart.y, (int)nadeEnd.x - 1, (int)nadeEnd.y, g_cfg.esp.grenade_prediction_tracer_color);
				render::get().line((int)nadeStart.x + 1, (int)nadeStart.y, (int)nadeEnd.x + 1, (int)nadeEnd.y, g_cfg.esp.grenade_prediction_tracer_color);
			}

			prev = *it;
		}

		for (auto it = others.begin(), end = others.end(); it != end; ++it)
			render::get().DrawFilled3DBox(it->first, 4, 4, g_cfg.esp.grenade_prediction_color, g_cfg.esp.grenade_prediction_color);

		if (math::world_to_screen(prev, nadeEnd))
			render::get().text(fonts[GRENADES], nadeEnd.x, nadeEnd.y, g_cfg.esp.grenade_prediction_color, HFONT_CENTERED_X | HFONT_CENTERED_Y, weapon->get_icon());

		Vector endpos = path[path.size() - 1];

		if (weapon->m_iItemDefinitionIndex() == WEAPON_SMOKEGRENADE)
			render::get().Draw3DCircle(endpos, 144, g_cfg.esp.grenade_prediction_tracer_color);
		else if (weapon->m_iItemDefinitionIndex() == WEAPON_MOLOTOV || weapon->m_iItemDefinitionIndex() == WEAPON_INCGRENADE)
			render::get().Draw3DCircle(endpos, 150, g_cfg.esp.grenade_prediction_tracer_color);
	}
}

void GrenadePrediction::Setup(Vector& vecSrc, Vector& vecThrow, Vector& viewangles)
{
	Vector angThrow = viewangles;
	float pitch = math::normalize_pitch(angThrow.x);

	float a = pitch - (90.0f - fabs(pitch)) * 10.0f / 90.0f;
	angThrow.x = a;

	float flVel = 750.0f * 0.9f;
	static const float power[] = { 1.0f, 1.0f, 0.5f, 0.0f };
	float b = power[act];
	b = b * 0.7f; b = b + 0.3f;
	flVel *= b;

	Vector vForward, vRight, vUp;
	math::angle_vectors(angThrow, &vForward, &vRight, &vUp);

	vecSrc = g_ctx.local()->GetAbsOrigin() + g_ctx.local()->m_vecViewOffset();
	float off = power[act] * 12.0f - 12.0f;
	vecSrc.z += off;

	trace_t tr;
	Vector vecDest = vecSrc;
	vecDest += vForward * 22.0f;

	TraceHull(vecSrc, vecDest, tr);

	Vector vecBack = vForward; vecBack *= 6.0f;
	vecSrc = tr.endpos;
	vecSrc -= vecBack;

	vecThrow = g_ctx.local()->m_vecVelocity(); vecThrow *= 1.25f;
	vecThrow += vForward * flVel;
}

void GrenadePrediction::Simulate(CViewSetup* setup)
{
	Vector vecSrc, vecThrow;
	Vector angles; m_engine()->GetViewAngles(angles);
	Setup(vecSrc, vecThrow, angles);

	float interval = m_globals()->m_intervalpertick;
	int logstep = (int)(0.05f / interval);
	int logtimer = 0;

	path.clear(); others.clear();

	for (int i = 0; i < path.max_size() - 1; ++i)
	{
		if (!logtimer)
			path.push_back(vecSrc);

		int s = Step(vecSrc, vecThrow, i, interval);

		if (s & 1)
			break;

		if (s & 2 || logtimer >= logstep)
			logtimer = 0;

		else
			++logtimer;
	}
	path.push_back(vecSrc);
}

void VectorAngles(const Vector& forward, QAngle& angles)
{
	if (forward[1] == 0.0f && forward[0] == 0.0f)
	{
		angles[0] = (forward[2] > 0.0f) ? 270.0f : 90.0f;
		angles[1] = 0.0f;
	}
	else
	{
		angles[0] = atan2(-forward[2], forward.Length2D()) * -180 / M_PI;
		angles[1] = atan2(forward[1], forward[0]) * 180 / M_PI;

		if (angles[1] > 90) angles[1] -= 180;
		else if (angles[1] < 90) angles[1] += 180;
		else if (angles[1] == 90) angles[1] = 0;
	}

	angles[2] = 0.0f;
}

int GrenadePrediction::Step(Vector& vecSrc, Vector& vecThrow, int tick, float interval)
{
	Vector move; AddGravityMove(move, vecThrow, interval, false);
	trace_t tr; PushEntity(vecSrc, move, tr);

	int result = 0;
	if (CheckDetonate(vecThrow, tr, tick, interval))
		result |= 1;

	if (tr.fraction != 1.0f)
	{
		result |= 2;
		ResolveFlyCollisionCustom(tr, vecThrow, interval);

		QAngle angles;
		VectorAngles((tr.endpos - tr.startpos).Normalized(), angles);
		others.emplace_back(std::make_pair(tr.endpos, angles));
	}

	vecSrc = tr.endpos;

	return result;
}

bool GrenadePrediction::CheckDetonate(const Vector& vecThrow, const trace_t& tr, int tick, float interval)
{
	switch (type)
	{
	case WEAPON_SMOKEGRENADE:
	case WEAPON_DECOY:
		if (vecThrow.Length2D() < 0.1f)
		{
			int det_tick_mod = (int)(0.2f / interval);
			return !(tick % det_tick_mod);
		}
		return false;

	case WEAPON_MOLOTOV:
	case WEAPON_INCGRENADE:
		if (tr.fraction != 1.0f && tr.plane.normal.z > 0.7f)
			return true;

	case WEAPON_FLASHBANG:
	case WEAPON_HEGRENADE:
		return (float)tick * interval > 1.5f && !(tick % (int)(0.2f / interval));
	default:
		return false;
	}
}

void GrenadePrediction::TraceHull(Vector& src, Vector& end, trace_t& tr)
{
	Ray_t ray;
	CTraceFilterWorldAndPropsOnly filter;

	ray.Init(src, end, Vector(-2.0f, -2.0f, -2.0f), Vector(2.0f, 2.0f, 2.0f));
	m_trace()->TraceRay(ray, 0x200400B, &filter, &tr);
}

void GrenadePrediction::AddGravityMove(Vector& move, Vector& vel, float frametime, bool onground)
{
	Vector basevel(0.0f, 0.0f, 0.0f);
	move.x = (vel.x + basevel.x) * frametime;
	move.y = (vel.y + basevel.y) * frametime;

	if (onground)
		move.z = (vel.z + basevel.z) * frametime;
	else
	{
		float gravity = 800.0f * 0.4f;
		float newZ = vel.z - (gravity * frametime);
		move.z = ((vel.z + newZ) / 2.0f + basevel.z) * frametime;
		vel.z = newZ;
	}
}

void GrenadePrediction::PushEntity(Vector& src, const Vector& move, trace_t& tr)
{
	Vector vecAbsEnd = src;
	vecAbsEnd += move;
	TraceHull(src, vecAbsEnd, tr);
}

void GrenadePrediction::ResolveFlyCollisionCustom(trace_t& tr, Vector& vecVelocity, float interval)
{
	float flSurfaceElasticity = 1.0, flGrenadeElasticity = 0.45f;
	float flTotalElasticity = flGrenadeElasticity * flSurfaceElasticity;
	if (flTotalElasticity > 0.9f) flTotalElasticity = 0.9f;
	if (flTotalElasticity < 0.0f) flTotalElasticity = 0.0f;

	Vector vecAbsVelocity;
	PhysicsClipVelocity(vecVelocity, tr.plane.normal, vecAbsVelocity, 2.0f);
	vecAbsVelocity *= flTotalElasticity;

	float flSpeedSqr = vecAbsVelocity.LengthSqr();
	static const float flMinSpeedSqr = 20.0f * 20.0f;

	if (flSpeedSqr < flMinSpeedSqr)
	{
		vecAbsVelocity.x = 0.0f;
		vecAbsVelocity.y = 0.0f;
		vecAbsVelocity.z = 0.0f;
	}

	if (tr.plane.normal.z > 0.7f)
	{
		vecVelocity = vecAbsVelocity;
		vecAbsVelocity *= ((1.0f - tr.fraction) * interval);
		PushEntity(tr.endpos, vecAbsVelocity, tr);
	}
	else
		vecVelocity = vecAbsVelocity;
}

int GrenadePrediction::PhysicsClipVelocity(const Vector& in, const Vector& normal, Vector& out, float overbounce)
{
	static const float STOP_EPSILON = 0.1f;

	float backoff, change, angle;
	int   i, blocked;

	blocked = 0;
	angle = normal[2];

	if (angle > 0) blocked |= 1;
	if (!angle) blocked |= 2;

	backoff = in.Dot(normal) * overbounce;
	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
	return blocked;
}

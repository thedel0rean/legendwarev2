#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"

class Aimbot
{ //-V802
public:
	void OnMove(CUserCmd *m_pcmd);
	bool IsEnabled(CUserCmd *m_pcmd);
	float GetFovToPlayer(const Vector& viewAngle, const Vector& aimAngle);

	bool IsRcs();
	float GetSmooth();
	float GetFov();

private:
	void RCS(Vector &angle, player_t* target, bool should_run);
	bool IsLineGoesThroughSmoke(const Vector& vStartPos, const Vector& vEndPos);
	void Smooth(const Vector& currentAngle, const Vector& aimAngle, Vector& angle);
	bool IsNotSilent(float fov);
	player_t* GetClosestPlayer(CUserCmd* cmd, int &bestBone);
	float shot_delay_time = 0.0f;
	bool shot_delay = false;
	bool silent_enabled = false;
	Vector CurrentPunch = { 0,0,0 };
	Vector RCSLastPunch = { 0,0,0 };
	bool is_delayed = false;
	int kill_delay_time = 0;
	bool kill_delay = false;
	player_t* target = NULL;
};

extern Aimbot g_Aimbot;
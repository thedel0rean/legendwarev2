// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "hooks.hpp"

#include <tchar.h>
#include <iostream>
#include <d3d9.h>
#include <dinput.h>

#include "..\cheats\misc\logs.h"
#include "..\cheats\misc\misc.h"
#include "..\cheats\visuals\other_esp.h"
#include "..\cheats\visuals\radar.h"

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

#include <shlobj.h>
#include <shlwapi.h>
#include <thread>
#include "..\Bytesa.h"

std::vector <std::string> files;
std::vector <std::string> scripts;

int selected_script = 0;

ImFont* MainText;
ImFont* HeaderMenu;
ImFont* Porter;
ImFont* PorterBeta;
ImFont* Verdana16;
ImFont* VisitorSmall;
ImFont* Tabs;
ImFont* KeyBinds;
ImFont* KeyBindsPixel;
ImFont* Icons;
ImFont* VisIcons;
ImFont* TabsText;
ImFont* Segoi;

auto _visible = true;

auto rage_weapon = 0;
auto legit_weapon = 0;

auto itemIndex = 0;

std::string get_config_dir()
{
	std::string folder;
	static TCHAR path[MAX_PATH];

	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, path)))
		folder = std::string(path) + crypt_str("\\Legendware\\Configs\\");

	CreateDirectory(folder.c_str(), NULL);
	return folder;
}

void load_config()
{
	if (cfg_manager->files.empty())
		return;

	cfg_manager->load(cfg_manager->files.at(g_cfg.selected_config), false);
	c_lua::get().unload_all_scripts();

	for (auto& script : g_cfg.scripts.scripts)
		c_lua::get().load_script(c_lua::get().get_script_id(script));

	scripts = c_lua::get().scripts;

	if (selected_script >= scripts.size())
		selected_script = scripts.size() - 1;

	for (auto& current : scripts)
	{
		if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
			current.erase(current.size() - 5, 5);
		else if (current.size() >= 4)
			current.erase(current.size() - 4, 4);
	}

	g_cfg.scripts.scripts.clear();

	cfg_manager->load(cfg_manager->files.at(g_cfg.selected_config), true);
	cfg_manager->config_files();

	eventlogs::get().add(crypt_str("Loaded ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);
}

void save_config()
{
	if (cfg_manager->files.empty())
		return;

	g_cfg.scripts.scripts.clear();

	for (auto i = 0; i < c_lua::get().scripts.size(); ++i)
	{
		auto script = c_lua::get().scripts.at(i);

		if (c_lua::get().loaded.at(i))
			g_cfg.scripts.scripts.emplace_back(script);
	}

	cfg_manager->save(cfg_manager->files.at(g_cfg.selected_config));
	cfg_manager->config_files();

	eventlogs::get().add(crypt_str("Saved ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);
}

void remove_config()
{
	if (cfg_manager->files.empty())
		return;

	eventlogs::get().add(crypt_str("Removed ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);

	cfg_manager->remove(cfg_manager->files.at(g_cfg.selected_config));
	cfg_manager->config_files();

	files = cfg_manager->files;

	if (g_cfg.selected_config >= files.size())
		g_cfg.selected_config = files.size() - 1;

	for (auto& current : files)
		if (current.size() > 2)
			current.erase(current.size() - 3, 3);
}

void add_config()
{
	auto empty = true;

	for (auto current : g_cfg.new_config_name)
	{
		if (current != ' ')
		{
			empty = false;
			break;
		}
	}

	if (empty)
		g_cfg.new_config_name = crypt_str("config");

	eventlogs::get().add(crypt_str("Added ") + g_cfg.new_config_name + crypt_str(" config"), false);

	if (g_cfg.new_config_name.find(crypt_str(".lw")) == std::string::npos)
		g_cfg.new_config_name += crypt_str(".lw");

	cfg_manager->save(g_cfg.new_config_name);
	cfg_manager->config_files();

	g_cfg.selected_config = cfg_manager->files.size() - 1;
	files = cfg_manager->files;

	for (auto& current : files)
		if (current.size() > 2)
			current.erase(current.size() - 3, 3);
}

bool LabelClick(const char* label, bool* v, const char* unique_id)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	// The concatoff/on thingies were for my weapon config system so if we're going to make that, we still need this aids.
	char Buf[64];
	_snprintf(Buf, 62, crypt_str("%s"), label);

	char getid[128];
	sprintf_s(getid, 128, crypt_str("%s%s"), label, unique_id);


	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(getid);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	const ImRect check_bb(window->DC.CursorPos, ImVec2(label_size.y + style.FramePadding.y * 2 + window->DC.CursorPos.x, window->DC.CursorPos.y + label_size.y + style.FramePadding.y * 2));
	ImGui::ItemSize(check_bb, style.FramePadding.y);

	ImRect total_bb = check_bb;

	if (label_size.x > 0)
	{
		ImGui::SameLine(0, style.ItemInnerSpacing.x);
		const ImRect text_bb(ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y + style.FramePadding.y), ImVec2(window->DC.CursorPos.x + label_size.x, window->DC.CursorPos.y + style.FramePadding.y + label_size.y));

		ImGui::ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
		total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));
	}

	if (!ImGui::ItemAdd(total_bb, &id))
		return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
		*v = !(*v);

	if (*v)
		ImGui::PushStyleColor(ImGuiCol_Text, g_ctx.globals.menu_color);
	if (label_size.x > 0.0f)
		ImGui::RenderText(ImVec2(check_bb.GetTL().x + 12, check_bb.GetTL().y), Buf);
	if (*v)
		ImGui::PopStyleColor();

	return pressed;

}

float clip(float n, float lower, float upper)
{
	n = (n > lower) * n + !(n > lower) * lower;
	return (n < upper) * n + !(n < upper) * upper;
}

void KeyBindButton(key_bind* key_bind, const char* unique_id)
{
	if (key_bind->key == KEY_ESCAPE)
		key_bind->key = KEY_NONE;

	auto clicked = false;
	auto text = (std::string)m_inputsys()->ButtonCodeToString(key_bind->key);

	if (key_bind->key <= KEY_NONE || key_bind->key >= KEY_MAX)
		text = crypt_str("None");

	if (hooks::input_shouldListen && hooks::input_receivedKeyval == &key_bind->key)
	{
		clicked = true;
		text = crypt_str("...");
	}

	auto textsize = ImGui::CalcTextSize(text.c_str()).x;

	ImGui::SameLine();
	ImGui::PushFont(MainText);
	ImGui::PushStyleColor(ImGuiCol_Text, ImColor(255, 255, 255));

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);

	if (ImGui::Button(text.c_str(), unique_id, ImVec2(textsize < 55 ? 60 : textsize + 8, 19)))
		clicked = true;

	ImGui::PopStyleColor();
	ImGui::PopFont();

	if (clicked)
	{
		hooks::input_shouldListen = true;
		hooks::input_receivedKeyval = &key_bind->key;
	}

	static auto hold = false, toggle = false;

	switch (key_bind->mode)
	{
	case HOLD:
		hold = true;
		toggle = false;
		break;
	case TOGGLE:
		toggle = true;
		hold = false;
		break;
	}

	if (ImGui::BeginPopup(unique_id))
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);

		if (LabelClick(crypt_str("Hold"), &hold, unique_id))
		{
			if (hold)
			{
				toggle = false;
				key_bind->mode = HOLD;
			}
			else if (toggle)
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}
			else
			{
				toggle = false;
				key_bind->mode = HOLD;
			}

			ImGui::CloseCurrentPopup();
		}

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 11);

		if (LabelClick(crypt_str("Toggle"), &toggle, unique_id))
		{
			if (toggle)
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}
			else if (hold)
			{
				toggle = false;
				key_bind->mode = HOLD;
			}
			else
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}

			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void AddKeybind(const std::string& bind, const std::string& state)
{
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 3);
	ImGui::Text(bind.c_str());
	ImGui::SameLine();

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 7);
	ImGui::SetCursorPosX(187 - ImGui::CalcTextSize(state.c_str()).x);
	ImGui::Text(state.c_str());
}

void render_menu() //-V553
{
	auto style = ImGui::GetStyle();

	g_ctx.globals.menu_color = ImVec4((float)g_cfg.menu.menu_theme.r() / 255.0f, (float)g_cfg.menu.menu_theme.g() / 255.0f, (float)g_cfg.menu.menu_theme.b() / 255.0f, 220 / 255.f);

	ImGui::GetStyle().Colors[ImGuiCol_CheckMark] = g_ctx.globals.menu_color;
	ImGui::GetStyle().Colors[ImGuiCol_SliderGrab] = ImVec4(18 / 255.f, 125 / 255.f, 200 / 255.f, 0.0f);
	ImGui::GetStyle().Colors[ImGuiCol_SliderGrabActive] = ImVec4(18 / 255.f, 125 / 255.f, 200 / 255.f, 0.0f);

	ImVec2 window_pos;

	static auto opened = true;
	static auto first_open = false;

	const float width = 645.f;
	const float height = 540.f;
	auto deltatime = m_globals()->m_frametime * 65.f;

	static auto tab = 6;
	static auto backup_tab = -1;

	auto y_pos_need = 13.f + tab * 87.f - (tab == 6 ? 2.f : 0.f);
	static auto cur_y_pos_slider = 12.f + 6 * 87.f;

	ImGui::Begin(crypt_str("Menu"), &opened, ImVec2(width, height), 1.f, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
	{
		static auto reverse_anim = true;
		static auto animtab = 6;
		static auto width_tab = 93.f;

		window_pos = ImGui::GetWindowPos();

		static constexpr auto frequency = 1 / 0.55f;

		static auto alpha = 254;
		auto head = backup_tab == tab;

		if (backup_tab != tab)
		{
			if ((!reverse_anim && std::abs(y_pos_need - cur_y_pos_slider) < (std::abs(backup_tab - tab) > 1 ? 80.f : 57.f)) || !first_open)
				alpha = clip(alpha - (deltatime + alpha * 0.015), 0, 254);
			if (alpha == 0)
				backup_tab = tab;
		}
		else
		{
			first_open = true;
			alpha = 255;
		}

		ImGui::GetStyle().Colors[ImGuiCol_Border] = ImVec4(235 / 255.f, 235 / 255.f, 235 / 255.f, (float)(backup_tab == tab ? 255 : 255 - alpha) / 254.f / 10.f);
		ImVec2 p = ImGui::GetCursorScreenPos();

		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + width, p.y + height), ImColor(13, 13, 13, 255));
		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 98, p.y + 13), ImVec2(p.x + (width - 6), p.y + (height - 6)), ImColor(15, 15, 15, 255));
		ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x + 98, p.y + 13), ImVec2(p.x + (width - 6), p.y + (height - 6)), ImColor(210, 210, 210, 25));
		ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImVec2(p.x + 5, p.y + 6), ImVec2(p.x + (width - 5), p.y + 8), ImColor(158, 73, 107, 255), ImColor(93, 73, 158, 255), ImColor(93, 73, 158, 255), ImColor(158, 73, 107, 255));
		ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImVec2(p.x + 5, p.y + 8), ImVec2(p.x + (width - 5), p.y + 11), ImColor(158, 73, 107, 65), ImColor(93, 73, 158, 65), ImColor(93, 73, 158, 0), ImColor(158, 73, 107, 0));
		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 6, p.y + 13), ImVec2(p.x + 8, p.y + (height - 7)), ImColor(220, 220, 220, 40));

		ImGui::PushFont(MainText);

		if (animtab != tab)
		{
			if (reverse_anim)
			{
				if (width_tab == 93.f) //-V550
					width_tab = 20.f;

				if (width_tab != 2.f) //-V550
					width_tab = clip(width_tab - (1.f * deltatime * 1.45f), 2.f, 93.f);
				else
					reverse_anim = false;
			}
			else
			{
				if (cur_y_pos_slider < y_pos_need)
					cur_y_pos_slider += 0.5f + std::abs(cur_y_pos_slider - y_pos_need) * 0.014f * deltatime * 2.f;
				else if (cur_y_pos_slider > y_pos_need)
					cur_y_pos_slider -= 0.5f + std::abs(cur_y_pos_slider - y_pos_need) * 0.014f * deltatime * 2.f;
				if (std::abs(y_pos_need - cur_y_pos_slider) < 10.f)
				{
					if (alpha == 0)
						width_tab = 93.f;

					if (width_tab != 93.f) //-V550
						width_tab = clip(width_tab + (1.f * deltatime * 1.45f), 2.f, 93.f);
					else
					{
						reverse_anim = true;
						animtab = tab;
					}
				}
			}
		}

		std::string preview = crypt_str("None");

		ImGui::SetCursorPosX(8);
		ImGui::SetCursorPosY(13);
		ImGui::BeginGroup();
		{
			ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 6, p.y + cur_y_pos_slider - 87), ImVec2(p.x + 6 + 0 /*width_tab*/, p.y + cur_y_pos_slider), ImColor(15, 15, 15, 255));
			ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImVec2(p.x + 6, p.y + cur_y_pos_slider - 87), ImVec2(p.x + 8, p.y + cur_y_pos_slider), ImColor(93, 73, 158, 255), ImColor(93, 73, 158, 255), ImColor(158, 73, 107, 255), ImColor(158, 73, 107, 255));
			ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImVec2(p.x + 8, p.y + cur_y_pos_slider - 87), ImVec2(p.x + 8 + 10, p.y + cur_y_pos_slider), ImColor(93, 73, 158, 55), ImColor(93, 73, 158, 0), ImColor(158, 73, 107, 0), ImColor(158, 73, 107, 55));

			ImGui::PushFont(TabsText);
			ImGui::PushStyleColor(ImGuiCol_Button, ImColor(13, 13, 13, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor(13, 13, 13, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor(13, 13, 13, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));

			if (ImGui::Button(crypt_str("RAGE"), crypt_str("P"), VisIcons, 0.41f, ImVec2(90, 86)))
				tab = animtab == tab ? 1 : tab;

			if (ImGui::Button(crypt_str("LEGIT"), crypt_str("C"), Icons, 0.41f, ImVec2(90, 87)))
				tab = animtab == tab ? 2 : tab;

			if (ImGui::Button(crypt_str("PLAYERS"), crypt_str("D"), Icons, 0.45f, ImVec2(90, 87)))
				tab = animtab == tab ? 3 : tab;

			if (ImGui::Button(crypt_str("VISUALS"), crypt_str("V"), VisIcons, 0.4f, ImVec2(90, 87)))
				tab = animtab == tab ? 4 : tab;

			if (ImGui::Button(crypt_str("MISC"), crypt_str("G"), Icons, 0.41f, ImVec2(90, 87)))
				tab = animtab == tab ? 5 : tab;

			if (ImGui::Button(crypt_str("SETTINGS"), crypt_str("F"), Icons, 0.415f, ImVec2(90, 85)))
				tab = animtab == tab ? 6 : tab;

			ImGui::PopStyleVar();
			ImGui::PopStyleColor(3);
			ImGui::PopFont();
		}
		ImGui::EndGroup();

		ImGui::SetCursorPosY(22);
		ImGui::SetCursorPosX(120);

		ImGui::BeginGroup();
		{
			if (tab == 1)
			{
				static auto rage_tab = 0;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 8);

				ImGui::BeginGroup();
				{
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 10.f));
					ImGui::PushFont(TabsText);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 22);
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
					ImGui::BeginGroup();

					if (ImGui::SemiTab(crypt_str("general"), ImVec2(248 + 22, 20), !rage_tab))
						rage_tab = 0;

					ImGui::SameLine();

					if (ImGui::SemiTab(crypt_str("weapons"), ImVec2(249 + 22, 20), rage_tab == 1))
						rage_tab = 1;

					ImGui::EndGroup();
					ImGui::PopFont();

					if (rage_tab == 1)
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5);

						ImGui::BeginGroup();
						{
							ImGui::PushFont(Tabs);

							if (ImGui::SemiTabRage(crypt_str("J"), ImVec2(62, 20), rage_weapon == 0))
								rage_weapon = 0;

							ImGui::SameLine();

							if (ImGui::SemiTabRage(crypt_str("E"), ImVec2(62, 20), rage_weapon == 1))
								rage_weapon = 1;

							ImGui::SameLine();

							if (ImGui::SemiTabRage(crypt_str("K"), ImVec2(63, 20), rage_weapon == 2))
								rage_weapon = 2;

							ImGui::SameLine();

							if (ImGui::SemiTabRage(crypt_str("W"), ImVec2(63, 20), rage_weapon == 3))
								rage_weapon = 3;

							ImGui::SameLine();

							if (ImGui::SemiTabRage(crypt_str("Y"), ImVec2(62, 20), rage_weapon == 4))
								rage_weapon = 4;

							ImGui::SameLine();

							if (ImGui::SemiTabRage(crypt_str("a"), ImVec2(63, 20), rage_weapon == 5))
								rage_weapon = 5;

							ImGui::SameLine();

							if (ImGui::SemiTabRage(crypt_str("Z"), ImVec2(62, 20), rage_weapon == 6))
								rage_weapon = 6;

							ImGui::SameLine();

							if (ImGui::SemiTabRage(crypt_str("b"), ImVec2(62, 20), rage_weapon == 7))
								rage_weapon = 7;

							ImGui::PopFont();
						}
						ImGui::EndGroup();

						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
					}
					ImGui::PopStyleVar();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}
				ImGui::EndGroup();

				if (rage_tab == 1)
				{
					ImGui::BeginGroup();
					{
						ImGui::BeginChild(crypt_str("Hitscan"), ImVec2(245, 262), true);
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
							ImGui::BeginGroup();

							ImGui::Combo(crypt_str("Target selection"), &g_cfg.ragebot.weapon[rage_weapon].selection_type, selection, ARRAYSIZE(selection));

							for (auto i = 0, j = 0; i < ARRAYSIZE(hitboxes); i++)
							{
								if (g_cfg.ragebot.weapon[rage_weapon].hitscan[i])
								{
									if (j)
										preview += crypt_str(", ") + (std::string)hitboxes[i];
									else
										preview = hitboxes[i];

									j++;
								}
							}

							if (ImGui::BeginCombo(crypt_str("Hitboxes"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(hitboxes))))
							{
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
								ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

								ImGui::BeginGroup();
								{
									ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

									for (auto i = 0; i < ARRAYSIZE(hitboxes); i++)
										ImGui::Selectable(hitboxes[i], (bool*)&g_cfg.ragebot.weapon[rage_weapon].hitscan[i], (ARRAYSIZE(hitboxes) - i == 1) ? true : false, ImGuiSelectableFlags_DontClosePopups);

									ImGui::PopStyleVar();
								}
								ImGui::EndGroup();

								ImGui::EndCombo();
							}

							preview = crypt_str("None");

							for (auto i = 0, j = 0; i < ARRAYSIZE(bodyaim); i++)
							{
								if (g_cfg.ragebot.weapon[rage_weapon].baim_settings[i])
								{
									if (j)
										preview += crypt_str(", ") + (std::string)bodyaim[i];
									else
										preview = bodyaim[i];

									j++;
								}
							}

							ImGui::Text(crypt_str("Body aim"));
							KeyBindButton(&g_cfg.ragebot.baim_key, crypt_str("bodyaim"));

							if (ImGui::BeginCombo(crypt_str("Body aim conditions"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(bodyaim))))
							{
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
								ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

								ImGui::BeginGroup();
								{
									ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

									for (auto i = 0; i < ARRAYSIZE(bodyaim); i++)
										ImGui::Selectable(bodyaim[i], (bool*)&g_cfg.ragebot.weapon[rage_weapon].baim_settings[i], ARRAYSIZE(bodyaim) - i == 1, ImGuiSelectableFlags_DontClosePopups);

									ImGui::PopStyleVar();
								}
								ImGui::EndGroup();

								ImGui::EndCombo();
							}

							preview = crypt_str("None");

							auto enabled_body_aim_conditions = false;

							for (auto i = 0; i < ARRAYSIZE(bodyaim); i++)
								if (g_cfg.ragebot.weapon[rage_weapon].baim_settings[i])
									enabled_body_aim_conditions = true;

							if (enabled_body_aim_conditions)
								ImGui::Combo(crypt_str("Body aim level"), &g_cfg.ragebot.weapon[rage_weapon].baim_level, bodyaimlevel, ARRAYSIZE(bodyaimlevel));

							ImGui::Checkbox(crypt_str("Ignore limbs when moving"), &g_cfg.ragebot.weapon[rage_weapon].ignore_limbs);
							ImGui::EndGroup();

							if (backup_tab != tab)
								ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
						}
						ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

						ImGui::BeginChild(crypt_str("Exploits"), ImVec2(245, 180), true);
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
							ImGui::BeginGroup();

							ImGui::Checkbox(crypt_str("Double tap"), &g_cfg.ragebot.double_tap);

							if (g_cfg.ragebot.double_tap)
							{
								ImGui::Checkbox(crypt_str("Instant"), &g_cfg.ragebot.instant);

								if (g_cfg.ragebot.instant)
									ImGui::Checkbox(crypt_str("Slow teleport"), &g_cfg.ragebot.slow_teleport);

								ImGui::Text(crypt_str("Double tap key"));
								KeyBindButton(&g_cfg.ragebot.double_tap_key, crypt_str("doubletap"));
							}

							ImGui::Checkbox(crypt_str("Hide shots"), &g_cfg.antiaim.hide_shots);

							if (g_cfg.antiaim.hide_shots)
							{
								ImGui::Text(crypt_str("Hide shots key"));
								KeyBindButton(&g_cfg.antiaim.hide_shots_key, crypt_str("hideshots"));
							}

							ImGui::EndGroup();

							if (backup_tab != tab)
								ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
						}
						ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
					}
					ImGui::EndGroup();

					ImGui::SameLine();

					ImGui::BeginChild(crypt_str("Accuracy"), ImVec2(245, 452), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
						ImGui::BeginGroup();

						ImGui::Checkbox(crypt_str("Automatic stop"), &g_cfg.ragebot.weapon[rage_weapon].autostop);

						if (g_cfg.ragebot.weapon[rage_weapon].autostop)
						{
							for (auto i = 0, j = 0; i < ARRAYSIZE(autostop_modifiers); i++)
							{
								if (g_cfg.ragebot.weapon[rage_weapon].autostop_modifiers[i])
								{
									if (j)
										preview += crypt_str(", ") + (std::string)autostop_modifiers[i];
									else
										preview = autostop_modifiers[i];

									j++;
								}
							}

							if (ImGui::BeginCombo(crypt_str("Modifiers"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(autostop_modifiers))))
							{
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
								ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

								ImGui::BeginGroup();
								{
									ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

									for (auto i = 0; i < ARRAYSIZE(autostop_modifiers); i++)
										ImGui::Selectable(autostop_modifiers[i], (bool*)&g_cfg.ragebot.weapon[rage_weapon].autostop_modifiers[i], ARRAYSIZE(autostop_modifiers) - i == 1, ImGuiSelectableFlags_DontClosePopups);

									ImGui::PopStyleVar();
								}
								ImGui::EndGroup();

								ImGui::EndCombo();
							}

							preview = crypt_str("None");
						}

						ImGui::Checkbox(crypt_str("Hitchance"), &g_cfg.ragebot.weapon[rage_weapon].hitchance);

						if (g_cfg.ragebot.weapon[rage_weapon].hitchance)
						{
							ImGui::SliderInt(crypt_str("Hitchance amount"), &g_cfg.ragebot.weapon[rage_weapon].hitchance_amount, 1, 100);
							ImGui::Checkbox(crypt_str("Accuracy boost"), &g_cfg.ragebot.weapon[rage_weapon].accuracy_boost);

							if (g_cfg.ragebot.weapon[rage_weapon].accuracy_boost)
								ImGui::SliderInt(crypt_str("Accuracy boost amount"), &g_cfg.ragebot.weapon[rage_weapon].accuracy_boost_amount, 1, 100);

							ImGui::Checkbox(crypt_str("Skip hitchance if low inaccuracy"), &g_cfg.ragebot.weapon[rage_weapon].skip_hitchance_if_low_inaccuracy);
						}

						if (g_cfg.ragebot.double_tap && rage_weapon <= 4)
						{
							ImGui::Checkbox(crypt_str("Double tap hitchance"), &g_cfg.ragebot.weapon[rage_weapon].double_tap_hitchance);

							if (g_cfg.ragebot.weapon[rage_weapon].double_tap_hitchance)
								ImGui::SliderInt(crypt_str("Double tap hitchance amount"), &g_cfg.ragebot.weapon[rage_weapon].double_tap_hitchance_amount, 1, 100);
						}

						ImGui::SliderInt(crypt_str("Minimum visible damage"), &g_cfg.ragebot.weapon[rage_weapon].minimum_visible_damage, 1, 120, true);

						if (g_cfg.ragebot.autowall)
							ImGui::SliderInt(crypt_str("Minimum wall damage"), &g_cfg.ragebot.weapon[rage_weapon].minimum_damage, 1, 120, true);

						ImGui::Text(crypt_str("Damage override "));
						KeyBindButton(&g_cfg.ragebot.weapon[rage_weapon].damage_override_key, crypt_str("damageoverride"));

						if (g_cfg.ragebot.weapon[rage_weapon].damage_override_key.key > KEY_NONE && g_cfg.ragebot.weapon[rage_weapon].damage_override_key.key < KEY_MAX)
							ImGui::SliderInt(crypt_str("Minimum override damage"), &g_cfg.ragebot.weapon[rage_weapon].minimum_override_damage, 1, 120, true);

						if (g_cfg.ragebot.weapon[rage_weapon].hitscan[0])
							ImGui::SliderFloat(crypt_str("Head scale"), &g_cfg.ragebot.weapon[rage_weapon].pointscale_head, 0.0f, 1.0f, true);

						if (g_cfg.ragebot.weapon[rage_weapon].hitscan[1])
							ImGui::SliderFloat(crypt_str("Chest scale"), &g_cfg.ragebot.weapon[rage_weapon].pointscale_chest, 0.0f, 1.0f, true);

						if (g_cfg.ragebot.weapon[rage_weapon].hitscan[2])
							ImGui::SliderFloat(crypt_str("Stomach scale"), &g_cfg.ragebot.weapon[rage_weapon].pointscale_stomach, 0.0f, 1.0f, true);

						if (g_cfg.ragebot.weapon[rage_weapon].hitscan[3])
							ImGui::SliderFloat(crypt_str("Pelvis scale"), &g_cfg.ragebot.weapon[rage_weapon].pointscale_pelvis, 0.0f, 1.0f, true);

						if (g_cfg.ragebot.weapon[rage_weapon].hitscan[5])
							ImGui::SliderFloat(crypt_str("Legs scale"), &g_cfg.ragebot.weapon[rage_weapon].pointscale_legs, 0.0f, 1.0f, true);

						auto enabled_body_hitboxes = false;

						for (auto i = 0; i < ARRAYSIZE(hitboxes); i++)
						{
							if (i < 1 || i > 3)
								continue;

							if (g_cfg.ragebot.weapon[rage_weapon].hitscan[i])
							{
								enabled_body_hitboxes = true;
								break;
							}
						}

						if (enabled_body_hitboxes && (g_cfg.ragebot.weapon[rage_weapon].hitscan[1] && g_cfg.ragebot.weapon[rage_weapon].pointscale_chest || g_cfg.ragebot.weapon[rage_weapon].hitscan[2] && g_cfg.ragebot.weapon[rage_weapon].pointscale_stomach || g_cfg.ragebot.weapon[rage_weapon].hitscan[3] && g_cfg.ragebot.weapon[rage_weapon].pointscale_pelvis))
							ImGui::Checkbox(crypt_str("Safe body points"), &g_cfg.ragebot.weapon[rage_weapon].safe_body_points);

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}
					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
				}

				if (!rage_tab)
				{
					ImGui::BeginGroup();
					{
						ImGui::BeginChild(crypt_str("General"), ImVec2(245, 272), true);
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
							ImGui::BeginGroup();

							ImGui::Checkbox(crypt_str("Enable"), &g_cfg.ragebot.enable);

							if (g_cfg.ragebot.enable)
								g_cfg.legitbot.enabled = false;

							ImGui::SliderInt(crypt_str("Field of view"), &g_cfg.ragebot.field_of_view, 1, 180);
							ImGui::Checkbox(crypt_str("Silent aim"), &g_cfg.ragebot.silent_aim);
							ImGui::Checkbox(crypt_str("Automatic wall"), &g_cfg.ragebot.autowall);
							ImGui::Checkbox(crypt_str("Aimbot with zeus"), &g_cfg.ragebot.zeus_bot);
							ImGui::Checkbox(crypt_str("Aimbot with knife"), &g_cfg.ragebot.knife_bot);
							ImGui::Checkbox(crypt_str("Automatic fire"), &g_cfg.ragebot.autoshoot);
							ImGui::Checkbox(crypt_str("Automatic scope"), &g_cfg.ragebot.autoscope);
							ImGui::Checkbox(crypt_str("Pitch anti-aim correction"), &g_cfg.ragebot.pitch_antiaim_correction);

							ImGui::EndGroup();

							if (backup_tab != tab)
								ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
						}
						ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

						ImGui::BeginChild(crypt_str("Exploits"), ImVec2(245, 200), true);
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
							ImGui::BeginGroup();

							ImGui::Checkbox(crypt_str("Double tap"), &g_cfg.ragebot.double_tap);

							if (g_cfg.ragebot.double_tap)
							{
								ImGui::Checkbox(crypt_str("Instant"), &g_cfg.ragebot.instant);

								if (g_cfg.ragebot.instant)
									ImGui::Checkbox(crypt_str("Slow teleport"), &g_cfg.ragebot.slow_teleport);

								ImGui::Text(crypt_str("Double tap key"));
								KeyBindButton(&g_cfg.ragebot.double_tap_key, crypt_str("doubletap"));
							}

							ImGui::Checkbox(crypt_str("Hide shots"), &g_cfg.antiaim.hide_shots);

							if (g_cfg.antiaim.hide_shots)
							{
								ImGui::Text(crypt_str("Hide shots key"));
								KeyBindButton(&g_cfg.antiaim.hide_shots_key, crypt_str("hideshots"));
							}

							ImGui::EndGroup();

							if (backup_tab != tab)
								ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
						}
						ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
					}
					ImGui::EndGroup();

					ImGui::SameLine();
					ImGui::BeginGroup();

					ImGui::BeginChild(crypt_str("Anti-aim"), ImVec2(245, 482), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
						ImGui::BeginGroup();

						static auto type = 0;

						ImGui::Checkbox(crypt_str("Enable"), &g_cfg.antiaim.enable);
						ImGui::Combo(crypt_str("Anti-aim type"), &g_cfg.antiaim.antiaim_type, antiaim_type, ARRAYSIZE(antiaim_type));

						if (g_cfg.antiaim.antiaim_type)
						{
							ImGui::Combo(crypt_str("Desync"), &g_cfg.antiaim.desync, desync, ARRAYSIZE(desync));

							if (g_cfg.antiaim.desync)
							{
								ImGui::Combo(crypt_str("LBY type"), &g_cfg.antiaim.legit_lby_type, lby_type, ARRAYSIZE(lby_type));

								if (g_cfg.antiaim.desync == 1)
								{
									ImGui::Text(crypt_str("Invert desync "));
									KeyBindButton(&g_cfg.antiaim.flip_desync, crypt_str("invertdesync"));
								}
							}
						}
						else
						{
							ImGui::Combo(crypt_str("Movement type"), &type, movement_type, ARRAYSIZE(movement_type));
							ImGui::Combo(crypt_str("Pitch"), &g_cfg.antiaim.type[type].pitch, pitch, ARRAYSIZE(pitch));
							ImGui::Combo(crypt_str("Yaw"), &g_cfg.antiaim.type[type].yaw, yaw, ARRAYSIZE(yaw));
							ImGui::Combo(crypt_str("Base angle"), &g_cfg.antiaim.type[type].base_angle, baseangle, ARRAYSIZE(baseangle));

							if (g_cfg.antiaim.type[type].yaw)
							{
								ImGui::SliderInt(g_cfg.antiaim.type[type].yaw == 1 ? crypt_str("Jitter range") : crypt_str("Spin range"), &g_cfg.antiaim.type[type].range, 1, 180);

								if (g_cfg.antiaim.type[type].yaw == 2)
									ImGui::SliderInt(crypt_str("Spin speed"), &g_cfg.antiaim.type[type].speed, 1, 15);
							}

							ImGui::Combo(crypt_str("Desync"), &g_cfg.antiaim.type[type].desync, desync, ARRAYSIZE(desync));

							if (g_cfg.antiaim.type[type].desync)
							{
								if (type == ANTIAIM_STAND)
									ImGui::Combo(crypt_str("LBY type"), &g_cfg.antiaim.lby_type, lby_type, ARRAYSIZE(lby_type));

								if (type != ANTIAIM_STAND || !g_cfg.antiaim.lby_type)
								{
									ImGui::SliderInt(crypt_str("Body lean"), &g_cfg.antiaim.type[type].body_lean, 0, 100);
									ImGui::SliderInt(crypt_str("Inverted body lean"), &g_cfg.antiaim.type[type].inverted_body_lean, 0, 100);
								}

								if (g_cfg.antiaim.type[type].desync == 1)
								{
									ImGui::Text(crypt_str("Invert desync "));
									KeyBindButton(&g_cfg.antiaim.flip_desync, crypt_str("invertdesync"));
								}
							}

							ImGui::Text(crypt_str("Manual back "));
							KeyBindButton(&g_cfg.antiaim.manual_back, crypt_str("manualback"));

							ImGui::Text(crypt_str("Manual left "));
							KeyBindButton(&g_cfg.antiaim.manual_left, crypt_str("manualleft"));

							ImGui::Text(crypt_str("Manual right "));
							KeyBindButton(&g_cfg.antiaim.manual_right, crypt_str("manualright"));

							if (g_cfg.antiaim.manual_back.key > KEY_NONE && g_cfg.antiaim.manual_back.key < KEY_MAX || g_cfg.antiaim.manual_left.key > KEY_NONE && g_cfg.antiaim.manual_left.key < KEY_MAX || g_cfg.antiaim.manual_right.key > KEY_NONE && g_cfg.antiaim.manual_right.key < KEY_MAX)
							{
								ImGui::Checkbox(crypt_str("Manuals indicator"), &g_cfg.antiaim.flip_indicator);
								ImGui::ColorEdit4(crypt_str("invc"), &g_cfg.antiaim.flip_indicator_color, ImGuiColorEditFlags_NoInputs);
							}
						}

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}

					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
					ImGui::EndGroup();
				}
			}
			else if (tab == 2)
			{
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 8);

				ImGui::BeginGroup();
				{
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 10.f));

					ImGui::PushFont(Tabs);

					if (ImGui::SemiTabRage(crypt_str("E"), ImVec2(100, 20), legit_weapon == 0))
						legit_weapon = 0;

					ImGui::SameLine();

					if (ImGui::SemiTabRage(crypt_str("W"), ImVec2(100, 20), legit_weapon == 1))
						legit_weapon = 1;

					ImGui::SameLine();

					if (ImGui::SemiTabRage(crypt_str("K"), ImVec2(99, 20), legit_weapon == 2))
						legit_weapon = 2;

					ImGui::SameLine();

					if (ImGui::SemiTabRage(crypt_str("Z"), ImVec2(100, 20), legit_weapon == 3))
						legit_weapon = 3;

					ImGui::SameLine();

					if (ImGui::SemiTabRage(crypt_str("b"), ImVec2(100, 20), legit_weapon == 4))
						legit_weapon = 4;

					ImGui::PopFont();
					ImGui::PopStyleVar();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}
				ImGui::EndGroup();

				ImGui::BeginChild(crypt_str("Main"), ImVec2(245, 482), true);
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
					ImGui::BeginGroup();

					ImGui::Checkbox(crypt_str("Enable"), &g_cfg.legitbot.enabled);

					if (g_cfg.legitbot.enabled)
						g_cfg.ragebot.enable = false;

					if (legit_weapon == 3)
						ImGui::Checkbox(crypt_str("Only in zoom"), &g_cfg.legitbot.only_in_zoom);

					ImGui::Combo(crypt_str("Field of view type"), &g_cfg.legitbot.weapon[legit_weapon].fov_type, LegitFov, ARRAYSIZE(LegitFov));
					ImGui::SliderFloat(crypt_str("Aimbot field of view"), &g_cfg.legitbot.weapon[legit_weapon].fov, 1.0f, 30.0f);

					ImGui::Combo(crypt_str("Aimbot type"), &g_cfg.legitbot.weapon[legit_weapon].aim_type, AimType, ARRAYSIZE(AimType));
					ImGui::Combo(crypt_str("Hitbox"), &g_cfg.legitbot.weapon[legit_weapon].hitbox, LegitHitbox, ARRAYSIZE(LegitHitbox));
					ImGui::Combo(crypt_str("Selection type"), &g_cfg.legitbot.weapon[legit_weapon].priority, LegitSelection, ARRAYSIZE(LegitSelection));

					ImGui::Checkbox(crypt_str("Check teammates"), &g_cfg.legitbot.deathmatch);

					if (!legit_weapon)
						ImGui::Checkbox(crypt_str("Automatic pistol"), &g_cfg.legitbot.autopistol);

					ImGui::Checkbox(crypt_str("Automatic penetration"), &g_cfg.legitbot.autowall);

					if (g_cfg.legitbot.autowall)
						ImGui::SliderInt(crypt_str("Penetration damage"), &g_cfg.legitbot.weapon[legit_weapon].min_damage, 1, 100);

					ImGui::Checkbox(crypt_str("Silent aim"), &g_cfg.legitbot.silent);

					if (g_cfg.legitbot.silent)
						ImGui::SliderFloat(crypt_str("Silent field of view"), &g_cfg.legitbot.weapon[legit_weapon].silent_fov, 1.0f, 30.0f);

					ImGui::Checkbox(crypt_str("Automatic stop"), &g_cfg.legitbot.autostop);
					ImGui::Checkbox(crypt_str("Aimbot on key"), &g_cfg.legitbot.on_key);

					if (g_cfg.legitbot.on_key)
					{
						ImGui::Text(crypt_str("Aimbot "));
						KeyBindButton(&g_cfg.legitbot.key, crypt_str("aimbot"));
					}

					ImGui::Checkbox(crypt_str("Automatic fire on key"), &g_cfg.legitbot.autofire);

					if (g_cfg.legitbot.autofire)
					{
						ImGui::Text(crypt_str("Automatic fire "));
						KeyBindButton(&g_cfg.legitbot.autofire_key, crypt_str("autofire"));
					}

					ImGui::Checkbox(crypt_str("Smoke check"), &g_cfg.legitbot.weapon[legit_weapon].smoke_check);
					ImGui::Checkbox(crypt_str("Flash check "), &g_cfg.legitbot.weapon[legit_weapon].flash_check);
					ImGui::Checkbox(crypt_str("Jump check"), &g_cfg.legitbot.weapon[legit_weapon].jump_check);

					ImGui::EndGroup();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}
				ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

				ImGui::SameLine();

				ImGui::BeginChild(crypt_str("Accuracy"), ImVec2(245, 482), true);
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
					ImGui::BeginGroup();

					ImGui::Combo(crypt_str("Smooth type"), &g_cfg.legitbot.weapon[legit_weapon].smooth_type, LegitSmooth, ARRAYSIZE(LegitSmooth));
					ImGui::SliderFloat(crypt_str("Smooth"), &g_cfg.legitbot.weapon[legit_weapon].smooth, 1.0f, 20.0f);

					if (legit_weapon != 3)
					{
						ImGui::Checkbox(crypt_str("RCS"), &g_cfg.legitbot.weapon[legit_weapon].rcs);

						if (g_cfg.legitbot.weapon[legit_weapon].rcs)
						{
							ImGui::Combo(crypt_str("RCS type"), &g_cfg.legitbot.weapon[legit_weapon].rcs_type, RCSType, ARRAYSIZE(RCSType));
							ImGui::Checkbox(crypt_str("RCS custom field of view"), &g_cfg.legitbot.weapon[legit_weapon].rcs_fov_enabled);

							if (g_cfg.legitbot.weapon[legit_weapon].rcs_fov_enabled)
								ImGui::SliderFloat(crypt_str("Custom field of view"), &g_cfg.legitbot.weapon[legit_weapon].rcs_fov, 1.0f, 30.0f);

							ImGui::Checkbox(crypt_str("RCS custom smooth"), &g_cfg.legitbot.weapon[legit_weapon].rcs_smooth_enabled);

							if (g_cfg.legitbot.weapon[legit_weapon].rcs_smooth_enabled)
								ImGui::SliderFloat(crypt_str("Custom smooth"), &g_cfg.legitbot.weapon[legit_weapon].rcs_smooth, 1, 15);

							ImGui::SliderInt(crypt_str("RCS pitch"), &g_cfg.legitbot.weapon[legit_weapon].rcs_x, 0, 100);
							ImGui::SliderInt(crypt_str("RCS yaw"), &g_cfg.legitbot.weapon[legit_weapon].rcs_y, 0, 100);
							ImGui::SliderInt(crypt_str("RCS start"), &g_cfg.legitbot.weapon[legit_weapon].rcs_start, 1, 30);
						}
					}

					ImGui::SliderInt(crypt_str("Shot delay"), &g_cfg.legitbot.weapon[legit_weapon].shot_delay, 0, 100);
					ImGui::SliderInt(crypt_str("Kill delay"), &g_cfg.legitbot.weapon[legit_weapon].kill_delay, 0, 1000);

					ImGui::EndGroup();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}
				ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
			}
			else if (tab == 3)
			{
				static auto player = 0;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 8);

				ImGui::BeginGroup();
				{
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 10.f));
					ImGui::PushFont(TabsText);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 22);
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
					ImGui::BeginGroup();

					if (ImGui::SemiTab(crypt_str("enemy"), ImVec2(135, 20), player == 0))
						player = 0;

					ImGui::SameLine();

					if (ImGui::SemiTab(crypt_str("team"), ImVec2(135, 20), player == 1))
						player = 1;

					ImGui::SameLine();

					if (ImGui::SemiTab(crypt_str("local"), ImVec2(135, 20), player == 2))
						player = 2;

					ImGui::SameLine();

					if (ImGui::SemiTab(crypt_str("player list"), ImVec2(136, 20), player == 3))
						player = 3;

					ImGui::EndGroup();
					ImGui::PopFont();
					ImGui::PopStyleVar();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}
				ImGui::EndGroup();

				if (player < 3)
				{
					ImGui::BeginChild(crypt_str("ESP"), ImVec2(245, 482), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);

						ImGui::BeginGroup();
						ImGui::Checkbox(crypt_str("Enable"), &g_cfg.player.enable);

						if (player == ENEMY)
						{
							ImGui::Checkbox(crypt_str("OOF arrows"), &g_cfg.player.arrows);
							ImGui::ColorEdit4(crypt_str("arrowscolor"), &g_cfg.player.arrows_color, ImGuiColorEditFlags_NoInputs);

							if (g_cfg.player.arrows)
							{
								ImGui::SliderInt(crypt_str("Arrows distance"), &g_cfg.player.distance, 1, 100);
								ImGui::SliderInt(crypt_str("Arrows size"), &g_cfg.player.size, 1, 100);
							}
						}

						ImGui::Checkbox(crypt_str("Bounding box"), &g_cfg.player.type[player].box);
						ImGui::ColorEdit4(crypt_str("boxcolor"), &g_cfg.player.type[player].box_color, ImGuiColorEditFlags_NoInputs);

						ImGui::Checkbox(crypt_str("Name"), &g_cfg.player.type[player].name);
						ImGui::ColorEdit4(crypt_str("namecolor"), &g_cfg.player.type[player].name_color, ImGuiColorEditFlags_NoInputs);

						ImGui::Checkbox(crypt_str("Health bar"), &g_cfg.player.type[player].health);
						ImGui::Checkbox(crypt_str("Health color"), &g_cfg.player.type[player].custom_health_color);
						ImGui::ColorEdit4(crypt_str("healthcolor"), &g_cfg.player.type[player].health_color, ImGuiColorEditFlags_NoInputs);

						for (auto i = 0, j = 0; i < ARRAYSIZE(flags); i++)
						{
							if (g_cfg.player.type[player].flags[i])
							{
								if (j)
									preview += crypt_str(", ") + (std::string)flags[i];
								else
									preview = flags[i];

								j++;
							}
						}

						if (ImGui::BeginCombo(crypt_str("Flags"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(flags))))
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

							ImGui::BeginGroup();
							{
								ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

								for (auto i = 0; i < ARRAYSIZE(flags); i++)
									ImGui::Selectable(flags[i], (bool*)&g_cfg.player.type[player].flags[i], ARRAYSIZE(flags) - i == 1, ImGuiSelectableFlags_DontClosePopups);

								ImGui::PopStyleVar();
							}

							ImGui::EndGroup();
							ImGui::EndCombo();
						}

						preview = crypt_str("None");

						for (auto i = 0, j = 0; i < ARRAYSIZE(weaponplayer); i++)
						{
							if (g_cfg.player.type[player].weapon[i])
							{
								if (j)
									preview += crypt_str(", ") + (std::string)weaponplayer[i];
								else
									preview = weaponplayer[i];

								j++;
							}
						}

						if (ImGui::BeginCombo(crypt_str("Weapon"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(weaponplayer))))
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

							ImGui::BeginGroup();
							{
								ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

								for (auto i = 0; i < ARRAYSIZE(weaponplayer); i++)
									ImGui::Selectable(weaponplayer[i], (bool*)&g_cfg.player.type[player].weapon[i], ARRAYSIZE(weaponplayer) - i == 1, ImGuiSelectableFlags_DontClosePopups);

								ImGui::PopStyleVar();
							}

							ImGui::EndGroup();
							ImGui::EndCombo();
						}

						preview = crypt_str("None");

						if (g_cfg.player.type[player].weapon[WEAPON_ICON] || g_cfg.player.type[player].weapon[WEAPON_TEXT])
						{
							ImGui::Text(crypt_str("Color "));
							ImGui::ColorEdit4(crypt_str("weapcolor"), &g_cfg.player.type[player].weapon_color, ImGuiColorEditFlags_NoInputs);
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
						}

						ImGui::Checkbox(crypt_str("Skeleton"), &g_cfg.player.type[player].skeleton);
						ImGui::ColorEdit4(crypt_str("skeletoncolor"), &g_cfg.player.type[player].skeleton_color, ImGuiColorEditFlags_NoInputs);

						ImGui::Checkbox(crypt_str("Ammo bar"), &g_cfg.player.type[player].ammo);
						ImGui::ColorEdit4(crypt_str("ammocolor"), &g_cfg.player.type[player].ammobar_color, ImGuiColorEditFlags_NoInputs);

						ImGui::Checkbox(crypt_str("Footsteps"), &g_cfg.player.type[player].footsteps);
						ImGui::ColorEdit4(crypt_str("footstepscolor"), &g_cfg.player.type[player].footsteps_color, ImGuiColorEditFlags_NoInputs);

						if (g_cfg.player.type[player].footsteps)
						{
							ImGui::SliderInt(crypt_str("Thickness"), &g_cfg.player.type[player].thickness, 1, 10);
							ImGui::SliderInt(crypt_str("Radius"), &g_cfg.player.type[player].radius, 50, 500);
						}

						if (player == ENEMY || player == TEAM)
						{
							ImGui::Checkbox(crypt_str("Snap lines"), &g_cfg.player.type[player].snap_lines);
							ImGui::ColorEdit4(crypt_str("snapcolor"), &g_cfg.player.type[player].snap_lines_color, ImGuiColorEditFlags_NoInputs);

							if (player == ENEMY)
							{
								if (g_cfg.ragebot.enable)
								{
									ImGui::Checkbox(crypt_str("Aimbot points"), &g_cfg.player.show_multi_points);
									ImGui::ColorEdit4(crypt_str("showmultipointscolor"), &g_cfg.player.show_multi_points_color, ImGuiColorEditFlags_NoInputs);
								}

								ImGui::Checkbox(crypt_str("Aimbot hitboxes"), &g_cfg.player.lag_hitbox);
								ImGui::ColorEdit4(crypt_str("lagcompcolor"), &g_cfg.player.lag_hitbox_color, ImGuiColorEditFlags_NoInputs);
							}
						}
						else
						{
							ImGui::Checkbox(crypt_str("Hold firing animation"), &g_cfg.misc.hold_firing_animation);
							ImGui::Combo(crypt_str("Player model T"), &g_cfg.player.player_model_t, player_model_t, ARRAYSIZE(player_model_t));
							ImGui::Combo(crypt_str("Player model CT"), &g_cfg.player.player_model_ct, player_model_ct, ARRAYSIZE(player_model_ct));
						}

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}

					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
					ImGui::SameLine();
					ImGui::BeginGroup();

					ImGui::BeginChild(crypt_str("Chams"), ImVec2(245, 362), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);

						ImGui::BeginGroup();
						{
							if (player == LOCAL)
								ImGui::Combo(crypt_str("Type"), &g_cfg.player.local_chams_type, local_chams_type, ARRAYSIZE(local_chams_type));

							if (player != LOCAL || !g_cfg.player.local_chams_type)
							{
								for (auto i = 0, j = 0; i < ARRAYSIZE(chamsvis); i++)
								{
									if (g_cfg.player.type[player].chams[i] && (i != PLAYER_CHAMS_INVISIBLE || g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE]))
									{
										if (j)
											preview += crypt_str(", ") + (std::string)chamsvis[i];
										else
											preview = chamsvis[i];

										j++;
									}
								}

								if (ImGui::BeginCombo(crypt_str("Chams"), preview.c_str(), ImVec2(175, 2 + 20 * (g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] ? ARRAYSIZE(chamsvis) : ARRAYSIZE(chamsvis) - 1))))
								{
									ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
									ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

									ImGui::BeginGroup();
									{
										ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

										for (auto i = 0; i < ARRAYSIZE(chamsvis); i++)
											if (i != PLAYER_CHAMS_INVISIBLE || g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE])
												ImGui::Selectable(chamsvis[i], (bool*)&g_cfg.player.type[player].chams[i], ((g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] ? ARRAYSIZE(chamsvis) : ARRAYSIZE(chamsvis) - 1) - i == 1), ImGuiSelectableFlags_DontClosePopups);

										ImGui::PopStyleVar();
									}
									ImGui::EndGroup();

									ImGui::EndCombo();
								}
							}

							preview = crypt_str("None");

							if (g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] || player == LOCAL && g_cfg.player.local_chams_type) //-V648
							{
								if (player == LOCAL && g_cfg.player.local_chams_type)
								{
									ImGui::Checkbox(crypt_str("Enable desync chams"), &g_cfg.player.fake_chams_enable);
									ImGui::Checkbox(crypt_str("Visualize lag"), &g_cfg.player.visualize_lag);
									ImGui::Checkbox(crypt_str("Layered"), &g_cfg.player.layered);

									ImGui::Combo(crypt_str("Player chams material"), &g_cfg.player.fake_chams_type, chamstype, ARRAYSIZE(chamstype));

									ImGui::Text(crypt_str("Color "));
									ImGui::ColorEdit4(crypt_str("fakechamscolor"), &g_cfg.player.fake_chams_color, ImGuiColorEditFlags_NoInputs);
									ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);

									if (g_cfg.player.fake_chams_type != 6)
									{
										ImGui::Checkbox(crypt_str("Double material "), &g_cfg.player.fake_double_material);
										ImGui::ColorEdit4(crypt_str("doublematerialcolor"), &g_cfg.player.fake_double_material_color, ImGuiColorEditFlags_NoInputs);
									}

									ImGui::Checkbox(crypt_str("Animated material"), &g_cfg.player.fake_animated_material);
									ImGui::ColorEdit4(crypt_str("animcolormat"), &g_cfg.player.fake_animated_material_color, ImGuiColorEditFlags_NoInputs);
								}
								else
								{
									ImGui::Combo(crypt_str("Player chams material"), &g_cfg.player.type[player].chams_type, chamstype, ARRAYSIZE(chamstype));

									if (g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE])
									{
										ImGui::Text(crypt_str("Visible "));
										ImGui::ColorEdit4(crypt_str("chamsvisible"), &g_cfg.player.type[player].chams_color, ImGuiColorEditFlags_NoInputs);

										if (!g_cfg.player.type[player].chams[PLAYER_CHAMS_INVISIBLE])
											ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
									}

									if (g_cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] && g_cfg.player.type[player].chams[PLAYER_CHAMS_INVISIBLE])
									{
										ImGui::Text(crypt_str("Invisible "));
										ImGui::ColorEdit4(crypt_str("chamsinvisible"), &g_cfg.player.type[player].xqz_color, ImGuiColorEditFlags_NoInputs);
										ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
									}

									if (g_cfg.player.type[player].chams_type != 6)
									{
										ImGui::Checkbox(crypt_str("Double material "), &g_cfg.player.type[player].double_material);
										ImGui::ColorEdit4(crypt_str("doublematerialcolor"), &g_cfg.player.type[player].double_material_color, ImGuiColorEditFlags_NoInputs);
									}

									ImGui::Checkbox(crypt_str("Animated material"), &g_cfg.player.type[player].animated_material);
									ImGui::ColorEdit4(crypt_str("animcolormat"), &g_cfg.player.type[player].animated_material_color, ImGuiColorEditFlags_NoInputs);

									if (player == ENEMY)
									{
										ImGui::Checkbox(crypt_str("Backtrack chams"), &g_cfg.player.backtrack_chams);

										if (g_cfg.player.backtrack_chams)
										{
											ImGui::Combo(crypt_str("Backtrack chams material"), &g_cfg.player.backtrack_chams_material, chamstype, ARRAYSIZE(chamstype));

											ImGui::Text(crypt_str("Color "));
											ImGui::ColorEdit4(crypt_str("backtrackcolor"), &g_cfg.player.backtrack_chams_color, ImGuiColorEditFlags_NoInputs);
										}
									}
								}
							}

							if (player == ENEMY || player == TEAM)
							{
								ImGui::Checkbox(crypt_str("Ragdoll chams"), &g_cfg.player.type[player].ragdoll_chams);

								if (g_cfg.player.type[player].ragdoll_chams)
								{
									ImGui::Combo(crypt_str("Ragdoll chams material"), &g_cfg.player.type[player].ragdoll_chams_material, chamstype, ARRAYSIZE(chamstype));

									ImGui::Text(crypt_str("Color "));
									ImGui::ColorEdit4(crypt_str("ragdollcolor"), &g_cfg.player.type[player].ragdoll_chams_color, ImGuiColorEditFlags_NoInputs);
								}
							}
							else if (!g_cfg.player.local_chams_type)
							{
								ImGui::Checkbox(crypt_str("Transparency in scope"), &g_cfg.player.transparency_in_scope);

								if (g_cfg.player.transparency_in_scope)
									ImGui::SliderFloat(crypt_str("Alpha"), &g_cfg.player.transparency_in_scope_amount, 0.0f, 1.0f);
							}
						}

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}

					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

					ImGui::BeginChild(crypt_str("Glow"), ImVec2(245, 110), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
						ImGui::BeginGroup();

						ImGui::Checkbox(crypt_str("Glow"), &g_cfg.player.type[player].glow);

						if (g_cfg.player.type[player].glow)
						{
							ImGui::Combo(crypt_str("Glow type"), &g_cfg.player.type[player].glow_type, glowtype, ARRAYSIZE(glowtype));
							ImGui::Text(crypt_str("Color "));
							ImGui::ColorEdit4(crypt_str("glowcolor"), &g_cfg.player.type[player].glow_color, ImGuiColorEditFlags_NoInputs);
						}

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}

					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
				}
				else
				{
					static std::vector <Player_list_data> players;

					if (!g_cfg.player_list.refreshing)
					{
						players.clear();

						for (auto player : g_cfg.player_list.players)
							players.emplace_back(player);
					}

					static auto current_player = 0;

					ImGui::BeginChild(crypt_str("Players"), ImVec2(245, 482), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);

						ImGui::BeginGroup();

						if (!players.empty())
						{
							std::vector <std::string> player_names;

							for (auto player : players)
								player_names.emplace_back(player.name);

							ImGui::PushItemWidth(229);
							ImGui::ListBoxConfigArray(crypt_str("##PLAYERLIST"), &current_player, player_names, (backup_tab != tab) ? alpha : 0, 20);
							ImGui::PopItemWidth();
						}

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}
					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

					ImGui::SameLine();
					ImGui::BeginGroup();

					ImGui::BeginChild(crypt_str("Settings"), ImVec2(245, 482), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
						ImGui::BeginGroup();

						if (!players.empty())
						{
							if (current_player >= players.size())
								current_player = players.size() - 1;

							ImGui::Checkbox(crypt_str("White list"), &g_cfg.player_list.white_list[players.at(current_player).i]);

							if (!g_cfg.player_list.white_list[players.at(current_player).i])
							{
								ImGui::Checkbox(crypt_str("High priority"), &g_cfg.player_list.high_priority[players.at(current_player).i]);
								ImGui::Checkbox(crypt_str("Force body aim"), &g_cfg.player_list.force_body_aim[players.at(current_player).i]);
							}
						}

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}
					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
				}

				ImGui::EndGroup();
			}
			else if (tab == 4)
			{
				static auto vis_tab = 0;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 8);

				ImGui::BeginGroup();
				{
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 10.f));
					ImGui::PushFont(TabsText);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 22);
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
					ImGui::BeginGroup();

					if (ImGui::SemiTab(crypt_str("world"), ImVec2(248 + 22, 20), vis_tab == 0 ? true : false))
						vis_tab = 0;

					ImGui::SameLine();

					if (ImGui::SemiTab(crypt_str("viewmodel"), ImVec2(249 + 22, 20), vis_tab == 1 ? true : false))
						vis_tab = 1;

					ImGui::EndGroup();
					ImGui::PopFont();
					ImGui::PopStyleVar();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}
				ImGui::EndGroup();

				if (!vis_tab)
				{
					ImGui::BeginChild(crypt_str("General"), ImVec2(245, 482), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
						ImGui::BeginGroup();

						ImGui::Checkbox(crypt_str("Enable"), &g_cfg.player.enable);

						for (auto i = 0, j = 0; i < ARRAYSIZE(indicators); i++)
						{
							if (g_cfg.esp.indicators[i])
							{
								if (j)
									preview += crypt_str(", ") + (std::string)indicators[i];
								else
									preview = indicators[i];

								j++;
							}
						}

						if (ImGui::BeginCombo(crypt_str("Indicators"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(indicators))))
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

							ImGui::BeginGroup();
							{
								ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

								for (auto i = 0; i < ARRAYSIZE(indicators); i++)
									ImGui::Selectable(indicators[i], (bool*)&g_cfg.esp.indicators[i], ARRAYSIZE(indicators) - i == 1, ImGuiSelectableFlags_DontClosePopups);

								ImGui::PopStyleVar();
							}
							ImGui::EndGroup();

							ImGui::EndCombo();
						}

						preview = crypt_str("None");

						for (auto i = 0, j = 0; i < ARRAYSIZE(removals); i++)
						{
							if (g_cfg.esp.removals[i])
							{
								if (j)
									preview += crypt_str(", ") + (std::string)removals[i];
								else
									preview = removals[i];

								j++;
							}
						}

						if (ImGui::BeginCombo(crypt_str("Removals"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(removals))))
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

							ImGui::BeginGroup();
							{
								ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

								for (auto i = 0; i < ARRAYSIZE(removals); i++)
									ImGui::Selectable(removals[i], (bool*)&g_cfg.esp.removals[i], ARRAYSIZE(removals) - i == 1, ImGuiSelectableFlags_DontClosePopups);

								ImGui::PopStyleVar();
							}
							ImGui::EndGroup();

							ImGui::EndCombo();
						}

						preview = crypt_str("None");

						if (g_cfg.esp.removals[REMOVALS_ZOOM])
							ImGui::Checkbox(crypt_str("Fix zoom sensivity"), &g_cfg.esp.fix_zoom_sensivity);

						ImGui::Checkbox(crypt_str("Grenade prediction"), &g_cfg.esp.grenade_prediction);
						ImGui::ColorEdit4(crypt_str("grenpredcolor"), &g_cfg.esp.grenade_prediction_color, ImGuiColorEditFlags_NoInputs);

						if (g_cfg.esp.grenade_prediction)
						{
							ImGui::Text(crypt_str("Tracer color "));
							ImGui::ColorEdit4(crypt_str("tracergrenpredcolor"), &g_cfg.esp.grenade_prediction_tracer_color, ImGuiColorEditFlags_NoInputs);
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
						}

						ImGui::Checkbox(crypt_str("Grenade projectiles"), &g_cfg.esp.projectiles);

						if (g_cfg.esp.projectiles)
						{
							for (auto i = 0, j = 0; i < ARRAYSIZE(proj_combo); i++)
							{
								if (g_cfg.esp.grenade_esp[i])
								{
									if (j)
										preview += crypt_str(", ") + (std::string)proj_combo[i];
									else
										preview = proj_combo[i];

									j++;
								}
							}

							if (ImGui::BeginCombo(crypt_str("Grenade ESP"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(proj_combo))))
							{
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
								ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

								ImGui::BeginGroup();
								{
									ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

									for (auto i = 0; i < ARRAYSIZE(proj_combo); i++)
										ImGui::Selectable(proj_combo[i], (bool*)&g_cfg.esp.grenade_esp[i], (ARRAYSIZE(proj_combo) - i == 1) ? true : false, ImGuiSelectableFlags_DontClosePopups);

									ImGui::PopStyleVar();
								}
								ImGui::EndGroup();

								ImGui::EndCombo();
							}

							preview = crypt_str("None");

							if (g_cfg.esp.grenade_esp[GRENADE_ICON] || g_cfg.esp.grenade_esp[GRENADE_TEXT])
							{
								ImGui::Text(crypt_str("Color "));
								ImGui::ColorEdit4(crypt_str("projectcolor"), &g_cfg.esp.projectiles_color, ImGuiColorEditFlags_NoInputs);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
							}

							if (g_cfg.esp.grenade_esp[GRENADE_BOX])
							{
								ImGui::Text(crypt_str("Box color "));
								ImGui::ColorEdit4(crypt_str("grenade_box_color"), &g_cfg.esp.grenade_box_color, ImGuiColorEditFlags_NoInputs);
							}

							if (g_cfg.esp.grenade_esp[GRENADE_GLOW])
							{
								ImGui::Text(crypt_str("Glow color "));
								ImGui::ColorEdit4(crypt_str("grenade_glow_color"), &g_cfg.esp.grenade_glow_color, ImGuiColorEditFlags_NoInputs);
							}
						}

						ImGui::Checkbox(crypt_str("Fire timer"), &g_cfg.esp.molotov_timer);
						ImGui::ColorEdit4(crypt_str("molotovcolor"), &g_cfg.esp.molotov_timer_color, ImGuiColorEditFlags_NoInputs);

						ImGui::Checkbox(crypt_str("Bomb indicator"), &g_cfg.esp.bomb_timer);

						for (auto i = 0, j = 0; i < ARRAYSIZE(weaponesp); i++)
						{
							if (g_cfg.esp.weapon[i])
							{
								if (j)
									preview += crypt_str(", ") + (std::string)weaponesp[i];
								else
									preview = weaponesp[i];

								j++;
							}
						}

						if (ImGui::BeginCombo(crypt_str("Weapon ESP"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(weaponesp))))
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

							ImGui::BeginGroup();
							{
								ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

								for (auto i = 0; i < ARRAYSIZE(weaponesp); i++)
									ImGui::Selectable(weaponesp[i], (bool*)&g_cfg.esp.weapon[i], ARRAYSIZE(weaponesp) - i == 1, ImGuiSelectableFlags_DontClosePopups);

								ImGui::PopStyleVar();
							}
							ImGui::EndGroup();

							ImGui::EndCombo();
						}

						preview = crypt_str("None");

						if (g_cfg.esp.weapon[WEAPON_ICON] || g_cfg.esp.weapon[WEAPON_TEXT] || g_cfg.esp.weapon[WEAPON_DISTANCE])
						{
							ImGui::Text(crypt_str("Color "));
							ImGui::ColorEdit4(crypt_str("weaponcolor"), &g_cfg.esp.weapon_color, ImGuiColorEditFlags_NoInputs);
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
						}

						if (g_cfg.esp.weapon[WEAPON_BOX])
						{
							ImGui::Text(crypt_str("Box color "));
							ImGui::ColorEdit4(crypt_str("weaponboxcolor"), &g_cfg.esp.box_color, ImGuiColorEditFlags_NoInputs);
						}

						if (g_cfg.esp.weapon[WEAPON_GLOW])
						{
							ImGui::Text(crypt_str("Glow color "));
							ImGui::ColorEdit4(crypt_str("weaponglowcolor"), &g_cfg.esp.weapon_glow_color, ImGuiColorEditFlags_NoInputs);
						}

						if (g_cfg.esp.weapon[WEAPON_AMMO])
						{
							ImGui::Text(crypt_str("Ammo bar color "));
							ImGui::ColorEdit4(crypt_str("weaponammocolor"), &g_cfg.esp.weapon_ammo_color, ImGuiColorEditFlags_NoInputs);
						}

						ImGui::Checkbox(crypt_str("Client bullet impacts"), &g_cfg.esp.client_bullet_impacts);
						ImGui::ColorEdit4(crypt_str("clientbulletimpacts"), &g_cfg.esp.client_bullet_impacts_color, ImGuiColorEditFlags_NoInputs);

						ImGui::Checkbox(crypt_str("Server bullet impacts"), &g_cfg.esp.server_bullet_impacts);
						ImGui::ColorEdit4(crypt_str("serverbulletimpacts"), &g_cfg.esp.server_bullet_impacts_color, ImGuiColorEditFlags_NoInputs);

						ImGui::Checkbox(crypt_str("Local bullet tracers"), &g_cfg.esp.bullet_tracer);
						ImGui::ColorEdit4(crypt_str("bulltracecolor"), &g_cfg.esp.bullet_tracer_color, ImGuiColorEditFlags_NoInputs);

						ImGui::Checkbox(crypt_str("Enemy bullet tracers"), &g_cfg.esp.enemy_bullet_tracer);
						ImGui::ColorEdit4(crypt_str("enemybulltracecolor"), &g_cfg.esp.enemy_bullet_tracer_color, ImGuiColorEditFlags_NoInputs);

						ImGui::Checkbox(crypt_str("Hitmarker"), &g_cfg.esp.hitmarker);
						ImGui::Checkbox(crypt_str("Damage marker"), &g_cfg.esp.damage_marker);
						ImGui::Checkbox(crypt_str("Kill effect"), &g_cfg.esp.kill_effect);

						if (g_cfg.esp.kill_effect)
							ImGui::SliderFloat(crypt_str("Duration"), &g_cfg.esp.kill_effect_duration, 0.01f, 3.0f);

						ImGui::Text(crypt_str("Thirdperson "));
						KeyBindButton(&g_cfg.misc.thirdperson_toggle, crypt_str("thirdperson"));

						ImGui::Checkbox(crypt_str("Thirdperson when spectating"), &g_cfg.misc.thirdperson_when_spectating);

						if (g_cfg.misc.thirdperson_toggle.key > KEY_NONE && g_cfg.misc.thirdperson_toggle.key < KEY_MAX)
							ImGui::SliderInt(crypt_str("Thirdperson distance"), &g_cfg.misc.thirdperson_distance, 100, 300);

						ImGui::SliderInt(crypt_str("Field of view"), &g_cfg.esp.fov, 0, 89);
						ImGui::Checkbox(crypt_str("Taser range"), &g_cfg.esp.taser_range);
						ImGui::Checkbox(crypt_str("Show spread"), &g_cfg.esp.show_spread);
						ImGui::ColorEdit4(crypt_str("spredcolor"), &g_cfg.esp.show_spread_color, ImGuiColorEditFlags_NoInputs);
						ImGui::Checkbox(crypt_str("Penetration crosshair"), &g_cfg.esp.penetration_reticle);

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));

					} ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

					ImGui::SameLine();

					ImGui::BeginGroup();
					{
						ImGui::BeginChild(crypt_str("World changer"), ImVec2(245, 178), true);
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
							ImGui::BeginGroup();

							ImGui::Combo(crypt_str("Skybox"), &g_cfg.esp.skybox, skybox, ARRAYSIZE(skybox));
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2);
							ImGui::Text(crypt_str("Color "));
							ImGui::ColorEdit4(crypt_str("skyboxcolor"), &g_cfg.esp.skybox_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);

							if (g_cfg.esp.skybox == 21)
							{
								static char sky_custom[64] = "\0";

								if (!g_cfg.esp.custom_skybox.empty())
									strcpy_s(sky_custom, sizeof(sky_custom), g_cfg.esp.custom_skybox.c_str());

								ImGui::Text(crypt_str("Name"));
								ImGui::PushItemWidth(229);

								if (ImGui::InputText(crypt_str("##customsky"), sky_custom, sizeof(sky_custom)))
									g_cfg.esp.custom_skybox = sky_custom;

								ImGui::PopItemWidth();
							}

							ImGui::Checkbox(crypt_str("Full bright"), &g_cfg.esp.bright);
							ImGui::Checkbox(crypt_str("Night mode"), &g_cfg.esp.nightmode);

							if (g_cfg.esp.nightmode)
								ImGui::SliderInt(crypt_str("Night mode brightness"), &g_cfg.esp.nightmode_value, 1, 100);

							ImGui::Checkbox(crypt_str("Asus props"), &g_cfg.esp.asus_props);

							if (g_cfg.esp.asus_props)
								ImGui::SliderFloat(crypt_str("Asus amount"), &g_cfg.esp.asus_props_amount, 0.0f, 1.0f);

							ImGui::EndGroup();

							if (backup_tab != tab)
								ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));

						}
						ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

						ImGui::BeginChild(crypt_str("Extra"), ImVec2(245, 146), true);
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
							ImGui::BeginGroup();

							ImGui::Checkbox(crypt_str("World modulation"), &g_cfg.esp.world_modulation);

							if (g_cfg.esp.world_modulation)
							{
								ImGui::SliderFloat(crypt_str("Bloom"), &g_cfg.esp.bloom, 0.0f, 750.0f);
								ImGui::SliderFloat(crypt_str("Exposure"), &g_cfg.esp.exposure, 0.0f, 2000.0f);
								ImGui::SliderFloat(crypt_str("Ambient"), &g_cfg.esp.ambient, 0.0f, 1500.0f);
							}

							ImGui::EndGroup();

							if (backup_tab != tab)
								ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
						}
						ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

						ImGui::BeginChild(crypt_str("Fog"), ImVec2(245, 138), true);
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
							ImGui::BeginGroup();

							ImGui::Checkbox(crypt_str("Fog modulation"), &g_cfg.esp.fog);

							if (g_cfg.esp.fog)
							{
								ImGui::SliderInt(crypt_str("Distance"), &g_cfg.esp.fog_distance, 0, 2500);
								ImGui::SliderInt(crypt_str("Density"), &g_cfg.esp.fog_density, 0, 100);

								ImGui::Text(crypt_str("Color "));
								ImGui::ColorEdit4(crypt_str("fogcolor"), &g_cfg.esp.fog_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
							}

							ImGui::EndGroup();

							if (backup_tab != tab)
								ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
						}

						ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
					}
					ImGui::EndGroup();
				}
				else if (vis_tab == 1)
				{
					ImGui::BeginGroup();
					ImGui::BeginChild(crypt_str("Skins"), ImVec2(245, 431), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
						ImGui::BeginGroup();

						ImGui::Checkbox(crypt_str("Enable"), &g_cfg.skins.enable);
						ImGui::Checkbox(crypt_str("Rare animations"), &g_cfg.skins.rare_animations);

						ImGui::Combo(crypt_str("Weapon"), &itemIndex, [](void* data, int idx, const char** out_text)
							{
								*out_text = game_data::weapon_names[idx].name;
								return true;
							}, nullptr, IM_ARRAYSIZE(game_data::weapon_names));


						auto& selected_entry = g_cfg.skins.skinChanger[itemIndex];
						selected_entry.itemIdIndex = itemIndex;

						if (!itemIndex)
						{
							ImGui::Combo(crypt_str("Knife"), &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text)
								{
									*out_text = game_data::knife_names[idx].name;
									return true;
								}, nullptr, IM_ARRAYSIZE(game_data::knife_names));
						}
						else if (itemIndex == 1)
						{
							ImGui::Combo(crypt_str("Glove"), &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text)
								{
									*out_text = game_data::glove_names[idx].name;
									return true;
								}, nullptr, IM_ARRAYSIZE(game_data::glove_names));
						}
						else
							selected_entry.definition_override_vector_index = 0;

						ImGui::Text(crypt_str("Search"));

						static char search_skins[64] = "\0";
						static auto item_index = selected_entry.paint_kit_vector_index;

						ImGui::PushItemWidth(229);

						if (ImGui::InputText(crypt_str("##search"), search_skins, sizeof(search_skins)))
							item_index = -1;

						ImGui::PopItemWidth();
						ImGui::PushItemWidth(229);

						auto main_kits = itemIndex == 1 ? SkinChanger::gloveKits : SkinChanger::skinKits;
						auto display_index = 0;

						SkinChanger::displayKits = main_kits;

						if (strcmp(search_skins, crypt_str(""))) //-V526
						{
							for (auto i = 0; i < main_kits.size(); i++)
							{
								auto main_name = main_kits.at(i).name;

								for (auto i = 0; i < main_name.size(); i++)
									if (isalpha(main_name.at(i)))
										main_name.at(i) = tolower(main_name.at(i));

								char search_name[64];
								strcpy_s(search_name, sizeof(search_name), search_skins);

								for (auto i = 0; i < sizeof(search_name); i++)
									if (isalpha(search_name[i]))
										search_name[i] = tolower(search_name[i]);

								if (main_name.find(search_name) != std::string::npos)
								{
									SkinChanger::displayKits.at(display_index) = main_kits.at(i);
									display_index++;
								}
							}

							SkinChanger::displayKits.erase(SkinChanger::displayKits.begin() + display_index, SkinChanger::displayKits.end());
						}
						else
							item_index = selected_entry.paint_kit_vector_index;

						if (ImGui::ListBox(crypt_str("##PAINTKITS"), &item_index, [](void* data, int idx, const char** out_text)
							{
								while (SkinChanger::displayKits.at(idx).name.find(crypt_str("С‘")) != std::string::npos) //-V807
									SkinChanger::displayKits.at(idx).name.replace(SkinChanger::displayKits.at(idx).name.find(crypt_str("С‘")), 2, crypt_str("Рµ"));

								*out_text = SkinChanger::displayKits.at(idx).name.c_str();
								return true;
							}, nullptr, SkinChanger::displayKits.size(), 9, backup_tab != tab ? alpha : 0))
						{
							if (!SkinChanger::displayKits.empty())
							{
								auto i = 0;

								while (i < main_kits.size())
								{
									if (main_kits.at(i).id == SkinChanger::displayKits.at(item_index).id)
									{
										selected_entry.paint_kit_vector_index = i;
										break;
									}

									i++;
								}
							}
						}
							ImGui::PopItemWidth();

							ImGui::InputInt(crypt_str("Seed"), &selected_entry.seed, 1, 100, ImGuiInputTextFlags_CharsDecimal);
							ImGui::InputInt(crypt_str("StatTrak"), &selected_entry.stat_trak, 1, 100, 0, ImGuiInputTextFlags_CharsDecimal);
							ImGui::SliderFloat(crypt_str("Wear"), &selected_entry.wear, 0.0f, 1.0f);

							ImGui::Combo(crypt_str("Quality"), &selected_entry.entity_quality_vector_index, [](void* data, int idx, const char** out_text)
								{
									*out_text = game_data::quality_names[idx].name;
									return true;
								}, nullptr, IM_ARRAYSIZE(game_data::quality_names));

							if (!g_cfg.skins.custom_name_tag[itemIndex].empty())
								strcpy_s(selected_entry.custom_name, sizeof(selected_entry.custom_name), g_cfg.skins.custom_name_tag[itemIndex].c_str());

							ImGui::Text(crypt_str("Name Tag"));
							ImGui::PushItemWidth(229);

							if (ImGui::InputText(crypt_str("##nametag"), selected_entry.custom_name, sizeof(selected_entry.custom_name)))
								g_cfg.skins.custom_name_tag[itemIndex] = selected_entry.custom_name;

							ImGui::PopItemWidth();
							selected_entry.update();

							ImGui::EndGroup();

							if (backup_tab != tab)
								ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}
					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

					ImGui::BeginChild(crypt_str("##updatetab"), ImVec2(245, 41), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
						ImGui::BeginGroup();

						if (ImGui::Button(crypt_str("Update"), ImVec2(229, 25)))
							SkinChanger::scheduleHudUpdate();

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}
					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha, false);

					ImGui::EndGroup();

					ImGui::SameLine();
					ImGui::BeginGroup();

					ImGui::BeginChild(crypt_str("Extra"), ImVec2(245, 482), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
						ImGui::BeginGroup();

						ImGui::SliderInt(crypt_str("Viewmodel field of view"), &g_cfg.esp.viewmodel_fov, 0, 89);
						ImGui::SliderInt(crypt_str("Viewmodel X"), &g_cfg.esp.viewmodel_x, -50, 50);
						ImGui::SliderInt(crypt_str("Viewmodel Y"), &g_cfg.esp.viewmodel_y, -50, 50);
						ImGui::SliderInt(crypt_str("Viewmodel Z"), &g_cfg.esp.viewmodel_z, -50, 50);
						ImGui::SliderInt(crypt_str("Viewmodel roll"), &g_cfg.esp.viewmodel_roll, -180, 180);

						ImGui::Checkbox(crypt_str("Arms chams"), &g_cfg.esp.arms_chams);
						ImGui::ColorEdit4(crypt_str("armscolor"), &g_cfg.esp.arms_chams_color, ImGuiColorEditFlags_NoInputs);

						if (g_cfg.esp.arms_chams)
						{
							ImGui::Combo(crypt_str("Arms chams material"), &g_cfg.esp.arms_chams_type, chamstype, ARRAYSIZE(chamstype));

							if (g_cfg.esp.arms_chams_type != 6)
							{
								ImGui::Checkbox(crypt_str("Arms double material "), &g_cfg.esp.arms_double_material);
								ImGui::ColorEdit4(crypt_str("armsdoublematerial"), &g_cfg.esp.arms_double_material_color, ImGuiColorEditFlags_NoInputs);
							}

							ImGui::Checkbox(crypt_str("Arms animated material "), &g_cfg.esp.arms_animated_material);
							ImGui::ColorEdit4(crypt_str("armsanimatedmaterial"), &g_cfg.esp.arms_animated_material_color, ImGuiColorEditFlags_NoInputs);
						}

						ImGui::Checkbox(crypt_str("Weapon chams"), &g_cfg.esp.weapon_chams);
						ImGui::ColorEdit4(crypt_str("weaponchamscolors"), &g_cfg.esp.weapon_chams_color, ImGuiColorEditFlags_NoInputs);

						if (g_cfg.esp.weapon_chams)
						{
							ImGui::Combo(crypt_str("Weapon chams material"), &g_cfg.esp.weapon_chams_type, chamstype, ARRAYSIZE(chamstype));

							if (g_cfg.esp.weapon_chams_type != 6)
							{
								ImGui::Checkbox(crypt_str("Weapon double material "), &g_cfg.esp.weapon_double_material);
								ImGui::ColorEdit4(crypt_str("weapondoublematerial"), &g_cfg.esp.weapon_double_material_color, ImGuiColorEditFlags_NoInputs);
							}

							ImGui::Checkbox(crypt_str("Weapon animated material "), &g_cfg.esp.weapon_animated_material);
							ImGui::ColorEdit4(crypt_str("weaponanimatedmaterial"), &g_cfg.esp.weapon_animated_material_color, ImGuiColorEditFlags_NoInputs);
						}

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}

					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
					ImGui::EndGroup();
				}
			}
			else if (tab == 5)
			{
				ImGui::BeginGroup();

				ImGui::BeginChild(crypt_str("General"), ImVec2(245, 330), true);
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);

					ImGui::BeginGroup();

					ImGui::Checkbox(crypt_str("Anti-untrusted"), &g_cfg.misc.anti_untrusted);
					ImGui::Checkbox(crypt_str("Radar"), &g_cfg.misc.ingame_radar);
					ImGui::Checkbox(crypt_str("Rank reveal"), &g_cfg.misc.rank_reveal);
					ImGui::Checkbox(crypt_str("Unlock inventory access"), &g_cfg.misc.inventory_access);
					ImGui::Checkbox(crypt_str("Gravity ragdolls"), &g_cfg.misc.ragdolls);
					ImGui::Checkbox(crypt_str("Preserve killfeed"), &g_cfg.esp.preserve_killfeed);
					ImGui::Checkbox(crypt_str("Aspect ratio"), &g_cfg.misc.aspect_ratio);

					if (g_cfg.misc.aspect_ratio)
					{
						ImGui::SliderFloat(crypt_str("Amount"), &g_cfg.misc.aspect_ratio_amount, 1.0f, 2.0f);
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
					}

					ImGui::Checkbox(crypt_str("Fake lag"), &g_cfg.antiaim.fakelag);

					if (g_cfg.antiaim.fakelag)
					{
						ImGui::Combo(crypt_str("Fake lag type"), &g_cfg.antiaim.fakelag_type, fakelags, ARRAYSIZE(fakelags));

						for (auto i = 0, j = 0; i < ARRAYSIZE(lagstrigger); i++)
						{
							if (g_cfg.antiaim.fakelag_enablers[i])
							{
								if (j)
									preview += crypt_str(", ") + (std::string)lagstrigger[i];
								else
									preview = lagstrigger[i];

								j++;
							}
						}

						ImGui::SliderInt(crypt_str("Limit"), &g_cfg.antiaim.fakelag_amount, 1, 16);

						if (ImGui::BeginCombo(crypt_str("Fake lag triggers"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(lagstrigger))))
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

							ImGui::BeginGroup();
							{
								ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

								for (auto i = 0; i < ARRAYSIZE(lagstrigger); i++)
									ImGui::Selectable(lagstrigger[i], (bool*)&g_cfg.antiaim.fakelag_enablers[i], ARRAYSIZE(lagstrigger) - i == 1, ImGuiSelectableFlags_DontClosePopups);

								ImGui::PopStyleVar();
							}
							ImGui::EndGroup();

							ImGui::EndCombo();
						}

						preview = crypt_str("None");
						auto enabled_fakelag_triggers = false;

						for (auto i = 0; i < ARRAYSIZE(lagstrigger); i++)
							if (g_cfg.antiaim.fakelag_enablers[i])
								enabled_fakelag_triggers = true;

						if (enabled_fakelag_triggers)
							ImGui::SliderInt(crypt_str("Triggers limit"), &g_cfg.antiaim.triggers_fakelag_amount, 1, 16);
					}

					ImGui::Text(crypt_str("Teleport exploit"));
					KeyBindButton(&g_cfg.misc.teleport_exploit, crypt_str("teleport"));

					ImGui::EndGroup();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}
				ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

				ImGui::BeginChild(crypt_str("Information"), ImVec2(245, 164), true);
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
					ImGui::BeginGroup();

					ImGui::Checkbox(crypt_str("Watermark"), &g_cfg.menu.watermark);
					ImGui::Checkbox(crypt_str("Spectators list"), &g_cfg.misc.spectators_list);
					ImGui::Combo(crypt_str("Hitsound"), &g_cfg.esp.hitsound, sounds, ARRAYSIZE(sounds));
					ImGui::Checkbox(crypt_str("Killsound"), &g_cfg.esp.killsound);

					for (auto i = 0, j = 0; i < ARRAYSIZE(events); i++)
					{
						if (g_cfg.misc.events_to_log[i])
						{
							if (j)
								preview += crypt_str(", ") + (std::string)events[i];
							else
								preview = events[i];

							j++;
						}
					}

					if (ImGui::BeginCombo(crypt_str("Logs"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(events))))
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

						ImGui::BeginGroup();
						{
							ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

							for (auto i = 0; i < ARRAYSIZE(events); i++)
								ImGui::Selectable(events[i], (bool*)&g_cfg.misc.events_to_log[i], (ARRAYSIZE(events) - i == 1) ? true : false, ImGuiSelectableFlags_DontClosePopups);

							ImGui::PopStyleVar();
						}

						ImGui::EndGroup();
						ImGui::EndCombo();
					}

					preview = crypt_str("None");

					for (auto i = 0, j = 0; i < ARRAYSIZE(events_output); i++)
					{
						if (g_cfg.misc.log_output[i])
						{
							if (j)
								preview += crypt_str(", ") + (std::string)events_output[i];
							else
								preview = events_output[i];

							j++;
						}
					}

					if (ImGui::BeginCombo(crypt_str("Logs output"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(events_output))))
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

						ImGui::BeginGroup();
						{
							ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

							for (auto i = 0; i < ARRAYSIZE(events_output); i++)
								ImGui::Selectable(events_output[i], (bool*)&g_cfg.misc.log_output[i], ARRAYSIZE(events_output) - i == 1, ImGuiSelectableFlags_DontClosePopups);

							ImGui::PopStyleVar();
						}
						ImGui::EndGroup();

						ImGui::EndCombo();
					}

					preview = crypt_str("None");

					if (g_cfg.misc.events_to_log[EVENTLOG_HIT] || g_cfg.misc.events_to_log[EVENTLOG_ITEM_PURCHASES] || g_cfg.misc.events_to_log[EVENTLOG_BOMB])
					{
						ImGui::Text(crypt_str("Color "));
						ImGui::ColorEdit4(crypt_str("logcolor"), &g_cfg.misc.log_color, ImGuiColorEditFlags_NoInputs);
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
					}

					ImGui::Checkbox(crypt_str("Show CS:GO logs"), &g_cfg.misc.show_default_log);
					ImGui::EndGroup();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}
				ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
				ImGui::EndGroup();

				ImGui::SameLine();

				ImGui::BeginGroup();
				ImGui::BeginChild(crypt_str("Movement"), ImVec2(245, 245), true);
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
					ImGui::BeginGroup();

					ImGui::Combo(crypt_str("Automatic strafes"), &g_cfg.misc.airstrafe, strafes, ARRAYSIZE(strafes));

					if (g_cfg.misc.airstrafe >= 2)
						ImGui::SliderFloat(crypt_str("Retrack speed"), &g_cfg.misc.retrack_speed, 0.01f, 2.0f);

					ImGui::Checkbox(crypt_str("Automatic jump"), &g_cfg.misc.bunnyhop);
					ImGui::Checkbox(crypt_str("Crouch in air"), &g_cfg.misc.crouch_in_air);
					ImGui::Checkbox(crypt_str("Fast stop"), &g_cfg.misc.fast_stop);
					ImGui::Checkbox(crypt_str("Slide walk"), &g_cfg.misc.slidewalk);
					ImGui::Checkbox(crypt_str("No duck cooldown"), &g_cfg.misc.noduck);

					if (g_cfg.misc.noduck)
					{
						ImGui::Text(crypt_str("Fake duck"));
						KeyBindButton(&g_cfg.misc.fakeduck_key, crypt_str("fakeduck"));
					}

					ImGui::Text(crypt_str("Slow walk"));
					KeyBindButton(&g_cfg.misc.slowwalk_key, crypt_str("slowwalk"));

					ImGui::Text(crypt_str("Automatic peek"));
					KeyBindButton(&g_cfg.misc.automatic_peek, crypt_str("autopeek"));

					ImGui::Text(crypt_str("Edge jump"));
					KeyBindButton(&g_cfg.misc.edge_jump, crypt_str("edgejump"));

					ImGui::EndGroup();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}
				ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

				ImGui::BeginChild(crypt_str("Extra"), ImVec2(245, 249), true);
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
					ImGui::BeginGroup();

					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
					ImGui::Text(crypt_str("Menu color "));
					ImGui::ColorEdit4(crypt_str("menucolor"), &g_cfg.menu.menu_theme, ImGuiColorEditFlags_NoInputs);
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
					ImGui::Checkbox(crypt_str("Anti-screenshot"), &g_cfg.misc.anti_screenshot);
					ImGui::Checkbox(crypt_str("Clantag"), &g_cfg.misc.clantag_spammer);
					ImGui::Checkbox(crypt_str("Chat spam"), &g_cfg.misc.chat);
					ImGui::Checkbox(crypt_str("Buybot"), &g_cfg.misc.buybot_enable);

					if (g_cfg.misc.buybot_enable)
					{
						ImGui::Combo(crypt_str("Snipers"), &g_cfg.misc.buybot1, mainwep, ARRAYSIZE(mainwep));
						ImGui::Combo(crypt_str("Pistols"), &g_cfg.misc.buybot2, secwep, ARRAYSIZE(secwep));

						for (auto i = 0, j = 0; i < ARRAYSIZE(grenades); i++)
						{
							if (g_cfg.misc.buybot3[i])
							{
								if (j)
									preview += crypt_str(", ") + (std::string)grenades[i];
								else
									preview = grenades[i];

								j++;
							}
						}

						if (ImGui::BeginCombo(crypt_str("Other"), preview.c_str(), ImVec2(175, 2 + 20 * ARRAYSIZE(grenades))))
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);

							ImGui::BeginGroup();
							{
								ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

								for (auto i = 0; i < ARRAYSIZE(grenades); i++)
									ImGui::Selectable(grenades[i], (bool*)&g_cfg.misc.buybot3[i], (ARRAYSIZE(grenades) - i == 1) ? true : false, ImGuiSelectableFlags_DontClosePopups);

								ImGui::PopStyleVar();
							}
							ImGui::EndGroup();

							ImGui::EndCombo();
						}

						preview = crypt_str("None");
					}

					ImGui::EndGroup();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}
				ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);

				ImGui::EndGroup();
			}
			else if (tab == 6)
			{
				static auto settings_tab = 0;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 8);

				ImGui::BeginGroup();
				{
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 10.f));
					ImGui::PushFont(TabsText);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 22);
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
					ImGui::BeginGroup();

					if (ImGui::SemiTab(crypt_str("configs"), ImVec2(248 + 22, 20), !settings_tab))
						settings_tab = 0;

					ImGui::SameLine();

					if (ImGui::SemiTab(crypt_str("scripts"), ImVec2(249 + 22, 20), settings_tab == 1))
						settings_tab = 1;

					ImGui::EndGroup();
					ImGui::PopFont();
					ImGui::PopStyleVar();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}
				ImGui::EndGroup();

				if (!settings_tab)
				{
					ImGui::BeginChild(crypt_str("Configs"), ImVec2(245, 482), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
						ImGui::BeginGroup();

						static auto should_update = true;

						if (should_update)
						{
							should_update = false;

							cfg_manager->config_files();
							files = cfg_manager->files;

							for (auto& current : files)
								if (current.size() > 2)
									current.erase(current.size() - 3, 3);
						}

						static char config_name[64] = "\0";

						ImGui::PushItemWidth(229);
						ImGui::ListBoxConfigArray(crypt_str("##CONFIGS"), &g_cfg.selected_config, files, (backup_tab != tab) ? alpha : 0);
						ImGui::PopItemWidth();

						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 7);

						ImGui::PushItemWidth(229);
						ImGui::InputText(crypt_str("##configname"), config_name, sizeof(config_name));
						ImGui::PopItemWidth();

						if (ImGui::Button(crypt_str("Open configs folder"), ImVec2(229, 25)))
						{
							std::string folder;

							auto get_dir = [&folder]() -> void
							{
								static TCHAR path[MAX_PATH];

								if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, path)))
									folder = std::string(path) + crypt_str("\\Legendware\\Configs\\");

								CreateDirectory(folder.c_str(), NULL);
							};

							get_dir();
							ShellExecute(NULL, crypt_str("open"), folder.c_str(), NULL, NULL, SW_SHOWNORMAL);
						}

						if (ImGui::Button(crypt_str("Refresh"), ImVec2(229, 25)))
						{
							cfg_manager->config_files();
							files = cfg_manager->files;

							for (auto& current : files)
								if (current.size() > 2)
									current.erase(current.size() - 3, 3);
						}

						if (ImGui::Button(crypt_str("New"), ImVec2(229, 25)))
						{
							g_cfg.new_config_name = config_name;
							add_config();
						}

						if (ImGui::Button(crypt_str("Save"), ImVec2(229, 25)) && !cfg_manager->files.empty())
							ImGui::OpenPopup(crypt_str("save_ask"));

						if (ImGui::BeginPopupModal(crypt_str("save_ask"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar))
						{
							ImGui::SetWindowSize(ImVec2(150, 95));

							auto screen_pos = ImGui::GetCursorScreenPos();
							ImGui::GetWindowDrawList()->AddRect(ImVec2(screen_pos.x, screen_pos.y), ImVec2(screen_pos.x + 150, screen_pos.y + 95), ImColor(210, 210, 210, 30));

							ImGui::SetCursorPosX((ImVec2(150, 95).x / 2) - (ImGui::CalcTextSize(files.at(g_cfg.selected_config).c_str()).x / 2));
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);

							ImGui::TextWrapped(files.at(g_cfg.selected_config).c_str());

							ImGui::SetCursorPosX((ImVec2(150, 95).x / 2) - 37.5 - 25);
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);

							if (ImGui::Button(crypt_str("Save"), ImVec2(55, 20)))
							{
								save_config();
								ImGui::CloseCurrentPopup();
							}

							ImGui::SameLine();
							ImGui::SetCursorPosX((ImVec2(150, 95).x / 2) + 37.5 - 30);

							if (ImGui::Button(crypt_str("Cancel"), ImVec2(55, 20)))
								ImGui::CloseCurrentPopup();

							ImGui::EndPopup();
						}

						if (ImGui::Button(crypt_str("Load"), ImVec2(229, 25)))
							load_config();

						if (ImGui::Button(crypt_str("Remove"), ImVec2(229, 25)) && !cfg_manager->files.empty())
							ImGui::OpenPopup(crypt_str("remove_ask"));

						if (ImGui::BeginPopupModal(crypt_str("remove_ask"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar))
						{
							ImGui::SetWindowSize(ImVec2(150, 95));

							auto screen_pos = ImGui::GetCursorScreenPos();
							ImGui::GetWindowDrawList()->AddRect(ImVec2(screen_pos.x, screen_pos.y), ImVec2(screen_pos.x + 150, screen_pos.y + 95), ImColor(210, 210, 210, 30));

							ImGui::SetCursorPosX((ImVec2(150, 95).x / 2) - (ImGui::CalcTextSize(files.at(g_cfg.selected_config).c_str()).x / 2));
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);

							ImGui::TextWrapped(files.at(g_cfg.selected_config).c_str());

							ImGui::SetCursorPosX((ImVec2(150, 95).x / 2) - 37.5 - 25);
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);

							if (ImGui::Button(crypt_str("Remove"), ImVec2(55, 20)))
							{
								remove_config();
								ImGui::CloseCurrentPopup();
							}

							ImGui::SameLine();
							ImGui::SetCursorPosX((ImVec2(150, 95).x / 2) + 37.5 - 30);

							if (ImGui::Button(crypt_str("Cancel"), ImVec2(55, 20)))
								ImGui::CloseCurrentPopup();

							ImGui::EndPopup();
						}

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}
					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
				}
				else if (settings_tab == 1)
				{
					ImGui::BeginChild(crypt_str("Scripts"), ImVec2(245, 482), true);
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
						ImGui::BeginGroup();

						static auto should_update = true;

						if (should_update)
						{
							should_update = false;
							scripts = c_lua::get().scripts;

							for (auto& current : scripts)
							{
								if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
									current.erase(current.size() - 5, 5);
								else if (current.size() >= 4)
									current.erase(current.size() - 4, 4);
							}
						}

						if (scripts.empty())
						{
							ImGui::PushItemWidth(229);
							ImGui::ListBoxConfigArray(crypt_str("##SCRIPTS"), &selected_script, scripts, (backup_tab != tab) ? alpha : 0);
							ImGui::PopItemWidth();
						}
						else
						{
							auto backup_scripts = scripts;

							for (auto& script : scripts)
							{
								auto script_id = c_lua::get().get_script_id(script + crypt_str(".lua"));

								if (script_id == -1)
									continue;

								if (c_lua::get().loaded.at(script_id))
									scripts.at(script_id) += crypt_str(" [loaded]");
							}

							ImGui::PushItemWidth(229);
							ImGui::ListBoxConfigArray(crypt_str("##SCRIPTS"), &selected_script, scripts, (backup_tab != tab) ? alpha : 0);
							ImGui::PopItemWidth();

							scripts = std::move(backup_scripts);
						}

						if (ImGui::Button(crypt_str("Open scripts folder"), ImVec2(229, 25)))
						{
							std::string folder;

							auto get_dir = [&folder]() -> void
							{
								static TCHAR path[MAX_PATH];

								if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, path)))
									folder = std::string(path) + crypt_str("\\Legendware\\Scripts\\");

								CreateDirectory(folder.c_str(), NULL);
							};

							get_dir();
							ShellExecute(NULL, crypt_str("open"), folder.c_str(), NULL, NULL, SW_SHOWNORMAL);
						}

						if (ImGui::Button(crypt_str("Refresh"), ImVec2(229, 25)))
						{
							c_lua::get().refresh_scripts();
							scripts = c_lua::get().scripts;

							if (selected_script >= scripts.size())
								selected_script = scripts.size() - 1;

							for (auto& current : scripts)
							{
								if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
									current.erase(current.size() - 5, 5);
								else if (current.size() >= 4)
									current.erase(current.size() - 4, 4);
							}
						}

						if (ImGui::Button(crypt_str("Load"), ImVec2(229, 25)))
						{
							c_lua::get().load_script(selected_script);
							c_lua::get().refresh_scripts();

							scripts = c_lua::get().scripts;

							if (selected_script >= scripts.size())
								selected_script = scripts.size() - 1;

							for (auto& current : scripts)
							{
								if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
									current.erase(current.size() - 5, 5);
								else if (current.size() >= 4)
									current.erase(current.size() - 4, 4);
							}

							eventlogs::get().add(crypt_str("Loaded ") + scripts.at(selected_script) + crypt_str(" script"), false);
						}

						if (ImGui::Button(crypt_str("Unload"), ImVec2(229, 25)))
						{
							c_lua::get().unload_script(selected_script);
							c_lua::get().refresh_scripts();

							scripts = c_lua::get().scripts;

							if (selected_script >= scripts.size())
								selected_script = scripts.size() - 1;

							for (auto& current : scripts)
							{
								if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
									current.erase(current.size() - 5, 5);
								else if (current.size() >= 4)
									current.erase(current.size() - 4, 4);
							}

							eventlogs::get().add(crypt_str("Unloaded ") + scripts.at(selected_script) + crypt_str(" script"), false);
						}

						if (ImGui::Button(crypt_str("Remove"), ImVec2(229, 25)) && !scripts.empty())
							ImGui::OpenPopup(crypt_str("remove_script_ask"));

						if (ImGui::BeginPopupModal(crypt_str("remove_script_ask"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar))
						{
							ImGui::SetWindowSize(ImVec2(150, 95));

							auto screen_pos = ImGui::GetCursorScreenPos();
							ImGui::GetWindowDrawList()->AddRect(ImVec2(screen_pos.x, screen_pos.y), ImVec2(screen_pos.x + 150, screen_pos.y + 95), ImColor(210, 210, 210, 30));

							ImGui::SetCursorPosX((ImVec2(150, 95).x / 2) - (ImGui::CalcTextSize(scripts.at(selected_script).c_str()).x / 2));
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);

							ImGui::TextWrapped(scripts.at(selected_script).c_str());

							ImGui::SetCursorPosX((ImVec2(150, 95).x / 2) - 37.5 - 25);
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);

							if (ImGui::Button(crypt_str("Remove"), ImVec2(55, 20)))
							{
								c_lua::get().unload_script(selected_script);
								std::string folder, file;

								auto get_dir = [&folder, &file]() -> void
								{
									static TCHAR path[MAX_PATH];

									if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, path)))
									{
										folder = std::string(path) + crypt_str("\\Legendware\\Scripts\\");
										file = std::string(path) + crypt_str("\\Legendware\\Scripts\\") + c_lua::get().scripts.at(selected_script);
									}

									CreateDirectory(folder.c_str(), NULL);
								};

								get_dir();

								std::string path = file + '\0';
								std::remove(path.c_str());

								c_lua::get().refresh_scripts();

								eventlogs::get().add(crypt_str("Removed ") + scripts.at(selected_script) + crypt_str(" script"), false);
								scripts = c_lua::get().scripts;

								if (selected_script >= scripts.size())
									selected_script = scripts.size() - 1;

								for (auto& current : scripts)
								{
									if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
										current.erase(current.size() - 5, 5);
									else if (current.size() >= 4)
										current.erase(current.size() - 4, 4);
								}

								ImGui::CloseCurrentPopup();
							}

							ImGui::SameLine();
							ImGui::SetCursorPosX((ImVec2(150, 95).x / 2) + 37.5 - 30);

							if (ImGui::Button(crypt_str("Cancel"), ImVec2(55, 20)))
								ImGui::CloseCurrentPopup();

							ImGui::EndPopup();
						}

						if (ImGui::Button(crypt_str("Reload all scripts"), ImVec2(229, 25)))
						{
							c_lua::get().reload_all_scripts();
							c_lua::get().refresh_scripts();

							scripts = c_lua::get().scripts;

							if (selected_script >= scripts.size())
								selected_script = scripts.size() - 1;

							for (auto& current : scripts)
							{
								if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
									current.erase(current.size() - 5, 5);
								else if (current.size() >= 4)
									current.erase(current.size() - 4, 4);
							}
						}

						if (ImGui::Button(crypt_str("Unload all scripts"), ImVec2(229, 25)))
						{
							c_lua::get().unload_all_scripts();
							c_lua::get().refresh_scripts();

							scripts = c_lua::get().scripts;

							if (selected_script >= scripts.size())
								selected_script = scripts.size() - 1;

							for (auto& current : scripts)
							{
								if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
									current.erase(current.size() - 5, 5);
								else if (current.size() >= 4)
									current.erase(current.size() - 4, 4);
							}
						}

						ImGui::EndGroup();

						if (backup_tab != tab)
							ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
					}
					ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
				}

				ImGui::SameLine();

				ImGui::BeginChild(crypt_str("Script items"), ImVec2(245, 482), true);
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
					ImGui::BeginGroup();

					auto previous_check_box = false;

					for (auto& current : c_lua::get().scripts)
					{
						auto& items = c_lua::get().items.at(c_lua::get().get_script_id(current));

						for (auto& item : items)
						{
							std::string item_name;

							auto first_point = false;
							auto item_str = false;

							for (auto& c : item.first)
							{
								if (c == '.')
								{
									if (first_point)
									{
										item_str = true;
										continue;
									}
									else
										first_point = true;
								}

								if (item_str)
									item_name.push_back(c);
							}

							switch (item.second.type)
							{
							case NEXT_LINE:
								previous_check_box = false;
								break;
							case CHECK_BOX:
								previous_check_box = true;
								ImGui::Checkbox(item_name.c_str(), &item.second.check_box_value);
								break;
							case COMBO_BOX:
								previous_check_box = false;
								ImGui::Combo(item_name.c_str(), &item.second.combo_box_value, [](void* data, int idx, const char** out_text)
									{
										auto labels = (std::vector <std::string>*)data;
										*out_text = labels->at(idx).c_str();
										return true;
									}, &item.second.combo_box_labels, item.second.combo_box_labels.size());
								break;
							case SLIDER_INT:
								previous_check_box = false;
								ImGui::SliderInt(item_name.c_str(), &item.second.slider_int_value, item.second.slider_int_min, item.second.slider_int_max);
								break;
							case SLIDER_FLOAT:
								previous_check_box = false;
								ImGui::SliderFloat(item_name.c_str(), &item.second.slider_float_value, item.second.slider_float_min, item.second.slider_float_max);
								break;
							case COLOR_PICKER:
								if (previous_check_box)
									previous_check_box = false;
								else
									ImGui::Text((item_name + ' ').c_str());

								ImGui::ColorEdit4(item_name.c_str(), &item.second.color_picker_value, ImGuiColorEditFlags_NoInputs);
								ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
								break;
							}
						}
					}

					ImGui::EndGroup();

					if (backup_tab != tab)
						ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 99, p.y + 14), ImVec2(p.x + (width - 5), p.y + (height - 5)), ImColor(17, 17, 17, alpha));
				}

				ImGui::EndChild(backup_tab == tab ? 255 : 255 - alpha);
			}
		}
		ImGui::EndGroup();

		ImGui::PopFont();
		ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x + 3, p.y + 3), ImVec2(p.x + (width - 3), p.y + (height - 3)), ImColor(210, 210, 210, 25));

		if (backup_tab != tab && !first_open)
			ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + width, p.y + height), ImColor(17, 17, 17, alpha));
	}
	ImGui::End();
}

static bool d3d_init = false;

namespace INIT
{
	HMODULE Dll;
	HWND Window;
	WNDPROC OldWindow;
}

namespace hooks
{
	bool menu_open = false;
	bool input_shouldListen = false;

	ButtonCode_t* input_receivedKeyval;

	LRESULT __stdcall Hooked_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		static auto is_down = true;
		static auto is_clicked = false;

		if (GetAsyncKeyState(VK_INSERT))
		{
			is_clicked = false;
			is_down = true;
		}
		else if (!GetAsyncKeyState(VK_INSERT) && is_down)
		{
			is_clicked = true;
			is_down = false;
		}
		else
		{
			is_clicked = false;
			is_down = false;
		}

		if (is_clicked)
		{
			menu_open = !menu_open;

			if (menu_open && g_ctx.available())
			{
				if (g_ctx.globals.current_weapon != -1)
				{
					if (g_cfg.ragebot.enable)
						rage_weapon = g_ctx.globals.current_weapon;
					else if (g_cfg.legitbot.enabled)
						legit_weapon = g_ctx.globals.current_weapon;
				}

				if (g_cfg.skins.enable && g_ctx.local()->is_alive())
				{
					auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

					if (weapon->is_knife())
						itemIndex = 0;
					else
					{
						for (auto i = 2; i < 36; i++)
						{
							if (weapon->m_iItemDefinitionIndex() == game_data::weapon_names[i].definition_index)
							{
								itemIndex = i;
								break;
							}
						}
					}
				}
			}
		}

		auto pressed_buttons = false;
		auto pressed_menu_key = uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP || uMsg == WM_MOUSEWHEEL;

		if (g_ctx.available() && g_ctx.local()->is_alive() && g_ctx.get_command() && !pressed_menu_key && !g_ctx.globals.focused_on_input)
			pressed_buttons = g_ctx.get_command()->m_buttons;

		if (!pressed_buttons && d3d_init && menu_open && ImGui_ImplDX9_WndProcHandler(hWnd, uMsg, wParam, lParam) && !input_shouldListen)
			return true;

		return CallWindowProc(INIT::OldWindow, hWnd, uMsg, wParam, lParam);
	}

	long __stdcall Hooked_EndScene(IDirect3DDevice9* pDevice)
	{
		static auto original_fn = directx_hook->get_func_address <EndSceneFn>(42);

		if (!pDevice)
			return original_fn(pDevice);

		if (!m_engine()->IsActiveApp())
			return original_fn(pDevice);

		if (!d3d_init)
		{
			GUI_Init(pDevice);
		}

		POINT mp;
		GetCursorPos(&mp);

		ImGuiIO& io = ImGui::GetIO();

		io.MousePos.x = mp.x;
		io.MousePos.y = mp.y;

		pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);

		IDirect3DVertexDeclaration9* vertexDeclaration;
		pDevice->GetVertexDeclaration(&vertexDeclaration);

		ImGui_ImplDX9_NewFrame();

		if (menu_open)
			render_menu();

		if (g_ctx.globals.should_update_radar)
			Radar::get().OnMapLoad(m_engine()->GetLevelNameShort(), pDevice);
		if (!g_ctx.globals.should_update_radar)
			Radar::get().Render();

		otheresp::get().spread_crosshair(pDevice);

		ImGui::Render();
		ImGui_ImplDX9_RenderDrawLists(ImGui::GetDrawData());

		pDevice->SetVertexDeclaration(vertexDeclaration);
		vertexDeclaration->Release();

		return original_fn(pDevice);
	}

	long __stdcall Hooked_EndScene_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
	{
		static auto ofunc = directx_hook->get_func_address<EndSceneResetFn>(16);

		if (!d3d_init)
			return ofunc(pDevice, pPresentationParameters);

		ImGui_ImplDX9_InvalidateDeviceObjects();
		auto hr = ofunc(pDevice, pPresentationParameters);

		ImGui_ImplDX9_CreateDeviceObjects();
		return hr;
	}

	void GUI_Init(IDirect3DDevice9* pDevice)
	{
		ImGui_ImplDX9_Init(INIT::Window, pDevice);
		ImGuiIO& io = ImGui::GetIO();

		ImFontConfig font_config;
		font_config.OversampleH = 1;
		font_config.OversampleV = 1;
		font_config.PixelSnapH = 1;

		static const ImWchar ranges[] =
		{
			0x0020, 0x00FF,
			0x0400, 0x044F,
			0
		};

		char windows_directory[64];
		GetWindowsDirectory(windows_directory, 64);

		auto arial_directory = (std::string)windows_directory + crypt_str("\\Fonts\\Arial.ttf");
		auto verdana_directory = (std::string)windows_directory + crypt_str("\\Fonts\\Verdana.ttf");

		MainText = io.Fonts->AddFontFromFileTTF(arial_directory.c_str(), 12.f, &font_config, ranges);
		HeaderMenu = io.Fonts->AddFontFromFileTTF(verdana_directory.c_str(), 20.f);
		Porter = io.Fonts->AddFontFromMemoryTTF((void*)PorterBold, sizeof(PorterBold), 24.f);
		PorterBeta = io.Fonts->AddFontFromMemoryTTF((void*)PorterBold, sizeof(PorterBold), 10.f);
		Verdana16 = io.Fonts->AddFontFromMemoryTTF((void*)smallfont, sizeof(smallfont), 16.f);
		VisitorSmall = io.Fonts->AddFontFromMemoryTTF((void*)VISITOR20, sizeof(VISITOR20), 11.f);
		TabsText = io.Fonts->AddFontFromMemoryTTF((void*)RoomBold, sizeof(RoomBold), 13.f);
		KeyBinds = io.Fonts->AddFontFromFileTTF(verdana_directory.c_str(), 13.f);
		KeyBindsPixel = io.Fonts->AddFontFromMemoryTTF((void*)SMALLESTPIXEL7, sizeof(SMALLESTPIXEL7), 12.f);
		Tabs = io.Fonts->AddFontFromMemoryCompressedTTF(MyFontIcon_compressed_data, MyFontIcon_compressed_size, 19.f);

		Icons = io.Fonts->AddFontFromMemoryTTF((void*)cherryfont, sizeof(cherryfont), 22.f);
		VisIcons = io.Fonts->AddFontFromMemoryTTF((void*)Visuals, sizeof(Visuals), 22.f);

		Segoi = io.Fonts->AddFontFromMemoryTTF((void*)SegoiSemiBold, sizeof(SegoiSemiBold), 14.f);


		ImGuiStyle& style = ImGui::GetStyle();
		style.Alpha = 1.0f;
		style.WindowPadding = ImVec2(0, 0);
		style.WindowMinSize = ImVec2(32, 32);
		style.WindowRounding = 0.0f;
		style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
		style.ChildWindowRounding = 0.f;
		style.FramePadding = ImVec2(4, 3);
		style.FrameRounding = 2.0f;
		style.ItemSpacing = ImVec2(8, 10);
		style.ItemInnerSpacing = ImVec2(8, 8);
		style.TouchExtraPadding = ImVec2(0, 0);
		style.IndentSpacing = 21.0f;
		style.ColumnsMinSpacing = 0.0f;
		style.ScrollbarSize = 6.0f;
		style.ScrollbarRounding = 0.0f;
		style.GrabMinSize = 5.0f;
		style.GrabRounding = 0.0f;
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
		style.DisplayWindowPadding = ImVec2(22, 22);
		style.DisplaySafeAreaPadding = ImVec2(4, 4);
		style.AntiAliasedLines = true;
		style.AntiAliasedShapes = false;
		style.CurveTessellationTol = 1.f;

		auto colors = ImGui::GetStyle().Colors;

		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(17 / 255.f, 17 / 255.f, 27 / 255.f, 1.0f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(33 / 255.f, 34 / 255.f, 36 / 255.f, 1.0f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_ChildWindowBg] = ImVec4(20 / 255.f, 20 / 255.f, 20 / 255.f, 1.0f);
		colors[ImGuiCol_FrameBg] = ImVec4(33 / 255.f, 33 / 255.f, 33 / 255.f, 1.0f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(62 / 255.f, 62 / 255.f, 62 / 255.f, 1.0f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(62 / 255.f, 62 / 255.f, 62 / 255.f, 1.0f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f);
		colors[ImGuiCol_TitleBg] = ImVec4(35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.17f, 0.17f, 0.17f, 0.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 0.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 0.25f, 0.25f, 0.00f);
		colors[ImGuiCol_Button] = ImVec4(33 / 255.f, 35 / 255.f, 47 / 255.f, 1.0f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f); //
		colors[ImGuiCol_ButtonActive] = ImVec4(135 / 255.f, 135 / 255.f, 135 / 255.f, 1.0f); //
		colors[ImGuiCol_Header] = ImVec4(167 / 255.f, 24 / 255.f, 71 / 255.f, 1.0f); //multicombo, combo selected item color.
		colors[ImGuiCol_HeaderHovered] = ImVec4(35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f);
		colors[ImGuiCol_HeaderActive] = ImVec4(35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f);
		colors[ImGuiCol_Separator] = ImVec4(0, 0, 0, 1);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0, 0, 0, 1);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0, 0, 0, 1);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_CloseButton] = ImVec4(0, 0, 0, 0);
		colors[ImGuiCol_CloseButtonHovered] = ImVec4(0, 0, 0, 0);
		colors[ImGuiCol_HotkeyOutline] = ImVec4(0, 0, 0, 0);

		d3d_init = true;
	}

	DWORD original_getforeignfallbackfontname;
	DWORD original_setupbones;
	DWORD original_doextrabonesprocessing;
	DWORD original_standardblendingrules;
	DWORD original_updateclientsideanimation;
	DWORD original_physicssimulate;
	DWORD original_modifyeyeposition;
	DWORD original_calcviewmodelbob;

	vmthook* directx_hook;
	vmthook* client_hook;
	vmthook* clientstate_hook;
	vmthook* engine_hook;
	vmthook* clientmode_hook;
	vmthook* inputinternal_hook;
	vmthook* renderview_hook;
	vmthook* panel_hook;
	vmthook* modelcache_hook;
	vmthook* materialsys_hook;
	vmthook* modelrender_hook;
	vmthook* surface_hook;
	vmthook* bspquery_hook;
	vmthook* prediction_hook;
	vmthook* trace_hook;
	vmthook* filesystem_hook;

	C_HookedEvents hooked_events;
}

void __fastcall hooks::hooked_setkeycodestate(void* thisptr, void* edx, ButtonCode_t code, bool bDown)
{
	static auto original_fn = inputinternal_hook->get_func_address <SetKeyCodeState_t> (91);

	if (input_shouldListen && bDown)
	{
		input_shouldListen = false;

		if (input_receivedKeyval)
			*input_receivedKeyval = code;
	}

	return original_fn(thisptr, code, bDown);
}

void __fastcall hooks::hooked_setmousecodestate(void* thisptr, void* edx, ButtonCode_t code, MouseCodeState_t state)
{
	static auto original_fn = inputinternal_hook->get_func_address <SetMouseCodeState_t> (92);

	if (input_shouldListen && state == BUTTON_PRESSED)
	{
		input_shouldListen = false;

		if (input_receivedKeyval)
			*input_receivedKeyval = code;
	}

	return original_fn(thisptr, code, state);
}
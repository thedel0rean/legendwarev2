// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "nightmode.h"

std::vector <MaterialBackup> materials;

void nightmode::clear_stored_materials() 
{
	materials.clear();
}

void nightmode::modulate(MaterialHandle_t i, IMaterial* material, bool backup = false) 
{
	auto name = material->GetTextureGroupName();

	auto value_world = (float)g_cfg.esp.nightmode_value * 0.003f;
	auto value_prop = (float)g_cfg.esp.nightmode_value * 0.008f;

	if (strstr(name, crypt_str("World")))
	{
		if (backup) 
			materials.emplace_back(MaterialBackup(i, material));

		material->ColorModulate(value_world, value_world, value_world);
	}
	else if (strstr(name, crypt_str("StaticProp")))
	{
		if (backup) 
			materials.emplace_back(MaterialBackup(i, material));

		material->ColorModulate(value_prop, value_prop, value_prop);
	}
}

void nightmode::apply()
{
	if (!materials.empty())
	{
		for (auto i = 0; i < (int)materials.size(); i++)
			modulate(materials[i].handle, materials[i].material);

		return;
	}

	materials.clear();
	auto materialsystem = m_materialsystem();

	for (auto i = materialsystem->FirstMaterial(); i != materialsystem->InvalidMaterial(); i = materialsystem->NextMaterial(i))
	{
		auto material = materialsystem->GetMaterial(i);

		if (!material)
			continue;

		if (material->IsErrorMaterial())
			continue;

		modulate(i, material, true);
	}
}

void nightmode::remove() 
{
	for (auto i = 0; i < materials.size(); i++)
	{
		if (!materials[i].material)
			continue;

		if (materials[i].material->IsErrorMaterial())
			continue;

		materials[i].restore();
		materials[i].material->Refresh();
	}

	materials.clear();
}
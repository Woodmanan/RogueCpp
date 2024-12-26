#include "IO.h"

void MoveInput::Serialize()
{
	RogueSaveManager::AddOffset();
	RogueSaveManager::Write("Direction", direction);
	RogueSaveManager::RemoveOffset();
}

void MoveInput::Deserialize()
{
	RogueSaveManager::AddOffset();
	RogueSaveManager::Read("Direction", direction);
	RogueSaveManager::RemoveOffset();
}

void BeginSeededGameInput::Serialize()
{
	RogueSaveManager::AddOffset();
	RogueSaveManager::Write("Seed", seed);
	RogueSaveManager::RemoveOffset();
}

void BeginSeededGameInput::Deserialize()
{
	RogueSaveManager::AddOffset();
	RogueSaveManager::Read("Seed", seed);
	RogueSaveManager::RemoveOffset();
}

void LoadSaveInput::Serialize()
{
	RogueSaveManager::AddOffset();
	RogueSaveManager::Write("File Name", fileName);
	RogueSaveManager::RemoveOffset();
}

void LoadSaveInput::Deserialize()
{
	RogueSaveManager::AddOffset();
	RogueSaveManager::Read("File Name", fileName);
	RogueSaveManager::RemoveOffset();
}

namespace RogueSaveManager
{
	void Serialize(EInputType& value)
	{
		int asInt = value;
		Serialize(asInt);
	}

	void Deserialize(EInputType& value)
	{
		int asInt;
		Deserialize(asInt);
		value = (EInputType)asInt;
	}

	void Serialize(Input& value)
	{
		AddOffset();
		Write("Type", value.m_type);
		bool hasData = value.HasData();
		Write("Has Data", hasData);
		if (hasData)
		{
			WriteTabs();
			value.Get<InputBase>()->Serialize();
		}
		RemoveOffset();
	}

	void Deserialize(Input& value)
	{
		AddOffset();
		Read("Type", value.m_type);
		bool hasData;
		Read("Has Data", hasData);
		if (hasData)
		{
			switch (value.m_type)
			{
			default:
				HALT(); //Bad type! Whatever value this is needs a corresponding case in the switch.
			case EInputType::Movement:
				value.m_data = std::make_shared<MoveInput>();
				break;
			case EInputType::BeginSeededGame:
				value.m_data = std::make_shared<BeginSeededGameInput>();
				break;
			case EInputType::LoadSaveGame:
				value.m_data = std::make_shared<LoadSaveInput>();
				break;
			}

			ReadTabs();
			value.Get<InputBase>()->Deserialize();
		}
		RemoveOffset();
	}
}
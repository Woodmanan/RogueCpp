#include "StatManager.h"
#include "Data/SaveManager.h"
#include "Data/Resources.h"
#include "Utils/FileUtils.h"
#include "Utils/Utils.h"

const float& StatContainer::operator[](uint statID) const
{
	return m_values.at(statID);
}

float StatContainer::SetStat(uint statID, float newValue)
{
	if (m_values[statID] != newValue)
	{
		m_values[statID] = newValue;
		MarkDirty();
	}
}

void StatContainer::MarkDirty()
{
	m_dirty = true;
	if (m_parent.IsValid())
	{
		m_parent->MarkDirty();
	}
}

void StatContainer::CheckDirtyAndRefresh()
{
	if (m_dirty)
	{
		UpdateStats();
	}
}

void StatContainer::UpdateStats()
{

}

void StatManager::Init()
{
	std::vector<ResourcePointer> stats = ResourceManager::Get()->LoadFromConfigSynchronous("Stats", "stats");
	for (ResourcePointer& statArray : stats)
	{
		TResourcePointer<std::vector<StatDefinition>> pointer = statArray;
		for (auto it = pointer->begin(); it != pointer->end(); it++)
		{
			m_definitions.insert(std::pair(it->m_id, *it));
		}
	}
}

namespace RogueResources
{
	void PackStatDefinition(PackContext& packContext)
	{
		std::ifstream stream;
		stream.open(packContext.source);

		SkipLine(stream);

		std::vector<StatDefinition> stats;

		while (stream.is_open())
		{
			std::string line;
			std::getline(stream, line);
			if (line.empty()) { break; }

			std::vector<std::string> tokens = string_split(line, ",");

			ASSERT(tokens.size() <= 5);

			if (tokens[0].empty()) { continue; }

			//ID, Min, Max, Min Ref, Max Ref

			StatDefinition definition;
			definition.m_name = tokens[0];
			definition.m_id = (uint)std::hash<std::string>{}(tokens[0]);
			definition.m_min = optional_string_to_float(tokens, 1, -100000);
			definition.m_max = optional_string_to_float(tokens, 2,  100000);

			if (tokens.size() > 3 && !tokens[3].empty())
			{
				definition.m_minStatDefinition = (uint)std::hash<std::string>{}(tokens[3]);
			}

			if (tokens.size() > 4 && !tokens[4].empty())
			{
				definition.m_maxStatDefinition = (uint)std::hash<std::string>{}(tokens[4]);
			}

			stats.push_back(definition);
		}

		stream.close();

		OpenWritePackFile(packContext.destination, packContext.header);
		RogueSaveManager::Write("Stats", stats);
		RogueSaveManager::CloseWriteSaveFile();
	}

	std::shared_ptr<void> LoadStatDefinition(LoadContext& loadContext)
	{
		std::vector<StatDefinition>* stats = new std::vector<StatDefinition>();

		OpenReadPackFile(loadContext.source);
		RogueSaveManager::Read("Stats", *stats);
		RogueSaveManager::CloseReadSaveFile();

		return std::shared_ptr<std::vector<StatDefinition>>(stats);
	}
}
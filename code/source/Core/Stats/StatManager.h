#pragma once
#include "Core/CoreDataTypes.h"
#include "Data/Resources.h"
#include "Data/RogueDataManager.h"
#include <unordered_map>

struct StatDefinition
{
	std::string m_name;
	uint m_id = 0;
	float m_min;
	float m_max;
	uint m_minStatDefinition = 0;
	uint m_maxStatDefinition = 0;
};

class StatContainer
{
	virtual const float& operator[](uint statID) const;
	virtual float SetStat(uint statID, float newValue);

	std::unordered_map<uint, float>::const_iterator begin() {
		CheckDirtyAndRefresh();
		return constBegin();
	}

	std::unordered_map<uint, float>::const_iterator end()
	{
		CheckDirtyAndRefresh();
		return constEnd();
	}

	void MarkDirty();

protected:
	void CheckDirtyAndRefresh();
	virtual void UpdateStats();

	std::unordered_map<uint, float>::const_iterator constBegin() const { return m_values.begin(); }
	std::unordered_map<uint, float>::const_iterator constEnd() const { return m_values.end(); }

public:
	THandle<StatContainer> m_parent;
	std::unordered_map<uint, float> m_values;
	bool m_dirty = false;
};

class StatManager
{
public:
	void Init();

private:
	std::unordered_map<uint, StatDefinition> m_definitions;
};

namespace RogueResources
{
	void PackStatDefinition(PackContext& packContext);
	std::shared_ptr<void> LoadStatDefinition(LoadContext& loadContext);
}

namespace Serialization
{
	template<typename Stream>
	void Serialize(Stream& stream, const StatDefinition& value)
	{ 
		Write(stream, "Name", value.m_name);
		Write(stream, "ID", value.m_id);
		Write(stream, "Min", value.m_min);
		Write(stream, "Max", value.m_max);
		Write(stream, "MinStatDefinition", value.m_minStatDefinition);
		Write(stream, "MaxStatDefinition", value.m_maxStatDefinition);
	}

	template<typename Stream>
	void Deserialize(Stream& stream, StatDefinition& value)
	{
		Read(stream, "Name", value.m_name);
		Read(stream, "ID", value.m_id);
		Read(stream, "Min", value.m_min);
		Read(stream, "Max", value.m_max);
		Read(stream, "MinStatDefinition", value.m_minStatDefinition);
		Read(stream, "MaxStatDefinition", value.m_maxStatDefinition);
	}

	template<typename Stream>
	void Serialize(Stream& stream, const StatContainer& value)
	{
		Write(stream, "values", value.m_values);
		Write(stream, "parent", value.m_parent);
	}

	template<typename Stream>
	void Deserialize(Stream& stream, StatContainer& value)
	{
		Read(stream, "values", value.m_values);
		Read(stream, "parent", value.m_parent);
	}
}
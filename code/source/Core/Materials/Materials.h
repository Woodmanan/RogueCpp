#pragma once
#include "../Collections/FixedArrary.h"
#include "../../Data/Resouces.h"

#include <string>
#include <vector>

using namespace std;

enum Phase
{
	Solid,
	Liquid,
	Gas,
	Plasma
};

struct MaterialDefinition
{
	int ID;
	string name;
	float density;
	float meltingPoint;
	float boilingPoint;
	float specificHeat;
	float thermalConductivity;
	float electricalResistance;
	float hardness;
};

struct Material
{
	Material() {};
	Material(int materialID, float mass, bool isStatic) : m_mass(mass), m_materialID(materialID), m_static(isStatic) {}
	float m_mass = 0.0f;
	short m_materialID = -1;
	bool m_static = false;

	bool GetStatic() const { return m_static; }
	void SetStatic(bool value) { m_static = value; }

	const MaterialDefinition& GetMaterial() const;

	friend bool operator<(const Material& lhs, const Material& rhs)
	{
		return lhs.m_materialID < rhs.m_materialID;
	}
};

struct MaterialContainer
{
	static const int maxIndices = 10;

	FixedArray<Material, maxIndices> m_materials;
	FixedArray<int, maxIndices> m_layers;

	float m_heat = 0;

	void AddMaterial(int materialIndex, float mass, bool staticMaterial, int index = -1);
	void AddMaterial(const std::string& name, float mass, bool staticMaterial = false);
	void AddMaterial(const Material& material, int index = -1);
	void RemoveMaterial(const MaterialDefinition& material, float mass);
	void SortLayers();
	void CollapseDuplicates();

private:
	void SortLayerByDensity(int startIndex, int endIndex);
};

struct MixtureContainer
{
	static const int maxIndices = MaterialContainer::maxIndices * 2;
	FixedArray<Material, maxIndices> m_materials;
	FixedArray<float, maxIndices> m_mixture;

	float m_heat;

	void LoadMixture(MaterialContainer& one, MaterialContainer& two);
	void LoadMixture(MaterialContainer& single, int layer);
	int LoadContainer(MaterialContainer& container, FixedArray<Material, maxIndices>& sortedArray);
	int GetIndexOf(const MaterialDefinition& material);
};

struct Reaction
{
	std::string name;

	vector<Material> m_reactants;
	vector<Material> m_products;

	float m_minHeat;
	float m_deltaHeat;

	void SortReactantsByID();
	void Debug_Print();

	friend bool operator<(const Reaction& lhs, const Reaction& rhs)
	{
		if (lhs.m_reactants.size() > rhs.m_reactants.size()) { return true; }

		for (size_t i = 0; i < lhs.m_reactants.size(); i++)
		{
			if (lhs.m_reactants[i].m_materialID < rhs.m_reactants[i].m_materialID)
			{
				return true;
			}
		}

		if (lhs.m_minHeat > rhs.m_minHeat) { return true; }

		return false;
	}
};

class MaterialManager
{
public:
	MaterialManager();
	void Init();

	void AddMaterialDefinition(MaterialDefinition material);
	void AddReaction(Reaction reaction);

	void EvaluateReaction(MaterialContainer& one, MaterialContainer& two);
	void ExecuteReaction(Reaction& reaction, MixtureContainer& mixture, MaterialContainer& one, MaterialContainer& two);

	const MaterialDefinition& GetMaterialByID(int index);
	const MaterialDefinition& GetMaterialByName(const std::string& name);

	void PackMaterial(RogueResources::PackContext& packContext);
	std::shared_ptr<void> LoadMaterial(RogueResources::LoadContext& loadContext);

	void PackReaction(RogueResources::PackContext& packContext);
	std::shared_ptr<void> LoadReaction(RogueResources::LoadContext& loadContext);

	static MaterialManager* Get()
	{
		if (manager == nullptr)
		{
			manager = new MaterialManager();
		}

		return manager;
	}
	
private:
	void SortReactions();
	bool ReactionMatchesMixture(Reaction& reaction, MixtureContainer& mixture);
	float GetReactionMultiple(Reaction& reaction, MixtureContainer& mixture);

	static MaterialManager* manager;
	vector<MaterialDefinition> m_materialDefinitions;
	vector<Reaction> m_reactions;
};

namespace RogueSaveManager
{
	void Serialize(MaterialDefinition& value);
	void Deserialize(MaterialDefinition& value);
	void Serialize(Material& value);
	void Deserialize(Material& value);
	void Serialize(Reaction& value);
	void Deserialize(Reaction& value);
	void Serialize(MaterialContainer& value);
	void Deserialize(MaterialContainer& value);
}
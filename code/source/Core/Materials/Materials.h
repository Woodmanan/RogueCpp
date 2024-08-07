#pragma once
#include "../Collections/FixedArrary.h"

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
	Phase phase;
	float density;
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

	void AddMaterial(int materialIndex, float mass, bool staticMaterial, int index = -1);
	void AddMaterial(const Material& material, int index = -1);
	void SortLayers();
	void CollapseDuplicates();

	void Debug_Print();
private:
	void SortLayerByDensity(int startIndex, int endIndex);
};

struct MixtureContainer
{
	static const int maxIndices = MaterialContainer::maxIndices * 2;
	FixedArray<Material, maxIndices> m_materials;
	FixedArray<float, maxIndices> m_mixture;

	void LoadMixture(MaterialContainer& one, MaterialContainer& two);
	void LoadMixture(MaterialContainer& single, int layer);
	int LoadContainer(MaterialContainer& container, FixedArray<Material, maxIndices>& sortedArray);
};

struct Reaction
{
	float m_minHeat;
	float m_deltaHeat;

	vector<Material> m_reactants;
	vector<Material> m_products;

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

		return false;
	}
};

class MaterialManager
{

public:
	void AddMaterialDefinition(MaterialDefinition material);
	void AddReaction(Reaction reaction);

	void EvaluateReaction(MaterialContainer& one, MaterialContainer& two);

	const MaterialDefinition& GetMaterialByID(int index);

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

	static MaterialManager* manager;
	vector<MaterialDefinition> m_materialDefinitions;
	vector<Reaction> m_reactions;
};
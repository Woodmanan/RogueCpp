#pragma once
#include <string>
#include <vector>
#include <array>

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
};

struct MaterialContainer
{
	static const int maxIndices = 10;

	std::array<Material, maxIndices> m_materials;
	std::array<int, maxIndices> m_layers;
	int m_matSize = 0;
	int m_layerSize = 0;

	void AddMaterial(int materialIndex, float mass, bool staticMaterial, int index = -1);
	void AddMaterial(const Material& material, int index = -1);
	void SortLayers();
	void CollapseDuplicates();

	void Debug_Print();
private:
	void SortLayerByDensity(int startIndex, int endIndex);
};

struct MaterialReaction
{
	MaterialReaction() {}
	MaterialReaction(short matID, float mass) : m_materialID(matID), m_minMass(mass), m_maxMass(mass) {}
	MaterialReaction(short matID, float minMass, float maxMass) : m_materialID(matID), m_minMass(minMass), m_maxMass(maxMass) {}
	short m_materialID = -1;
	float m_minMass = 0.0f;
	float m_maxMass = 0.0f;
};

struct Reaction
{
	float m_minHeat;
	float m_deltaHeat;

	vector<Material> m_reactants;
	vector<Material> m_products;

	void SortReactantsByID();
	void Debug_Print();
};

class MaterialManager
{

public:
	void AddMaterialDefinition(MaterialDefinition material);
	void AddReaction(Reaction reaction);

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
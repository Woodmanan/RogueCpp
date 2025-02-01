#include "IO.h"

void TInput<Movement>::Serialize(DefaultStream& stream)
{
	Serialization::Write(stream, "Direction", m_direction);
}

void TInput<Movement>::Deserialize(DefaultStream& stream)
{
	Serialization::Read(stream, "Direction", m_direction);
}

void TInput<BeginSeededGame>::Serialize(DefaultStream& stream)
{
	Serialization::Write(stream, "Seed", seed);
}

void TInput<BeginSeededGame>::Deserialize(DefaultStream& stream)
{
	Serialization::Read(stream, "Seed", seed);
}

void TInput<LoadSaveGame>::Serialize(DefaultStream& stream)
{
	Serialization::Write(stream, "File Name", fileName);
}

void TInput<LoadSaveGame>::Deserialize(DefaultStream& stream)
{
	Serialization::Read(stream, "File Name", fileName);
}

void TOutput<ViewUpdated>::Serialize(DefaultStream& stream)
{
	Serialization::WriteRawBytes(stream, "data", m_data);
}

void TOutput<ViewUpdated>::Deserialize(DefaultStream& stream)
{
	Serialization::ReadRawBytes(stream, "data", m_data);
}
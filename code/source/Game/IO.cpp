#include "IO.h"

void MoveInput::Serialize(StreamType& stream)
{
	Serialization::Write(stream, "Direction", direction);
}

void MoveInput::Deserialize(StreamType& stream)
{
	Serialization::Read(stream, "Direction", direction);
}

void BeginSeededGameInput::Serialize(StreamType& stream)
{
	Serialization::Write(stream, "Seed", seed);
}

void BeginSeededGameInput::Deserialize(StreamType& stream)
{
	Serialization::Read(stream, "Seed", seed);
}

void LoadSaveInput::Serialize(StreamType& stream)
{
	Serialization::Write(stream, "File Name", fileName);
}

void LoadSaveInput::Deserialize(StreamType& stream)
{
	Serialization::Read(stream, "File Name", fileName);
}
#include "SaveManager.h"

namespace RogueSaveManager {
	std::ofstream outStream;
	std::ifstream inStream;
	char buffer[50];

	void WriteTabs()
	{
		for (int i = 0; i < offset; i++)
		{
			outStream.write(tabString, 4);
		}
	}

	void ReadTabs()
	{
		for (int i = 0; i < offset; i++)
		{
			inStream.read(buffer, 4);
		}
	}

	void WriteNewline()
	{
#ifdef JSON
		outStream.write("\n", 1);
		WriteTabs();
#endif // JSON
	}

	void ReadNewline()
	{
#ifdef JSON
		inStream.read(buffer, 1);
		ReadTabs();
#endif
	}

	void AddOffset()
	{
#ifdef JSON
		if (outStream.is_open())
		{
			//WriteTabs();
			outStream << "{\n";
		}
		if (inStream.is_open())
		{
			//ReadTabs();
			inStream.read(buffer, 2);
		}
#endif
		offset++;
	}

	void RemoveOffset()
	{
		offset--;
#ifdef JSON
		if (outStream.is_open())
		{
			WriteTabs();
			outStream << "}";
		}
		if (inStream.is_open())
		{
			ReadTabs();
			inStream.read(buffer, 1);
		}
#endif
	}
}

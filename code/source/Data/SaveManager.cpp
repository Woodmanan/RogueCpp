#include "SaveManager.h"

namespace RogueSaveManager {
	std::ofstream outStream;
	std::ifstream inStream;
	char buffer[200];

	void WriteTabs()
	{
#ifdef JSON
		for (int i = 0; i < offset; i++)
		{
			outStream.write(tabString, 4);
		}
#endif
	}

	void ReadTabs()
	{
#ifdef JSON
		for (int i = 0; i < offset; i++)
		{
			inStream.read(buffer, 4);
		}
#endif
	}

	void WriteNewline()
	{
#ifdef JSON
		outStream.write("\n", 1);
#endif // JSON
	}

	void ReadNewline()
	{
#ifdef JSON
		inStream.read(buffer, 1);
#endif
	}

	void AddOffset()
	{
#ifdef JSON
		if (outStream.is_open())
		{
			outStream << "{\n";
		}
		if (inStream.is_open())
		{
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

	void AddListSeparator(bool isLast)
	{
#ifdef JSON
		if (!isLast)
		{
			outStream << ",";
		}
		WriteNewline();
#endif
	}

	void RemoveListSeparator(bool isLast)
	{
#ifdef JSON
		if (!isLast)
		{
			inStream.read(buffer, 1);
		}
		ReadNewline();
#endif
	}
}

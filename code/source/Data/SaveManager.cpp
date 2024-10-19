#include "Data/SaveManager.h"

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

	int HexToInt(char hex)
	{
		switch (hex)
		{
		case '0':
			return 0;
		case '1':
			return 1;
		case '2':
			return 2;
		case '3':
			return 3;
		case '4':
			return 4;
		case '5':
			return 5;
		case '6':
			return 6;
		case '7':
			return 7;
		case '8':
			return 8;
		case '9':
			return 9;
		case 'a':
			return 10;
		case 'b':
			return 11;
		case 'c':
			return 12;
		case 'd':
			return 13;
		case 'e':
			return 14;
		case 'f':
			return 15;
		}

		HALT();
		return -1;
	}

	char IntToHex(int val)
	{
		ASSERT(val >= 0 && val < 16);
		switch (val)
		{
		case 0:
			return '0';
		case 1:
			return '1';
		case 2:
			return '2';
		case 3:
			return '3';
		case 4:
			return '4';
		case 5:
			return '5';
		case 6:
			return '6';
		case 7:
			return '7';
		case 8:
			return '8';
		case 9:
			return '9';
		case 10:
			return 'a';
		case 11:
			return 'b';
		case 12:
			return 'c';
		case 13:
			return 'd';
		case 14:
			return 'e';
		case 15:
			return 'f';
		}

		HALT();
		return 'X';
	}
}

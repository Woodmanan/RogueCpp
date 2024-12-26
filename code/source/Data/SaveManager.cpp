#include "Data/SaveManager.h"

namespace RogueSaveManager {
	thread_local std::ofstream SaveStreams::outStream;
	thread_local std::ifstream SaveStreams::inStream;
	thread_local int SaveStreams::offset;
	thread_local char SaveStreams::buffer[200];

	void WriteTabs()
	{
#ifdef JSON
		for (int i = 0; i < SaveStreams::offset; i++)
		{
			SaveStreams::outStream.write(tabString, 4);
		}
#endif
	}

	void ReadTabs()
	{
#ifdef JSON
		for (int i = 0; i < SaveStreams::offset; i++)
		{
			SaveStreams::inStream.read(SaveStreams::buffer, 4);
		}
#endif
	}

	void WriteNewline()
	{
#ifdef JSON
		SaveStreams::outStream.write("\n", 1);
#endif // JSON
	}

	void ReadNewline()
	{
#ifdef JSON
		SaveStreams::inStream.read(SaveStreams::buffer, 1);
#endif
	}

	void AddOffset()
	{
#ifdef JSON
		if (SaveStreams::outStream.is_open())
		{
			SaveStreams::outStream << "{\n";
		}
		if (SaveStreams::inStream.is_open())
		{
			SaveStreams::inStream.read(SaveStreams::buffer, 2);
		}
#endif
		SaveStreams::offset++;
	}

	void RemoveOffset()
	{
		SaveStreams::offset--;
#ifdef JSON
		if (SaveStreams::outStream.is_open())
		{
			WriteTabs();
			SaveStreams::outStream << "}";
		}
		if (SaveStreams::inStream.is_open())
		{
			ReadTabs();
			SaveStreams::inStream.read(SaveStreams::buffer, 1);
		}
#endif
	}

	void AddListSeparator(bool isLast)
	{
#ifdef JSON
		if (!isLast)
		{
			SaveStreams::outStream << ",";
		}
		WriteNewline();
#endif
	}

	void RemoveListSeparator(bool isLast)
	{
#ifdef JSON
		if (!isLast)
		{
			SaveStreams::inStream.read(SaveStreams::buffer, 1);
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

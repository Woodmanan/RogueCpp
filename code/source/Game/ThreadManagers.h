/*
* Forward declaration file for thread local managers. Used to keep the game
* space clean for linking.
*/

//Forward declarations
class Game;
class RogueDataManager;
class MaterialManager;
class Map;
class TileMemory;

Game* GetGame();
RogueDataManager* GetDataManager();
MaterialManager* GetMaterialManager();
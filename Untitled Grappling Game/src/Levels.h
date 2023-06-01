#pragma once
static std::string GetLevelByNr(int levelNr) {
	switch (levelNr) {
	case 1:
		return "Levels/Level1.dat";
	case 2:
		return "Levels/Level2.dat";
	case 3:
		return "Levels/Level3.dat";
	default:
		return "Levels/Test.dat";
	}
}
/*
 * CGame.h
 *
 *  Created on: 01.05.2009
 *      Author: gerstrong
 */

#ifndef CGAME_H_
#define CGAME_H_

#include <string>
#include "vorticon/CEGAGraphics.h"
#include "vorticon/CMessages.h"

// references to cinematics/CommonEnding
int endsequence(stCloneKeenPlus *pCKP);

// Reference from game.cpp
void initgame(stLevelControl *p_levelcontrol);

class CGame {
public:
	CGame();
	virtual ~CGame();

	bool loadResources(unsigned short Episode, const std::string& DataDirectory);
	void freeResources(void);
	short runCycle(stCloneKeenPlus *pCKP);
	void preallocateCKP(stCloneKeenPlus *pCKP);

private:
	unsigned short m_Episode;
	std::string m_DataDirectory;
	int current_demo;

	CEGAGraphics *EGAGraphics;
	CMessages *m_Messages;
	CTileLoader *TileLoader;
};

#endif /* CGAME_H_ */

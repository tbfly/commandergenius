/*
 * fileio.h
 *
 *  Created on: 09.01.2009
 *      Author: gerstrong
 */

#ifndef __CG_FILEIO_H__
#define __CG_FILEIO_H__

#include <string>

unsigned int loadmap(const std::string& filename, const std::string& path,
									int lvlnum, stLevelControl* p_levelcontrol);
short checkConsistencyofGameData(stGameData *p_GameData);
std::string formatPathString(const std::string& path);
void assignChangeTileAttribute(stTile *tile, int episode);

#endif

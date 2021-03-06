/* GAMEPDO.C
  Contains all of the gamepdo_xxx functions...which are called from the
  main game loop. These functions perform some task that is done each
  time around the game loop and are related to the player.
*/

char tempbuf[200];

#include "keen.h"

#include "sdl/CInput.h"
#include "sdl/CVideoDriver.h"
#include "sdl/CTimer.h"
#include "sdl/sound/CSound.h"

#include "include/game.h"
#include "include/gamepdo.h"
#include "include/gamedo.h"
#include "include/gm_pdowm.h"
#include "include/misc.h"
#include "include/menu.h"

#include "vorticon/CDialog.h"
#include "CGraphics.h"

// player handler mother-function, calls all needed "gamepdo"
// functions for player cp
void gamepdo_HandlePlayer(int cp, stCloneKeenPlus *pCKP)
{
char doFall;

    if (player[cp].pdie)
    {
       gamepdo_dieanim(cp, pCKP);
       if (!pCKP->Control.levelcontrol.gameovermode)
         gamepdo_StatusBox(cp, pCKP);
    }
    else
    {
       if (!pCKP->Control.levelcontrol.gameovermode)
       {
    	  player[cp].inhibitwalking = 0;
    	  player[cp].inhibitfall = 0;

          gamepdo_StatusBox(cp, pCKP);

       	  gamepdo_ProcessInput(cp, pCKP);

          gamepdo_setdir(cp, pCKP);

          gamepdo_setblockedlru(cp, pCKP);
          gamepdo_getgoodies(cp, pCKP);

          if (pCKP->Control.levelcontrol.episode==3) gamepdo_ankh(cp);

          gamepdo_raygun(cp, pCKP);

          gamepdo_keencicle(cp, pCKP);

          if(!player[cp].pjumping && !player[cp].pfalling)
          {
        	  gamepdo_walking(cp, pCKP);
        	  gamepdo_walkinganim(cp, pCKP);
          }


          if (fade.mode==NO_FADE || fade.dir==FADE_IN || pCKP->Control.levelcontrol.demomode)
          {
            gamepdo_playpushed(cp, pCKP);
            gamepdo_InertiaAndFriction_X(cp, pCKP);
          }

       	  gamepdo_JumpAndPogo(cp, pCKP);

          // decide if player should fall
          doFall = 1;
          if (player[cp].inhibitfall) doFall = 0;

          if (doFall)
        	  gamepdo_falling(cp, pCKP);
          else
          {
        	  if(player[cp].pjumping == PJUMPED)
        		  player[cp].pfalling = 0;
        	  player[cp].psupportingtile = 145;
        	  player[cp].psupportingobject = 0;
          }

       }
       else
       { // we're in game-over mode

       }
    }

    gamepdo_SelectFrame(cp);
}

void gamepdo_walkbehindexitdoor(int cp, stCloneKeenPlus *pCKP)
{
int x, diff, width;

    /* don't draw keen as he walks through the door (past exitXpos) */
    // X pixel position of right side of player
    x = (player[cp].x >> CSF) + PLAYERSPRITE_WIDTH;
    diff = (x - pCKP->Control.levelcontrol.exitXpos);        // dist between keen and door
    if (diff >= 0)                             // past exitXpos?
    {
       width = (PLAYERSPRITE_WIDTH - diff);    // get new width of sprite
       if (width < 0) width = 0;               // don't set to negative

       // set new width of all player walk frames
       sprites[playerbaseframes[cp]+0].xsize = width;
       sprites[playerbaseframes[cp]+1].xsize = width;
       sprites[playerbaseframes[cp]+2].xsize = width;
       sprites[playerbaseframes[cp]+3].xsize = width;
    }
}

void gamepdo_dieanim(int cp, stCloneKeenPlus *pCKP)
{
   if (!player[cp].pdie) return;                // should never happen...
   if (player[cp].pdie==PDIE_DEAD) return;      // if true animation is over
   if (player[cp].pdie==PDIE_FELLOFFMAP)
   {
     // wait for falling sound to complete, then kill the player
     if (!g_pSound->isPlaying(SOUND_KEEN_FALL))
     {
        player[cp].pdie = 0;
        killplayer(cp);
     }
     else return;
   }

   // peridocally toggle dying animation frame
   if (player[cp].pdietimer > DIE_ANIM_RATE)
   {
     player[cp].pdieframe = 1 - player[cp].pdieframe;
     player[cp].pdietimer = 0;
   }
   else player[cp].pdietimer++;

   // is it time to start flying off the screen?
   if (!player[cp].pdietillfly)
   {  // time to fly off the screen
     if (((player[cp].y>>CSF)+96 > scroll_y) && (player[cp].y>(16<<CSF)))
     {  // player has not reached top of screen
        // make player fly up
        player[cp].y += PDIE_RISE_SPEED;
        if (player[cp].x > (4<<CSF))
        {
          player[cp].x += player[cp].pdie_xvect;
        }
     }
     else
     {  // reached top of screen. he's done.
       player[cp].pdie = PDIE_DEAD;
       if (player[cp].inventory.lives<0)
       {
         pCKP->Control.levelcontrol.gameovermode = true;
         g_pSound->playSound(SOUND_GAME_OVER, PLAY_NOW);
       }
       else
       {
         endlevel(0,&(pCKP->Control.levelcontrol));
       }
     }
   }
   else
   {  // not yet time to fly off screen, decrement timer
     player[cp].pdietillfly--;
   }  // end "time to fly"

}

void gamepdo_keencicle(int cp, stCloneKeenPlus *pCKP)
{
   // keencicle code (when keen gets hit by an ice chunk)
   if (player[cp].pfrozentime)
   {
     if (player[cp].pfrozentime > PFROZEN_THAW)
     {
        if (player[cp].pfrozenanimtimer > PFROZENANIMTIME)
        {
          if (player[cp].pfrozenframe) player[cp].pfrozenframe=0; else player[cp].pfrozenframe=1;
          player[cp].pfrozenanimtimer = 0;
        }
        else player[cp].pfrozenanimtimer++;
     }
     else
     { // thawing out, show the thaw frame
        if (pCKP->Control.levelcontrol.episode==3)
          player[cp].pfrozenframe = 2;
        else
          player[cp].pfrozenframe = 3;
     }

     player[cp].pfrozentime--;
     player[cp].inhibitwalking = 1;
   }

}

// Only checks if keen exits the level and if two-button firing should be triggered!
void gamepdo_ProcessInput(unsigned int cp, stCloneKeenPlus *pCKP)
{
	stLevelControl *p_levelcontrol;

	p_levelcontrol = &(pCKP->Control.levelcontrol);

   // are we doing the keen-walking-through-exit door animation?
   if (pCKP->Control.levelcontrol.level_done &&
		   pCKP->Control.levelcontrol.level_finished_by==cp)
   {
     // don't let player control keen

	 memset(player[cp].playcontrol,0,PA_MAX_ACTIONS);

      player[cp].inhibitfall = 1;
     if (pCKP->Control.levelcontrol.level_done==LEVEL_DONE_WALK)
     {
        // keep him going right
        player[cp].pdir = player[cp].pshowdir = RIGHT;
        // make keen walk slowly through the exit door
        player[cp].playcontrol[PA_X] = 80;
        if (player[cp].pinertia_x > PMAXEXITDOORSPEED)
        {
          player[cp].pinertia_x = PMAXEXITDOORSPEED;
        }
     }
     else
     {
        // he's all the way through the door; hold him still
        player[cp].pinertia_x = 0;
        player[cp].pwalking = 0;
     }

     if (p_levelcontrol->level_done_timer > LEVEL_DONE_TOTAL_WAIT_TIME)
     {
       if (p_levelcontrol->level_done != LEVEL_DONE_FADEOUT)
       {
    	   p_levelcontrol->level_done = LEVEL_DONE_FADEOUT;
         endlevel(1, &(pCKP->Control.levelcontrol));
       }
     }
     else if (p_levelcontrol->level_done_timer > LEVEL_DONE_STOPWALKING_TIME)
     {
    	 p_levelcontrol->level_done = LEVEL_DONE_WAIT;
     }

     p_levelcontrol->level_done_timer++;
     return;
   }

   if(options[OPT_TWOBUTTON].value)
   {
	   if(player[cp].playcontrol[PA_JUMP] && player[cp].playcontrol[PA_POGO])
	   {
		   player[cp].playcontrol[PA_FIRE] = 1;
		   player[cp].playcontrol[PA_JUMP] = 0;
		   player[cp].playcontrol[PA_POGO] = 0;
	   }
   }

   if(g_pInput->getPressedKey(KP))
   {
	   // Open the Pause Dialog
	   CDialog	*PauseDialog;

	   PauseDialog = new CDialog;

	   PauseDialog->setDimensions(10,10,20,3);
	   PauseDialog->addOptionText("Game Paused");
	   PauseDialog->animateDialogBox(true);

	   PauseDialog->setVisible(true);

	   do
	   {
		   g_pInput->pollEvents();
		   g_pTimer->SpeedThrottle();
		   gamedo_fades();

		   gamedo_render_drawobjects();
		   gamedo_AnimatedTiles();
		   PauseDialog->renderDialog();
		   gamedo_frameskipping_blitonly();
		} while(!g_pInput->getPressedAnyKey());

	   delete PauseDialog;

	   map_redraw();
   }

}

// if player not sliding and not jumping, allow
// them to change their direction. if jumping,
// we can change direction but it will not be shown
// in the frame.
void gamepdo_setdir(int cp, stCloneKeenPlus *pCKP)
{
int stuck;

   if (player[cp].pfrozentime) return;
   // can't change direction on ice,
   // UNLESS we're stuck up against a wall
   if (player[cp].psliding)
   {
     stuck = 0;
     if (player[cp].pshowdir == LEFT && player[cp].blockedl) stuck = 1;
     if (player[cp].pshowdir == RIGHT && player[cp].blockedr) stuck = 1;
     if (stuck)
     {
       // jumped off an ice block into a wall?
       if (player[cp].pjumping || player[cp].pfalling)
       {
         player[cp].psliding = 0;
       }
     }
     else
     {
       // since we're not stuck up against a wall, we can't change direction
       return;
     }
   }

   if (!player[cp].pjumping && !player[cp].pfiring)
   {
     if (player[cp].playcontrol[PA_X] < 0) { player[cp].pdir = player[cp].pshowdir = LEFT; }
     if (player[cp].playcontrol[PA_X] > 0) { player[cp].pdir = player[cp].pshowdir = RIGHT; }
   }
   else
   {
     if (player[cp].playcontrol[PA_X] < 0) { player[cp].pdir = player[cp].pshowdir = LEFT;  }
     if (player[cp].playcontrol[PA_X] > 0) { player[cp].pdir = player[cp].pshowdir = RIGHT;  }
   }
}

// set blockedl/r/u...is Keen up against a solid object or a the edge of the level?
void gamepdo_setblockedlru(unsigned int cp, stCloneKeenPlus *pCKP)
{
	// This function has a lot of bugs now!
   int tx,ty;
   unsigned int i;
   stOption *p_option;


#define PLAYERHEIGHT (player[cp].h)
#define PLAYERWIDTH  (player[cp].w) // bug here! Sometimes it detects 16 and sometimes 24. Why?!

   p_option = pCKP->Option;

   player[cp].blockedl = player[cp].blockedr = 0;
   player[cp].blockedu = 0;
   if (pCKP->Control.levelcontrol.level_done &&
		   pCKP->Control.levelcontrol.level_finished_by==cp) return;

   if ((player[cp].x>>CSF) < 2) player[cp].blockedl = 1;

   if (!p_option[OPT_CHEATS].value || g_pInput->getHoldedKey(KTAB)==0)   // holding down TAB will turn off clipping
   {
      tx = (player[cp].x>>CSF);
      ty = (player[cp].y>>CSF);

      if((TileProperty[getmaptileat(tx,ty)][BUP] == 1) && (TileProperty[getmaptileat(tx,ty)][BEHAVIOR] == 1))
      {
    	  killplayer(cp); // Whines
      }

      // set psliding if we're on ice
      if (TileProperty[getmaptileat(tx,ty+PLAYERHEIGHT)][BUP] == 3 && !player[cp].ppogostick)
      {
        player[cp].psliding = 1;
        player[cp].pshowdir = player[cp].pdir;
      }
      else
      {
        player[cp].psliding = 0;
      }
      // set psemisliding if we're on an ice block
      if (TileProperty[getmaptileat(tx,ty+PLAYERHEIGHT)][BUP] == 2) // 2*pheighth because the ice down off the player must be detected
      {
        player[cp].psemisliding = 1;
      }
      else
      {
        player[cp].psemisliding = 0;
      }

       // we use checkissolidl to check for blockedr, and vice versa
       // for blockedl. this is because here we are checking for the
       // right of the player--which will be the left of a potential tile.
       // Also check floor and ceiling tiles
      // TODO: Try to optimize this!

      for( i=5 ; i < PLAYERWIDTH ; i++ )
      {
    	  if(TileProperty[getmaptileat((tx+i),ty)][BDOWN] || checkobjsolid((tx+i)<<CSF,(ty)<<CSF,cp))
		  {
			  player[cp].blockedu = 1;
			  break;
		  }
      }

      for( i=2 ; i < PLAYERHEIGHT ; i++ )
      {
    	  if (checkissolidl((player[cp].x>>CSF)+PLAYERWIDTH+1, (player[cp].y>>CSF)+i,cp, pCKP))
    	  {
    		  player[cp].blockedr = 1;
    		  player[cp].widejump = false;
    		  break;
    	  }
      }

      for( i=2 ; i < PLAYERHEIGHT ; i++ )
      {
    	  if (checkissolidr((player[cp].x>>CSF)+2, (player[cp].y>>CSF)+i,cp, pCKP))
    	  {
    		  player[cp].blockedl = 1;
    		  player[cp].widejump = false;
    		  break;
    	  }
      }
      for( i=5 ; i < PLAYERWIDTH+1 ; i++ )
      {
    	  if(checkobjsolid((tx+i)<<CSF,((ty<<CSF)+PLAYERHEIGHT),cp))
		  {
			  player[cp].blockedu = 1;
			  break;
		  }
      }

      // for one-way force fields in ep3, so you can back out if you're
      // not all the way through yet
      if (player[cp].blockedr && TileProperty[player[cp].blockedby][BLEFT] && !TileProperty[player[cp].blockedby][BRIGHT])
      {
        if (((player[cp].x>>CSF)+13)>>4<<4 != ((player[cp].x>>CSF)+11))
        {  // not on a tile boundary.
          player[cp].blockedr = 0;
        }
      }


      if (player[cp].blockedl && TileProperty[player[cp].blockedby][BRIGHT] && !TileProperty[player[cp].blockedby][BLEFT])
      {
        if ((((player[cp].x>>CSF)+2)>>4<<4)+12 != ((player[cp].x>>CSF)+2))
        {  // not on a tile boundary.
          player[cp].blockedl = 0;
        }
      }

      // Check if the player is going out of the level map
      if( player[cp].y <= (2<< 4 << CSF) ) // Upper edge or ceiling
    	  player[cp].blockedu = 1;
      else if( player[cp].y >= (map.ysize << 4 << CSF) )
      {
    	  player[cp].blockedd = 1;
    	  player[cp].pdie = 1;

      }

      if( (player[cp].x) >= ((map.xsize-3) << 4 << CSF) )
    	  player[cp].blockedr = 1;

      else if( (player[cp].x) <= ( 2 << 4 << CSF ) )
    	  player[cp].blockedl = 1;
   }
   else player[cp].playpushed_x = 0;
}

// let's have keen be able to pick up goodies
void gamepdo_getgoodies(int cp, stCloneKeenPlus *pCKP)
{
   if ((TileProperty[getmaptileat((player[cp].x>>CSF)+9, (player[cp].y>>CSF)+1)][BEHAVIOR] > 0) &&
		   ( TileProperty[getmaptileat((player[cp].x>>CSF)+9, (player[cp].y>>CSF)+1)][BEHAVIOR] < 31 ) )
      { keen_get_goodie((player[cp].x>>CSF)+9, (player[cp].y>>CSF)+1, cp, pCKP); return; }

   else if ((TileProperty[getmaptileat((player[cp].x>>CSF)+4, (player[cp].y>>CSF)+8)][BEHAVIOR] > 0) &&
		   ( TileProperty[getmaptileat((player[cp].x>>CSF)+4, (player[cp].y>>CSF)+8)][BEHAVIOR] < 31 ) )
      { keen_get_goodie((player[cp].x>>CSF)+4, (player[cp].y>>CSF)+8, cp, pCKP); return; }

   else if ((TileProperty[getmaptileat((player[cp].x>>CSF)+9, (player[cp].y>>CSF)+16)][BEHAVIOR] > 0) &&
		   ( TileProperty[getmaptileat((player[cp].x>>CSF)+9, (player[cp].y>>CSF)+16)][BEHAVIOR] < 31 ) )
      { keen_get_goodie((player[cp].x>>CSF)+9, (player[cp].y>>CSF)+16, cp, pCKP); return; }

   else if ((TileProperty[getmaptileat((player[cp].x>>CSF)+4, (player[cp].y>>CSF)+23)][BEHAVIOR] > 0) &&
		   ( TileProperty[getmaptileat((player[cp].x>>CSF)+4, (player[cp].y>>CSF)+23)][BEHAVIOR] < 31 ) )
      { keen_get_goodie((player[cp].x>>CSF)+4, (player[cp].y>>CSF)+23, cp, pCKP); return; }
}

// animation for walking
void gamepdo_walkinganim(int cp, stCloneKeenPlus *pCKP)
{
    // no walk animation while sliding
    if (player[cp].inhibitwalking || player[cp].psliding ) return;

    // should we do walk animation?
    if (player[cp].pwalking  || player[cp].playpushed_x || player[cp].psemisliding)
    {
    	int walkanimrate; // walk animation speed according to player speed


    	if(!player[cp].psemisliding)
    	{
    		walkanimrate = 101*PWALKANIMRATE/(player[cp].treshold+1);
        	if(walkanimrate > 150)
        			walkanimrate = 150;
    	}
    	else
    		walkanimrate = PWALKANIMRATE;

    	// ** do walk animation **
        if (player[cp].pwalkanimtimer > walkanimrate)
        { // time to change walking frame
          // make walk noise
          if (!player[cp].pjumping && !player[cp].pfalling)
          {
            if (!player[cp].pfrozentime && player[cp].pwalking)
            {
               if (player[cp].pwalkframea&1)
                 {
            	   g_pSound->playStereofromCoord(SOUND_KEEN_WALK, PLAY_NOW, objects[player[cp].useObject].scrx);
                 }
               else
                 {
            	   g_pSound->playStereofromCoord(SOUND_KEEN_WALK2, PLAY_NOW, objects[player[cp].useObject].scrx);
                 }

               if(!map.isworldmap && (player[cp].blockedr || player[cp].blockedl))
               {
            	   g_pSound->playStereofromCoord(SOUND_KEEN_BUMPHEAD, PLAY_NOW, objects[player[cp].useObject].scrx);
				   // It is not bumping a head, but walking in some direction and being blocked
               }
               else if(map.isworldmap)
               {
            	   // Same on world map!

            	   short play=0;

            	   if (player[cp].blockedu && player[cp].pdir == UP)
            		   play=1;

            	   if (player[cp].blockedd && player[cp].pdir == DOWN)
            		   play=1;

            	   if (player[cp].blockedl && player[cp].pdir == LEFT)
            		   play=1;

            	   if (player[cp].blockedr && player[cp].pdir == RIGHT)
            		   play=1;

            	   if (play)
            		   g_pSound->playStereofromCoord(SOUND_KEEN_BUMPHEAD, PLAY_NOW, objects[player[cp].useObject].scrx);
               }

            }
          }
          // increase walk frame and wrap it to 1st frame if needed
          if (player[cp].pwalkframea < 4)
            { player[cp].pwalkframea++; }
          else
            { player[cp].pwalkframea=1; }

          player[cp].pwalkanimtimer = 0;
        }
        else
        {  // did not change walk frame
          player[cp].pwalkanimtimer++;
        }

        // set walk frame: map frame "4" to frame "2", this gives a
        // sequence of 1,2,3,2,1,2,3,2,1,2,3,2....
        if (player[cp].pwalkframea==4)
        {
          player[cp].pwalkframe = 2;
        }
        else
        {
          player[cp].pwalkframe = player[cp].pwalkframea;
        }
    }
}

// handle playpushed_x: for yorps/walkers/etc pushing keen
void gamepdo_playpushed(int cp, stCloneKeenPlus *pCKP)
{
    if (pCKP->Option[OPT_CHEATS].value && g_pInput->getHoldedKey(KTAB)) return;

    // if we're being pushed...
    if (player[cp].playpushed_x)
    {
      // do friction on push force...
      if (player[cp].playpushed_decreasetimer>PLAYPUSH_DECREASERATE)
      {
        // push playpushed_x towards zero
        if (player[cp].playpushed_x < 0)
        {
          player[cp].playpushed_x++;
        }
        else
        {
          player[cp].playpushed_x--;
        }
        player[cp].playpushed_decreasetimer = 0;
      }
      else player[cp].playpushed_decreasetimer++;

      // if we run up against a wall all push inertia stops
      if (player[cp].playpushed_x > 0 && player[cp].blockedr) player[cp].playpushed_x = 0;
      if (player[cp].playpushed_x < 0 && player[cp].blockedl) player[cp].playpushed_x = 0;
    }

}
// handles inertia and friction for the X direction
// (this is where the inertia/playpushed_x is actually applied to playx)
void gamepdo_InertiaAndFriction_X(unsigned int cp, stCloneKeenPlus *pCKP)
{
	stLevelControl *p_levelcontrol;
	int friction_rate;

	p_levelcontrol = &(pCKP->Control.levelcontrol);

   if (player[cp].hideplayer)
   {
     player[cp].pinertia_x = 0;
     return;
   }
   // don't move when firing
   if (player[cp].pfiring && !player[cp].pjumping && !player[cp].pfalling)
   {
     player[cp].pinertia_x = 0;
   }

   // Check walking boost and pogoing. It is similar to inertia
   if(player[cp].pjumping || player[cp].pfalling)
   {
	   if (player[cp].playcontrol[PA_X] < 0 && !player[cp].pfrozentime)
	   {
		   if(player[cp].pboost_x > 0 && !player[cp].ppogostick)
		   {
			   player[cp].pboost_x = 0;
			   player[cp].pinertia_x /= 2;
		   }
		   else
		   {
			   player[cp].pboost_x-= player[cp].ppogostick ? 3 : 1;
		   }
	   }
	   if (player[cp].playcontrol[PA_X] > 0 && !player[cp].pfrozentime)
	   {
		   if(player[cp].pboost_x < 0 && !player[cp].ppogostick)
		   {
			   player[cp].pboost_x = 0;
			   player[cp].pinertia_x /= 2;
		   }
		   else
		   {
			   player[cp].pboost_x+= player[cp].ppogostick ? 3 : 1;
		   }
	   }

	   if(player[cp].pboost_x >= PJUMPINERTIA ||
			   player[cp].pboost_x <= -PJUMPINERTIA)
	   {
		   if(player[cp].ppogostick)
		   {
			   player[cp].pinertia_x += 3*player[cp].pboost_x/(PJUMPINERTIA*2);
		   }
		   else
		   {
			   // This is a normal jump without obstacles
			   if(player[cp].widejump)
			   {
				   player[cp].pinertia_x += 2*player[cp].pboost_x/PJUMPINERTIA + player[cp].chargedjump;
				   player[cp].chargedjump = 0;
			   }

			   // When falling get some inertia
			   if(player[cp].pfalling)
			   {
				   player[cp].pinertia_x += 2*player[cp].pboost_x/PJUMPINERTIA;
			   }
			   else
			   {
				   player[cp].pinertia_x += 2*player[cp].pboost_x/PJUMPINERTIA;
			   }
		   }
		   player[cp].pboost_x = 0;
	   }
   }

   // Calculate Threshold of your analog device for walking animation speed!
   if(!player[cp].pfrozentime)
	   player[cp].treshold = player[cp].playcontrol[PA_X];

  int pmaxspeed = 0;

  if(!player[cp].pjumping && !player[cp].pfalling &&
	 !player[cp].psemisliding && !player[cp].psliding && !player[cp].ppogostick && !player[cp].pslowingdown)
  {
	  if(player[cp].treshold < player[cp].playcontrol[PA_Y] && player[cp].playcontrol[PA_Y] > 0 )
		  player[cp].treshold = player[cp].playcontrol[PA_Y];

	  if(player[cp].treshold > player[cp].playcontrol[PA_Y] && player[cp].playcontrol[PA_Y] < 0 )
		  player[cp].treshold = player[cp].playcontrol[PA_Y];

	  if(player[cp].treshold < 0)
		  player[cp].treshold *= (-1);
  }
  else
  {
	  player[cp].treshold = 100;
  }

  // if we hit a solid object do not move, and keep inertia...
  // * at slow speed: if we're falling or jumping and we're facing
  //                  the right direction, we want to keep
  //                  a small amount of inertia pent up so we can
  //                  easily get into tight spaces.
  // * at zero:       otherwise, or if we're not walking, we want
  //                  to hold the inertia at zero so we don't "shoot"
  //                  off of platforms, etc which looks weird.
  if (!p_levelcontrol->level_done || p_levelcontrol->level_finished_by!=cp)
  {
    if (player[cp].pinertia_x < 0 && player[cp].blockedl)
    {
      if (!player[cp].pwalking || (!player[cp].pfalling && !player[cp].pjumping) || player[cp].pdir==RIGHT || (player[cp].pfrozentime && p_levelcontrol->episode==1))
      {
		 player[cp].pinertia_x = player[cp].pboost_x = 0;
		 player[cp].widejump = false;
      }
      else if (player[cp].pinertia_x < -PFASTINCMAXSPEED)
      {
    	  //player[cp].pinertia_x--;
    	  player[cp].pinertia_x >>= 1;
      }
      return;
    }
    else if (player[cp].pinertia_x > 0 && player[cp].blockedr)
    {
      if (!player[cp].pwalking || (!player[cp].pfalling && !player[cp].pjumping) || player[cp].pdir==LEFT || (player[cp].pfrozentime&&p_levelcontrol->episode==1))
      {
    	 player[cp].pinertia_x = player[cp].pboost_x = 0;
    	 player[cp].widejump = false;
      }
      else if (player[cp].pinertia_x > PFASTINCMAXSPEED)
      {
    	  //player[cp].pinertia_x++;
    	  player[cp].pinertia_x >>= 1;
      }
   	  return;
    }
  }

  pmaxspeed = player[cp].treshold*PMAXSPEED/100;

	  if(player[cp].pinertia_x > pmaxspeed)
		  player[cp].pinertia_x = pmaxspeed;
	  if(player[cp].pinertia_x < -pmaxspeed)
		  player[cp].pinertia_x = -pmaxspeed;



   // apply pinertia_x and playpushed_x inertia
   // (unless we're about to make a pogo jump)
   if ( player[cp].pjumping != PPREPAREPOGO)
   {
     player[cp].x += (player[cp].pinertia_x + player[cp].playpushed_x);
   }


   // if we stopped walking (i.e. left or right not held down) apply friction
   // there's no friction if we're semisliding
   if (!(player[cp].playcontrol[PA_X] < 0) && !(player[cp].playcontrol[PA_X] > 0) && \
       !player[cp].psemisliding)
   {
     // determine friction rate--different rates for on ground and in air
     if (map.isworldmap)
     {
       friction_rate = PFRICTION_RATE_WM;
     }
     else
     {
        if (!player[cp].pfalling & !player[cp].pjumping)
        {
          friction_rate = PFRICTION_RATE_ONGROUND;
        }
        else
        {
          friction_rate = PFRICTION_RATE_INAIR;
        }
     }


     // and apply friction to pinertia_x
     // when pogoing apply friction till we get down to PFASTINCMAXSPEED
     // then stop the friction
     if (!player[cp].ppogostick || (player[cp].pinertia_x > PFASTINCMAXSPEED) || (player[cp].pinertia_x < -PFASTINCMAXSPEED))
     {
        if (player[cp].pfriction_timer_x > friction_rate)
        {
          if (!player[cp].pfrozentime || p_levelcontrol->episode!=1)
          {  // disable friction while frozen
        	  // here the wall animation must be applied!
             if (player[cp].pinertia_x < 0) player[cp].pinertia_x++;
             else if (player[cp].pinertia_x > 0) player[cp].pinertia_x--;
          }

          player[cp].pfriction_timer_x = 0;
        }
        else player[cp].pfriction_timer_x++;
     }
   }

}
void gamepdo_Jump(int cp)
{
	   // handle the JUMP key, both for normal jumps and (high) pogo jumps
	   if (!player[cp].pjumping && !player[cp].pfalling && !player[cp].pfiring)
	   {
		 player[cp].pboost_x = 0;
	     // give em the chance to jump
	     if (player[cp].playcontrol[PA_JUMP] && !player[cp].ppogostick && !player[cp].pfrozentime)
	     {
	       player[cp].pjumping = PPREPAREJUMP;
	       player[cp].pjumpframe = PPREPAREJUMPFRAME;
	       player[cp].pjumpanimtimer = 0;
	       player[cp].pwalking = 0;
	     }
	     else if (player[cp].ppogostick)
	     {
	       player[cp].pjumping = PPREPAREPOGO;
	       player[cp].pjumpanimtimer = 0;
	       player[cp].pwalking = 0;
	     }
	   }

	   // Preparing to jump. Charge the jump here
	   if(player[cp].pjumping == PPREPAREJUMP)
	   {
		   player[cp].pjumping = PJUMPUP;
		   player[cp].pinertia_y = -25;

		   // Code for the charge jump which really collects pinertia_y
	   }

	   // He is jumping up
	   if(player[cp].pjumping == PJUMPUP)
	   {
		   // handle the jump upwards. Works like friction. The inertia_y is lost by a unit at every loop

		   // Did he bonk the ceiling
		   if(!player[cp].blockedu)
		   {
			   player[cp].y += player[cp].pinertia_y;
		   }
		   else
		   {
    		   player[cp].pinertia_y = 0;
    		   player[cp].pjumpfloattimer = 0;
    		   player[cp].pjumping = PJUMPED;
		   }
	       player[cp].pjumpfloattimer++;

	       if(player[cp].pjumpfloattimer >= 5)
	       {
	    	   player[cp].pjumpfloattimer = 0;
	    	   if(player[cp].pinertia_y < 0)
	    	   {
	    		   player[cp].pinertia_y++;
	    	   }
	    	   else
	    	   {
	    		   player[cp].pinertia_y = 0;
	    		   player[cp].pjumpfloattimer = 0;
	    		   player[cp].pjumping = PJUMPED;
	    	   }
	       }
	   }

	   // The player has jumped and should be about to fall
	   if(player[cp].pjumping == PJUMPED)
	   {
			  // Maybe not needed!
		   player[cp].pjumping = 0;
		   player[cp].pfalling = 1;
	   }
}

void gamepdo_JumpAndPogo(int cp, stCloneKeenPlus *pCKP)
{
int mx, my, t, l;
int platx, platy;
signed char pxoff, pyoff;
int o;
int try2;

stLevelControl *p_levelcontrol;

p_levelcontrol = &(pCKP->Control.levelcontrol);

   // toggle pogo when KPOGO key is pressed
   if (player[cp].playcontrol[PA_POGO] && !player[cp].lastplaycontrol[PA_POGO] && !player[cp].pfrozentime)
   {
	   // if we are at a switch hit the switch instead
	   mx = (player[cp].x>>CSF)+8;
	   my = (player[cp].y>>CSF)+9;
	   try2 = 0;
	   retry: ;
	   t = getmaptileat(mx, my);
	   if (!player[cp].ppogostick && (t==TILE_SWITCH_UP || t==TILE_SWITCH_DOWN))
	   { // switch to extend platform

		 // figure out where the platform is supposed to extend
		 // (this is coded in the object layer...high byte is the Y offset
		 //  and the low byte is the X offset)
		 l = getlevelat(mx, my);
		 // if zero it's the switch on a tantalus ray!
		 if (l==0)
		 {
			 g_pSound->playStereofromCoord(SOUND_SWITCH_TOGGLE, PLAY_NOW, objects[player[cp].useObject].scrx);

		   map_chgtile(mx>>4,my>>4,TILE_SWITCH_DOWN);
		   p_levelcontrol->success = 0;
		   p_levelcontrol->command = LVLC_TANTALUS_RAY;
		   return;
		 }
		 pxoff = (l & 0x00ff);
		 pyoff = (l & 0xff00) >> 8;
		 platx = (mx >> 4) + pxoff;
		 platy = (my >> 4) + pyoff;

		 if (PlatExtending)       // don't allow player to hit switch again while
		 {                        // plat is moving as this will glitch the plat
		   return;
		 }
		 else PlatExtending = 1;

		 g_pSound->playStereofromCoord(SOUND_SWITCH_TOGGLE, PLAY_NOW, objects[player[cp].useObject].scrx);

		 if (t==TILE_SWITCH_UP)
		 {  // switch toggled from up to down--extend platform
		   map_chgtile(mx>>4,my>>4,TILE_SWITCH_DOWN);
		   o = spawn_object((mx>>4<<4)<<CSF,(my>>4<<4)<<CSF,OBJ_SECTOREFFECTOR);
		   objects[o].ai.se.type = SE_EXTEND_PLATFORM;
		   objects[o].ai.se.platx = platx;
		   objects[o].ai.se.platy = platy;
		 }
		 else
		 {  // switch toggled from down to up--remove platform
		   map_chgtile(mx>>4,my>>4,TILE_SWITCH_UP);
		   o = spawn_object((mx>>4<<4)<<CSF,(my>>4<<4)<<CSF,OBJ_SECTOREFFECTOR);
		   objects[o].ai.se.type = SE_RETRACT_PLATFORM;
		   objects[o].ai.se.platx = platx;
		   objects[o].ai.se.platy = platy;
		 }
	 }
	 else if (!player[cp].ppogostick && t==TILE_LIGHTSWITCH)
	 { // lightswitch
	   p_levelcontrol->dark ^= 1;
	   g_pGraphics->initPalette(p_levelcontrol->dark);
	   g_pGraphics->fadePalette(PAL_FADE_SHADES);
	   g_pSound->playStereofromCoord(SOUND_SWITCH_TOGGLE, PLAY_NOW, objects[player[cp].useObject].scrx);
	 }
	 else
	 {
		if (!try2)
		{
			my = (player[cp].y>>CSF)+1;
			try2 = 1;
			goto retry;
		}
		 // toggle pogo
		 if (player[cp].inventory.HasPogo)
		 {
		   player[cp].ppogostick = !player[cp].ppogostick;
		 }
	 }
   }

   // handle the JUMP key, both for normal jumps and (high) pogo jumps
   if (!player[cp].pjumping && !player[cp].pfalling && !player[cp].pfiring)
   {
	 player[cp].pboost_x = 0;
     // give em the chance to jump
     if (player[cp].playcontrol[PA_JUMP] && !player[cp].ppogostick && !player[cp].pfrozentime)
     {
       player[cp].pjumping = PPREPAREJUMP;
       player[cp].pjumpframe = PPREPAREJUMPFRAME;
       player[cp].pjumpanimtimer = 0;
       player[cp].pwalking = 0;
     }
     else if (player[cp].ppogostick)
     {
       player[cp].pjumping = PPREPAREPOGO;
       player[cp].pjumpanimtimer = 0;
       player[cp].pwalking = 0;
     }
   }

    switch(player[cp].pjumping)
    {
      case PPREPAREPOGO:
          if (player[cp].pjumpanimtimer>PPOGO_PREPARE_TIME)
          {
             // continously bounce while pogo stick is out
        	  g_pSound->playStereofromCoord(SOUND_KEEN_JUMP, PLAY_NOW, objects[player[cp].useObject].scrx);

             // jump high if JUMP key down, else bounce low
             if (player[cp].playcontrol[PA_JUMP])
             {
               if (!pCKP->Option[OPT_SUPERPOGO].value)
               {  // normal high pogo jump
                  player[cp].pjumpupspeed = PPOGOUP_SPEED;
                  player[cp].pjumptime = PJUMP_NORMALTIME_POGO_LONG;
                  player[cp].pjumpupdecreaserate = PJUMP_UPDECREASERATE_POGO_LONG;
               }
               else
               {
                  player[cp].pjumpupspeed = PPOGOUP_SPEED_SUPER;
                  player[cp].pjumptime = PJUMP_NORMALTIME_POGO_LONG_SUPER;
                  player[cp].pjumpupdecreaserate = PJUMP_UPDECREASERATE_POGO_LONG_SUPER;
               }
             }
             else
             {
               if(player[cp].ppogostick)
               {
            	   player[cp].pjumpupspeed = PJUMPUP_SPEED;
				   player[cp].pjumptime = PJUMP_NORMALTIME_POGO_SHORT;
				   player[cp].pjumpupdecreaserate = PJUMP_UPDECREASERATE_POGO_SHORT;
               }
             }
             player[cp].pjumpframe = PJUMP_PREPARE_LAST_FRAME;
             player[cp].pjumping = PPOGOING;
             player[cp].pjumpupspeed_decreasetimer = 0;
             player[cp].pjustjumped = 1;

          } else player[cp].pjumpanimtimer++;
          break;
      case PPREPAREJUMP:
			 player[cp].widejump = true;

   			 if(player[cp].psliding)
   			 {
   				 if(player[cp].pdir == LEFT)
   					 player[cp].chargedjump-=2;
   				 else if(player[cp].pdir == RIGHT)
   					player[cp].chargedjump+=2;
   			 }
   			 else
   			 {
   	   			 if(g_pInput->getHoldedCommand(IC_LEFT))
   	   				 player[cp].chargedjump-=2;
   	   			 else if(g_pInput->getHoldedCommand(IC_RIGHT))
   	   				 player[cp].chargedjump+=2;
   			 }

             player[cp].pinertia_x = 0;     // prevent moving while preparing to jump
             if (player[cp].pjumpanimtimer > PJUMP_PREPARE_ANIM_RATE)
             {
                  if (player[cp].pjumpframe == PJUMP_PREPARE_LAST_FRAME || !player[cp].playcontrol[PA_JUMP])
                  {  // time to start the jump
                  // select a jump depending on how long keen was preparing
                       player[cp].pjumpupspeed = PJUMPUP_SPEED;
                       switch(player[cp].pjumpframe)
                       {
                       case PPREPAREJUMPFRAME:
                            player[cp].pjumptime = PJUMP_NORMALTIME_6;
                            player[cp].pjumpupdecreaserate = PJUMP_UPDECREASERATE_6;
                            player[cp].pjumpupspeed = 1;
                            player[cp].chargedjump = player[cp].chargedjump >> 5;
                            break;
                       case PPREPAREJUMPFRAME+1:
                            player[cp].pjumptime = PJUMP_NORMALTIME_5;
                            player[cp].pjumpupdecreaserate = PJUMP_UPDECREASERATE_5;
                            player[cp].pjumpupspeed = 2;
                            player[cp].chargedjump = player[cp].chargedjump >> 4;
                            break;
                       case PPREPAREJUMPFRAME+2:
                            player[cp].pjumptime = PJUMP_NORMALTIME_4;
                            player[cp].pjumpupdecreaserate = PJUMP_UPDECREASERATE_4;
                            player[cp].pjumpupspeed = 4;
                            player[cp].chargedjump = player[cp].chargedjump >> 3;
                            break;
                       case PPREPAREJUMPFRAME+3:
                            player[cp].pjumptime = PJUMP_NORMALTIME_3;
                            player[cp].pjumpupdecreaserate = PJUMP_UPDECREASERATE_3;
                            player[cp].pjumpupspeed = 8;
                            player[cp].chargedjump = player[cp].chargedjump >> 2;
                            break;
                       case PPREPAREJUMPFRAME+4:
                            player[cp].pjumptime = PJUMP_NORMALTIME_2;
                            player[cp].pjumpupdecreaserate = PJUMP_UPDECREASERATE_2;
                            player[cp].pjumpupspeed = 16;
                            player[cp].chargedjump = player[cp].chargedjump >> 1;
                            break;
                       default:
                            player[cp].pjumptime = PJUMP_NORMALTIME_1;
                            player[cp].pjumpupdecreaserate = PJUMP_UPDECREASERATE_1;
                            break;
                       }

                    player[cp].pjumpframe = PJUMP_PREPARE_LAST_FRAME;

                    g_pSound->playStereofromCoord(SOUND_KEEN_JUMP, PLAY_NOW, objects[player[cp].useObject].scrx);
                    player[cp].pjumping = PJUMPUP;
                    player[cp].pjumpupspeed_decreasetimer = 0;
                    player[cp].pjustjumped = 1;
                    player[cp].pjumpfloattimer = 0;

                    // make so if we're jumping left or right
                    // the walk code will start at full speed
                    player[cp].pwalking = 1;
                    player[cp].pwalkanimtimer = 0;
                    player[cp].pwalkframe = 1;
                    if (TileProperty[player[cp].psupportingtile][BEHAVIOR] == 3)
                    { // on ice, always jump direction facing
                      if (player[cp].pshowdir==LEFT)
                        {  player[cp].pdir=LEFT; }
                      else
                        {  player[cp].pdir=RIGHT; }
                    }
                    else
                    {
                      player[cp].pjumpdir = UP;

                    }

                    if (player[0].playcontrol[PA_X] < 0)
                    {
                   	 player[cp].pinertia_x--;
                    }
                    if (player[0].playcontrol[PA_X] > 0)
                    {
                   	 player[cp].pinertia_x++;
                    }

                    player[cp].pwalkincreasetimer = 0;
                  }
                  else
                  {
                    player[cp].pjumpframe++;
                  }
                  player[cp].pjumpanimtimer=0;
             } else player[cp].pjumpanimtimer++;
             break;
        case PJUMPUP:
        	player[cp].pinertia_x += player[cp].chargedjump;
        	player[cp].chargedjump = 0;
        case PPOGOING:
        // check for hitting a ceiling
         if (player[cp].blockedu)   // did we bonk something?
         {  // immediatly abort the jump
            player[cp].pjumping = PNOJUMP;
            g_pSound->playStereofromCoord(SOUND_KEEN_BUMPHEAD, PLAY_NOW, objects[player[cp].useObject].scrx);
         }

         // do the jump
         if (!player[cp].pjumptime)
         {
           if (player[cp].pjumpupspeed_decreasetimer>player[cp].pjumpupdecreaserate)
           {
              if (!player[cp].pjumpupspeed)
              {
                player[cp].pjumping = PNOJUMP;
              } else player[cp].pjumpupspeed--;
              player[cp].pjumpupspeed_decreasetimer=0;
           } else player[cp].pjumpupspeed_decreasetimer++;
         }
         else player[cp].pjumptime--;

         player[cp].y -= player[cp].pjumpupspeed;
         break;
    }

}

void gamepdo_falling(int cp, stCloneKeenPlus *pCKP)
{
unsigned int temp;
int objsupport;
short tilsupport;

	player[cp].pfalling = 0;         // assume not falling if not jumped to the maximum height

    // do not fall if we're jumping
    if (player[cp].pjumping)
    {
      player[cp].psemisliding = 0;
      return;
    }

    // ** determine if player should fall (nothing solid is beneath him) **

    player[cp].psupportingtile = BG_GRAY;
    player[cp].psupportingobject = 0;
    // test if tile under player is solid; if so set psupportingtile
    objsupport = checkobjsolid(player[cp].x+(4<<CSF), player[cp].y+(sprites[0].ysize<<CSF),cp);

    tilsupport = TileProperty[getmaptileat((player[cp].x>>CSF)+5, (player[cp].y>>CSF)+sprites[0].ysize)][BUP];
    if(TileProperty[getmaptileat((player[cp].x>>CSF)+4, (player[cp].y>>CSF)+sprites[0].ysize)][BEHAVIOR] >= 2&&
       TileProperty[getmaptileat((player[cp].x>>CSF)+4, (player[cp].y>>CSF)+sprites[0].ysize)][BEHAVIOR] <= 5)
    	tilsupport = 1; // This workaround prevents the player from falling through doors.

    if (!tilsupport && !objsupport)
    { // lower-left isn't solid
      objsupport = checkobjsolid(player[cp].x+(12<<CSF), player[cp].y+(sprites[0].ysize<<CSF),cp);
      tilsupport = TileProperty[getmaptileat((player[cp].x>>CSF)+10, (player[cp].y>>CSF)+sprites[0].ysize)][BUP];
      //tilsupport = tiles[getmaptileat((player[cp].x>>CSF)+12, (player[cp].y>>CSF)+sprites[0].ysize)].solidfall;
      if (!tilsupport && !objsupport)
      {  // lower-right isn't solid
         player[cp].pfalling = 1;        // so fall.
         player[cp].pjustfell = 1;
      }
      else
      {  // lower-left isn't solid but lower-right is
        if (objsupport)
        {
          player[cp].psupportingtile = PSUPPORTEDBYOBJECT;
          player[cp].psupportingobject = objsupport;
        }
      }
    }
    else
    {   // lower-left is solid
        if (objsupport)
        {
          player[cp].psupportingtile = PSUPPORTEDBYOBJECT;
          player[cp].psupportingobject = objsupport;
        }
    }

    // if not on a tile boundary, always fall, prevents being able
    // to land in the middle of a tile.
    if (!player[cp].pfalling && player[cp].psupportingtile!=PSUPPORTEDBYOBJECT)
    {
       temp = (player[cp].y>>CSF)+sprites[0].ysize;    // bottom of player
       if ((temp>>4)<<4 != temp)   // true if it's not a multiple of 16
       {
          player[cp].pfalling = 1;   // not on a tile boundary. fall.
          player[cp].pjustfell = 1;
          player[cp].psupportingtile = BG_GRAY;
          player[cp].psupportingobject = 0;
       }
    }
    // if supported by an object make sure we're at the top of
    // the object else fall
    if (!player[cp].pfalling && player[cp].psupportingtile==PSUPPORTEDBYOBJECT)
    {
       if ((player[cp].y>>CSF)+sprites[0].ysize > (objects[player[cp].psupportingobject].y>>CSF)+4)
       {
          if (!tilsupport)
          {
            player[cp].pfalling = 1;
            player[cp].pjustfell = 1;
            player[cp].psupportingtile = BG_GRAY;
            player[cp].psupportingobject = 0;
          }
       }
    }

    // the first time we land on an object, line us up to be exactly on
    // top of the object
    if (player[cp].psupportingobject && !player[cp].lastsupportingobject)
    {
       player[cp].y = objects[player[cp].psupportingobject].y - (sprites[0].ysize<<CSF);
    }
    player[cp].lastsupportingobject = player[cp].psupportingobject;

    // ** if the player should be falling, well what are we waiting for?
    //    make him fall! **
    if (pCKP->Option[OPT_CHEATS].value && g_pInput->getHoldedKey(KPLUS)) { player[cp].pfalling = 1; player[cp].pjustfell = 1; }

    if (player[cp].pfalling)
    {  // nothing solid under player, let's make him fall
       player[cp].psemisliding = 0;

       // just now started falling? (wasn't falling last time)
       if (player[cp].plastfalling == 0)
       {
         // set initial fall speed and make the AAAAAUUUUHHHHH noise
         player[cp].pfallspeed = 1;
         player[cp].pfallspeed_increasetimer = 0;
         if (!player[cp].pjustjumped)
         {
        	 g_pSound->playStereofromCoord(SOUND_KEEN_FALL, PLAY_NOW, objects[player[cp].useObject].scrx);
         }
       }

       // gradually increase the fall speed up to maximum rate
       if (player[cp].pfallspeed_increasetimer>PFALL_INCREASERATE)
       {
          if (player[cp].pfallspeed<PFALL_MAXSPEED)
          {
            player[cp].pfallspeed++;
          }
          player[cp].pfallspeed_increasetimer=0;
       } else player[cp].pfallspeed_increasetimer++;

       // add current fall speed to player Y
       player[cp].y += player[cp].pfallspeed;

    }
    else
    {  // not falling

       if (player[cp].plastfalling)
       {  // just now stopped falling
          if (player[cp].pdie != PDIE_FELLOFFMAP)
        	  g_pSound->stopSound(SOUND_KEEN_FALL);  // terminate fall noise
          // thud noise
          if (!player[cp].ppogostick)
        	  g_pSound->playStereofromCoord(SOUND_KEEN_LAND, PLAY_NOW, objects[player[cp].useObject].scrx);

          // fix "sliding" effect when you fall, go one way, then
          // before you land turn around and as you hit the ground
          // you're starting to move the other direction
          // (set inertia to 0 if it's contrary to player's current dir)
       }


    }   // close "not falling"

    // save fall state so we can detect the high/low-going edges
    player[cp].plastfalling = player[cp].pfalling;

    // ensure no sliding if we fall or jump off of ice
    if (player[cp].pfalling||player[cp].pjumping) player[cp].psliding=0;
}

// wouldn't it be cool if keen had a raygun, and he could shoot things?
// oh wait, he does, and here's the code for it.
void gamepdo_raygun(int cp, stCloneKeenPlus *pCKP)
{
int o;
int canRefire;

	stOption *p_option;
	p_option = pCKP->Option;

   if (player[cp].pfireframetimer) player[cp].pfireframetimer--;

   // FIRE button down, and not keencicled?
   if (player[cp].playcontrol[PA_FIRE] && !player[cp].pfrozentime)
   {
     player[cp].inhibitwalking = 1;    // prevent moving
     player[cp].pfiring = 1;           // flag that we're firing
     player[cp].ppogostick = false;        // put away pogo stick if out

     if (!player[cp].lastplaycontrol[PA_FIRE] || pCKP->Option[OPT_FULLYAUTOMATIC].value)
     { // fire is newly pressed

       // limit how quickly shots can be fired
       if (pCKP->Option[OPT_FULLYAUTOMATIC].value)
       {
         if (player[cp].pfireframetimer < PFIRE_LIMIT_SHOT_FREQ_FA)
         {
           canRefire = 1;
         }
         else canRefire = 0;
       }

       else
       {
         if (player[cp].pfireframetimer < PFIRE_LIMIT_SHOT_FREQ)
         {
           canRefire = 1;
         }
         else canRefire = 0;
       }

       if (canRefire)
       {
          // show raygun for a minimum time even if FIRE is immediatly released
          player[cp].pfireframetimer = PFIRE_SHOWFRAME_TIME;

          // try to fire off a blast
          if (player[cp].inventory.charges)
          {  // we have enough charges

             player[cp].inventory.charges--;
             player[cp].pshowdir = player[cp].pdir;

             g_pSound->playStereofromCoord(SOUND_KEEN_FIRE, PLAY_NOW, objects[player[cp].useObject].scrx);

             if (player[cp].pdir==RIGHT)
             {  // fire a blast to the right
                o = spawn_object(player[cp].x+((sprites[0].xsize-4)<<CSF), player[cp].y+(9<<CSF), OBJ_RAY);
                objects[o].ai.ray.direction = RIGHT;
             }
             else
             {  // fire a blast to the left
                o = spawn_object(player[cp].x-(12<<CSF), player[cp].y+(9<<CSF), OBJ_RAY);
                objects[o].ai.ray.direction = LEFT;
             }
              // if '-nopk' argument set don't kill other players
              if (p_option[OPT_ALLOWPKING].value)
              {
                objects[o].ai.ray.dontHitEnable = 0;
              }
              else
              {
                objects[o].ai.ray.dontHitEnable = 1;
                objects[o].ai.ray.dontHit = OBJ_PLAYER;
              }
          }
          else
          { // oh shit, out of bullets
            // click!
        	  g_pSound->playStereofromCoord(SOUND_GUN_CLICK, PLAY_NOW, objects[player[cp].useObject].scrx);

          }  // end "do we have charges?"

       } // end "limit how quickly shots can be fired"

     } // end "fire is newly pressed"
   } // end "fire button down and not keencicled"
   else
   { // FIRE button is NOT down
      // put away ray gun after it's shown for the minimum period of time
      if (!player[cp].pfireframetimer)
      {  // ray gun shown for minimum time
        player[cp].pfiring = 0;
      }
      else
      {  // minimum time not expired
        player[cp].pfiring = 1;
        player[cp].inhibitwalking = 1;
      }
   }

}

// select the appropriate player frame based on what he's doing
void gamepdo_SelectFrame(int cp)
{
    player[cp].playframe = 0;      // basic standing

    // select the frame assuming he's pointing right. ep1 does not select
    // a walk frame while fading--this is for the bonus teleporter in L13.
    if (player[cp].pdie) player[cp].playframe = PDIEFRAME + player[cp].pdieframe;
    else
    {
        if (player[cp].pfrozentime) player[cp].playframe = PFRAME_FROZEN + player[cp].pfrozenframe;
        else if (player[cp].pfiring) player[cp].playframe = PFIREFRAME;
        else if (player[cp].ppogostick) player[cp].playframe = PFRAME_POGO + (player[cp].pjumping==PPREPAREPOGO);
        else if (player[cp].pjumping) player[cp].playframe += player[cp].pjumpframe;
        else if (player[cp].pfalling) player[cp].playframe += 13;
        else if (player[cp].pwalking || player[cp].playpushed_x) player[cp].playframe += player[cp].pwalkframe;
    }

    // if he's going left switch the frame selected above to the
    // appropriate one for the left direction
    if (player[cp].pshowdir && !player[cp].pdie && !player[cp].pfrozentime)
    {
       if (player[cp].pfiring)
       {
          player[cp].playframe++;
       }
       else if (player[cp].ppogostick)
       {
          player[cp].playframe+=2;
       }
       else if (player[cp].pjumping || player[cp].pfalling)
       {
          player[cp].playframe+=6;
       }
       else
       {
          player[cp].playframe+=4;
       }
    }
}

// handles walking. the walking animation is handled by gamepdo_walkinganim()
void gamepdo_walking(int cp, stCloneKeenPlus *pCKP)
{
	int cur_pfastincrate;
    if (player[cp].inhibitwalking && !player[cp].psliding)
    {
      if (!player[cp].pfrozentime||pCKP->Control.levelcontrol.episode!=1)
        if (!player[cp].pjumping && !player[cp].pfalling)
          player[cp].pinertia_x = 0;
      return;
    }

    // this prevents a "slipping" effect if you jump, say, right, then
    // start walking left just as you hit the ground
    if (player[cp].pjustjumped && ((player[cp].pinertia_x > 0 && player[cp].pdir==LEFT) ||\
                        (player[cp].pinertia_x < 0 && player[cp].pdir==RIGHT)))\
    {
    	if(!player[cp].ppogostick)
    		player[cp].pinertia_x = 0;
    }

    // this code makes it so that if you jump/fall onto a semi-sliding
    // block you'll start moving a little
    if (!player[cp].pjumping && !player[cp].pfalling)
    {
      // on left/right press clear pjustjumped
      if ((player[cp].playcontrol[PA_X] < 0)||(player[cp].playcontrol[PA_X] > 0))
      {
        player[cp].pjustjumped = 0;
        player[cp].pjustfell = 0;
      }

      // if we fall onto a semislide tile with no inertia
      // don't move!.
      if (player[cp].pjustfell && player[cp].psemisliding)
      {
        if (player[cp].pdir==RIGHT)
        {
          if (player[cp].blockedr)
          {
            player[cp].pjustjumped = 0;
            player[cp].pjustfell = 0;
          }
          else
          {
            player[cp].pshowdir = player[cp].pdir;
          }
        }
        else
        {
          if (player[cp].blockedl)
          {
            player[cp].pjustjumped = 0;
            player[cp].pjustfell = 0;
          }
          else
          {
            player[cp].pshowdir = player[cp].pdir;
          }
        }
      }
    }

    // test if we're trying to walk
    if ((player[cp].psemisliding&&player[cp].pinertia_x!=0) || (((player[cp].playcontrol[PA_X] < 0) || (player[cp].playcontrol[PA_X] > 0) || (((player[cp].playcontrol[PA_Y] < 0) || (player[cp].playcontrol[PA_Y] > 0))&&map.isworldmap)) && !player[cp].inhibitwalking))
    {
      // we just started walking or we changed directions suddenly?
      if (player[cp].pwalking == 0 || ((player[cp].lastpdir==RIGHT && player[cp].pdir==LEFT)||(player[cp].lastpdir==LEFT && player[cp].pdir==RIGHT)))
      {
    	player[cp].widejump = false;
        player[cp].pwalkanimtimer = 0;
        player[cp].pwalkframe = 1;
        player[cp].pwalkincreasetimer = 0;
        player[cp].pfriction_timer_x = 0;
        player[cp].pfriction_timer_y = 0;

        if (!player[cp].pjumping && !player[cp].pfalling)
        {
            if(!player[cp].ppogostick) // Only if he stays on the ground (No pogoing)
          		player[cp].pinertia_x /= 2;
          player[cp].pinertia_y = 0;
        }

        player[cp].lastpdir = player[cp].pdir;
        player[cp].pwalking = 1;
      }
    }
    else
    {   // end "d-pad down and bit sliding"
    	if(player[cp].pinertia_x != 0)
    		player[cp].pslowingdown=1;
    	else
    	{
    		player[cp].pslowingdown=0;
    		player[cp].pwalking = 0;
    	}
    }

      /* when sliding on ice force maximum speed */
      if (player[cp].psliding)
      {
         if (player[cp].pjumping != PPREPAREJUMP &&
             player[cp].pjumping != PPREPAREPOGO)
         {
           // reset walk frame because we have no walk animation while on ice
           player[cp].pwalkframe = 0;
           // keep player sliding at maximum speed
           if (player[cp].pdir==RIGHT)
           {
             player[cp].pinertia_x = PMAXSPEED;
           }
           else if (player[cp].pdir==LEFT)
           {
             player[cp].pinertia_x = -PMAXSPEED;
           }
         }
         return;
      }
      else if (!player[cp].pwalking) return;    // don't run rest of sub if not walking
      // if we get here we're walking and not sliding

     /* increase player inertia while walk key held down */
     if (player[cp].ppogostick)
        cur_pfastincrate = PFASTINCRATE_POGO;
      else
        cur_pfastincrate = PFASTINCRATE;

	  if((player[cp].pjumpdir != UP) && (player[cp].pjumping != 0))
	  {
		  return;
	  }

      if (player[cp].playcontrol[PA_X] > 0)
      { // RIGHT key down

          // quickly reach PFASTINCMAXSPEED
          if (player[cp].pwalkincreasetimer>=cur_pfastincrate && player[cp].pinertia_x<PFASTINCMAXSPEED)
          {
             player[cp].pinertia_x++;
             player[cp].pwalkincreasetimer=0;
          }
          else
          {
             player[cp].pwalkincreasetimer++;
          }

          // prevent sliding on map
          if (map.isworldmap )
          {
        	  if(player[cp].pinertia_x < 0)
        	  {
        		  player[cp].pinertia_x = 0;
        	  }
        	  else
        	  {
        		  player[cp].pinertia_x=player[cp].playcontrol[PA_X]*PFASTINCMAXSPEED/100;
        	  }
          }

          // increase up to max speed every time frame is changed
          if (!player[cp].pwalkanimtimer && player[cp].pinertia_x < PMAXSPEED)
          {
             player[cp].pinertia_x++;
          }
      }
      else if (player[cp].playcontrol[PA_X] < 0)
      {
          // quickly reach PFASTINCMAXSPEED
          if (player[cp].pwalkincreasetimer>=cur_pfastincrate && player[cp].pinertia_x>-PFASTINCMAXSPEED)
          {
             player[cp].pinertia_x--;
             player[cp].pwalkincreasetimer=0;
          }
          else
          {
             player[cp].pwalkincreasetimer++;
          }

          // prevent sliding on map
          if (map.isworldmap )
          {
        	  if(player[cp].pinertia_x > 0)
        	  {
        		  player[cp].pinertia_x = 0;
        	  }
        	  else
        	  {
        		  player[cp].pinertia_x=player[cp].playcontrol[PA_X]*PFASTINCMAXSPEED/100;
        	  }
          }
          // increase up to max speed every time frame is changed
          if (!player[cp].pwalkanimtimer && player[cp].pinertia_x>-PMAXSPEED)
          {
             player[cp].pinertia_x--;
          }
      }

      if (player[cp].playcontrol[PA_Y] > 0)
      {
          // quickly reach PFASTINCMAXSPEED
          if (player[cp].pwalkincreasetimer>=PFASTINCRATE && player[cp].pinertia_y<PFASTINCMAXSPEED)
          {
             player[cp].pinertia_y++;
             player[cp].pwalkincreasetimer=0;
          }
          else
          {
             player[cp].pwalkincreasetimer++;
          }
          // increase up to max speed every time frame is changed
          if (!player[cp].pwalkanimtimer && player[cp].pinertia_y<PMAXSPEED)
          {
             player[cp].pinertia_y++;
          }

          // prevent sliding on map
          if (map.isworldmap )
          {
        	  if(player[cp].pinertia_y < 0)
        	  {
        		  player[cp].pinertia_y = 0;
        	  }
        	  else
        	  {
        		  player[cp].pinertia_y=player[cp].playcontrol[PA_Y]*PFASTINCMAXSPEED/100;
        	  }
          }

      }
      else if (player[cp].playcontrol[PA_Y] < 0)
      {
          // quickly reach PFASTINCMAXSPEED
          if (player[cp].pwalkincreasetimer>=PFASTINCRATE && player[cp].pinertia_y>-PFASTINCMAXSPEED)
          {
             player[cp].pinertia_y--;
             player[cp].pwalkincreasetimer=0;
          }
          else
          {
             player[cp].pwalkincreasetimer++;
          }
          // increase up to max speed every time frame is changed
          if (!player[cp].pwalkanimtimer && player[cp].pinertia_y>-PMAXSPEED)
          {
             player[cp].pinertia_y--;
          }

          // prevent sliding on map
          if (map.isworldmap )
          {
        	  if(player[cp].pinertia_y > 0)
        	  {
        		  player[cp].pinertia_y = 0;
        	  }
        	  else
        	  {
        		  player[cp].pinertia_y=player[cp].playcontrol[PA_Y]*PFASTINCMAXSPEED/100;
        	  }
          }
      }

}

void gamepdo_ankh(int cp)
{
int o;
  if (!player[cp].ankhtime) return;

  o = player[cp].ankhshieldobject;
  objects[o].x = player[cp].x - (8<<CSF);
  objects[o].y = player[cp].y - (8<<CSF);

  player[cp].ankhtime--;
  if (!player[cp].ankhtime)
  {
    objects[o].exists = 0;
  }
  else if (player[cp].ankhtime < ANKH_STAGE3_TIME)
  {
    objects[o].ai.se.state = ANKH_STATE_FLICKERSLOW;
  }
  else if (player[cp].ankhtime < ANKH_STAGE2_TIME)
  {
    objects[o].ai.se.state = ANKH_STATE_FLICKERFAST;
  }
  else
  {
    objects[o].ai.se.state = ANKH_STATE_NOFLICKER;
  }
}

void gamepdo_StatusBox(int cp, stCloneKeenPlus *pCKP)
{
  if (fade.mode != NO_FADE) return;

  if(player[cp].playcontrol[PA_STATUS] && !player[cp].lastplaycontrol[PA_STATUS])
  {
	  showinventory(cp, pCKP);
  }

}

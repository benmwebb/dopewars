/* AIPlayer.h   Header file for dopewars computer player code           */
/* Copyright (C)  1998-2000  Ben Webb                                   */
/*                Email: ben@bellatrix.pcl.ox.ac.uk                     */
/*                WWW: http://bellatrix.pcl.ox.ac.uk/~ben/dopewars/     */

/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU General Public License          */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */

/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */

/* You should have received a copy of the GNU General Public License    */
/* along with this program; if not, write to the Free Software          */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston,               */
/*                   MA  02111-1307, USA.                               */


#ifndef __AIPLAYER_H__
#define __AIPLAYER_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

void AIPlayerLoop(void);

#if NETWORKING
int HandleAIMessage(char *Message,Player *AIPlay);
void PrintAIMessage(char *Text);
void AIDealDrugs(Player *AIPlay);
void AIJet(Player *AIPlay);
void AIHandleQuestion(char *Data,char AICode,Player *AIPlay,Player *From);
void AIGunShop(Player *AIPlay);
void AIPayLoan(Player *AIPlay);
void AISendRandomMessage(Player *AIPlay);
void AISetName(Player *AIPlay);
#endif /* NETWORKING */

#endif

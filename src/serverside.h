/************************************************************************
 * serverside.h   Server-side parts of dopewars                         *
 * Copyright (C)  1998-2002  Ben Webb                                   *
 *                Email: ben@bellatrix.pcl.ox.ac.uk                     *
 *                WWW: http://dopewars.sourceforge.net/                 *
 *                                                                      *
 * This program is free software; you can redistribute it and/or        *
 * modify it under the terms of the GNU General Public License          *
 * as published by the Free Software Foundation; either version 2       *
 * of the License, or (at your option) any later version.               *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, write to the Free Software          *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,               *
 *                   MA  02111-1307, USA.                               *
 ************************************************************************/

#ifndef __SERVERSIDE_H__
#define __SERVERSIDE_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dopewars.h"

extern GSList *FirstServer;
extern char *PidFile;

void CleanUpServer(void);
void BreakHandle(int sig);
void ClientLeftServer(Player *Play);
void StopServer(void);
Player *HandleNewConnection(void);
void ServerLoop(void);
void HandleServerPlayer(Player *Play);
void HandleServerMessage(gchar *buf, Player *ReallyFrom);
void FinishGame(Player *Play, char *Message);
void SendHighScores(Player *Play, gboolean EndGame, char *Message);
void SendEvent(Player *To);
void SendDrugsHere(Player *To, gboolean DisplayBusts);
void BuyObject(Player *From, char *data);
int RandomOffer(Player *To);
void HandleAnswer(Player *From, Player *To, char *answer);
void ClearPrices(Player *Play);
int LoseBitch(Player *Play, Inventory *Guns, Inventory *Drugs);
void GainBitch(Player *Play);
void SetFightTimeout(Player *Play);
void ClearFightTimeout(Player *Play);
int GetMinimumTimeout(GSList *First);
GSList *HandleTimeouts(GSList *First);
void ConvertHighScoreFile(void);
void OpenHighScoreFile(void);
gboolean CheckHighScoreFileConfig(void);
void CloseHighScoreFile(void);
gboolean HighScoreRead(FILE *fp, struct HISCORE *MultiScore,
                       struct HISCORE *AntiqueScore, gboolean ReadHeader);
void CopsAttackPlayer(Player *Play);
void AttackPlayer(Player *Play, Player *Attacked);
gboolean IsOpponent(Player *Play, Player *Other);
void Fire(Player *Play);
void WithdrawFromCombat(Player *Play);
void RunFromCombat(Player *Play, int ToLocation);
gboolean CanPlayerFire(Player *Play);
gboolean CanRunHere(Player *Play);
Player *GetNextShooter(Player *Play);
void DropPrivileges(void);

#ifdef GUI_SERVER
void GuiServerLoop(gboolean is_service);
#ifdef CYGWIN
void ServiceMain(void);
#endif
#endif
#ifndef CYGWIN
gchar *GetLocalSocket(void);
#endif

#endif

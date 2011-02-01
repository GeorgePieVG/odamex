// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2010 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Moving object handling. Spawn functions.
//
//-----------------------------------------------------------------------------

#include "m_alloc.h"
#include "i_system.h"
#include "z_zone.h"
#include "m_random.h"
#include "doomdef.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "s_sound.h"
#include "doomstat.h"
#include "v_video.h"
#include "c_cvars.h"
#include "vectors.h"
#include "g_game.h"
#include "p_mobj.h"

#define WATER_SINK_FACTOR		3
#define WATER_SINK_SMALL_FACTOR	4
#define WATER_SINK_SPEED		(FRACUNIT/2)
#define WATER_JUMP_SPEED		(FRACUNIT*7/2)

extern bool predicting;
extern fixed_t attackrange;

void P_ExplodeMissile(AActor* mo);
void SV_SpawnMobj(AActor *mobj);
void SV_SendDestroyActor(AActor *);
void SV_ExplodeMissile(AActor *);

EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(sv_itemsrespawn)
EXTERN_CVAR(sv_itemrespawntime)

mapthing2_t     itemrespawnque[ITEMQUESIZE];
int             itemrespawntime[ITEMQUESIZE];
int             iquehead;
int             iquetail;

NetIDHandler ServerNetID;

IMPLEMENT_SERIAL(AActor, DThinker)

AActor::~AActor ()
{
    // Please avoid calling the destructor directly (or through delete)!
    // Use Destroy() instead.

    // Zero all pointers generated by this->ptr()
    self.update_all(NULL);
}

void MapThing::Serialize (FArchive &arc)
{
	if (arc.IsStoring ())
	{
		arc << thingid << x << y << z << angle << type << flags << special
			<< args[0] << args[1] << args[2] << args[3] << args[4];
	}
	else
	{
		arc >> thingid >> x >> y >> z >> angle >> type >> flags >> special
			>> args[0] >> args[1] >> args[2] >> args[3] >> args[4];
	}
}

AActor::AActor () :
    x(0), y(0), z(0), snext(NULL), sprev(NULL), angle(0), sprite(SPR_UNKN), frame(0),
    pitch(0), roll(0), effects(0), bnext(NULL), bprev(NULL), subsector(NULL),
    floorz(0), ceilingz(0), radius(0), height(0), momx(0), momy(0), momz(0),
    validcount(0), type(MT_UNKNOWNTHING), info(NULL), tics(0), state(NULL), 
    flags(0), flags2(0), special1(0), special2(0), health(0), movedir(0), movecount(0),
    visdir(0), reactiontime(0), threshold(0), player(NULL), lastlook(0), special(0), inext(NULL),
    iprev(NULL), translation(NULL), translucency(0), waterlevel(0), onground(0),
    touching_sectorlist(NULL), deadtic(0), oldframe(0), rndindex(0), netid(0), 
    tid(0)
{
	self.init(this);
} 

AActor::AActor (const AActor &other) :
    x(other.x), y(other.y), z(other.z), snext(other.snext), sprev(other.sprev),
    angle(other.angle), sprite(other.sprite), frame(other.frame),
    pitch(other.pitch), roll(other.roll), effects(other.effects),
    bnext(other.bnext), bprev(other.bprev), subsector(other.subsector),
    floorz(other.floorz), ceilingz(other.ceilingz), radius(other.radius),
    height(other.height), momx(other.momx), momy(other.momy), momz(other.momz),
    validcount(other.validcount), type(other.type), info(other.info),
    tics(other.tics), state(other.state), flags(other.flags), flags2(other.flags2),
    special1(other.special1), special2(other.special2),
    health(other.health), movedir(other.movedir), movecount(other.movecount),
    visdir(other.visdir), reactiontime(other.reactiontime),
    threshold(other.threshold), player(other.player), lastlook(other.lastlook),
    special(other.special),inext(other.inext), iprev(other.iprev), translation(other.translation),
    translucency(other.translucency), waterlevel(other.waterlevel),
    onground(other.onground), touching_sectorlist(other.touching_sectorlist),
    deadtic(other.deadtic), oldframe(other.oldframe), 
    rndindex(other.rndindex), netid(other.netid), tid(other.tid)
{
	self.init(this);
}

AActor &AActor::operator= (const AActor &other)
{
	x = other.x;
    y = other.y;
    z = other.z;
    snext = other.snext;
    sprev = other.sprev;
    angle = other.angle;
    sprite = other.sprite;
    frame = other.frame;
    pitch = other.pitch;
    roll = other.roll;
    effects = other.effects;
    bnext = other.bnext;
    bprev = other.bprev;
    subsector = other.subsector;
    floorz = other.floorz;
    ceilingz = other.ceilingz;
    radius = other.radius;
    height = other.height;
    momx = other.momx;
    momy = other.momy;
    momz = other.momz;
    validcount = other.validcount;
    type = other.type;
    info = other.info;
    tics = other.tics;
    state = other.state;
    flags = other.flags;
    flags2 = other.flags2;
    special1 = other.special1;
    special2 = other.special2;
    health = other.health;
    movedir = other.movedir;
    movecount = other.movecount;
    visdir = other.visdir;
    reactiontime = other.reactiontime;
    threshold = other.threshold;
    player = other.player;
    lastlook = other.lastlook;
    inext = other.inext;
    iprev = other.iprev;
    translation = other.translation;
    translucency = other.translucency;
    waterlevel = other.waterlevel;
    onground = other.onground;
    touching_sectorlist = other.touching_sectorlist;
    deadtic = other.deadtic;
    oldframe = other.oldframe;
    rndindex = other.rndindex;
    netid = other.netid;
    tid = other.tid;
    special = other.special;

	return *this;
}

//
//
// P_SpawnMobj
//
//

AActor::AActor (fixed_t ix, fixed_t iy, fixed_t iz, mobjtype_t itype) :
    x(0), y(0), z(0), snext(NULL), sprev(NULL), angle(0), sprite(SPR_UNKN), frame(0),
    pitch(0), roll(0), effects(0), bnext(NULL), bprev(NULL), subsector(NULL),
    floorz(0), ceilingz(0), radius(0), height(0), momx(0), momy(0), momz(0),
    validcount(0), type(MT_UNKNOWNTHING), info(NULL), tics(0), state(NULL), flags(0), flags2(0),
    special1(0), special2(0), health(0), movedir(0), movecount(0), visdir(0),
    reactiontime(0), threshold(0), player(NULL), lastlook(0), inext(NULL),
    iprev(NULL), translation(NULL), translucency(0), waterlevel(0), onground(0),
    touching_sectorlist(NULL), deadtic(0), oldframe(0), rndindex(0), netid(0),
    tid(0)
{
	state_t *st;

	// Fly!!! fix it in P_RespawnSpecial
	if ((unsigned int)itype >= NUMMOBJTYPES)
	{
		I_Error ("Tried to spawn actor type %d\n", itype);
	}

	self.init(this);
	info = &mobjinfo[itype];
	type = itype;
	x = ix;
	y = iy;
	radius = info->radius;
	height = info->height;
	flags = info->flags;
	flags2 = info->flags2;
	health = info->spawnhealth;
	translucency = info->translucency;
	rndindex = M_Random();

    if (multiplayer && serverside)
        netid = ServerNetID.ObtainNetID();

	if (sv_skill != sk_nightmare)
		reactiontime = info->reactiontime;

    if (clientside)
        lastlook = P_Random() % MAXPLAYERS_VANILLA;
    else
        lastlook = P_Random() % MAXPLAYERS;

    // do not set the state with P_SetMobjState,
    // because action routines can not be called yet
	st = &states[info->spawnstate];
	state = st;
	tics = st->tics;
	sprite = st->sprite;
	frame = st->frame;
	touching_sectorlist = NULL;	// NULL head of sector list // phares 3/13/98

	// set subsector and/or block links
	LinkToWorld ();

	if(!subsector)
		return;

	floorz = subsector->sector->floorheight;
	ceilingz = subsector->sector->ceilingheight;

	if (iz == ONFLOORZ)
	{
		z = floorz;
	}
	else if (iz == ONCEILINGZ)
	{
		z = ceilingz - height;
	}
	else if (flags2 & MF2_FLOATBOB)
	{
		z = floorz + iz;		// artifact z passed in as height
	}
	else
	{
		z = iz;
	}
}


//
// P_RemoveMobj
//
void AActor::Destroy ()
{
	SV_SendDestroyActor(this);

    // Add special to item respawn queue if it is destined to be respawned
	if ((flags & MF_SPECIAL) && !(flags & MF_DROPPED))
	{
		if (type != MT_INV && type != MT_INS && 
            (type < MT_BSOK || type > MT_RDWN))
		{
			itemrespawnque[iquehead] = spawnpoint;
			itemrespawntime[iquehead] = level.time;
			iquehead = (iquehead+1)&(ITEMQUESIZE-1);

			// lose one off the end?
			if (iquehead == iquetail)
				iquetail = (iquetail+1)&(ITEMQUESIZE-1);
		}
	}
	
	// [RH] Unlink from tid chain
	RemoveFromHash ();

	// unlink from sector and block lists
	UnlinkFromWorld ();

	// Delete all nodes on the current sector_list			phares 3/16/98
	if (sector_list)
	{
		P_DelSeclist (sector_list);
		sector_list = NULL;
	}

	// stop any playing sound
    if (clientside)
		S_RelinkSound (this, NULL);

	Super::Destroy ();
}

//
//
// P_SetMobjState
//
// Returns true if the mobj is still present.
//
//
bool P_SetMobjState(AActor *mobj, statenum_t state)
{
    state_t*	st;

	// denis - prevent harmful state cycles
	static unsigned int callstack;
	if(callstack++ > 16)
	{
		callstack = 0;
		I_Error("P_SetMobjState: callstack depth exceeded bounds");
	}

    do
    {
		if (state == S_NULL)
		{
			mobj->state = (state_t *) S_NULL;
			mobj->Destroy();

			callstack--;
			return false;
		}

		st = &states[state];
		mobj->state = st;
		mobj->tics = st->tics;
		mobj->sprite = st->sprite;
		mobj->frame = st->frame;

		// Modified handling.
		// Call action functions when the state is set
		if (st->action.acp1)
			st->action.acp1(mobj);

		state = st->nextstate;
    } while (!mobj->tics);

	callstack--;
    return true;
}

//
// P_XYMovement
//
void P_XYMovement(AActor *mo)
{
//	angle_t angle;
	fixed_t ptryx, ptryy;
	player_t *player = NULL;
	fixed_t xmove, ymove;

	if (!mo->subsector)
		return;

	if (!mo->momx && !mo->momy)
	{
		if (mo->flags & MF_SKULLFLY)
		{
			// the skull slammed into something
			mo->flags &= ~MF_SKULLFLY;
			mo->momx = mo->momy = mo->momz = 0;

			P_SetMobjState (mo, mo->info->spawnstate);
		}
		return;
	}

	player = mo->player;

	if(!player || !player->mo)
		player = NULL;

	int maxmove = (mo->waterlevel < 2) || (mo->flags & MF_MISSILE) ? MAXMOVE : MAXMOVE/4;

	if (mo->momx > maxmove)
		mo->momx = maxmove;
	else if (mo->momx < -maxmove)
		mo->momx = -maxmove;

	if (mo->momy > maxmove)
		mo->momy = maxmove;
	else if (mo->momy < -maxmove)
		mo->momy = -maxmove;

	xmove = mo->momx;
	ymove = mo->momy;

	maxmove /= 2;

	do
	{
		if (xmove > maxmove || ymove > maxmove )
		{
			ptryx = mo->x + xmove/2;
			ptryy = mo->y + ymove/2;
			xmove >>= 1;
			ymove >>= 1;
		}
		else
		{
			ptryx = mo->x + xmove;
			ptryy = mo->y + ymove;
			xmove = ymove = 0;
		}

		// killough 3/15/98: Allow objects to drop off
		if (!P_TryMove (mo, ptryx, ptryy, true))
		{
            if (mo->flags2 & MF2_SLIDE)
			{
				// try to slide along it
				if (BlockingMobj == NULL)
				{ // slide against wall
					if (mo->player && mo->waterlevel && mo->waterlevel < 3
						&& (mo->player->cmd.ucmd.forwardmove | mo->player->cmd.ucmd.sidemove))
					{
						mo->momz = WATER_JUMP_SPEED;
					}
					P_SlideMove (mo);
				}
				else
				{ // slide against mobj
					if (P_TryMove (mo, mo->x, ptryy, true))
					{
						mo->momx = 0;
					}
					else if (P_TryMove (mo, ptryx, mo->y, true))
					{
						mo->momy = 0;
					}
					else
					{
						mo->momx = mo->momy = 0;
					}
				}
			}
			else if (mo->flags & MF_MISSILE)
			{
				// explode a missile
				if (ceilingline &&
					ceilingline->backsector &&
					ceilingline->backsector->ceilingpic == skyflatnum)
				{
					// Hack to prevent missiles exploding
					// against the sky.
					// Does not handle sky floors.
					mo->Destroy ();
					return;
				}
				P_ExplodeMissile (mo);
			}
			else
			{
				mo->momx = mo->momy = 0;
			}
		}
	} while (xmove || ymove);

    // slow down
	if (player && player->mo == mo && player->cheats & CF_NOMOMENTUM)
	{
		// debug option for no sliding at all
		mo->momx = mo->momy = 0;
		return;
	}

	if (mo->flags & (MF_MISSILE | MF_SKULLFLY))
    {
		return; 	// no friction for missiles ever
    }

	if (mo->z > mo->floorz && !(mo->flags2 & MF2_ONMOBJ) && !(mo->flags2 & MF2_FLY)
        && !mo->waterlevel)
		return;		// no friction when airborne

	if (mo->flags & MF_CORPSE)
	{
		// do not stop sliding
		//  if halfway off a step with some momentum
		if (mo->momx > FRACUNIT/4
			|| mo->momx < -FRACUNIT/4
			|| mo->momy > FRACUNIT/4
			|| mo->momy < -FRACUNIT/4)
		{
			if (mo->floorz != mo->subsector->sector->floorheight)
				return;
		}
	}

	// killough 11/98:
	// Stop voodoo dolls that have come to rest, despite any
	// moving corresponding player:
	if (mo->momx > -STOPSPEED && mo->momx < STOPSPEED
		&& mo->momy > -STOPSPEED && mo->momy < STOPSPEED
		&& (!player || (player->mo != mo)
		|| !(player->cmd.ucmd.forwardmove | player->cmd.ucmd.sidemove)))
	{
		// if in a walking frame, stop moving
		// killough 10/98:
		// Don't affect main player when voodoo dolls stop:
		if (player && (unsigned)((player->mo->state - states) - S_PLAY_RUN1) < 4
			&& (player->mo == mo))
		{
			P_SetMobjState (player->mo, S_PLAY);
		}

		mo->momx = mo->momy = 0;
	}
	else
	{
		// phares 3/17/98
		// Friction will have been adjusted by friction thinkers for icy
		// or muddy floors. Otherwise it was never touched and
		// remained set at ORIG_FRICTION
		//
		// killough 8/28/98: removed inefficient thinker algorithm,
		// instead using touching_sectorlist in P_GetFriction() to
		// determine friction (and thus only when it is needed).
		//
		// killough 10/98: changed to work with new bobbing method.
		// Reducing player momentum is no longer needed to reduce
		// bobbing, so ice works much better now.

		fixed_t friction = P_GetFriction (mo, NULL);

		mo->momx = FixedMul (mo->momx, friction);
		mo->momy = FixedMul (mo->momy, friction);
	}
}

//
// P_ZMovement
// joek - from choco with love
//
void P_ZMovement(AActor *mo)
{
   fixed_t	dist;
   fixed_t	delta;

    // check for smooth step up
   if (mo->player && mo->z < mo->floorz)
   {
      mo->player->viewheight -= mo->floorz-mo->z;

      mo->player->deltaviewheight
            = (VIEWHEIGHT - mo->player->viewheight)>>3;
   }

    // adjust height
    // GhostlyDeath <Jun, 4 2008> -- Floating monsters shouldn't adjust to spectator height
   mo->z += mo->momz;

   if ( mo->flags & MF_FLOAT
        && mo->target && !(mo->target->player && mo->target->player->spectator))
   {
	// float down towards target if too close
      if ( !(mo->flags & MF_SKULLFLY)
             && !(mo->flags & MF_INFLOAT) )
      {
         dist = P_AproxDistance (mo->x - mo->target->x,
                                 mo->y - mo->target->y);

         delta =(mo->target->z + (mo->height>>1)) - mo->z;

         if (delta<0 && dist < -(delta*3) )
            mo->z -= FLOATSPEED;
         else if (delta>0 && dist < (delta*3) )
            mo->z += FLOATSPEED;
      }

   }
	if (mo->player && (mo->flags2 & MF2_FLY) && (mo->z > mo->floorz))
	{
		mo->z += finesine[(FINEANGLES/80*level.time)&FINEMASK]/8;
		mo->momz = FixedMul (mo->momz, FRICTION_FLY);
	}

    // clip movement
   if (mo->z <= mo->floorz)
   {
	// hit the floor

	// Note (id):
	//  somebody left this after the setting momz to 0,
	//  kinda useless there.
      //
	// cph - This was the a bug in the linuxdoom-1.10 source which
	//  caused it not to sync Doom 2 v1.9 demos. Someone
	//  added the above comment and moved up the following code. So
	//  demos would desync in close lost soul fights.
	// Note that this only applies to original Doom 1 or Doom2 demos - not
	//  Final Doom and Ultimate Doom.  So we test demo_compatibility *and*
	//  gamemission. (Note we assume that Doom1 is always Ult Doom, which
	//  seems to hold for most published demos.)
      //
        //  fraggle - cph got the logic here slightly wrong.  There are three
        //  versions of Doom 1.9:
      //
        //  * The version used in registered doom 1.9 + doom2 - no bounce
        //  * The version used in ultimate doom - has bounce
        //  * The version used in final doom - has bounce
      //
        // So we need to check that this is either retail or commercial
        // (but not doom2)

      int correct_lost_soul_bounce = (gamemode == retail) ||
                                     ((gamemode == commercial
                                     && (gamemission == pack_tnt ||
                                         gamemission == pack_plut)));

      if (correct_lost_soul_bounce && mo->flags & MF_SKULLFLY)
      {
	    // the skull slammed into something
        mo->momz = -mo->momz;
      }

      if (mo->momz < 0)
      {
         
         if (mo->player)
         {
             mo->player->jumpTics = 7;	// delay any jumping for a short while
             if (mo->momz < -GRAVITY*8 && !(mo->player->spectator))
             {
                // Squat down.
                // Decrease viewheight for a moment
                // after hitting the ground (hard),
                // and utter appropriate sound.
                mo->player->deltaviewheight = mo->momz>>3;

                if (clientside && !predicting)
                    S_Sound (mo, CHAN_AUTO, "*land1", 1, ATTN_NORM);
            }
         }
         mo->momz = 0;
      }
      mo->z = mo->floorz;


	// cph 2001/05/26 -
	// See lost soul bouncing comment above. We need this here for bug
	// compatibility with original Doom2 v1.9 - if a soul is charging and
	// hit by a raising floor this incorrectly reverses its Y momentum.
      //

      if (!correct_lost_soul_bounce && mo->flags & MF_SKULLFLY)
         mo->momz = -mo->momz;

      if ( (mo->flags & MF_MISSILE)
            && !(mo->flags & MF_NOCLIP) )
      {
         P_ExplodeMissile (mo);
         return;
      }
   }
   else if (! (mo->flags & MF_NOGRAVITY) )
   {
      if (mo->momz == 0)
         mo->momz = -GRAVITY*2;
      else
         mo->momz -= GRAVITY;
   }

   if (mo->z + mo->height > mo->ceilingz)
   {
	// hit the ceiling
      if (mo->momz > 0)
         mo->momz = 0;
      {
         mo->z = mo->ceilingz - mo->height;
      }

      if (mo->flags & MF_SKULLFLY)
      {	// the skull slammed into something
         mo->momz = -mo->momz;
      }

      if ( (mo->flags & MF_MISSILE)
            && !(mo->flags & MF_NOCLIP) )
      {
         P_ExplodeMissile (mo);
         return;
      }
   }
}

//
// PlayerLandedOnThing
//
void PlayerLandedOnThing(AActor *mo, AActor *onmobj)
{
	mo->player->deltaviewheight = mo->momz>>3;
	S_Sound (mo, CHAN_AUTO, "*land1", 1, ATTN_IDLE);
//	mo->player->centering = true;
}

//
// P_NightmareRespawn
//
void P_NightmareRespawn (AActor *mobj)
{
    fixed_t         x;
    fixed_t         y;
    fixed_t         z;
    subsector_t*    ss;
    mapthing2_t*    mthing;
    AActor          *mo;

    x = mobj->spawnpoint.x << FRACBITS;
    y = mobj->spawnpoint.y << FRACBITS;

    // something is occupying it's position?
    if (!P_CheckPosition (mobj, x, y))
		return; // no respawn

    // spawn a teleport fog at old spot
    // because of removal of the body?
	mo = new AActor(
        mobj->x,
        mobj->y,
        mobj->subsector->sector->floorheight,
        MT_TFOG
    );
	// initiate teleport sound
    if (clientside)
        S_Sound (mo, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);

    // spawn a teleport fog at the new spot
    ss = R_PointInSubsector (x,y);

	// spawn a teleport fog at the new spot
    mo = new AActor (x, y, ss->sector->floorheight , MT_TFOG);
    if (clientside)
        S_Sound (mo, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);

    // spawn the new monster
    mthing = &mobj->spawnpoint;

    // spawn it
    if (mobj->info->flags & MF_SPAWNCEILING)
		z = ONCEILINGZ;
	else if (mobj->info->flags2 & MF2_FLOATBOB)
		z = mthing->z << FRACBITS;		
    else
		z = ONFLOORZ;

	// spawn it
	// inherit attributes from deceased one
	if(serverside)
	{
		mo = new AActor (x, y, z, mobj->type);
		mo->spawnpoint = mobj->spawnpoint;
		mo->angle = ANG45 * (mthing->angle/45);

		if (mthing->flags & MTF_AMBUSH)
			mo->flags |= MF_AMBUSH;

        if (serverside)
            SV_SpawnMobj(mo);

		mo->reactiontime = 18;
	}

	// remove the old monster,
	mobj->Destroy ();
}

AActor *AActor::TIDHash[128];

//
// [RH] Some new functions to work with Thing IDs. ------->
//

//
// P_ClearTidHashes
//
// Clears the tid hashtable.
//

void AActor::ClearTIDHashes ()
{
	int i;

	for (i = 0; i < 128; i++)
		TIDHash[i] = NULL;
}

//
// P_AddMobjToHash
//
// Inserts an mobj into the correct chain based on its tid.
// If its tid is 0, this function does nothing.
//
void AActor::AddToHash ()
{
	if (tid == 0)
	{
		inext = iprev = NULL;
		return;
	}
	else
	{
		int hash = TIDHASH (tid);

		inext = TIDHash[hash];
		iprev = NULL;
		TIDHash[hash] = this;
	}
}

//
// P_RemoveMobjFromHash
//
// Removes an mobj from its hash chain.
//
void AActor::RemoveFromHash ()
{
	if (tid == 0)
		return;
	else
	{
		if (iprev == NULL)
		{
			// First mobj in the chain (probably)
			int hash = TIDHASH(tid);

			if (TIDHash[hash] == this)
				TIDHash[hash] = inext;
			if (inext)
			{
				inext->iprev = NULL;
				inext = NULL;
			}
		}
		else
		{
			// Not the first mobj in the chain
			iprev->inext = inext;
			if (inext)
			{
				inext->iprev = iprev;
				inext = NULL;
			}
			iprev = NULL;
		}
	}
}

//
// P_FindMobjByTid
//
// Returns the next mobj with the tid after the one given,
// or the first with that tid if no mobj is passed. Returns
// NULL if there are no more.
//
AActor *AActor::FindByTID (int tid) const
{
	return FindByTID (this, tid);
}

AActor *AActor::FindByTID (const AActor *actor, int tid)
{
	// Mobjs without tid are never stored.
	if (tid == 0)
		return NULL;

	if (!actor)
		actor = TIDHash[TIDHASH(tid)];
	else
		actor = actor->inext;

	while (actor && actor->tid != tid)
		actor = actor->inext;

	return const_cast<AActor *>(actor);
}

//
// P_FindGoal
//
// Like FindByTID except it also matches on type.
//
AActor *AActor::FindGoal (int tid, int kind) const
{
	return FindGoal (this, tid, kind);
}

AActor *AActor::FindGoal (const AActor *actor, int tid, int kind)
{
	do
	{
		actor = FindByTID (actor, tid);
	} while (actor && actor->type != kind);

	return const_cast<AActor *>(actor);
}

// <------- [RH] End new functions

//
// GAME SPAWN FUNCTIONS
//

//
// P_SpawnPuff
//
void P_SpawnPuff (fixed_t x, fixed_t y, fixed_t z, angle_t dir, int updown)
{
    if (!serverside)
        return;

    AActor *puff;

	z += (P_RandomDiff () << 10);

    puff = new AActor(x, y, z, MT_PUFF);
    puff->momz = FRACUNIT;
    puff->tics -= P_Random(puff) & 3;

    if (puff->tics < 1)
        puff->tics = 1;

	// don't make punches spark on the wall
	if (attackrange == MELEERANGE)
        P_SetMobjState(puff, S_PUFF3);
    if (serverside)
        SV_SpawnMobj(puff);
}

//
// P_SpawnBlood
//
void P_SpawnBlood (fixed_t x, fixed_t y, fixed_t z, angle_t dir, int damage)
{
	// denis - not clientside
	if(!serverside)
		return;

	AActor *th;

	z += P_RandomDiff () << 10;
	th = new AActor (x, y, z, MT_BLOOD);
	th->momz = FRACUNIT*2;
	th->tics -= P_Random (th) & 3;

	if (th->tics < 1)
		th->tics = 1;

	if (damage <= 12 && damage >= 9)
		P_SetMobjState (th, S_BLOOD2);
	else if (damage < 9)
		P_SetMobjState (th, S_BLOOD3);
    if (serverside)
        SV_SpawnMobj(th);
}

//
// P_CheckMissileSpawn
// Moves the missile forward a bit
//	and possibly explodes it right there.
//
bool P_CheckMissileSpawn (AActor* th)
{
	th->tics -= P_Random (th) & 3;
	if (th->tics < 1)
		th->tics = 1;

	// move a little forward so an angle can
	// be computed if it immediately explodes
	th->x += th->momx>>1;
	th->y += th->momy>>1;
	th->z += th->momz>>1;

	// killough 3/15/98: no dropoff (really = don't care for missiles)

	if (!P_TryMove (th, th->x, th->y, false))
	{
		P_ExplodeMissile (th);
		return false;
	}
	return true;
}

//
// P_SpawnMissile
//
AActor* P_SpawnMissile (AActor *source, AActor *dest, mobjtype_t type)
{
    AActor *th;
    angle_t	an;
    int		dist;
    fixed_t     dest_x, dest_y, dest_z, dest_flags;

	// denis: missile spawn code from chocolate doom
	//
    // fraggle: This prevents against *crashes* when dest == NULL.
    // For example, when loading a game saved when a mancubus was
    // in the middle of firing, mancubus->target == NULL.  SpawnMissile
    // then gets called with dest == NULL.
    //
    // However, this is not the *correct* behavior.  At the moment,
    // the missile is aimed at 0,0,0.  In reality, monsters seem to aim
    // somewhere else.

    if (dest)
    {
        dest_x = dest->x;
        dest_y = dest->y;
        dest_z = dest->z;
        dest_flags = dest->flags;
    }
    else
    {
        dest_x = 0;
        dest_y = 0;
        dest_z = 0;
        dest_flags = 0;
    }

	th = new AActor (source->x, source->y, source->z + 4*8*FRACUNIT, type);

    if (th->info->seesound)
		S_Sound (th, CHAN_VOICE, th->info->seesound, 1, ATTN_NORM);

    th->target = source->ptr();	// where it came from
    an = P_PointToAngle (source->x, source->y, dest_x, dest_y);

    // fuzzy player
    if (dest_flags & MF_SHADOW)
		an += P_RandomDiff()<<20;

    th->angle = an;
    an >>= ANGLETOFINESHIFT;
    th->momx = FixedMul (th->info->speed, finecosine[an]);
    th->momy = FixedMul (th->info->speed, finesine[an]);

    dist = P_AproxDistance (dest_x - source->x, dest_y - source->y);
    dist = dist / th->info->speed;

    if (dist < 1)
		dist = 1;

    th->momz = (dest_z - source->z) / dist;

    P_CheckMissileSpawn (th);

    SV_SpawnMobj(th);

    return th;
}

//
// P_SpawnPlayerMissile
// Tries to aim at a nearby monster
//
void P_SpawnPlayerMissile (AActor *source, mobjtype_t type)
{
	if(!serverside)
		return;

	angle_t an;
	fixed_t slope;
	fixed_t pitchslope = finetangent[FINEANGLES/4-(source->pitch>>ANGLETOFINESHIFT)];

	// see which target is to be aimed at
	an = source->angle;

	slope = P_AimLineAttack (source, an, 16*64*FRACUNIT);

	if (!linetarget)
	{
		an += 1<<26;
		slope = P_AimLineAttack (source, an, 16*64*FRACUNIT);

		if (!linetarget)
		{
			an -= 2<<26;
			slope = P_AimLineAttack (source, an, 16*64*FRACUNIT);
		}

		if (!linetarget)
		{
			an = source->angle;

			if(sv_freelook)
				slope = pitchslope;
			else
				slope = 0;
		}
	}

	// GhostlyDeath <June 19, 2006> -- fix flawed logic here (!linetarget not linetarget)
	if (!linetarget && source->player)
	{
		if (sv_freelook && abs(slope - pitchslope) > source->player->userinfo.aimdist)
		{
			an = source->angle;
			slope = pitchslope;
		}
	}

	AActor *th = new AActor (source->x, source->y, source->z + 4*8*FRACUNIT, type);

	fixed_t speed = th->info->speed;

	th->target = source->ptr();
	th->angle = an;
    th->momx = FixedMul(speed, finecosine[an>>ANGLETOFINESHIFT]);
    th->momy = FixedMul(speed, finesine[an>>ANGLETOFINESHIFT]);
    th->momz = FixedMul(speed, slope);

	SV_SpawnMobj(th);

	if (th->info->seesound)
		S_Sound (th, CHAN_VOICE, th->info->seesound, 1, ATTN_NORM);

	P_CheckMissileSpawn (th);
}


//
// P_RespawnSpecials
//
void P_RespawnSpecials (void)
{
	fixed_t 			x;
	fixed_t 			y;
	fixed_t 			z;

	subsector_t*			ss;
	AActor* 						mo;
	mapthing2_t* 		mthing;

	int 				i;

    // clients do no control respawning of items
	if(!serverside)
		return;

    // allow respawning if we specified it
	if (!sv_itemsrespawn)
		return;

	// nothing left to respawn?
	if (iquehead == iquetail)
		return;

	// wait a certain number of seconds before respawning this special
	if (level.time - itemrespawntime[iquetail] < sv_itemrespawntime*TICRATE)
		return;

	mthing = &itemrespawnque[iquetail];

	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;

	// find which type to spawn
	for (i=0 ; i< NUMMOBJTYPES ; i++)
	{
		if (mthing->type == mobjinfo[i].doomednum)
			break;
	}

	// [Fly] crashes sometimes without it
	if (i >= NUMMOBJTYPES)
	{
		// pull it from the que
		iquetail = (iquetail+1)&(ITEMQUESIZE-1);
		return;
	}

	if (mobjinfo[i].flags & MF_SPAWNCEILING)
		z = ONCEILINGZ;
	else
		z = ONFLOORZ;

	// spawn a teleport fog at the new spot
	ss = R_PointInSubsector (x, y);
	mo = new AActor (x, y, z, MT_IFOG);
	SV_SpawnMobj(mo);
    if (clientside)
        S_Sound (mo, CHAN_VOICE, "misc/spawn", 1, ATTN_IDLE);

	// spawn it
	mo = new AActor (x, y, z, (mobjtype_t)i);
	mo->spawnpoint = *mthing;
	mo->angle = ANG45 * (mthing->angle/45);

	if (z == ONFLOORZ)
		mo->z += mthing->z << FRACBITS;
	else if (z == ONCEILINGZ)
		mo->z -= mthing->z << FRACBITS;

	if (mo->flags2 & MF2_FLOATBOB)
	{ // Seed random starting index for bobbing motion
		mo->health = M_Random();
		mo->special1 = mthing->z << FRACBITS;
	}
	
	mo->special = 0;
	
	// pull it from the que
	iquetail = (iquetail+1)&(ITEMQUESIZE-1);

	SV_SpawnMobj(mo);
}


//
// P_ExplodeMissile
//
void P_ExplodeMissile (AActor* mo)
{
	SV_ExplodeMissile(mo);
	
	mo->momx = mo->momy = mo->momz = 0;

	P_SetMobjState (mo, mobjinfo[mo->type].deathstate);
	if (mobjinfo[mo->type].deathstate != S_NULL)
	{
		// [RH] If the object is already translucent, don't change it.
		// Otherwise, make it 66% translucent.
		//if (mo->translucency == FRACUNIT)
		//	mo->translucency = TRANSLUC66;

		mo->translucency = FRACUNIT;

		mo->tics -= P_Random(mo) & 3;

		if (mo->tics < 1)
			mo->tics = 1;

		mo->flags &= ~MF_MISSILE;

		if (mo->info->deathsound)
			S_Sound (mo, CHAN_VOICE, mo->info->deathsound, 1, ATTN_NORM);

		mo->effects = 0;		// [RH]
	}
}

VERSION_CONTROL (p_mobj_cpp, "$Id$")

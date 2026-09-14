/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "server.h"

/*
=============================================================================

Encode a client frame onto the network channel

=============================================================================
*/

#ifndef NPROFILE
unsigned long r1q2DeltaOptimizedBytes = 0;
#endif

#if KINGPIN
void SV_WriteDeltaEntity (const entity_state_t *from, const entity_state_t *to, qboolean force, qboolean newentity, qboolean deltaorigin, int protocol_version)
#else
void SV_WriteDeltaEntity (const entity_state_t *from, const entity_state_t *to, qboolean force, qboolean newentity, int cl_protocol, int protocol_version)
#endif
{
	int		bits;

	if (to == NULL)
	{
		bits = U_REMOVE;
		if (from->number >= 256)
			bits |= U_NUMBER16 | U_MOREBITS1;

		MSG_WriteByte (bits&255 );
		if (bits & 0x0000ff00)
			MSG_WriteByte ((bits>>8)&255 );

		if (bits & U_NUMBER16)
			MSG_WriteShort (from->number);
		else
			MSG_WriteByte (from->number);

		return;
	}

	if (from->number >= MAX_EDICTS || from->number < 0)
		Com_Error (ERR_FATAL, "SV_WriteDeltaEntity: Bad 'from' entity number %d", from->number);

	if (to->number >= MAX_EDICTS || to->number < 1)
		Com_Error (ERR_FATAL, "SV_WriteDeltaEntity: Bad 'to' entity number %d", to->number);

// send an update
	bits = 0;

// MH: moved below to prevent wasting bandwidth when no other bits are set
/*	if (to->number >= 256)
		bits |= U_NUMBER16;		// number8 is implicit otherwise*/

	if (!Float_RoughCompare (to->origin[0], from->origin[0]))
	{
		if (!Float_ByteCompare (to->origin[0], from->origin[0]))
			bits |= U_ORIGIN1;
#ifndef NPROFILE
		else
			r1q2DeltaOptimizedBytes += 2;
#endif
	}

	if (!Float_RoughCompare (to->origin[1], from->origin[1]))
	{
		if (!Float_ByteCompare (to->origin[1], from->origin[1]))
			bits |= U_ORIGIN2;
#ifndef NPROFILE
		else
			r1q2DeltaOptimizedBytes += 2;
#endif
	}

	if (!Float_RoughCompare (to->origin[2], from->origin[2]))
	{
		if (!Float_ByteCompare (to->origin[2], from->origin[2]))
			bits |= U_ORIGIN3;
#ifndef NPROFILE
		else
			r1q2DeltaOptimizedBytes += 2;
#endif
	}

	/*if (to->origin[0] != from->origin[0])
		bits |= U_ORIGIN1;
	if (to->origin[1] != from->origin[1])
		bits |= U_ORIGIN2;
	if (to->origin[2] != from->origin[2])
		bits |= U_ORIGIN3;*/

#if KINGPIN
	if (deltaorigin && !newentity && (bits & (U_ORIGIN1 | U_ORIGIN2 | U_ORIGIN3)))
	{
		if (fabs(to->origin[0] - from->origin[0]) < 32
			&& fabs(to->origin[1] - from->origin[1]) < 32
			&& fabs(to->origin[2] - from->origin[2]) < 32)
			bits |= U_ORIGINDELTA;
	}
#endif

	if (!Float_RoughCompare (to->angles[0], from->angles[0]))
	{
		if (!Float_AngleCompare (to->angles[0], from->angles[0]))
			bits |= U_ANGLE1;
#ifndef NPROFILE
		else
			r1q2DeltaOptimizedBytes += 1;
#endif
	}

	if (!Float_RoughCompare (to->angles[1], from->angles[1]))
	{
		if (!Float_AngleCompare (to->angles[1], from->angles[1]))
			bits |= U_ANGLE2;
#ifndef NPROFILE
		else
			r1q2DeltaOptimizedBytes += 1;
#endif
	}

	if (!Float_RoughCompare (to->angles[2], from->angles[2]))
	{
		if (!Float_AngleCompare (to->angles[2], from->angles[2]))
			bits |= U_ANGLE3;
#ifndef NPROFILE
		else
			r1q2DeltaOptimizedBytes += 1;
#endif
	}

	/*if ( to->angles[0] != from->angles[0] )
		bits |= U_ANGLE1;		
	if ( to->angles[1] != from->angles[1] )
		bits |= U_ANGLE2;
	if ( to->angles[2] != from->angles[2] )
		bits |= U_ANGLE3;*/
		
	if ( to->skinnum != from->skinnum )
	{
		if ((uint32)to->skinnum < 256)
			bits |= U_SKIN8;
		else if ((uint32)to->skinnum < 0x10000)
			bits |= U_SKIN16;
		else
			bits |= (U_SKIN8|U_SKIN16);
	}

	if ( to->frame != from->frame )
	{
		if (to->frame < 256)
			bits |= U_FRAME8;
		else
			bits |= U_FRAME16;
	}

	if ( to->effects != from->effects )
	{
		if (to->effects < 256)
			bits |= U_EFFECTS8;
		else if (to->effects < 0x8000)
			bits |= U_EFFECTS16;
		else
			bits |= U_EFFECTS8|U_EFFECTS16;
	}
	
#if KINGPIN
	if ( to->renderfx != from->renderfx || to->renderfx2 != from->renderfx2 )
	{
		int v = to->renderfx2 > to->renderfx ? to->renderfx2 : to->renderfx;
		if (v < 256)
			bits |= U_RENDERFX8;
		else if (v < 0x8000)
			bits |= U_RENDERFX16;
		else
			bits |= U_RENDERFX8|U_RENDERFX16;
	}
#else
	if ( to->renderfx != from->renderfx )
	{
		if (to->renderfx < 256)
			bits |= U_RENDERFX8;
		else if (to->renderfx < 0x8000)
			bits |= U_RENDERFX16;
		else
			bits |= U_RENDERFX8|U_RENDERFX16;
	}
#endif

	if ( to->solid != from->solid )
		bits |= U_SOLID;

	// event is not delta compressed, just 0 compressed
	if ( to->event  )
		bits |= U_EVENT;
	
	if ( to->modelindex != from->modelindex )
		bits |= U_MODEL;
#if !KINGPIN
	if ( to->modelindex2 != from->modelindex2 )
		bits |= U_MODEL2;
	if ( to->modelindex3 != from->modelindex3 )
		bits |= U_MODEL3;
	if ( to->modelindex4 != from->modelindex4 )
		bits |= U_MODEL4;
#endif

#if KINGPIN
	if (to->modelindex != 255 || protocol_version < 114)
	{
		if (to->num_parts != from->num_parts)
			bits |= U_NUMPARTS;
		if (memcmp(to->model_parts, from->model_parts, sizeof(to->model_parts)))
			bits |= U_MODELPARTS;
	}
	else
	{
//		if (to->model_parts[3].modelindex == 255 || (from->model_parts[3].modelindex == 255 && to->model_parts[3].modelindex != from->model_parts[3].modelindex))
		if (to->model_parts[3].modelindex != from->model_parts[3].modelindex)
			bits |= U_PART3MODEL;
	}
#endif

	if ( to->sound != from->sound )
	{
#if KINGPIN
		if (to->sound > 255)
			bits |= U_SOUND16;
		else
#endif
		bits |= U_SOUND;
	}

	if (
#if KINGPIN
		to->renderfx & RF_BEAM
#else
		(
			to->renderfx & RF_FRAMELERP ||
			(
				(
					(to->renderfx & RF_BEAM) &&
					cl_protocol == PROTOCOL_ORIGINAL
				)
			)
			||
			(
				(to->renderfx & RF_BEAM) &&
				cl_protocol == PROTOCOL_R1Q2 &&
				!VectorCompare (to->old_origin, from->old_origin)
			)
		)
#endif
	   )
	{
		bits |= U_OLDORIGIN;
	}

	//r1: pointless sending this if it matches baseline/old!!
	if (newentity)
	{
		if (!Vec_RoughCompare (to->old_origin, from->old_origin))
		{
			// MH: fixed (these are sent *8 not *4)
			if (!Float_ByteCompare(to->old_origin[0], from->old_origin[0])
				|| !Float_ByteCompare(to->old_origin[1], from->old_origin[1])
				|| !Float_ByteCompare(to->old_origin[2], from->old_origin[2]))
				bits |= U_OLDORIGIN;
#ifndef NPROFILE
		else
			r1q2DeltaOptimizedBytes += 6;
#endif
		}
	}

#if KINGPIN
	if (memcmp(&from->model_lighting, &to->model_lighting, sizeof(from->model_lighting)))
		bits |= U_MODELLIGHT;

	if (to->scale != from->scale)
		bits |= U_SCALE;
#endif

	//
	// write the message
	//
	if (!bits && !force)
		return;		// nothing to send!

	// MH: moved here from above
	if (to->number >= 256)
		bits |= U_NUMBER16;		// number8 is implicit otherwise

	//----------

	if (bits & 0xff000000)
		bits |= U_MOREBITS3 | U_MOREBITS2 | U_MOREBITS1;
	else if (bits & 0x00ff0000)
		bits |= U_MOREBITS2 | U_MOREBITS1;
	else if (bits & 0x0000ff00)
		bits |= U_MOREBITS1;

	MSG_WriteByte (bits&255 );

	if (bits & 0xff000000)
	{
		MSG_WriteByte ((bits>>8)&255 );
		MSG_WriteByte ((bits>>16)&255 );
		MSG_WriteByte ((bits>>24)&255 );
	}
	else if (bits & 0x00ff0000)
	{
		MSG_WriteByte ((bits>>8)&255 );
		MSG_WriteByte ((bits>>16)&255 );
	}
	else if (bits & 0x0000ff00)
	{
		MSG_WriteByte ((bits>>8)&255 );
	}

	//----------

	if (bits & U_NUMBER16)
		MSG_WriteShort (to->number);
	else
		MSG_WriteByte (to->number);

	if (bits & U_MODEL)
		MSG_WriteByte (to->modelindex);
#if !KINGPIN
	if (bits & U_MODEL2)
		MSG_WriteByte (to->modelindex2);
	if (bits & U_MODEL3)
		MSG_WriteByte (to->modelindex3);
	if (bits & U_MODEL4)
		MSG_WriteByte (to->modelindex4);
#endif

#if KINGPIN
	if (bits & U_NUMPARTS)
		MSG_WriteByte (to->num_parts);
	if (bits & U_MODELPARTS)
	{
		int a, c = 0;
		for (a=0; a<MAX_MODEL_PARTS; a++)
			if (memcmp(&to->model_parts[a], &from->model_parts[a], sizeof(to->model_parts[a])))
				c |= 1 << a;
		MSG_WriteByte (c);
		for (a=0; a<MAX_MODEL_PARTS; a++)
		{
			if (c&(1<<a))
			{
				int c2 = 0;
				if (to->model_parts[a].modelindex != from->model_parts[a].modelindex)
					c2 |= 1;
				if (to->model_parts[a].invisible_objects != from->model_parts[a].invisible_objects)
					c2 |= 2;
				if (memcmp(&to->model_parts[a].skinnum, &from->model_parts[a].skinnum, sizeof(to->model_parts[a].skinnum)))
					c2 |= 4;
				MSG_WriteByte (c2);
				if (c2 & 1)
					MSG_WriteByte (to->model_parts[a].modelindex);
				if (c2 & 2)
					MSG_WriteByte (to->model_parts[a].invisible_objects);
				if (c2 & 4)
				{
					int a2, c3 = 0;
					for (a2=0; a2<MAX_MODELPART_OBJECTS; a2++)
						if (to->model_parts[a].skinnum[a2] != from->model_parts[a].skinnum[a2])
							c3 |= 1 << a2;
					MSG_WriteByte (c3);
					for (a2=0; a2<MAX_MODELPART_OBJECTS; a2++)
						if (c3 & (1 << a2))
							MSG_WriteByte (to->model_parts[a].skinnum[a2]);
				}
			}
		}
	}
	if (bits & U_PART3MODEL)
		MSG_WriteByte (to->model_parts[3].modelindex);
#endif

	if (bits & U_FRAME8)
		MSG_WriteByte (to->frame);
	else if (bits & U_FRAME16)
		MSG_WriteShort (to->frame);

	if ((bits & U_SKIN8) && (bits & U_SKIN16))		//used for laser colors
		MSG_WriteLong (to->skinnum);
	else if (bits & U_SKIN8)
		MSG_WriteByte (to->skinnum);
	else if (bits & U_SKIN16)
		MSG_WriteShort (to->skinnum);


	if ( (bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16) )
		MSG_WriteLong (to->effects);
	else if (bits & U_EFFECTS8)
		MSG_WriteByte (to->effects);
	else if (bits & U_EFFECTS16)
		MSG_WriteShort (to->effects);

	if ( (bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16) )
		MSG_WriteLong (to->renderfx);
	else if (bits & U_RENDERFX8)
		MSG_WriteByte (to->renderfx);
	else if (bits & U_RENDERFX16)
		MSG_WriteShort (to->renderfx);

#if KINGPIN
	if ((bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16))
		MSG_WriteLong (to->renderfx2);
	else if (bits & U_RENDERFX8)
		MSG_WriteByte (to->renderfx2);
	else if (bits & U_RENDERFX16)
		MSG_WriteShort (to->renderfx2);

	if (bits & U_ORIGINDELTA)
	{
		if (bits & U_ORIGIN1)
			MSG_WriteByte ((int)((to->origin[0] - from->origin[0]) * 4) + 128);
		if (bits & U_ORIGIN2)
			MSG_WriteByte ((int)((to->origin[1] - from->origin[1]) * 4) + 128);
		if (bits & U_ORIGIN3)
			MSG_WriteByte ((int)((to->origin[2] - from->origin[2]) * 4) + 128);
	}
	else
#endif
	{
		if (bits & U_ORIGIN1)
			MSG_WriteCoord (to->origin[0]);		
		if (bits & U_ORIGIN2)
			MSG_WriteCoord (to->origin[1]);
		if (bits & U_ORIGIN3)
			MSG_WriteCoord (to->origin[2]);
	}

	if (bits & U_ANGLE1)
		MSG_WriteAngle(to->angles[0]);
	if (bits & U_ANGLE2)
		MSG_WriteAngle(to->angles[1]);
	if (bits & U_ANGLE3)
		MSG_WriteAngle(to->angles[2]);

	if (bits & U_OLDORIGIN)
	{
		MSG_WriteCoord (to->old_origin[0]);
		MSG_WriteCoord (to->old_origin[1]);
		MSG_WriteCoord (to->old_origin[2]);
	}

	if (bits & U_SOUND)
		MSG_WriteByte (to->sound);
#if KINGPIN
	else if (bits & U_SOUND16)
		MSG_WriteShort (to->sound);
#endif

	if (bits & U_EVENT)
		MSG_WriteByte (to->event);

	if (bits & U_SOLID)
	{
#if !KINGPIN
		if (protocol_version >= MINOR_VERSION_R1Q2_32BIT_SOLID)
			MSG_WriteLong (svs.entities[to->number].solid2);
		else
#endif
			MSG_WriteShort (to->solid);
	}

#if KINGPIN
	if (bits & U_SCALE)
		MSG_WriteByte ((int)((to->scale + 1) * 127));

	if (bits & U_MODELLIGHT)
	{
		int c=0, a;
		for (a=0; a<to->model_lighting.num_dir_lights; a++)
			c += (to->model_lighting.light_indexes[a] + 1) << (a * 10);
		MSG_WriteLong(c);
	}
#endif
}

/*
=============
SV_EmitPacketEntities

Writes a delta update of an entity_state_t list to the message.
=============
*/
static void SV_EmitPacketEntities (client_t *cl, const client_frame_t /*@null@*/*from, const client_frame_t *to, sizebuf_t *msg)
{
	const entity_state_t	*oldent;
	entity_state_t			*newent;

	int				oldindex, newindex;
	int				oldnum, newnum;
	int				from_num_entities;
	int				new_entities = 0; // MH: new entity count

	//r1: pointless waste of byte since this is already inside an svc_frame
#if !KINGPIN
	if (cl->protocol != PROTOCOL_R1Q2)
#endif
	{
		MSG_BeginWriting (svc_packetentities);
	}
#ifndef NPROFILE
	else
	{
		svs.proto35BytesSaved++;
#ifdef _DEBUG
		if (MSG_GetLength())
			Sys_DebugBreak ();
#endif
	}
#endif

	if (!from)
		from_num_entities = 0;
	else
		from_num_entities = from->num_entities;

	newindex = 0;
	oldindex = 0;

	newent = NULL;
	oldent = NULL;

	while (newindex < to->num_entities || oldindex < from_num_entities)
	{
		if (newindex >= to->num_entities)
			newnum = 9999;
		else
		{
			newent = &svs.client_entities[(to->first_entity+newindex)%svs.num_client_entities];
			newnum = newent->number;
			// MH: check for removed entity in reduced frame
			if (!newnum)
			{
				newindex++;
				continue;
			}
		}

		if (oldindex >= from_num_entities)
			oldnum = 9999;
		else
		{
			//Com_Printf ("server: its in old entities!\n");
			oldent = &svs.client_entities[(from->first_entity+oldindex)%svs.num_client_entities];
			oldnum = oldent->number;
			// MH: check for removed entity in reduced frame
			if (!oldnum)
			{
				oldindex++;
				continue;
			}
		}

		//don't send player to self
		//FIXME: need to do this for s.event on self to work etc...
		/*if (newnum == cl->edict->s.number) {
			//Com_Printf ("didn't send edict %d\n", newnum);
			newindex++;
			oldindex++;
			continue;
		}*/

		if (newnum == oldnum)
		{	// delta update from old position
			// because the force parm is false, this will not result
			// in any bytes being emited if the entity has not changed at all
			// note that players are always 'newentities', this updates their oldorigin always
			// and prevents warping

#if KINGPIN
			SV_WriteDeltaEntity (oldent, newent, false, false, false /*((sv.framenum + newnum) % 20)*/, cl->protocol_version);
#else
			SV_WriteDeltaEntity (oldent, newent, false, newent->number <= maxclients->intvalue, cl->protocol, cl->protocol_version);
#endif

			oldindex++;
			newindex++;
			continue;
		}
	
		if (newnum < oldnum)
		{	// this is a new entity, send it from the baseline
			// MH: limit new entities (except players) for reduced frame size
			if (newnum > maxclients->intvalue && ++new_entities > cl->new_entities)
				newent->number = 0;
			else
#if KINGPIN
				SV_WriteDeltaEntity (&cl->lastlines[newnum], newent, true, true, false, cl->protocol_version);
#else
				SV_WriteDeltaEntity (&cl->lastlines[newnum], newent, true, true, cl->protocol, cl->protocol_version);
#endif
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{	// the old entity isn't present in the new message
#if KINGPIN
			SV_WriteDeltaEntity (oldent, NULL, true, false, false, cl->protocol_version);
#else
			SV_WriteDeltaEntity (oldent, NULL, true, false, cl->protocol, cl->protocol_version);
#endif
			oldindex++;
			continue;
		}
	}

	MSG_WriteShort (0);	// end of packetentities
	MSG_EndWriting (msg);

	// MH: update new entity limit
	if (cl->new_entities > new_entities)
		cl->new_entities = new_entities;
}

#define Vec_RangeCap(x,minv,maxv) \
do { \
	if ((x)[0] > (maxv)) (x)[0] = (maxv); else if ((x)[0] < (minv)) (x)[0] = (minv); \
	if ((x)[1] > (maxv)) (x)[1] = (maxv); else if ((x)[1] < (minv)) (x)[1] = (minv); \
	if ((x)[2] > (maxv)) (x)[2] = (maxv); else if ((x)[2] < (minv)) (x)[2] = (minv); \
} while(0)

/*
=============
SV_WritePlayerstateToClient

=============
*/
static int SV_WritePlayerstateToClient (const client_frame_t /*@null@*/*from, client_frame_t *to, sizebuf_t *msg, const client_t *client)
{
	int							i;
	int							pflags;
	static player_state_t		null_playerstate;
	int							statbits;
	int							extraflags;
	qboolean					enhanced;
	player_state_t			*ps;
	const player_state_t		*ops;
	qboolean					needViewAngleDeltas;

	ps = &to->ps;

	extraflags = 0;
#if KINGPIN
	enhanced = 0;
#else
	enhanced = (client->protocol == PROTOCOL_R1Q2);
#endif

	if (!from)
		ops = &null_playerstate;
	else
		ops = &from->ps;

	//r1: cap to byte range for these
#if !KINGPIN
	Vec_RangeCap (ps->viewoffset, -32, 31.75);
#endif
	Vec_RangeCap (ps->kick_angles, -32, 31.75);

	//r1: fix signed char range errors
	Vec_RangeCap (ps->gunoffset, -32, 31.75);
	Vec_RangeCap (ps->gunangles, -32, 31.75);

	//and these are written as bytes too
	if (ps->blend[0] > 1)
		ps->blend[0] = 1;

	if (ps->blend[1] > 1)
		ps->blend[1] = 1;

	if (ps->blend[2] > 1)
		ps->blend[2] = 1;

	if (ps->blend[3] > 1)
		ps->blend[3] = 1;

	//
	// determine what needs to be sent
	//
	pflags = 0;

	if (ps->pmove.pm_type != ops->pmove.pm_type)
		pflags |= PS_M_TYPE;

	//r1
	if (ps->pmove.origin[0] != ops->pmove.origin[0] || ps->pmove.origin[1] != ops->pmove.origin[1])
	{
		if (!enhanced)
			extraflags |= EPS_PMOVE_ORIGIN2;

		pflags |= PS_M_ORIGIN;
	}

	if (ps->pmove.origin[2] != ops->pmove.origin[2])
	{
		if (!enhanced)
			pflags |= PS_M_ORIGIN;
		extraflags |= EPS_PMOVE_ORIGIN2;
	}

	//r1
	if (ps->pmove.velocity[0] != ops->pmove.velocity[0] || ps->pmove.velocity[1] != ops->pmove.velocity[1])
	{
		if (!enhanced)
			extraflags |= EPS_PMOVE_VELOCITY2;

		pflags |= PS_M_VELOCITY;
	}

	if (ps->pmove.velocity[2] != ops->pmove.velocity[2])
	{
		if (!enhanced)
			pflags |= PS_M_VELOCITY;
		extraflags |= EPS_PMOVE_VELOCITY2;
	}

	if (ps->pmove.pm_time != ops->pmove.pm_time)
		pflags |= PS_M_TIME;

	if (ps->pmove.pm_flags != ops->pmove.pm_flags)
		pflags |= PS_M_FLAGS;

	if (ps->pmove.gravity != ops->pmove.gravity)
		pflags |= PS_M_GRAVITY;

	if (ps->pmove.delta_angles[0] != ops->pmove.delta_angles[0]
		|| ps->pmove.delta_angles[1] != ops->pmove.delta_angles[1]
		|| ps->pmove.delta_angles[2] != ops->pmove.delta_angles[2] )
		pflags |= PS_M_DELTA_ANGLES;


	/*if (ps->viewoffset[0] != ops->viewoffset[0]
		|| ps->viewoffset[1] != ops->viewoffset[1]
		|| ps->viewoffset[2] != ops->viewoffset[2] )*/

	if (!Vec_RoughCompare (ps->viewoffset, ops->viewoffset))
	{
#if !KINGPIN
		if (!Vec_ByteCompare (ps->viewoffset, ops->viewoffset))
#endif
			pflags |= PS_VIEWOFFSET;
#ifndef NPROFILE
		else
			svs.r1q2OptimizedBytes += 3;
#endif
	}

	needViewAngleDeltas =
#if KINGPIN
		true;
#else
		((client->settings[CLSET_RECORDING]) || client->demofile ||
        (sv_optimize_deltas->intvalue == 1 && client->protocol != PROTOCOL_R1Q2) ||
        (ps->pmove.pm_type >= PM_DEAD) ||
		(sv_optimize_deltas->intvalue == 0));
#endif

	//why are we even sending these back to client? optimize.
	if (needViewAngleDeltas)
	{
		/*qboolean	deltaHack;
	
		if (deltaOptimize)
			deltaHack = (ps->pmove.pm_type >= PM_DEAD && ops->pmove.pm_type < PM_DEAD);
		else
			deltaHack = false;*/

		if (*(int *)&ps->viewangles[0] != *(int *)&ops->viewangles[0] || *(int *)&ps->viewangles[1] != *(int *)&ops->viewangles[1])
		{
			if (ANGLE2SHORT(ps->viewangles[0]) != ANGLE2SHORT(ops->viewangles[0]) || ANGLE2SHORT(ps->viewangles[1]) != ANGLE2SHORT(ops->viewangles[1]))
			{
				if (!enhanced)
					extraflags |= EPS_VIEWANGLE2;
				pflags |= PS_VIEWANGLES;
			}
#ifndef NPROFILE
			else
			{
				if (!enhanced)
					svs.r1q2OptimizedBytes += 6;
				else
					svs.r1q2OptimizedBytes += 4;
			}
#endif
		}
		
		if (!(extraflags & EPS_VIEWANGLE2) && *(int *)&ps->viewangles[2] != *(int *)&ops->viewangles[2])
		{
			if (ANGLE2SHORT(ps->viewangles[2]) != ANGLE2SHORT(ops->viewangles[2]))
			{
				if (!enhanced)
					pflags |= PS_VIEWANGLES;

				extraflags |= EPS_VIEWANGLE2;
			}
#ifndef NPROFILE
			else
			{
				if (!enhanced)
					svs.r1q2OptimizedBytes += 6;
				else
					svs.r1q2OptimizedBytes += 2;
			}
#endif
		}
	}
	else
	{
#ifndef NPROFILE
		if (*(int *)&ps->viewangles[0] != *(int *)&ops->viewangles[0] || *(int *)&ps->viewangles[1] != *(int *)&ops->viewangles[1])
		{
			if (!enhanced)
				svs.r1q2CustomBytes += 6;
			else
				svs.r1q2CustomBytes += 4;
		}
		if (enhanced && *(int *)&ps->viewangles[2] != *(int *)&ops->viewangles[2])
			svs.r1q2CustomBytes += 2;
#endif
		//force no delta if condition changes. this is really nasty...
		//*(int *)&ps->viewangles[0] = ~*(int *)&ps->viewangles[0];
		//*(int *)&ps->viewangles[2] = ~*(int *)&ps->viewangles[2];
		ps->viewangles[0] = ops->viewangles[0];
		ps->viewangles[1] = ops->viewangles[1];
		ps->viewangles[2] = ops->viewangles[2];
	}

	/*if (ps->kick_angles[0] != ops->kick_angles[0]
		|| ps->kick_angles[1] != ops->kick_angles[1]
		|| ps->kick_angles[2] != ops->kick_angles[2] )*/

	if (!Vec_RoughCompare (ps->kick_angles, ops->kick_angles))
	{
		if (!Vec_ByteCompare (ps->kick_angles, ops->kick_angles))
			pflags |= PS_KICKANGLES;
#ifndef NPROFILE
		else
			svs.r1q2OptimizedBytes += 3;
#endif
	}


	/*if (ps->blend[0] != ops->blend[0]
		|| ps->blend[1] != ops->blend[1]
		|| ps->blend[2] != ops->blend[2]
		|| ps->blend[3] != ops->blend[3] )*/

	//we can afford to do this as float load/compares below are relatively expensive
	if (*(int *)&ps->blend[0] != *(int *)&ops->blend[0] ||
		*(int *)&ps->blend[1] != *(int *)&ops->blend[1] ||
		*(int *)&ps->blend[2] != *(int *)&ops->blend[2] ||
		*(int *)&ps->blend[3] != *(int *)&ops->blend[3])
	{
#if !KINGPIN
		if (!client->settings[CLSET_NOBLEND] || client->settings[CLSET_RECORDING] || client->demofile)
#endif
		{
			//special range checking here since we aren't *4 any more
			if ((int)(ps->blend[0]*255) != (int)(ops->blend[0]*255) ||
				(int)(ps->blend[1]*255) != (int)(ops->blend[1]*255) ||
				(int)(ps->blend[2]*255) != (int)(ops->blend[2]*255) ||
				(int)(ps->blend[3]*255) != (int)(ops->blend[3]*255))
			{
				pflags |= PS_BLEND;
			}
#ifndef NPROFILE
			else
			{
				svs.r1q2OptimizedBytes += 4;
			}
#endif
		}
#ifndef NPROFILE
		else
		{
			svs.r1q2CustomBytes += 4;
		}
#endif
	}

	if ((int)ps->fov != (int)ops->fov)
		pflags |= PS_FOV;

	if (ps->rdflags != ops->rdflags)
		pflags |= PS_RDFLAGS;

	if (ps->gunframe != ops->gunframe)
	{
#if !KINGPIN
		if (!client->settings[CLSET_NOGUN] || client->settings[CLSET_RECORDING] || client->demofile)
#endif
		{
			pflags |= PS_WEAPONFRAME;
			if (!enhanced)
			{
				extraflags |= EPS_GUNANGLES|EPS_GUNOFFSET;
			}
			else
			{
				if (!Vec_RoughCompare (ps->gunangles, ops->gunangles))
				{
					if (!Vec_ByteCompare (ps->gunangles, ops->gunangles))
						extraflags |= EPS_GUNANGLES;
#ifndef NPROFILE
					else
						svs.r1q2OptimizedBytes += 3;
#endif
				}

				if (!Vec_RoughCompare (ps->gunoffset, ops->gunoffset))
				{
					if (!Vec_ByteCompare (ps->gunoffset, ops->gunoffset))
						extraflags |= EPS_GUNOFFSET;
#ifndef NPROFILE
					else
						svs.r1q2OptimizedBytes += 3;
#endif
				}
			}
		}
#ifndef NPROFILE
		else
		{
			svs.r1q2CustomBytes++;

			if (!Vec_RoughCompare (ps->gunangles, ops->gunangles))
				svs.r1q2CustomBytes += 3;

			if (!Vec_RoughCompare (ps->gunoffset, ops->gunoffset))
				svs.r1q2CustomBytes += 3;
		}
#endif
	}

#if KINGPIN
	if (ps->gunindex != ops->gunindex)
		pflags |= PS_WEAPONINDEX;
	else
	{
		for (i=0; i<MAX_MODEL_PARTS; i++)
			if (memcmp(&ps->model_parts[i], &ops->model_parts[i], sizeof(ps->model_parts[i])))
			{
				pflags |= PS_WEAPONINDEX;
				break;
			}
	}
#else
	if (ps->gunindex != ops->gunindex)
	{
		if (!client->settings[CLSET_NOGUN] || client->settings[CLSET_RECORDING] || client->demofile)
			pflags |= PS_WEAPONINDEX;
#ifndef NPROFILE
		else
			svs.r1q2CustomBytes++;
#endif
	}
#endif

	//
	// write it
	//

	//r1: pointless waste of byte since this is already inside an svc_frame
	if (!enhanced)
	{
		MSG_BeginWriting (svc_playerinfo);
	}
#ifndef NPROFILE
	else
	{
		svs.proto35BytesSaved++;
#ifdef _DEBUG
		if (MSG_GetLength())
			Sys_DebugBreak ();
#endif
	}
#endif

	MSG_WriteShort (pflags);

	//
	// write the pmove_state_t
	//
	if (pflags & PS_M_TYPE)
		MSG_WriteByte (ps->pmove.pm_type);

	if (pflags & PS_M_ORIGIN)
	{
		MSG_WriteShort (ps->pmove.origin[0]);
		MSG_WriteShort (ps->pmove.origin[1]);
	}

	//r1
	if (extraflags & EPS_PMOVE_ORIGIN2)
		MSG_WriteShort (ps->pmove.origin[2]);
#ifndef NPROFILE
	else if (pflags & PS_M_ORIGIN)
		svs.proto35BytesSaved += 2;
#endif

	if (pflags & PS_M_VELOCITY)
	{
		MSG_WriteShort (ps->pmove.velocity[0]);
		MSG_WriteShort (ps->pmove.velocity[1]);
	}

	//r1
	if (extraflags & EPS_PMOVE_VELOCITY2)
		MSG_WriteShort (ps->pmove.velocity[2]);
#ifndef NPROFILE
	else if (pflags & PS_M_VELOCITY)
		svs.proto35BytesSaved += 2;
#endif

	if (pflags & PS_M_TIME)
		MSG_WriteByte (ps->pmove.pm_time);

	if (pflags & PS_M_FLAGS)
		MSG_WriteByte (ps->pmove.pm_flags);

	if (pflags & PS_M_GRAVITY)
		MSG_WriteShort (ps->pmove.gravity);

	if (pflags & PS_M_DELTA_ANGLES)
	{
		MSG_WriteShort (ps->pmove.delta_angles[0]);
		MSG_WriteShort (ps->pmove.delta_angles[1]);
		MSG_WriteShort (ps->pmove.delta_angles[2]);
	}

	//
	// write the rest of the player_state_t
	//
	if (pflags & PS_VIEWOFFSET)
	{
		MSG_WriteChar ((int)(ps->viewoffset[0]*4));
		MSG_WriteChar ((int)(ps->viewoffset[1]*4));
#if KINGPIN
		MSG_WriteAngle16 (ps->viewoffset[2]);
#else
		MSG_WriteChar ((int)(ps->viewoffset[2]*4));
#endif
	}

	if (pflags & PS_VIEWANGLES)
	{
		MSG_WriteAngle16 (ps->viewangles[0]);
		MSG_WriteAngle16 (ps->viewangles[1]);
	}

	//r1
	if (extraflags & EPS_VIEWANGLE2)
		MSG_WriteAngle16 (ps->viewangles[2]);	//this one rarely changes
#ifndef NPROFILE
	else if (pflags & PS_VIEWANGLES)
		svs.proto35BytesSaved += 2;
#endif

	if (pflags & PS_KICKANGLES)
	{
		//r1: fixed, these are read as chars on client!
		MSG_WriteChar ((int)(ps->kick_angles[0]*4));
		MSG_WriteChar ((int)(ps->kick_angles[1]*4));
		MSG_WriteChar ((int)(ps->kick_angles[2]*4));
	}

	if (pflags & PS_WEAPONINDEX)
	{
		MSG_WriteByte (ps->gunindex);
#if KINGPIN
		{
			int i, c = 0;
			for (i=0; i<MAX_MODEL_PARTS; i++)
				if (memcmp(&ps->model_parts[i], &ops->model_parts[i], sizeof(ps->model_parts[i])))
					c |= 1 << i;
			MSG_WriteByte (c);
			for (i=0; i<MAX_MODEL_PARTS; i++)
			{
				if (c&(1<<i))
				{
					int c2 = 0;
					if (ps->model_parts[i].modelindex != ops->model_parts[i].modelindex)
						c2 |= 1;
					if (ps->model_parts[i].invisible_objects != ops->model_parts[i].invisible_objects)
						c2 |= 2;
					if (memcmp(&ps->model_parts[i].skinnum, &ops->model_parts[i].skinnum, sizeof(ps->model_parts[i].skinnum)))
						c2 |= 4;
					MSG_WriteByte (c2);
					if (c2 & 1)
						MSG_WriteByte (ps->model_parts[i].modelindex);
					if (c2 & 2)
						MSG_WriteByte (ps->model_parts[i].invisible_objects);
					if (c2 & 4)
					{
						int i2, c3 = 0;
						for (i2=0; i2<MAX_MODEL_PARTS; i2++)
						{
							if (ps->model_parts[i].skinnum[i2]!=ops->model_parts[i].skinnum[i2])
								c3 |= 1 << i2;
						}
						MSG_WriteByte (c3);
						for (i2=0; i2<MAX_MODEL_PARTS; i2++)
							if (c3 & (1 << i2))
								MSG_WriteByte (ps->model_parts[i].skinnum[i2]);
					}
				}
			}
		}
#endif
	}

	if (pflags & PS_WEAPONFRAME)
	{
		MSG_WriteByte (ps->gunframe);
	}
	
	//r1
	if (extraflags & EPS_GUNOFFSET)
	{
		MSG_WriteChar ((int)(ps->gunoffset[0]*4));
		MSG_WriteChar ((int)(ps->gunoffset[1]*4));
		MSG_WriteChar ((int)(ps->gunoffset[2]*4));
	}
#ifndef NPROFILE
	else if (pflags & PS_WEAPONFRAME)
	{
		svs.proto35BytesSaved += 3;
	}
#endif

	//r1
	if (extraflags & EPS_GUNANGLES)
	{
		MSG_WriteChar ((int)(ps->gunangles[0]*4));
		MSG_WriteChar ((int)(ps->gunangles[1]*4));
		MSG_WriteChar ((int)(ps->gunangles[2]*4));
	}
#ifndef NPROFILE
	else if (pflags & PS_WEAPONFRAME)
	{
		svs.proto35BytesSaved += 3;
	}
#endif

	if (pflags & PS_BLEND)
	{
		//r1: fix byte overflow making this lower than it was supposed to be
		MSG_WriteByte ((int)(ps->blend[0]*255));
		MSG_WriteByte ((int)(ps->blend[1]*255));
		MSG_WriteByte ((int)(ps->blend[2]*255));
		MSG_WriteByte ((int)(ps->blend[3]*255));
	}

	if (pflags & PS_FOV)
		MSG_WriteByte ((int)(ps->fov));

	if (pflags & PS_RDFLAGS)
		MSG_WriteByte (ps->rdflags);

	/*if (pflags & PS_BBOX)
	{
		int j, k;
		int solid;

		i = (int)(ps->maxs[0]/8);
		if (i<1)
			i = 1;
		if (i>31)
			i = 31;

		// z is not symetric
		j = (int)((-ps->mins[2])/8);
		if (j<1)
			j = 1;
		if (j>31)
			j = 31;

		// and z maxs can be negative...
		k = (int)((ps->maxs[2]+32)/8);
		if (k<1)
			k = 1;
		if (k>63)
			k = 63;

		solid = (k<<10) | (j<<5) | i;

		MSG_WriteShort (solid);
	}*/

	// send stats
	statbits = 0;
	for (i=0 ; i<MAX_STATS ; i++)
		if (ps->stats[i] != ops->stats[i])
			statbits |= 1<<i;

	if (!enhanced || statbits)
		extraflags |= EPS_STATS;

	if (extraflags & EPS_STATS)
	{
		MSG_WriteLong (statbits);
		for (i=0 ; i<MAX_STATS ; i++)
			if (statbits & (1<<i) )
				MSG_WriteShort (ps->stats[i]);
	}
#ifndef NPROFILE
	else
	{
		svs.proto35BytesSaved += 4;
	}
#endif

	MSG_EndWriting (msg);

	return extraflags;
}

#if !KINGPIN
void SV_SendPlayerUpdates (int msec_to_next_frame)
{
	client_t		*cl, *target;
	client_frame_t	*frame;
	entity_state_t	*ent;
	int				framenum, i, interval;
	unsigned int	requested;
	sizebuf_t		buff;
	byte			playerbuff[1024];
	qboolean		wrote;

	if (!sv_max_player_updates->intvalue)
		return;

	framenum = sv.framenum + sv.randomframe;

	SZ_Init (&buff, playerbuff, sizeof(playerbuff));
	buff.allowoverflow = true;

	for (cl = svs.clients; cl < svs.clients + maxclients->intvalue; cl++)
	{
		if (cl->state != cs_spawned)
			continue;

		if (cl->protocol != PROTOCOL_R1Q2)
			continue;

		requested = cl->settings[CLSET_PLAYERUPDATE_REQUESTS];

		if (!requested)
			continue;

		if (requested > sv_max_player_updates->intvalue)
			requested = sv_max_player_updates->intvalue;

		//due to timer inaccuracies this can happen
		if (cl->player_updates_sent == requested)
			continue;

		requested++;

		interval = 100 / requested; 
		if ((100 - msec_to_next_frame) / interval > cl->player_updates_sent)
		{
			cl->player_updates_sent = (100 - msec_to_next_frame) / interval;

			Com_DPrintf ("sending update to %s (%d for %d)\n", cl->name, msec_to_next_frame, cl->player_updates_sent);

			buff.cursize = 0;
			wrote = false;

			frame = &cl->frames[framenum & UPDATE_MASK];

			for (i = 0; i < frame->num_entities; i++)
			{
				ent = &svs.client_entities[(frame->first_entity+i)%svs.num_client_entities];
				if (ent->number <= maxclients->intvalue)
				{
					target = svs.clients + ent->number - 1;
					if (target != cl)
					{
						if (!wrote)
						{
							MSG_BeginWriting (svc_playerupdate);
							MSG_WriteLong (framenum);
							wrote = true;
						}
						MSG_WritePos (target->edict->s.origin);
					}
				}
			}

			if (wrote)
				MSG_EndWriting (&buff);

			if (buff.cursize && !buff.overflowed)
			{
				unsigned int	real_sequence;

				//im so very sorry... but we can't let the client know we've received their usercmd
				//until we send out the playerstate to them, or cl prediction screws up since it
				//acts on stuff we've never sent
				real_sequence = cl->netchan.incoming_sequence;
				cl->netchan.incoming_sequence = cl->last_incoming_sequence;
				Netchan_Transmit (&cl->netchan, buff.cursize, buff.data);
				cl->netchan.incoming_sequence = real_sequence;
			}
		}
	}
}
#endif

/*
==================
SV_WriteFrameToClient
==================
*/
void SV_WriteFrameToClient (client_t *client, sizebuf_t *msg)
{
	client_frame_t		*frame, *oldframe;
	int					lastframe, framenum;
	int					extraDataIndex, serverByteIndex;
#if !KINGPIN
	int					extraflags;
#endif

	framenum = sv.randomframe;

#if KINGPIN
	framenum += sv.framenum;
#else
	if (client->state < cs_spawned)
		framenum += sv.framenum;
	else
		framenum += sv.time / (1000 / client->settings[CLSET_FPS]);
#endif

//Com_Printf ("%i -> %i\n", client->lastframe, sv.framenum);
	// this is the frame we are creating
	frame = &client->frames[framenum & UPDATE_MASK];

	// MH: client demo needs to start with a non-delta frame
	if (client->demofile && client->lastframe >= 0 && client->demostart > (unsigned int)client->lastframe)
	{
		if (client->demostart > (unsigned int)framenum)
			client->demostart = framenum;
		oldframe = NULL;
		lastframe = -1;
	}
	else if (client->lastframe <= 0)
	{	// client is asking for a retransmit
		oldframe = NULL;
		lastframe = -1;
	}
	else if (framenum - client->lastframe >= (UPDATE_BACKUP - 2) )
	{	// client hasn't gotten a good message through in a long time
//		Com_Printf ("%s: Delta request from out-of-date packet.\n", client->name);
		oldframe = NULL;
		lastframe = -1;
	}
	else
	{	// we have a valid message to delta from
		oldframe = &client->frames[client->lastframe & UPDATE_MASK];
		lastframe = client->lastframe;
	}

#if KINGPIN
	// MH: workaround for facing ceiling/floor after spawning/teleporting bug in client
	if (!client->patched && oldframe && abs(frame->ps.pmove.delta_angles[0] - oldframe->ps.pmove.delta_angles[0]) > 0x8000 && !frame->ps.viewangles[0])
	{
		MSG_BeginWriting(svc_stufftext);
		MSG_WriteString("centerview\n");
		MSG_EndWriting(msg);
	}
#endif

	serverByteIndex = msg->cursize;
	SZ_WriteByte (msg, 0);

#if !KINGPIN
	//pack pack pack....
	if (client->protocol == PROTOCOL_R1Q2)
	{
		uint32		offset;
		uint32		encodedFrame;

		//we don't need full 32bits for framenum - 27 gives enough for 155 days on the same map :)
		//could even go lower if we need more bits later
		encodedFrame = framenum & 0x07FFFFFF;

		if (lastframe == -1)
			offset = 31;		//special case
		else
			offset = framenum - lastframe;

		//first 5 bits of framenum = offset for delta
		encodedFrame += (offset << 27);

		SZ_WriteLong (msg, encodedFrame);
#ifndef NPROFILE
		svs.proto35BytesSaved += 4;
#endif
	}
	else
#endif
	{
		SZ_WriteLong (msg, framenum);
		SZ_WriteLong (msg, lastframe);	// what we are delta'ing from
	}
	extraDataIndex = msg->cursize;
	SZ_WriteByte (msg, 0);
	//SZ_WriteByte (msg, client->surpressCount);	// rate dropped packets
	

	// send over the areabits
	SZ_WriteByte (msg, frame->areabytes);
	SZ_Write (msg, frame->areabits, frame->areabytes);

	// delta encode the playerstate
#if KINGPIN
	SV_WritePlayerstateToClient (oldframe, frame, msg, client);
#else
	extraflags = SV_WritePlayerstateToClient (oldframe, frame, msg, client);
#endif

#if !KINGPIN
	//HOLY CHRIST
	if (client->protocol == PROTOCOL_R1Q2)
	{
		msg->data[serverByteIndex] = svc_frame + ((extraflags & 0xF0) << 1);
		msg->data[extraDataIndex] = client->surpressCount + ((extraflags & 0x0F) << 4);
	}
	else
#endif
	{
		msg->data[serverByteIndex] = svc_frame;
		msg->data[extraDataIndex] = client->surpressCount;
	}

	client->surpressCount = 0;

	// delta encode the entities
	SV_EmitPacketEntities (client, oldframe, frame, msg);
} 


/*
=============================================================================

Build a client frame structure

=============================================================================
*/

static byte		fatpvs[65536/8];	// 32767 is MAX_MAP_LEAFS

/*
============
SV_FatPVS

The client will interpolate the view position,
so we can't use a single PVS point
===========
*/
static void SV_FatPVS (vec3_t org)
{
	int		leafs[64];
	int		i, j, count;
	int		longs;
	byte	*src;
	vec3_t	mins, maxs;

	mins[0] = org[0] - 8;
	maxs[0] = org[0] + 8;
	mins[1] = org[1] - 8;
	maxs[1] = org[1] + 8;
	mins[2] = org[2] - 8;
	maxs[2] = org[2] + 8;

	count = CM_BoxLeafnums (mins, maxs, leafs, 64, NULL);
	if (count < 1)
		Com_Error (ERR_FATAL, "SV_FatPVS: count < 1");
	longs = (CM_NumClusters+31)>>5;

	// convert leafs to clusters
	for (i=0 ; i<count ; i++)
		leafs[i] = CM_LeafCluster(leafs[i]);

	memcpy (fatpvs, CM_ClusterPVS(leafs[0]), longs<<2);
	// or in all the other leaf bits
	for (i=1 ; i<count ; i++)
	{
		for (j=0 ; j<i ; j++)
			if (leafs[i] == leafs[j])
				break;
		if (j != i)
			continue;		// already have the cluster we want
		src = CM_ClusterPVS(leafs[i]);
		for (j=0 ; j<longs ; j++)
			((int32 *)fatpvs)[j] |= ((int32 *)src)[j];
	}
}

static qboolean SV_CheckPlayerVisible(vec3_t Angles, vec3_t start, const edict_t *ent, qboolean fullCheck, qboolean predictEnt)
{
	int		i;
	vec3_t	ends[9];
	vec3_t	entOrigin;
	trace_t	trace;
	int		num;

	FastVectorCopy (ent->s.origin, entOrigin);
	
	if ( predictEnt && ent->client )
	{
		for (i = 0; i < 3; i++)
			entOrigin[i] += ent->client->ps.pmove.velocity[i] * 0.125f * 0.15f;
	}

	FastVectorCopy (entOrigin, ends[0]);

	// MH: get mid-point
	ends[0][0] += (ent->mins[0]+ent->maxs[0])/2;
	ends[0][1] += (ent->mins[1]+ent->maxs[1])/2;
	ends[0][2] += (ent->mins[2]+ent->maxs[2])/2;

	if ( fullCheck )
	{
		for (i = 1; i < 9; i++)
			FastVectorCopy (entOrigin, ends[i]);

		ends[1][0] += ent->mins[0];
		ends[2][0] += ent->mins[0];
		ends[3][0] += ent->mins[0];
		ends[4][0] += ent->mins[0];
		ends[1][1] += ent->maxs[1];
		ends[2][1] += ent->maxs[1];
		ends[3][1] += ent->mins[1];
		ends[4][1] += ent->mins[1];
		ends[1][2] += ent->maxs[2];
		ends[2][2] += ent->mins[2];
		ends[3][2] += ent->maxs[2];
		ends[4][2] += ent->mins[2];

		ends[5][0] += ent->maxs[0];
		ends[6][0] += ent->maxs[0];
		ends[7][0] += ent->maxs[0];
		ends[8][0] += ent->maxs[0];
		ends[5][1] += ent->maxs[1];
		ends[6][1] += ent->maxs[1];
		ends[7][1] += ent->mins[1];
		ends[8][1] += ent->mins[1];
		ends[5][2] += ent->maxs[2];
		ends[6][2] += ent->mins[2];
		ends[7][2] += ent->maxs[2];
		ends[8][2] += ent->mins[2];

		num = 9;
	}
	else
	{
		// MH: check the top too when not doing all corners
		ends[1][0] = ends[0][0];
		ends[1][1] = ends[0][1];
		ends[1][2] = entOrigin[2] + ent->maxs[2];

		num = 2;
	}

	for (i = 0; i < num; i++)
	{
		trace = SV_Trace(start, NULL, NULL, ends[i], NULL, CONTENTS_SOLID);

		if (trace.fraction == 1)
			return true;

#if KINGPIN
		// MH: handle see-thru obstacles
		#define	RF2_SURF_ALPHA		512
		#define CONTENTS_ALPHA		128
		if ((trace.ent->s.renderfx2 & RF2_SURF_ALPHA) || (trace.contents & CONTENTS_ALPHA))
		{
			vec3_t	dir;
			vec3_t	alpha_start;
			VectorSubtract (ends[i], start, dir);
			VectorNormalize (dir);
			VectorMA (trace.endpos, 4*3, dir, alpha_start);
			trace = SV_Trace(alpha_start, NULL, NULL, ends[i], NULL, CONTENTS_SOLID);
			if (trace.fraction == 1)
				return true;
		}
#endif
	}

	return false;
}


/*
=============
SV_BuildClientFrame

Decides which entities are going to be visible to the client, and
copies off the playerstat and areabits.
=============
*/
void SV_BuildClientFrame (client_t *client)
{
	int						e, i;
	vec3_t					org;
	edict_t					*ent;
	const edict_t			*clent;
	client_frame_t			*frame;
	entity_state_t			*state;

	int						l;
	int						clientarea, clientcluster;
	int						leafnum, framenum;
	const byte				*clientphs;
	const byte				*bitvector;

#if KINGPIN
	int						parts = 0;
#endif

	// *********** NiceAss Start ************
	qboolean	visible;
	vec3_t		start;
	// ***********  NiceAss End  ************

	// MH: predicted position stuff
	vec3_t					start2;
	qboolean				got_start2 = false;

	//union player_state_t	*hax;
	//player_state_t		*ps;

	clent = client->edict;
	if (!clent->client)
		return;		// not in game yet

	// this is the frame we are creating

	framenum = sv.randomframe;

#if KINGPIN
	framenum += sv.framenum;
#else
	if (client->state < cs_spawned)
		framenum += sv.framenum;
	else
		framenum += sv.time / (1000 / client->settings[CLSET_FPS]);
#endif

	frame = &client->frames[framenum & UPDATE_MASK];

	frame->senttime = curtime; // save it for ping calc later (MH: using wall clock (not server time))

	//hax = &clent->client->ps;

	
	//	ps = (player_state_t *)&hax->new_ps;
	//else
	//	ps = (player_state_t *)&hax->old_ps;

	// find the client's PVS
	for (i=0 ; i<3 ; i++)
	{
			org[i] = clent->client->ps.pmove.origin[i]*0.125f + clent->client->ps.viewoffset[i];
	}

	leafnum = CM_PointLeafnum (org);
	clientarea = CM_LeafArea (leafnum);
	clientcluster = CM_LeafCluster (leafnum);

	// calculate the visible areas
	frame->areabytes = CM_WriteAreaBits (frame->areabits, clientarea);

	// grab the current player_state_t
	frame->ps = clent->client->ps;

	SV_FatPVS (org);
	clientphs = CM_ClusterPHS (clientcluster);

	// build up the list of visible entities
	frame->num_entities = 0;
	frame->first_entity = svs.next_client_entities;

#if KINGPIN
	// MH: don't send any entities if disabled by game DLL
	if (!(svs.game_features & GMF_CLIENTNOENTS) || !clent->client->noents)
#endif
	for (e=1 ; e<ge->num_edicts ; e++)
	{
		ent = EDICT_NUM(e);

		// ignore ents without visible models
		if (ent->svflags & SVF_NOCLIENT)
			continue;

		// ignore ents without visible models unless they have an effect
#if KINGPIN
		if (!ent->s.modelindex && !ent->s.effects && !ent->s.sound
			&& !ent->s.event && !ent->s.num_parts)
#else
		if (!ent->s.modelindex && !ent->s.effects && !ent->s.sound
			&& !ent->s.event && !client->entity_events[e])
#endif
			continue;

		if (!ent->inuse)
		{
			if (sv_gamedebug->intvalue)
				Com_Printf ("GAME WARNING: Entity %d is marked as unused but still contains state and thus may be sent to clients!\n", LOG_SERVER|LOG_WARNING|LOG_GAMEDEBUG, e);

			if (sv_entity_inuse_hack->intvalue)
				continue;
		}

		// ignore if not touching a PV leaf
		if (ent != clent)
		{
			// check area
			if (!CM_AreasConnected (clientarea, ent->areanum))
			{	// doors can legally straddle two areas, so
				// we may need to check another one
				if (!ent->areanum2
					|| !CM_AreasConnected (clientarea, ent->areanum2))
					continue;		// blocked by a door
			}

			// beams just check one point for PHS
			if (ent->s.renderfx & RF_BEAM)
			{
				l = ent->clusternums[0];
				if ( !(clientphs[l >> 3] & (1 << (l&7) )) )
					continue;
			}
			else
			{
				// FIXME: if an ent has a model and a sound, but isn't
				// in the PVS, only the PHS, clear the model

				//r1: will this even happen? ent->s.sound attenuates rapidly, it's very unlikely
				//something with a sound and not in PVS would still be hearable...

				/*if (ent->s.sound)
				{
					bitvector = fatpvs;	//clientphs;
				}
				else*/
					bitvector = fatpvs;

				if (ent->num_clusters == -1)
				{	// too many leafs for individual check, go by headnode
					if (!CM_HeadnodeVisible (ent->headnode, bitvector))
						continue;
				}
				else
				{	// check individual leafs
					for (i=0 ; i < ent->num_clusters ; i++)
					{
						l = ent->clusternums[i];
						if (bitvector[l >> 3] & (1 << (l&7) ))
							break;
					}
					if (i == ent->num_clusters)
						continue;		// not visible
				}

#if KINGPIN
				if (!ent->s.modelindex && !ent->s.num_parts && !ent->s.effects)
#else
				if (ent->s.sound && !ent->s.modelindex && !ent->s.effects && !ent->s.event)
#endif
				{	
					// don't send sounds if they will be attenuated away
					vec3_t	delta;
					float	len;

					VectorSubtract (org, ent->s.origin, delta);
					len = VectorLength (delta);
					if (len > 400)
						continue;
				}
			}
		}

		visible = true;

		if (svs.game_features & GMF_CLIENTNUM)
		{
			//send player to himself when he's current POV, but not otherwise
			if (e == clent->client->clientNum + 1 && clent != ent)
				visible = false;
		}

#if KINGPIN
		// MH: don't bother hiding things from spectators or checking the client itself
		if (!(clent->svflags & SVF_NOCLIENT) && ent != clent)
#endif
		if (visible && sv_nc_visibilitycheck->intvalue && !(sv_nc_clientsonly->intvalue && !ent->client) && ent->solid != SOLID_BSP /*&& ent->solid != SOLID_TRIGGER*/ && ent->solid != SOLID_NOT) // MH: hide pickups too if non-clients enabled, and don't remove non-solids
		{
			// *********** NiceAss Start ************
			FastVectorCopy (org, start);
			visible = SV_CheckPlayerVisible (clent->client->ps.viewangles, start, ent, true, false);
		
			if (!visible)
			{
				// If the first direct check didn't see the player, check a little ahead
				// of yourself based on your current velocity, lag, frame update speed (100ms). 
				// This will compensate for clients predicting where they will be due to lag
				// (cl_predict)

				// MH: calculate predicted position only once
				if (!got_start2)
				{
					vec3_t vel;
					trace_t	trace;

					// MH: velocity
					for (i = 0; i < 3; i++)
						vel[i] = clent->client->ps.pmove.velocity[i] * 0.125f;

					for (i = 0; i < 3; i++)
						start2[i] = org[i] + vel[i] * ( 0.15f + (float)clent->client->ping * 0.001f );

					// MH: check if hitting an obstacle
					trace = SV_Trace(org, (vec_t*)clent->mins, (vec_t*)clent->maxs, start2, NULL, CONTENTS_SOLID);
					if (trace.fraction < 1)
					{
						// MH: slide along the obstacle
						PM_ClipVelocity (vel, trace.plane.normal, vel, 1.01f);
						for (i = 0; i < 3; i++)
							start2[i] = trace.endpos[i] + vel[i] * ( 0.15f + (float)clent->client->ping * 0.001f ) * (1 - trace.fraction);
						trace = SV_Trace(trace.endpos, (vec_t*)clent->mins, (vec_t*)clent->maxs, start2, NULL, CONTENTS_SOLID);
						FastVectorCopy (trace.endpos, start2);
					}
					got_start2 = true;
				}
				visible = SV_CheckPlayerVisible (clent->client->ps.viewangles, start2, ent, false, true);

#if KINGPIN
				// MH: check if player was visible last frame and keep them visible for extra frames (in case of back-and-forth movement)
				if (!visible)
				{
					client_frame_t *oldframe = &client->frames[(framenum - 1) & UPDATE_MASK];
					for (i=0; i<oldframe->num_entities; i++)
					{
						state = &svs.client_entities[(oldframe->first_entity+i)%svs.num_client_entities];
						if (state->number == e && state->modelindex && state->broken_flag <= (clent->client->ping + 50) / 100)
						{
							visible = 2 + state->broken_flag;
							break;
						}
						if (state->number >= e)
							break;
					}
				}
#endif
			}

			// Don't send player at all. 100% secure but no footsteps unless you see the person.
			if (!visible && sv_nc_visibilitycheck->intvalue == 2)
				continue;

			// Don't send player at all IF there are no events/sounds/etc (like footsteps!) even
			// if the visibilitycheck is "1" and not "2". Hopefully harder on wallhackers.
			if (!visible &&
#if KINGPIN
				!ent->s.effects && !ent->s.sound && !ent->s.event)
#else
				!ent->s.effects && !ent->s.sound && !ent->s.event && !client->entity_events[e])
#endif
				continue;
		}
		// ***********  NiceAss End  ************

		// add it to the circular client_entities array
		state = &svs.client_entities[svs.next_client_entities%svs.num_client_entities];

		if (ent->s.number != e)
		{
			//Com_DPrintf ("FIXING ENT->S.NUMBER!!!\n");
			//Com_Error (ERR_DROP, "Bad entity state on entity %d", e);
			if (sv_gamedebug->intvalue)
				Com_Printf ("GAME WARNING: Entity state on entity %d corrupted (bad ent->s.number %d)\n", LOG_SERVER|LOG_GAMEDEBUG|LOG_WARNING, e, ent->s.number);

			ent->s.number = e;
		}

		*state = ent->s;

#if KINGPIN
		// MH: store number of extra frames since becoming invisible
		if (visible > 1)
			state->broken_flag = visible - 1; // broken_flag isn't sent to clients
#endif

#if !KINGPIN
		//hack for variable FPS and events
		if (!ent->s.event && client->entity_events[e])
		{
			state->event = client->entity_events[e];
			client->entity_events[e] = 0;
		}
#endif

		// *********** NiceAss Start ************
		// Send the entity, but don't associate a model with it. Less secure than sv_nc_visibilitycheck 2
		// but you can hear footsteps. Default functionality.
		if (!visible)
		{
			// Remove any model associations. Invisible.
#if KINGPIN
			state->modelindex = state->num_parts = 0;

			// MH: prevent chasecam target's name appearing in crosshair
			if ((svs.game_features & GMF_CLIENTNUM) && e == clent->client->clientNum + 1)
				state->solid = 0;
#else
			state->modelindex = state->modelindex2 = state->modelindex3 = state->modelindex4 = 0;
	
			// I think this holds the weapon for VWEP
			state->skinnum = 0;
#endif
		}
		// ***********  NiceAss End  ************

		// don't mark players missiles as solid
		if (ent->owner == clent)
			state->solid = 0;

#if KINGPIN
		if (client->patched < 4)
		{
			// MH: EF_COLOR_SHELL + RF2_MONEYBAG = r_cullaliasmodel errors
			#define RF2_MONEYBAG		256
			if ((state->effects & EF_COLOR_SHELL) && (state->renderfx2 & RF2_MONEYBAG))
				state->renderfx2 &= ~RF2_MONEYBAG;
		}

		// MH: underwater sound
		if (sv_underwater_sound->intvalue && (ent == clent || ((svs.game_features & GMF_CLIENTNUM) && e == clent->client->clientNum + 1))
			 && (clent->client->ps.rdflags & RDF_UNDERWATER) && state->sound != sv.snd_fry)
			state->sound = SV_SoundIndex("world/watr_un.wav");

		// MH: count entities
		if (state->modelindex || state->num_parts)
			parts += (state->num_parts ? state->num_parts : 1);
#endif

		//r1: extended uptime fix (hmm, changing to unsigned should work just as well)
		svs.next_client_entities++;
		//if (++svs.next_client_entities == svs.num_client_entities)
		//	svs.next_client_entities = 0;

		//Com_Printf ("next will be %d, should be %d\n", 1 % svs.num_client_entities, svs.next_client_entities%svs.num_client_entities);

		//r1: break out at 128 ents since the client renderer dll can only process 128 anyway...
		if (++frame->num_entities == 128)
			break;
	}

#if KINGPIN
/*
	MH: The player's gun can sometimes disappear because the (unpatched) Kingpin client tries to
	add the gun to its entity list last, which it can't do if the list is full (128 other entities).
	Each entity can need multiple entities to display on the client side. "parts" counts them, but
	it may be 1-2 short for each player with a hat/cigar and it doesn't include temp entities (up to
	48 explosions/gibs), so the actual number can be much higher. To leave room for those extra
	entities, we'll try to limit the entities to 70 by removing any that are behind.
*/
	if (!client->patched && parts > 70 && clent->client->ps.gunindex && !(clent->client->ps.pmove.pm_flags & PMF_CHASECAM))
	{
		int j;
		vec3_t front;
		AngleVectors(clent->client->ps.viewangles, front, NULL, NULL);
		for (j=i=0; i<frame->num_entities; i++)
		{
			int slot = (frame->first_entity+i)%svs.num_client_entities;
			state = svs.client_entities + slot;
			e = state->number;
			ent = EDICT_NUM(e);
			// not removing live players or invisible entities
			if (parts > 70 && (!ent->client || (ent->svflags & SVF_DEADMONSTER)) && (state->modelindex || state->num_parts))
			{
				vec3_t d;
				vec_t s = (ent->size[0] > ent->size[1] ? ent->size[0] : ent->size[1]);
				if (s < ent->size[2]) s = ent->size[2];
				d[0] = (ent->absmax[0]+ent->absmin[0])/2 - org[0] + front[0]*s;
				d[1] = (ent->absmax[1]+ent->absmin[1])/2 - org[1] + front[1]*s;
				d[2] = (ent->absmax[2]+ent->absmin[2])/2 - org[2] + front[2]*s;
				VectorNormalize(d);
				if (acos(DotProduct(d, front)) > 170.f / 360 * M_PI)
				{
					parts -= (state->num_parts ? state->num_parts : 1);
					// remove the entity if it has no sound and hasn't changed since the baseline (takes minimal bandwidth to put back)
					if (!state->sound && !state->event && !memcmp(state, client->lastlines + e, sizeof(*state)))
						continue;
					// otherwise just hide it
					state->modelindex = state->num_parts = 0;
				}
			}
			if (i != j)
				svs.client_entities[(frame->first_entity+j)%svs.num_client_entities] = *state;
			j++;
		}
		frame->num_entities = j;
		svs.next_client_entities -= i - j;
	}
#endif
}


/*
==================
SV_RecordDemoMessage

Save everything in the world out without deltas.
Used for recording footage for merged or assembled demos
==================
*/
void SV_RecordDemoMessage (void)
{
	int			e;
	edict_t		*ent;
	sizebuf_t	buf;
	byte		buf_data[32768];
	int			len;

	if (!svs.demofile)
		return;

	SZ_Init (&buf, buf_data, sizeof(buf_data));

	// write a frame message that doesn't contain a player_state_t
	SZ_WriteByte (&buf, svc_frame);
	SZ_WriteLong (&buf, sv.framenum);

	SZ_WriteByte (&buf, svc_packetentities);

	e = 1;
	ent = EDICT_NUM(e);
	while (e < ge->num_edicts) 
	{
		// ignore ents without visible models unless they have an effect
		if (ent->inuse &&
			ent->s.number && 
#if KINGPIN
//			ent->client &&
			(ent->s.modelindex || ent->s.effects || ent->s.sound || ent->s.event || ent->s.num_parts) && 
#else
			(ent->s.modelindex || ent->s.effects || ent->s.sound || ent->s.event) && 
#endif
			!(ent->svflags & SVF_NOCLIENT))
		{
#if KINGPIN
			SV_WriteDeltaEntity (&null_entity_state, &ent->s, false, true, false, 100);
#else
			SV_WriteDeltaEntity (&null_entity_state, &ent->s, false, true, PROTOCOL_ORIGINAL, 0);
#endif
			MSG_EndWriting (&buf);
		}

		e++;
		ent = EDICT_NUM(e);
	}

	SZ_WriteShort (&buf, 0);		// end of packetentities

	// now add the accumulated multicast information
	SZ_Write (&buf, svs.demo_multicast.data, svs.demo_multicast.cursize);
	SZ_Clear (&svs.demo_multicast);

	// now write the entire message to the file, prefixed by the length
	len = LittleLong (buf.cursize);
	fwrite (&len, 4, 1, svs.demofile);
	fwrite (buf.data, buf.cursize, 1, svs.demofile);
}

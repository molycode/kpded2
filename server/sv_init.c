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

#ifdef _WIN32
#include <process.h>
#endif

extern	time_t	server_start_time;

server_static_t	svs;				// persistant server info
server_t		sv;					// local server

/*
================
SV_FindIndex

================
*/
int SV_FindIndex (const char *name, int start, int maxIndex, qboolean create)
{
	int		i;
	
	if (!name || !name[0])
	{
		if (sv_gamedebug->intvalue)
			Com_Printf ("GAME WARNING: SV_FindIndex: NULL or empty name, ignored\n", LOG_SERVER|LOG_WARNING|LOG_GAMEDEBUG);

		if (sv_gamedebug->intvalue >= 3)
			Sys_DebugBreak ();
		return 0;
	}

	for (i=1 ; i<maxIndex && sv.configstrings[start+i][0] ; i++)
	{
		if (!strcmp(sv.configstrings[start+i], name)) {
			return i;
		}
	}

	if (!create)
		return 0;

	if (i == maxIndex) {
		Com_Printf ("ERROR: Ran out of configstrings while attempting to add '%s' (%d,%d)\n", LOG_SERVER|LOG_ERROR, name, start, maxIndex);
		Com_Printf ("Dumping configstrings in use to 'configstrings.txt'...", LOG_SERVER|LOG_ERROR);
		{
			FILE *cs;
			cs = fopen ("configstrings.txt", "wb");
			if (!cs) {
				Com_Printf ("failed.\n", LOG_SERVER|LOG_ERROR);
			} else {
				fprintf (cs, "configstring dump:\n\nCS_SOUNDS:\n");
				for (i = CS_SOUNDS; i < CS_SOUNDS+MAX_SOUNDS; i++)
					fprintf (cs, "%i: %s\n", i, sv.configstrings[i]);

				fprintf (cs, "\nCS_MODELS:\n");
				for (i = CS_MODELS; i < CS_MODELS+MAX_MODELS; i++)
					fprintf (cs, "%i: %s\n", i, sv.configstrings[i]);

				fprintf (cs, "\nCS_ITEMS:\n");
				for (i = CS_MODELS; i < CS_ITEMS+MAX_MODELS; i++)
					fprintf (cs, "%i: %s\n", i, sv.configstrings[i]);

				fprintf (cs, "\nCS_IMAGES:\n");
				for (i = CS_IMAGES; i < CS_IMAGES+MAX_IMAGES; i++)
					fprintf (cs, "%i: %s\n", i, sv.configstrings[i]);

				fprintf (cs, "\nCS_LIGHTS:\n");
				for (i = CS_LIGHTS; i < CS_LIGHTS+MAX_LIGHTSTYLES; i++)
					fprintf (cs, "%i: %s\n", i, sv.configstrings[i]);

				fprintf (cs, "\nCS_GENERAL:\n");
				for (i = CS_GENERAL; i < CS_GENERAL+MAX_GENERAL; i++)
					fprintf (cs, "%i: %s\n", i, sv.configstrings[i]);

				fclose (cs);
				Com_Printf ("done.\n", LOG_SERVER|LOG_ERROR);
			}
		}
		Com_Error (ERR_GAME, "SV_FindIndex: overflow finding index for %s", name);
	}

	strncpy (sv.configstrings[start+i], name, sizeof(sv.configstrings[i])-1);

	if (sv.state != ss_loading)
	{	// send the update to everyone

		//r1: why clear?
		//SZ_Clear (&sv.multicast);
		
		MSG_BeginWriting (svc_configstring);
		MSG_WriteShort (start+i);
		MSG_WriteString (sv.configstrings[start+i]);
		SV_Multicast (NULL, MULTICAST_ALL_R);
		return i;
	}

	return i;
}


#if KINGPIN
/*
	MH: The Kingpin client can download models/skins/maps/textures but its support for
	sounds/images/skies is broken. The broken image download support can be taken
	advantage of though to allow those things to be downloaded, which is what the
	code below does. The "imageindex" function can be used by the game to add any
	files to the downloadable list (only files starting with "/pics/" will be added
	to the image list).
	
	When a client connects,	the server sends the downloadable files list in place
	of the image list. The image list is then sent when the client has finished
	connecting.
*/

static void DownloadIndex(const char *name)
{
	// some common files that the player should already have
	static const char *ignore_sound[] = {
		"actors/player/male/fry.wav",
		"actors/player/male/gasp1.wav",
		"actors/player/male/gasp2.wav",
		"misc/w_pkup.wav",
		"weapons/bullethit_tin.wav",
		"weapons/bullethit_tin2.wav",
		"weapons/bullethit_tin3.wav",
		"weapons/machinegun/machgf1b.wav",
		"weapons/melee/swing.wav",
		"weapons/pistol/silencer.wav",
		"weapons/pistol/silencerattatch.wav",
		"weapons/ric1.wav",
		"weapons/ric2.wav",
		"weapons/ric3.wav",
		"weapons/shotgun/shotgf1b.wav",
		"weapons/shotgun/shotgr1b.wav",
		"world/amb_wind.wav",
		"world/city.wav",
		"world/citybg.wav",
		"world/citybglow.wav",
		"world/citybglow2.wav",
		"world/crate1.wav",
		"world/crate2.wav",
		"world/crate3.wav",
		"world/doors/dr1_end.wav",
		"world/doors/dr1_mid.wav",
		"world/doors/dr1_strt.wav",
		"world/doors/dr2_endb.wav",
		"world/doors/dr2_mid.wav",
		"world/doors/dr2_strt.wav",
		"world/doors/dr3_endb.wav",
		"world/doors/dr3_mid.wav",
		"world/doors/dr3_strt.wav",
		"world/fire.wav",
		"world/fire_sm.wav",
		"world/lightf_broke.wav",
		"world/lightf_broker.wav",
		"world/lightf_hum.wav",
		"world/lightf_hum2.wav",
		"world/pickups/ammo.wav",
		"world/pickups/generic.wav",
		"world/pickups/health.wav",
		"world/plats/pt1_end.wav",
		"world/plats/pt1_mid.wav",
		"world/plats/pt1_strt.wav",
		"world/switches/butn2.wav",
		"world/trash1.wav",
		"world/trash2.wav",
		"world/trash3.wav",
		NULL
	};

	char lwrname[MAX_QPATH], *ext;
	int i;

	// only add to the downloadable files list during the map loading phase
	if (!allow_download->intvalue || sv.state != ss_loading)
		return;
	if (!name || !name[0] || strlen(name) >= sizeof(lwrname))
		return;
	strcpy(lwrname, name);
	Q_strlwr(lwrname);

	if (strstr(lwrname, ".."))
	{
		Com_Printf("WARNING: '%s' will not be downloadable.\n", LOG_SERVER | LOG_WARNING, lwrname);
		return;
	}

	ext = strrchr(lwrname, '.');
	if (ext && (!strcmp(ext+1, "dll") || !strcmp(ext+1, "exe")))
	{
		Com_Printf ("WARNING: '%s' will not be downloadable because it is an executable.\n", LOG_SERVER|LOG_WARNING, lwrname);
		return;
	}

	if (!strchr(lwrname, '/') && !Cvar_VariableString("clientdir")[0])
	{
		Com_Printf ("WARNING: '%s' will not be downloadable because it is not in a subdirectory.\n", LOG_SERVER|LOG_WARNING, lwrname);
		return;
	}

	if (!strncmp(lwrname, "pics/", 5))
	{
		if (ext && !strcmp(ext, ".tga") && !strncmp(lwrname+5, "h_", 2))
			return;
	}
	else if (!strncmp(lwrname, "sound/", 6))
	{
		for (i=0; ignore_sound[i]; i++)
			if (!strcmp(lwrname+6, ignore_sound[i]))
				return;
	}

	// don't add files that are in a PAK
	if (FS_LoadFile(lwrname, NULL) > 0 && lastpakfile)
		return;

	for (i=0; i<MAX_IMAGES && sv.dlconfigstrings[i][0]; i++)
		if (!strcmp(sv.dlconfigstrings[i], lwrname))
			return;
	if (i == MAX_IMAGES)
	{
		Com_Printf ("WARNING: Ran out of download configstrings while attempting to add '%s'\n", LOG_SERVER|LOG_WARNING, lwrname);
		return;
	}

	strcpy(sv.dlconfigstrings[i], lwrname);
}

int EXPORT SV_ModelIndex (const char *name)
{
	char lwrname[MAX_QPATH];
	int i;

	if (!name || !name[0] || strlen(name) >= sizeof(sv.configstrings[0]))
		return 0;
	strcpy(lwrname, name);
	Q_strlwr(lwrname);

	i = SV_FindIndex (lwrname, CS_MODELS, MAX_MODELS, true);
	if (i && !strncmp(lwrname, "models/objects/", 15))
	{
		// Kingpin doesn't auto-download model skins, so add them to downloadable files list
		void *data;
		FS_LoadFile(lwrname, &data);
		if (data)
		{
			dmdl_t *pheader = (dmdl_t*)data;
			if (LittleLong(pheader->ident) == IDALIASHEADER && LittleLong(pheader->version) == ALIAS_VERSION)
			{
				int a, ns = LittleLong(pheader->num_skins);
				for (a=0; a<ns; a++)
					DownloadIndex((char*)data + LittleLong(pheader->ofs_skins) + a * MAX_SKINNAME);
			}
			FS_FreeFile(data);
		}
	}
	return i;
}

int EXPORT SV_SoundIndex (const char *name)
{
	char lwrname[MAX_QPATH];
	int i;

	if (!name || !name[0] || strlen(name) >= sizeof(lwrname))
		return 0;
	strcpy(lwrname, name);
	Q_strlwr(lwrname);

	i = SV_FindIndex (lwrname, CS_SOUNDS, MAX_SOUNDS, true);
	if (i && lwrname[0] != '*')
	{
		char buf[MAX_QPATH + 1];
		Com_sprintf(buf, sizeof(buf), "sound/%s", lwrname);
		DownloadIndex(buf);
	}

	// keep the "standing in lava/slime" sound index for sv_underwater_sound check
	if (!strcmp(name, "actors/player/male/fry.wav"))
		sv.snd_fry = i;

	return i;
}

int EXPORT SV_ImageIndex (const char *name)
{
	char lwrname[MAX_QPATH];
	int i;

	if (!name || !name[0] || !strchr(name+1,'/') || strlen(name) >= sizeof(lwrname))
		return 0;
	strcpy(lwrname, name);
	Q_strlwr(lwrname);

	// Kingpin always begins image files with "/pics/"
	if (!strncmp(lwrname, "/pics/", 6))
		i = SV_FindIndex (lwrname, CS_IMAGES, MAX_IMAGES, true);
	else
		i = 0;
	DownloadIndex(lwrname[0] == '/' ? lwrname+1 : lwrname);
	return i;
}

int	EXPORT SV_SkinIndex(int modelindex, const char *name)
{
	char lwrname[MAX_QPATH];
	int i, len, len2;

	if ((uint32)modelindex >= MAX_MODELS)
		return 0;

	len = strlen(name);
	if (len != 3)
	{
		if (sv_gamedebug->intvalue)
		{
			Com_Printf("GAME WARNING: skinindex: Abnormal skin length (%d) for model %s\n", LOG_WARNING | LOG_GAMEDEBUG | LOG_SERVER, len, sv.configstrings[CS_MODELS + modelindex]);
			if (sv_gamedebug->intvalue >= 3)
				Sys_DebugBreak();
		}
		if ((size_t)len >= sizeof(lwrname))
			return 0;
	}
	strcpy(lwrname, name);
	Q_strlwr(lwrname);

	len2 = strlen(sv.configstrings[CS_MODELSKINS + modelindex]);
	for (i = 0; i < len2; i += len)
		if (!strncmp(sv.configstrings[CS_MODELSKINS + modelindex] + i, lwrname, len))
			return i;

	if (len2 + len >= MAX_QPATH)
	{
		Com_Printf("GAME ERROR: skinindex: Too many skins for model %s\n", LOG_ERROR | LOG_GAMEDEBUG | LOG_SERVER, sv.configstrings[CS_MODELS + modelindex]);
		if (sv_gamedebug->intvalue >= 2)
			Sys_DebugBreak();
		return 0;
	}

	strcpy(sv.configstrings[CS_MODELSKINS + modelindex] + len2, lwrname);

	if (sv.state != ss_loading)
	{
		MSG_BeginWriting (svc_configstring);
		MSG_WriteShort (CS_MODELSKINS + modelindex);
		MSG_WriteString (sv.configstrings[CS_MODELSKINS + modelindex]);
		MSG_WriteByte (len);
		SV_Multicast (NULL, MULTICAST_ALL_R);
	}
	return i;
}
#else
int EXPORT SV_ModelIndex (const char *name)
{
	return SV_FindIndex (name, CS_MODELS, MAX_MODELS, true);
}

int EXPORT SV_SoundIndex (const char *name)
{
	return SV_FindIndex (name, CS_SOUNDS, MAX_SOUNDS, true);
}

int EXPORT SV_ImageIndex (const char *name)
{
	return SV_FindIndex (name, CS_IMAGES, MAX_IMAGES, true);
}
#endif

/*
=================
SV_CheckForSavegame
=================
*/
static void SV_CheckForSavegame (void)
{
	char		name[MAX_OSPATH];
	FILE		*f;
	int			i;

	if (sv_noreload->intvalue)
		return;

	if (Cvar_IntValue ("deathmatch"))
		return;

	Com_sprintf (name, sizeof(name), "%s/save/current/%s.sav", FS_Gamedir(), sv.name);
	f = fopen (name, "rb");
	if (!f)
		return;		// no savegame

	fclose (f);

	SV_ClearWorld ();

	// get configstrings and areaportals
	SV_ReadLevelFile ();

	if (!sv.loadgame)
	{	// coming back to a level after being in a different
		// level, so run it for ten seconds

		// rlava2 was sending too many lightstyles, and overflowing the
		// reliable data. temporarily changing the server state to loading
		// prevents these from being passed down.
		server_state_t		previousState;		// PGM

		previousState = sv.state;				// PGM
		sv.state = ss_loading;					// PGM
		for (i=0 ; i<100 ; i++)
			ge->RunFrame ();

		sv.state = previousState;				// PGM
	}
}


/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.

================
*/
static void SV_SpawnServer (const char *server, const char *spawnpoint, server_state_t serverstate, qboolean attractloop, qboolean loadgame)
{
	int			i;
	uint32		checksum;
	char		*cmd;
	char		playerskins[MAX_CLIENTS][MAX_QPATH];

#if KINGPIN
	// clear download cache
	ClearCachedDownloads();
#endif

#if !KINGPIN
	//r1: get latched vars
	if (Cvar_GetNumLatchedVars() || sv_recycle->intvalue)
	{
		Com_Printf ("SV_SpawnServer: Reloading Game DLL.\n", LOG_SERVER);
		SV_InitGame();

		if (sv_recycle->intvalue != 2)
			Cvar_ForceSet ("sv_recycle", "0");
	}
	else
#endif
	// MH: apply latched cvars with NORELOAD set
	Cvar_GetLatchedVars();

	Cvar_ForceSet ("$mapname", server);

	// MH: disable idle mode
	Cvar_ForceSet ("g_idle", "0");

#ifndef DEDICATED_ONLY
	if (dedicated->intvalue)
	{
#endif
		cmd = Cmd_MacroExpandString("$sv_beginmapcmd");
		if (cmd)
			Cbuf_ExecuteScoped (cmd);
		else
			Com_Printf ("WARNING: Error expanding $sv_beginmapcmd, ignored.\n", LOG_SERVER|LOG_WARNING);

		// the hook may have shut the server down under us
		if (!svs.initialized)
		{
			Com_Printf ("WARNING: $sv_beginmapcmd stopped the server, abandoning map load.\n", LOG_SERVER|LOG_WARNING);
			return;
		}
#ifndef DEDICATED_ONLY
	}
#endif

	Z_Verify("SV_SpawnServer:START");

	if (attractloop)
		Cvar_Set ("paused", "0");

	Com_Printf ("------- Server Initialization -------\n", LOG_SERVER);

	Com_Printf ("SpawnServer: %s\n", LOG_DEBUG, server); // MH: changed from Com_DPrintf
	if (sv.demofile)
		fclose (sv.demofile);

	svs.spawncount++;		// any partially connected client will be
							// restarted

	//lookup any possible new IP
	if (dedicated->intvalue && (svs.spawncount % 10) == 0)
	{
		if (sv_global_master->intvalue)
			NET_StringToAdr (GLOBAL_MASTER, &master_adr[0]);
	}

	sv.state = ss_dead;
	Com_SetServerState (sv.state);

	// MH: retain player skins for downloads at start of new map
	memcpy(playerskins, sv.configstrings[CS_PLAYERSKINS], sizeof(playerskins));

	// wipe the entire per-level structure
	memset (&sv, 0, sizeof(sv));

	//r1: randomize serverframe to thwart some map timers
	if (sv_randomframe->intvalue)
		sv.randomframe = (int)(random() * 0xFFFF);

	svs.realtime = 0;
	sv.loadgame = loadgame;
	sv.attractloop = attractloop;

	// save name for levels that don't set message
	strncpy (sv.configstrings[CS_NAME], server, sizeof(sv.configstrings[CS_NAME])-1);

#if KINGPIN
	strcpy (sv.configstrings[CS_SERVER_VERSION], "200"); // 2.00
#else
	if (Cvar_IntValue ("deathmatch"))
	{
		Com_sprintf(sv.configstrings[CS_AIRACCEL], sizeof(sv.configstrings[CS_AIRACCEL]), "%d", sv_airaccelerate->intvalue);
		pm_airaccelerate = (qboolean)sv_airaccelerate->intvalue;
	}
	else
	{
		strcpy(sv.configstrings[CS_AIRACCEL], "0");
		pm_airaccelerate = false;
	}
#endif

	//SZ_Init (&sv.multicast, sv.multicast_buf, sizeof(sv.multicast_buf));

	strcpy (sv.name, server);

	// leave slots at start for clients only
	for (i=0 ; i<maxclients->intvalue ; i++)
	{
		// needs to reconnect
		if (svs.clients[i].state == cs_spawned)
			svs.clients[i].state = cs_connected;
		svs.clients[i].lastframe = -1;
		svs.clients[i].packetCount = 0;

		// MH: retain player skins for downloads at start of new map
		if (svs.clients[i].state == cs_connected)
			strcpy(sv.configstrings[CS_PLAYERSKINS + i], playerskins[i]);

		// MH: clear layout message buffer
		svs.clients[i].layout[0] = 0;

		// MH: don't count acks during reconnect
		svs.clients[i].netchan.countacks = false;

		// MH: reset connection quality measurement state
		svs.clients[i].quality_acc = svs.clients[i].quality_last = 0;
	}

	sv.time = 1000;
	
	//strcpy (sv.name, server);
	//strcpy (sv.configstrings[CS_NAME], server);

	if (serverstate != ss_game)
	{
		sv.models[1] = CM_LoadMap ("", false, &checksum);	// no real map
	}
	else
	{
#if !KINGPIN
		char	*p;
#endif
		Com_sprintf (sv.configstrings[CS_MODELS+1],sizeof(sv.configstrings[CS_MODELS+1]),
			"maps/%s.bsp", server);
		sv.models[1] = CM_LoadMap (sv.configstrings[CS_MODELS+1], false, &checksum);

		//FUCKING HUGE AND UGLY hack to allow map overriding
		strcpy (sv.configstrings[CS_MODELS+1], CM_MapName());
#if !KINGPIN // MH: this code doesn't seem to be necessary?
		strcpy (sv.name, CM_MapName() + 5);
		p = strrchr(sv.name, '.');
		if (!p)
			Com_Error (ERR_DROP, "Aiee, sv.name is missing its period: %s", sv.name);
		p[0] = 0;
#endif
	}

	Com_sprintf (sv.configstrings[CS_MAPCHECKSUM],sizeof(sv.configstrings[CS_MAPCHECKSUM]),
		"%i", checksum);

	//
	// clear physics interaction links
	//
	SV_ClearWorld ();
	
	for (i=1 ; i< CM_NumInlineModels ; i++)
	{
		Com_sprintf (sv.configstrings[CS_MODELS+1+i], sizeof(sv.configstrings[CS_MODELS+1+i]),
			"*%i", i);
		sv.models[i+1] = CM_InlineModel (sv.configstrings[CS_MODELS+1+i]);
	}

	//
	// spawn the rest of the entities on the map
	//	

	// precache and static commands can be issued during
	// map initialization
	sv.state = ss_loading;
	Com_SetServerState (sv.state);

#if KINGPIN
	// MH: make client files downloadable
	if (clientfiles.next)
	{
		linkednamelist_t *cf = &clientfiles;
		while (cf->next)
		{
			char buf[2*MAX_QPATH];
			cf = cf->next;
			Com_sprintf(buf, sizeof(buf), "%s/%s", FS_Gamedir(), cf->name);
			if (Sys_FileLength(buf) > 0)
				DownloadIndex(cf->name);
		}
	}

	// MH: look for PAK files to make downloadable (but not in "main")
	if (allow_download_paks->intvalue && Cvar_VariableString("clientdir")[0])
	{
		for (i=0; i<10; i++)
		{
			char buf[2*MAX_QPATH];
			Com_sprintf(buf, sizeof(buf), "%s/pak%d.pak", FS_Gamedir(), i);
			if (Sys_FileLength(buf) > 0)
			{
				if (!i)
				{
					Com_Printf ("WARNING: This game has a pak0.pak file but it will not be downloadable.\n", LOG_SERVER|LOG_WARNING);
					continue;
				}
				Com_sprintf(buf, sizeof(buf), "pak%d.pak", i);
				DownloadIndex(buf);
			}
		}
	}
#endif

	// load and spawn all other entities
	if (sv.attractloop)
	{
		strcpy (sv.configstrings[CS_MAXCLIENTS], "1");
	}
	else
	{
		char *entities = CM_EntityString();
#if KINGPIN
		// MH: workaround for a bug that goes back to the original Kingpin SDK!
		char *p = entities;
		while ((p = strstr(p, "\"weapon_barmachinegun\"")) != NULL)
			memcpy(p, "\"weapon_flamethrower\" ", 22);
#endif
		ge->SpawnEntities ( sv.name, entities, spawnpoint );

		//r1ch: override what the game dll may or may not have set for this with the true value
		Com_sprintf (sv.configstrings[CS_MAXCLIENTS], sizeof(sv.configstrings[CS_MAXCLIENTS]), "%d", maxclients->intvalue);

#if KINGPIN
		// MH: make a custom sky downloadable
		if (sv.configstrings[CS_SKY][0])
		{
			qboolean dodgy = !strncmp(sv.configstrings[CS_SKY], "../", 3); // for "../textures/" paths
			if (!dodgy)
			{
				// standard skies that the player should already have
				static const char *ignore[] = { "cp", "hl", "pv", "rc", "sr", "st", "ty", NULL };
				for (i = 0; ignore[i]; i++)
					if (!Q_stricmp(sv.configstrings[CS_SKY], ignore[i])) goto skipsky;
			}
			for (i = 0; i < 7; i++)
			{
				static const char *env_suf[7] = { "rt", "bk", "lf", "ft", "up", "dn", "winrefl" };
				char buf[MAX_QPATH];
				if (dodgy)
					Com_sprintf(buf, sizeof(buf), "%s%s.tga", sv.configstrings[CS_SKY] + 3, env_suf[i]);
				else
					Com_sprintf(buf, sizeof(buf), "env/%s%s.tga", sv.configstrings[CS_SKY], env_suf[i]);
				if (i < 6 || FS_LoadFile(buf, NULL) > 0)
					DownloadIndex(buf);
			}
		}
skipsky:
#endif

		// run two frames to allow everything to settle
		ge->RunFrame ();
		ge->RunFrame ();
	}

	//verify game didn't clobber important stuff
	if ((int)checksum != atoi(sv.configstrings[CS_MAPCHECKSUM]))
		Com_Error (ERR_DROP, "Game DLL corrupted server configstrings");

	// all precaches are complete
	sv.state = serverstate;
	Com_SetServerState (sv.state);
	
	// create a baseline for more efficient communications

	//r1: baslines are now allocated on a per client basis
	//SV_CreateBaseline ();

	// check for a savegame
	SV_CheckForSavegame ();

	// set serverinfo variable
	Cvar_FullSet ("mapname", sv.name, CVAR_SERVERINFO | CVAR_NOSET);

#ifndef DEDICATED_ONLY
	if (dedicated->intvalue)
	{
#endif
		cmd = Cmd_MacroExpandString("$sv_postbeginmapcmd");
		if (cmd)
			Cbuf_ExecuteScoped (cmd);
		else
			Com_Printf ("WARNING: Error expanding $sv_postbeginmapcmd, ignored.\n", LOG_SERVER|LOG_WARNING);

		// the hook may have shut the server down under us
		if (!svs.initialized)
		{
			Com_Printf ("WARNING: $sv_postbeginmapcmd stopped the server, abandoning map load.\n", LOG_SERVER|LOG_WARNING);
			return;
		}
#ifndef DEDICATED_ONLY
	}
#endif

	Com_Printf ("-------------------------------------\n", LOG_SERVER);
	Z_Verify("SV_SpawnServer:END");
}

/*
==============
SV_InitGame

A brand new game has been started
==============
*/
void SV_InitGame (void)
{
	int		i;
	Z_Verify("SV_InitGame:START");

	if (svs.initialized)
	{
		// cause any connected clients to reconnect
		SV_Shutdown ("Server restarted\n", true, false);
	}
	else
	{
#ifndef DEDICATED_ONLY
		// make sure the client is down
		CL_Drop (false, true);
		SCR_BeginLoadingPlaque ();
#endif
		// MH: only reset uptime if server is just starting
		server_start_time = time(NULL);
	}

	svs.initialized = true;

	// get any latched variable changes (maxclients, etc)
	Cvar_GetLatchedVars ();

	if (Cvar_IntValue ("coop") && Cvar_IntValue ("deathmatch"))
	{
		Com_Printf("Deathmatch and Coop both set, disabling Coop\n", LOG_SERVER|LOG_NOTICE);
		Cvar_FullSet ("coop", "0",  CVAR_SERVERINFO | CVAR_LATCH);
	}

	// dedicated servers are can't be single player and are usually DM
	// so unless they explicity set coop, force it to deathmatch
	if (dedicated->intvalue)
	{
		if (!Cvar_IntValue ("coop"))
			Cvar_FullSet ("deathmatch", "1",  CVAR_SERVERINFO | CVAR_LATCH);
	}

	// init clients
	if (Cvar_IntValue ("deathmatch"))
	{
		if (maxclients->intvalue <= 1)
			Cvar_FullSet ("maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH);
		else if (maxclients->intvalue > MAX_CLIENTS)
			Cvar_FullSet ("maxclients", va("%i", MAX_CLIENTS), CVAR_SERVERINFO | CVAR_LATCH);
	}
	else if (Cvar_IntValue ("coop"))
	{
		if (maxclients->intvalue <= 1 || maxclients->intvalue > MAX_CLIENTS)
			Cvar_FullSet ("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	}
	else	// non-deathmatch, non-coop is one player
	{
		Cvar_FullSet ("maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH);
	}

	svs.ratelimit_badrcon.period = 1000;
	svs.ratelimit_status.period = 1000;

	svs.spawncount = randomMT()&0x7FFFFFFF;

	svs.clients = Z_TagMalloc (sizeof(client_t)*maxclients->intvalue, TAGMALLOC_CLIENTS);
	memset (svs.clients, 0, sizeof(client_t)*maxclients->intvalue);

	svs.num_client_entities = maxclients->intvalue*UPDATE_BACKUP*64;
	// MH: optionally delay client_entities allocation for reduced memory usage (also no need to clear)
	if (!sv_minimize_memory->intvalue)
		svs.client_entities = Z_TagMalloc (sizeof(entity_state_t)*svs.num_client_entities, TAGMALLOC_CL_ENTS);

	// r1: spam warning for those stupid servers that run 250 maxclients and 32 player slots
	if (maxclients->intvalue > 64)
		Com_Printf ("WARNING: Setting maxclients higher than the maximum number of players you intend to have playing can negatively affect server performance and bandwidth use.\n", LOG_SERVER|LOG_WARNING);
	
	// init network stuff
	if (maxclients->intvalue > 1)
		NET_Config (NET_SERVER);

	// init game
	SV_InitGameProgs ();

	// MH: open GeoIP database
	sv_geoipdb->changed(sv_geoipdb, sv_geoipdb->string, sv_geoipdb->string);

	// heartbeats will always be sent to the id master
	svs.last_heartbeat = curtime - 295000;		// send immediately (r1: give few secs for configs to run) (MH: using wall clock (not server time))

	if (dedicated->intvalue)
	{
		if (sv_global_master->intvalue)
			NET_StringToAdr (GLOBAL_MASTER, &master_adr[0]);
	}

	//r1: ping masters now that the network is up
	if (Cvar_IntValue ("public") && dedicated->intvalue)
	{
		for (i=0 ; i<MAX_MASTERS ; i++)
		{
			if (master_adr[i].port)
			{
				Com_Printf ("Pinging master server %s\n", LOG_SERVER|LOG_NOTICE, NET_AdrToString (&master_adr[i]));
				Netchan_OutOfBandPrint (NS_SERVER, &master_adr[i], "ping");
			}
		}
	}

	Z_Verify("SV_InitGame:END");
}


/*
======================
SV_Map

  the full syntax is:

  map [*]<map>$<startspot>+<nextserver>

command from the console or progs.
Map can also be a.cin, .pcx, or .dm2 file
Nextserver is used to allow a cinematic to play, then proceed to
another level:

	map tram.cin+jail_e3
======================
*/
void SV_Map (qboolean attractloop, const char *levelstring, qboolean loadgame)
{
	char	*cmd;
	char	level[MAX_QPATH-9]; //save space for maps/*.bsp
	char	*ch;
	char	spawnpoint[MAX_QPATH];
	size_t	l;

	Z_Verify("SV_Map:START");

#if KINGPIN
	// MH: moved here from SV_SpawnServer (needs to be before "changing" broadcast)
	if (Cvar_GetNumLatchedVars() || sv_recycle->intvalue)
	{
		Com_Printf ("Reloading Game DLL.\n", LOG_SERVER);
		sv.state = ss_dead; // triggers SV_InitGame below

		if (sv_recycle->intvalue != 2)
			Cvar_ForceSet ("sv_recycle", "0");
	}
#endif

	sv.loadgame = loadgame;
	sv.attractloop = attractloop;

	//r1: yet another buffer overflow was here.
	Q_strncpy (level, levelstring, sizeof(level)-1);

	//warning: macro expansion will overwrite cmd_argv == levelstring.
	if (sv.state != ss_dead)
	{
		cmd = Cmd_MacroExpandString("$sv_endmapcmd");
		if (cmd)
			Cbuf_ExecuteScoped (cmd);
		else
			Com_Printf ("WARNING: Error expanding $sv_endmapcmd, ignored.\n", LOG_SERVER|LOG_WARNING);
	}

	if (sv.state == ss_dead && !sv.loadgame)
	{
		// the game is just starting
		SV_InitGame ();
	}
	// MH: prevent a pointless final game frame and pending messages being sent
	else
	{
		int i;
		for (i=0 ; i<maxclients->intvalue ; i++)
		{
			if (svs.clients[i].state == cs_spawned)
			{
				svs.clients[i].state = cs_connected;
				SV_ClearMessageList(&svs.clients[i]);
			}
		}
	}

	// if there is a + in the map, set nextserver to the remainder
	ch = strchr(level, '+');
	if (ch)
	{
		*ch = 0;
		Cvar_Set ("nextserver", va("gamemap \"%s\"", ch+1));
	}
	else
		Cvar_Set ("nextserver", "");

#if !KINGPIN
	//ZOID special hack for end game screen in coop mode
	if (Cvar_IntValue ("coop") && !Q_stricmp(level, "victory.pcx"))
		Cvar_Set ("nextserver", "gamemap \"*base1\"");
#endif

	// if there is a $, use the remainder as a spawnpoint
	ch = strchr(level, '$');
	if (ch)
	{
		*ch = 0;
		strcpy (spawnpoint, ch+1);
	}
	else
		spawnpoint[0] = 0;

	// skip the end-of-unit flag if necessary
	//r1: should be using memmove for this, overlapping strcpy = unreliable
	if (level[0] == '*')
		//strcpy (level, level+1);
		memmove (level, level+1, strlen(level)+1);

	l = strlen(level);
	if (l > 4 && !strcmp (level+l-4, ".cin") )
	{
		if (attractloop)
			Com_Error (ERR_HARD, "Demomap may only be used to replay demos (*.dm2)");
#ifndef DEDICATED_ONLY
		SCR_BeginLoadingPlaque ();			// for local system
#endif
		SV_BroadcastCommand ("changing\n");
		SV_SpawnServer (level, spawnpoint, ss_cinematic, attractloop, loadgame);
	}
	else if (l > 4 && !strcmp (level+l-4, ".dm2") )
	{
		if (!attractloop)
			Com_Printf ("WARNING: Loading a Game DLL while playing back a demo. Use 'demomap' if you encounter problems.\n", LOG_GENERAL);
			//Com_Error (ERR_HARD, "Demos should be replayed using the 'demomap' command");
#ifndef DEDICATED_ONLY
		SCR_BeginLoadingPlaque ();			// for local system
#endif
		SV_BroadcastCommand ("changing\n");
		sv.attractloop = attractloop = 1;
		SV_SpawnServer (level, spawnpoint, ss_demo, attractloop, loadgame);
	}
	else if (l > 4 && !strcmp (level+l-4, ".pcx") )
	{
		if (attractloop)
			Com_Error (ERR_HARD, "Demomap may only be used to replay demos (*.dm2)");
#ifndef DEDICATED_ONLY
		SCR_BeginLoadingPlaque ();			// for local system
#endif
		SV_BroadcastCommand ("changing\n");
		SV_SpawnServer (level, spawnpoint, ss_pic, attractloop, loadgame);
	}
	else
	{
		if (attractloop)
			Com_Error (ERR_HARD, "Demomap may only be used to replay demos (*.dm2)");
#ifndef DEDICATED_ONLY
		SCR_BeginLoadingPlaque ();			// for local system
#endif
		SV_BroadcastCommand ("changing\n");
		SV_SendClientMessages ();
		SV_SpawnServer (level, spawnpoint, ss_game, attractloop, loadgame);

		//r1: do we really need this?
		//Cbuf_CopyToDefer ();
	}

	//check the server is running proper Q2 physics model
	//if (!Sys_CheckFPUStatus ())
	//	Com_Error (ERR_FATAL, "FPU control word is not set as expected, Quake II physics model will break.");

	SV_BroadcastCommand ("reconnect\n");
	Z_Verify("SV_Map:END");
}

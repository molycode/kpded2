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
#include "../geoip/maxminddb.h"

#define	HEARTBEAT_SECONDS	300

netadr_t	master_adr[MAX_MASTERS];	// address of group servers
#if KINGPIN
qboolean	master_gs[MAX_MASTERS];		// MH: master servers are Gamespy?
#endif

client_t	*sv_client;			// current client

cvar_t	*sv_paused;
cvar_t	*sv_timedemo;
cvar_t	*sv_fpsflood;

cvar_t	*sv_enforcetime;
cvar_t	*sv_enforcetime_kick;	// MH: irregular timing kick threshold

cvar_t	*timeout;				// seconds without any message
cvar_t	*zombietime;			// seconds to sink messages after disconnect

cvar_t	*rcon_password;			// password for remote server commands
cvar_t	*lrcon_password;

cvar_t	*sv_enhanced_setplayer;

cvar_t	*allow_download;
cvar_t	*allow_download_players;
cvar_t	*allow_download_models;
cvar_t	*allow_download_sounds;
cvar_t	*allow_download_maps;
cvar_t	*allow_download_pics;
#if KINGPIN
cvar_t	*allow_download_paks;
#else
cvar_t	*allow_download_textures;
cvar_t	*allow_download_others;
#endif

#if !KINGPIN
cvar_t *sv_airaccelerate;
#endif

cvar_t	*sv_noreload;			// don't reload level state when reentering

cvar_t	*maxclients;			// FIXME: rename sv_maxclients

cvar_t	*sv_showclamp;

cvar_t	*hostname;
cvar_t	*public_server;			// should heartbeats be sent

cvar_t	*sv_locked;
cvar_t	*sv_restartmap;
cvar_t	*sv_password;

//r1: filtering of strings for non-ASCII
#if !KINGPIN
cvar_t	*sv_filter_q3names;
#endif
cvar_t	*sv_filter_userinfo;
cvar_t	*sv_filter_stringcmds;

//r1: stupid bhole code
cvar_t	*sv_blackholes;

//r1: allow server to ban people who use nodelta (uses 3-5x more bandwidth than regular client)
cvar_t	*sv_allownodelta;

#if !KINGPIN
//r1: allow server to block q2ace and associated hacks
cvar_t	*sv_deny_q2ace;
#endif

//r1: max connections from a single IP (prevent DoS)
cvar_t	*sv_iplimit;

//r1: allow server to send mini-motd
cvar_t	*sv_connectmessage;

//r1: for nocheat mods
cvar_t  *sv_nc_visibilitycheck;
cvar_t	*sv_nc_clientsonly;

//r1: max backup packets to allow from client
cvar_t	*sv_max_netdrop;

//r1: limit amount of info from status cmd
cvar_t	*sv_hidestatus;
cvar_t	*sv_hideplayers;

//r1: randomize server framenum
cvar_t	*sv_randomframe;

//r1: number of msecs to give
cvar_t	*sv_msecs;

#if !KINGPIN
cvar_t	*sv_nc_kick;
cvar_t	*sv_nc_announce;
cvar_t	*sv_filter_nocheat_spam ;
#endif

cvar_t	*sv_gamedebug;
cvar_t	*sv_recycle;

cvar_t	*sv_uptime;

#if !KINGPIN
cvar_t	*sv_strafejump_hack;
#endif
cvar_t	*sv_reserved_slots;
cvar_t	*sv_reserved_password;

cvar_t	*sv_allow_map;
cvar_t	*sv_allow_unconnected_cmds;

cvar_t	*sv_strict_userinfo_check;

#if !KINGPIN
cvar_t	*sv_calcpings_method;
#endif

cvar_t	*sv_no_game_serverinfo;

cvar_t	*sv_mapdownload_denied_message;
cvar_t	*sv_mapdownload_ok_message;
#if KINGPIN
cvar_t	*sv_pakdownload_message;
#endif

#if !KINGPIN
cvar_t	*sv_downloadserver;

cvar_t	*sv_max_traces_per_frame;
#endif

cvar_t	*sv_ratelimit_status;

#if !KINGPIN
cvar_t	*sv_new_entflags;
#endif

cvar_t	*sv_validate_playerskins;

cvar_t	*sv_idlekick;
cvar_t	*sv_entity_inuse_hack;

cvar_t	*sv_force_reconnect;
cvar_t	*sv_download_refuselimit;

cvar_t	*sv_download_drop_file;
cvar_t	*sv_download_drop_message;

cvar_t	*sv_blackhole_mask;
cvar_t	*sv_badcvarcheck;

cvar_t	*sv_rcon_buffsize;
cvar_t	*sv_rcon_showoutput;
cvar_t	*sv_rcon_ratelimit;

cvar_t	*sv_show_name_changes;

cvar_t	*sv_optimize_deltas;

cvar_t	*sv_predict_on_lag;
cvar_t	*sv_format_string_hack;

#if !KINGPIN
cvar_t	*sv_func_plat_hack;
#endif
cvar_t	*sv_max_packetdup;

// MH: default packetdup
cvar_t	*sv_packetdup;

cvar_t	*sv_redirect_address;
#if !KINGPIN
cvar_t	*sv_fps;

cvar_t	*sv_max_player_updates;
#endif
cvar_t	*sv_minpps;
cvar_t	*sv_disconnect_hack;

#if !KINGPIN
cvar_t	*sv_interpolated_pmove;
#endif

cvar_t	*sv_global_master;

#if !KINGPIN
cvar_t	*sv_disallow_download_sprites_hack;
#endif

cvar_t	*sv_features;
cvar_t	*g_features;

// MH: log connecting client userinfo
cvar_t	*sv_log_connectinfo;

// MH: handle name clashes?
cvar_t	*sv_unique_names;

// MH: broadcast connecting players?
cvar_t	*sv_show_connections;

// MH: disable fov zooming
cvar_t	*sv_no_zoom;

// MH: idle mode
cvar_t	*g_idle;
cvar_t	*sv_autoidle;

// MH: minimize memory usage
cvar_t	*sv_minimize_memory;

// MH: GeoIP database
cvar_t *sv_geoipdb;
static MMDB_s mmdb;

#if KINGPIN
// MH: allow models that are not on the server
cvar_t	*sv_allow_any_model;

// MH: compression level
cvar_t	*sv_compress;

// MH: compression level for downloads
cvar_t	*sv_compress_downloads;

// MH: pre-cache downloads to the system file cache
cvar_t	*sv_download_precache;

// MH: upstream bandwidth available
cvar_t	*sv_bandwidth;

// MH: underwater sound
cvar_t	*sv_underwater_sound;
#endif

//r1: not needed
//cvar_t	*sv_reconnect_limit;	// minimum seconds between connect messages

time_t	server_start_time;

// MH: game start time
time_t	game_start_time;

blackhole_t			blackholes;
varban_t			cvarbans;
varban_t			userinfobans;
bannedcommands_t	bannedcommands;
linkednamelist_t	nullcmds;
linkednamelist_t	lrconcmds;
linkedvaluelist_t	serveraliases;

#if KINGPIN
// MH: downloadable client files
linkednamelist_t	clientfiles;
#endif

//i hate you snake
netblock_t			blackhole_exceptions;

//void Master_Shutdown (void);

#if !KINGPIN
static int CharVerify (char c)
{
	if (sv_filter_q3names->intvalue == 2)
		return !(isalnum(c));
	else
		return (c == '^');
}

//============================================================================

static int NameColorFilterCheck (const char *newname)
{
	size_t	length;
	size_t	i;

	if (!sv_filter_q3names->intvalue)
		return 0;

	length = strlen (newname)-1;

	for (i = 0; i < length; i++)
	{
		if (CharVerify(*(newname+i)) && isdigit (*(newname+i+1)))	{
			return 1;
		}
	}

	return 0;
}
#endif

static int StringIsWhitespace (const char *name)
{
	const char	*p;

	p = name;

	while (p[0])
	{
		if (!isspace (p[0]))
			return 0;
		p++;
	}

	return 1;
}

/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/
void SV_DropClient (client_t *drop, qboolean notify)
{
	// add/force the disconnect
	if (notify)
	{
		MSG_BeginWriting (svc_disconnect);
		SV_AddMessage (drop, true);

		// MH: send immediately
		SV_WriteReliableMessages(drop, drop->netchan.message.buffsize);
		Netchan_Transmit(&drop->netchan, 0, NULL);
	}
	else
	{
		//they did something naughty so they won't be seeing anything else from us...
		if (drop->messageListData)
		{
			SV_ClearMessageList (drop);
			Z_Free (drop->messageListData);
		}

		drop->messageListData = drop->msgListEnd = drop->msgListStart = NULL;
	}

	if ((svs.game_features & GMF_WANT_ALL_DISCONNECTS) || drop->state == cs_spawned)
	{
		// call the prog function for removing a client
		// this will remove the body, among other things
		ge->ClientDisconnect (drop->edict);
	}

	//r1: fix for mods that don't clean score
	drop->edict->client->ps.stats[STAT_FRAGS] = 0;

	drop->state = cs_zombie;
	drop->name[0] = 0;
}

void SV_KickClient (client_t *cl, const char /*@null@*/*reason, const char /*@null@*/*cprintf)
{
	if (reason && cl->state == cs_spawned && cl->name[0])
		SV_BroadcastPrintf (PRINT_HIGH, "%s was dropped: %s\n", cl->name, reason);
	if (cprintf)
		SV_ClientPrintf (cl, PRINT_HIGH, "%s", cprintf);
	Com_Printf ("Dropping %s, %s.\n", LOG_SERVER|LOG_DROP, cl->name, reason ? reason : "SV_KickClient");
	SV_DropClient (cl, true);
}

//r1: this does the final cleaning up of a client after zombie state.
void SV_CleanClient (client_t *drop)
{
	//r1: drop message list
	if (drop->messageListData)
	{
		SV_ClearMessageList (drop);
		Z_Free (drop->messageListData);
	}

	//shouldn't be necessary, but...
	drop->messageListData = drop->msgListEnd = drop->msgListStart = NULL;

#if !KINGPIN
	//r1: free version string
	if (drop->versionString)
	{
		Z_Free (drop->versionString);
		drop->versionString = NULL;
	}
#endif

	// MH: free download stuff
	SV_CloseDownload(drop);

	//r1: free baselines
	if (drop->lastlines)
	{
		Z_Free (drop->lastlines);
		drop->lastlines = NULL;
	}

	// MH: client demo
	if (drop->demofile)
	{
		int len = -1;
		fwrite (&len, 4, 1, drop->demofile);
		fclose (drop->demofile);
		drop->demofile = NULL;
	}

	drop->state = cs_free; // MH: slot is now free

#ifdef _WIN32
	// MH: handle auto priority
	if (!strcmp(win_priority->string, "auto"))
		win_priority->changed(win_priority, "auto", win_priority->string);
#endif
}

const banmatch_t *SV_CheckUserinfoBans (char *userinfo, char *key)
{
	qboolean			waitForKey;
	char				myKey[MAX_INFO_VALUE];
	char				value[MAX_INFO_VALUE];
	const char			*s, *p;
	const banmatch_t	*match;

	if (!userinfobans.next)
		return NULL;

	s = userinfo;

	if (key[0])
		waitForKey = true;
	else
		waitForKey = false;

	while (s && *s)
	{
		s++;

		p = strchr (s, '\\');
		if (p)
		{
			Q_strncpy (myKey, s, p-s);
		}
		else
		{
			//uh oh...
			Com_Printf ("WARNING: Malformed userinfo string in SV_CheckUserinfoBans!\n", LOG_SERVER|LOG_WARNING);
			return NULL;
		}

		p++;
		s = strchr (p, '\\');
		if (s)
		{
			Q_strncpy (value, p, s-p);
		}
		else
		{
			Q_strncpy (value, p, MAX_INFO_VALUE-1);
		}

		if (waitForKey)
		{
			if (!strcmp (myKey, key))
				waitForKey = false;

			continue;
		}

		match = VarBanMatch (&userinfobans, myKey, value);

		if (match)
		{
			strcpy (key, myKey);
			return match;
		}
	}

	return NULL;
}

/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
===============
SV_StatusString

Builds the string that is sent as heartbeats and status replies
===============
*/

// MH: separated from SV_StatusString so that it can be used for Gamespy requests too
static char *StatusString (void)
{
	char	*serverinfo;

	serverinfo = Cvar_Serverinfo();

#if KINGPIN
	// MH: set "game" to client-side value
	Info_SetValueForKey(serverinfo, "game", Cvar_VariableString("clientdir"));
#endif

	//r1: add uptime info
	if (sv_uptime->intvalue)
		Info_SetValueForKey (serverinfo, "uptime", TimeDurationString(time(NULL) - server_start_time, false)); // MH: use new time duration string function

	//r1: hide reserved slots
	Info_SetValueForKey (serverinfo, "maxclients", va("%d", maxclients->intvalue - sv_reserved_slots->intvalue));

#if !KINGPIN
	//r1: set port to help nat traversal
	Info_SetValueForKey (serverinfo, "port", va("%d", server_port));
#endif

	// MH: let them know that the server shouldn't be listed publicly
	if (!public_server->intvalue)
		Info_SetValueForKey (serverinfo, "private", "1");

#if KINGPIN
	// MH: useful to see if the server is passworded/locked (Qtracker shows a padlock)
	if (sv_password->string[0] || sv_locked->intvalue)
		Info_SetValueForKey (serverinfo, "password", "1");
#endif

	return serverinfo;
}

// MH: get hostname string for server browsers, including game start timer if set
static char *HostnameString (void)
{
	if (game_start_time)
	{
		time_t t = time(NULL);
		if (t < game_start_time)
		{
			int d = (game_start_time - t + 59) / 60;
			return va("%s (starts in %s)", hostname->string, TimeDurationString(d * 60, false));
		}
		else
			game_start_time = 0;
	}
	return hostname->string;
}

static const char *SV_StatusString (void)
{
	char	*serverinfo;
	char	player[1024];
	static char	status[MAX_MSGLEN - 16];
	int		i;
	client_t	*cl;
	int		statusLength;
	int		playerLength;
//	player_state_t	*ps;

	serverinfo = StatusString();

	// MH: include game start timer with hostname if set
	if (game_start_time)
		Info_SetValueForKey (serverinfo, "hostname", HostnameString());

	strcpy (status, serverinfo);
	strcat (status, "\n");
	statusLength = (int)strlen(status);

	if (!sv_hideplayers->intvalue)
	{
		for (i=0 ; i<(int)maxclients->intvalue ; i++)
		{
			cl = &svs.clients[i];
			if (cl->state >= cs_connected)
			{
				Com_sprintf (player, sizeof(player), "%i %i \"%s\"\n", 
						cl->edict->client->ps.stats[STAT_FRAGS], cl->ping, cl->name);

				playerLength = (int)strlen(player);
				if (statusLength + playerLength >= sizeof(status) )
					break;		// can't hold any more
				strcpy (status + statusLength, player);
				statusLength += playerLength;
			}
		}
	}

	return status;
}

static qboolean RateLimited (ratelimit_t *limit, int maxCount)
{
	int diff;

	diff = curtime - limit->time; // MH: using wall clock (not server time)

	//a new sampling period
	if (diff > limit->period || diff < 0)
	{
		limit->time = curtime; // MH: using wall clock (not server time)
		limit->count = 0;
	}
	else
	{
		if (limit->count >= maxCount)
			return true;
	}

	return false;
}

static void RateSample (ratelimit_t *limit)
{
	int diff;

	diff = curtime - limit->time; // MH: using wall clock (not server time)

	//a new sampling period
	if (diff > limit->period || diff < 0)
	{
		limit->time = curtime; // MH: using wall clock (not server time)
		limit->count = 1;
	}
	else
	{
		limit->count++;
	}
}

// MH: check if current packet is from a master server
static int FromMaster()
{
	int i;

	for (i=0; i<MAX_MASTERS; i++)
	{
		if (master_adr[i].port && NET_CompareBaseAdr (&master_adr[i], &net_from))
			return i;
	}

	return -1;
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
================
*/
static void SVC_Status (void)
{
	if (sv_hidestatus->intvalue)
		return;

	RateSample (&svs.ratelimit_status);

	if (RateLimited (&svs.ratelimit_status, sv_ratelimit_status->intvalue))
	{
		Com_DPrintf ("SVC_Status: Dropped status request from %s\n", NET_AdrToString (&net_from));
		return;
	}

	Netchan_OutOfBandPrint (NS_SERVER, &net_from, "print\n%s", SV_StatusString());
}

/*
================
SVC_Ack

================
*/
static void SVC_Ack (void)
{
	//r1: could be used to flood server console - only show acks from masters.

	if (FromMaster() >= 0)
		Com_Printf ("Ping acknowledge from %s\n", LOG_SERVER|LOG_NOTICE, NET_AdrToString(&net_from));
}

/*
================
SVC_Info

Responds with short info for broadcast scans
The second parameter should be the current protocol version number.
================
*/
static void SVC_Info (void)
{
	char	string[80]; // MH: client supports 80
	int		i, count;
	int		version;

	if (maxclients->intvalue == 1)
		return;		// ignore in single player

	version = atoi (Cmd_Argv(1));

#if KINGPIN
	if (version != PROTOCOL_ORIGINAL)
#else
	if (version != PROTOCOL_ORIGINAL && version != PROTOCOL_R1Q2)
#endif
	{
		//r1: return instead of sending another packet. prevents spoofed udp packet
		//    causing server <-> server info loops.
		return;
	}
	else
	{
		count = 0;
		for (i=0 ; i<maxclients->intvalue ; i++)
			if (svs.clients[i].state >= cs_spawning)
				count++;

		// MH: crop hostname if required to make string fit buffer, and don't bother with padding
		Com_sprintf (string, sizeof(string), "%.*s %s %2i/%2i\n",
			sizeof(string) - strlen(sv.name) - 9, HostnameString(), sv.name, count, maxclients->intvalue - sv_reserved_slots->intvalue);
	}

	Netchan_OutOfBandPrint (NS_SERVER, &net_from, "info\n%s", string);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
static void SVC_Ping (void)
{
	Netchan_OutOfBandPrint (NS_SERVER, &net_from, "ack");
}


/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
static qboolean SV_ChallengeIsInUse (uint32 challenge)
{
	client_t	*cl;

	if (!challenge)
		return true;

	for (cl = svs.clients; cl < svs.clients + maxclients->intvalue; cl++)
	{
		if (cl->state == cs_free)
			continue;

		if (cl->challenge == challenge)
			return true;
	}
	return false;
}

static void SVC_GetChallenge (void)
{
	int		i;
	int		oldest = 0;
	uint32	oldestTime = 0, diff;

	// see if we already have a challenge for this ip
	for (i = 0 ; i < MAX_CHALLENGES ; i++)
	{
		if (NET_CompareBaseAdr (&net_from, &svs.challenges[i].adr))
			break;

		diff = curtime - svs.challenges[i].time;
		if (diff > oldestTime)
		{
			oldestTime = diff;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		do
		{
			svs.challenges[oldest].challenge = randomMT()&0x7FFFFFFF;
		} while (SV_ChallengeIsInUse (svs.challenges[oldest].challenge));
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = curtime;
		i = oldest;
	} else {
		do
		{
			svs.challenges[i].challenge = randomMT()&0x7FFFFFFF;
		} while (SV_ChallengeIsInUse (svs.challenges[i].challenge));
		svs.challenges[i].time = curtime;
	}

	// send it back
#if KINGPIN
	Netchan_OutOfBandPrint (NS_SERVER, &net_from, "challenge %d", svs.challenges[i].challenge);
#else
	Netchan_OutOfBandPrint (NS_SERVER, &net_from, "challenge %d p=34,35", svs.challenges[i].challenge);
#endif
}

#if 0
static qboolean CheckUserInfoFields (char *userinfo)
{
	if (!Info_KeyExists (userinfo, "name")) /* || !Info_KeyExists (userinfo, "rate") 
		|| !Info_KeyExists (userinfo, "msg") || !Info_KeyExists (userinfo, "hand")
		|| !Info_KeyExists (userinfo, "skin"))*/
		return false;

	return true;
}
#endif

static qboolean SV_UserinfoValidate (const char *userinfo)
{
	const char		*s;
	const char		*p;

	s = userinfo;

	while (s && s[0])
	{
		//missing key separator
		if (s[0] != '\\')
			return false;

		s++;

		p = strchr (s, '\\');
		if (p)
		{
			//oversized key
			if (p-s >= MAX_INFO_KEY)
				return false;
		}
		else
		{
			//missing value separator
			return false;
		}

		p++;

		//missing value
		if (!p[0])
			return false;

		s = strchr (p, '\\');
		if (s)
		{
			//oversized value
			if (s-p >= MAX_INFO_VALUE)
				return false;
		}
		else
		{
			//oversized value at end of string
			if (strlen (p) >= MAX_INFO_VALUE)
				return false;
		}
	}

	return true;
}

// MH: handle name clashes
static void SV_NameClash (client_t *cl, char *userinfo)
{
	char	*val;
	int		i;

	if (!sv_unique_names->intvalue)
		return;

	val = Info_ValueForKey (userinfo, "name");

	if (val[0])
	{
		//truncate
#if KINGPIN
		val[13] = 0;
#else
		val[15] = 0;
#endif

		for (i=0; i<maxclients->intvalue; i++)
		{
			client_t *cl2 = &svs.clients[i];

			if (cl2 == cl || cl2->state <= cs_zombie)
				continue;

			if (!strcmp(cl2->name, val))
			{
				if (cl->name[0])
					strcpy(val, cl->name); // keep the existing name
				else
				{ // append number to name
					int n, len;
#if KINGPIN
					val[11] = 0;
#else
					val[13] = 0;
#endif
					len = strlen(val);
					for (n = 2; ; n++)
					{
						sprintf(val + len, "%2d", n);
						for (i=0 ; i<maxclients->intvalue ; i++)
						{
							cl2 = &svs.clients[i];
							if (cl2 == cl || cl2->state <= cs_zombie)
								continue;
							if (!strcmp(cl2->name, val))
								break;
						}
						if (i == maxclients->intvalue)
							break;
					}
					cl->notes |= NOTE_NAMECLASH;
				}
				Info_SetValueForKey (userinfo, "name", val);
				break;
			}
		}
	}
}

static void SV_UpdateUserinfo (client_t *cl, qboolean notifyGame)
{
	char	*val;
	int		i;

#if KINGPIN
	// MH: only allow models that the server has (let game dll handle teamplay)
	cl->badmodel[0] = 0;
	if (!sv_allow_any_model->intvalue && !Cvar_IntValue("teamplay"))
	{
		char *s;
		val = Info_ValueForKey(cl->userinfo, "skin");
		s = strrchr(val, '/');
		if (s)
		{
			*s = 0;
			Q_strlwr(val);
			if (strcmp(val, "female_chick") && strcmp(val, "male_thug") && strcmp(val, "male_runt"))
			{
				if (FS_LoadFile(va("players/%s/head.mdx", val), NULL) <= 0)
				{
					strcpy(cl->badmodel, val);
					if (cl->skin[0])
						Info_SetValueForKey(cl->userinfo, "skin", cl->skin);
					else if (strstr(val, "female"))
						Info_SetValueForKey(cl->userinfo, "skin", va("female_chick/%03d %03d %03d", 1 + (rand() % 22), 12 + (rand() % 10), 1 + (rand() % 6)));
					else
						Info_SetValueForKey(cl->userinfo, "skin", va("male_thug/%03d %03d %03d", 8 + (rand() % 24), 1 + (rand() % 36), 1 + (rand() % 17)));
				}
				else
				{
					// check the skins too
					qboolean replaced = false;
					if (FS_LoadFile(va("players/%s/head_%.3s.tga", val, s + 1), NULL) <= 0
						&& FS_LoadFile(va("players/%s/head_001.tga", val), NULL) > 0)
					{
						memcpy(s + 1, "001", 3);
						replaced = true;
					}
					if (FS_LoadFile(va("players/%s/body_%.3s.tga", val, s + 5), NULL) <= 0
						&& FS_LoadFile(va("players/%s/body_001.tga", val), NULL) > 0)
					{
						memcpy(s + 5, "001", 3);
						replaced = true;
					}
					if (FS_LoadFile(va("players/%s/legs_%.3s.tga", val, s + 9), NULL) <= 0
						&& FS_LoadFile(va("players/%s/legs_001.tga", val), NULL) > 0)
					{
						memcpy(s + 9, "001", 3);
						replaced = true;
					}
					if (replaced)
					{
						s[0] = '/';
						strcpy(cl->badmodel, Info_ValueForKey(cl->userinfo, "skin"));
						Info_SetValueForKey(cl->userinfo, "skin", val);
					}
				}
			}
		}
	}
#endif

	// MH: disable fov zooming
	if (sv_no_zoom->intvalue)
	{
		int fov = atoi(Info_ValueForKey(cl->userinfo, "fov"));
		if (fov < 90)
		{
			if (cl->state >= cs_connected)
				SV_ClientPrintf (cl, PRINT_HIGH, "FOV zooming is not allowed\n");
			Info_SetValueForKey(cl->userinfo, "fov", "90");
		}
	}

	// MH: handle name clashes
	SV_NameClash(cl, cl->userinfo);

	// call prog code to allow overrides
	if (notifyGame)
		ge->ClientUserinfoChanged (cl->edict, cl->userinfo);

	val = Info_ValueForKey (cl->userinfo, "name");

	if (val[0])
	{
		//truncate
		val[15] = 0;

		//r1: notify console
		if (cl->name[0] && val[0] && strcmp (cl->name, val))
		{
			Com_Printf ("%s[%s] changed name to %s.\n", LOG_SERVER|LOG_NAME, cl->name, NET_AdrToString (&cl->netchan.remote_address), val);
			if (sv_show_name_changes->intvalue)
				SV_BroadcastPrintf (PRINT_HIGH, "%s changed name to %s.\n", cl->name, val);
		}
		
		// name for C code
		strcpy (cl->name, val);

		// mask off high bit
		for (i=0 ; i<sizeof(cl->name)-1; i++)
			cl->name[i] &= 127;
	}
	else
	{
		cl->name[0] = 0;
	}

	// rate command
	val = Info_ValueForKey (cl->userinfo, "rate");
	if (val[0])
	{
		i = atoi(val);
		cl->rate = i;
		// MH: minumum raised from 100 to 1000, and maximum from 15000 to 99000
		if (cl->rate < 1000)
			cl->rate = 1000;
		if (cl->rate > 99000)
			cl->rate = 99000;
	}
	else
		cl->rate = 5000;

	// msg command
	val = Info_ValueForKey (cl->userinfo, "msg");
	if (val[0])
	{
		cl->messagelevel = atoi(val);

		//safety check for integer overflow...
		if (cl->messagelevel < 0)
			cl->messagelevel = 0;
	}

#if KINGPIN
	// MH: retain current skin
	val = Info_ValueForKey (cl->userinfo, "skin");
	if (val[0])
		strncpy(cl->skin, val, sizeof(cl->skin) - 1);
#endif
}

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
static void SVC_DirectConnect (void)
{
	netadr_t	*adr;

	client_t	*cl, *newcl;

	edict_t		*ent;

	//const banmatch_t	*match;

	int			i;
	unsigned	edictnum;
	unsigned	protocol;
	unsigned	version;
	int			challenge;
	int			previousclients;

	qboolean	allowed;
	qboolean	reconnected;

	char		*pass;

	char		saved_var[32];
	char		saved_val[32];
	char		userinfo[MAX_INFO_STRING];
	//char		key[MAX_INFO_KEY];

	int			reserved;
	unsigned	msglen;

	uint16		qport;

	adr = &net_from;

	Com_DPrintf ("SVC_DirectConnect (%s)\n", NET_AdrToString (adr));

	//r1: check version first of all
	protocol= atoi(Cmd_Argv(1));
#if KINGPIN
	if (protocol != PROTOCOL_ORIGINAL)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nYou need Kingpin to play on this server.\n");
#else
	if (protocol != PROTOCOL_ORIGINAL && protocol != PROTOCOL_R1Q2)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nYou need Quake II 3.19 or higher to play on this server.\n");
#endif
		Com_DPrintf ("    rejected connect from protocol %i\n", protocol);
		return;
	}

	qport = atoi(Cmd_Argv(2));

#if !KINGPIN
	if (protocol == PROTOCOL_R1Q2)
	{
		//work around older protocol 35 clients
		if (qport > 0xFF)
			qport = 0;

		//max message length
		if (Cmd_Argv(5)[0])
		{
			msglen = strtoul (Cmd_Argv(5), NULL, 10);

			//512 is arbitrary, any messages larger will overflow the client anyway
			//so choosing low values is fairly silly anyway. this simply prevents someone
			//using msglength 30 and spamming the messagelist queue.

			if (msglen == 0)
			{
				msglen = MAX_USABLEMSG;
			}
			else if (msglen > MAX_USABLEMSG || msglen < 512)
			{
				Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nInvalid maximum message length.\n");
				Com_DPrintf ("    rejected msglen %u\n", msglen);
				return;
			}

			i = Cvar_IntValue ("net_maxmsglen");

			if (i && msglen > i)
				msglen = i;
		}
		else
		{
			msglen = 1390;
		}

		//protocol 5 subversion
		if (Cmd_Argv(6)[0])
		{
			version = strtoul (Cmd_Argv(6), NULL, 10);
		}
		else
		{
			//1903 is assumed by older clients that don't transmit.
			version = 1903;
		}
	}
	else
#endif
	{
		msglen = 0;
		version = 0;
	}

	challenge = atoi(Cmd_Argv(3));

	// see if the challenge is valid
	if (!NET_IsLocalHost (adr))
	{
		for (i=0 ; i<MAX_CHALLENGES ; i++)
		{
			if (svs.challenges[i].challenge && NET_CompareBaseAdr (adr, &svs.challenges[i].adr))
			{
				//r1: reset challenge
				if (challenge == svs.challenges[i].challenge)
				{
					svs.challenges[i].challenge = 0;
					break;
				}
				Com_DPrintf ("    bad challenge %i\n", challenge);
				Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nBad challenge.\n");
				return;
			}
		}
		if (i == MAX_CHALLENGES)
		{
			Com_DPrintf ("    no challenge\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nNo challenge for address.\n");
			return;
		}
	}

	//r1: deny if server is locked
	if (sv_locked->intvalue)
	{
		Com_DPrintf ("    server locked\n");
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is locked.\n");
		return;
	}

	Q_strncpy (userinfo, Cmd_Argv(4), sizeof(userinfo)-1);

	//r1: check it is not overflowed, save enough bytes for /ip/111.222.333.444:55555
	if (strlen(userinfo) + 25 >= sizeof(userinfo)-1)
	{
		Com_DPrintf ("    userinfo length exceeded\n");
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nUserinfo string length exceeded.\n");
		return;
	}
	else if (!userinfo[0])
	{
		Com_DPrintf ("    empty userinfo string\n");
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nBad userinfo string.\n");
		return;
	}
	else if (!Info_Validate (userinfo))
	{
		Com_DPrintf ("    invalid userinfo string\n");
		Com_Printf ("WARNING: Info_Validate failed for %s (%s)\n", LOG_SERVER|LOG_WARNING, NET_AdrToString (adr), MakePrintable (userinfo, 0));
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nInvalid userinfo string.\n");
		return;
	}

	if (!SV_UserinfoValidate (userinfo))
	{
		//Com_Printf ("WARNING: SV_UserinfoValidate failed for %s (%s)\n", LOG_SERVER|LOG_WARNING, NET_AdrToString (adr), MakePrintable (userinfo));
		Com_Printf ("EXPLOIT: Client %s supplied an illegal userinfo string: %s\n", LOG_EXPLOIT|LOG_SERVER, NET_AdrToString(adr), MakePrintable (userinfo, 0));
		Blackhole (adr, true, sv_blackhole_mask->intvalue, BLACKHOLE_SILENT, "illegal userinfo string");
		return;
	}

	//r1ch: ban anyone trying to use the end-of-message-in-string exploit
	if (strchr(userinfo, '\xFF'))
	{
		char		*ptr;
		const char	*p;
		ptr = strchr (userinfo, '\xFF');
		ptr -= 8;
		if (ptr < userinfo)
			ptr = userinfo;
		p = MakePrintable (ptr, 0);
		Com_Printf ("EXPLOIT: Client %s supplied userinfo string containing 0xFF: %s\n", LOG_EXPLOIT|LOG_SERVER, NET_AdrToString(adr), p);
		Blackhole (adr, true, sv_blackhole_mask->intvalue, BLACKHOLE_SILENT, "0xFF in userinfo (%.32s)", p);
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n");
		return;
	}

	if (sv_strict_userinfo_check->intvalue)
	{
		if (!Info_CheckBytes (userinfo))
		{
			Com_Printf ("WARNING: Info_CheckBytes failed for %s (%s)\n", LOG_SERVER|LOG_WARNING, NET_AdrToString (adr), MakePrintable (userinfo, 0));
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nUserinfo contains illegal bytes.\n");
			return;
		}
	}

	//r1ch: allow filtering of stupid "fun" names etc.
	if (sv_filter_userinfo->intvalue)
		StripHighBits(userinfo, (int)sv_filter_userinfo->intvalue == 2);

	//r1ch: simple userinfo validation
	/*if (!CheckUserInfoFields(userinfo)) {
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nMalformed userinfo string.\n");
		Com_DPrintf ("illegal userinfo from %s\n", NET_AdrToString (adr));
		return;
	}*/

	/*key[0] = 0;
	while ((match = SV_CheckUserinfoBans (userinfo, key)))
	{
		if (match->message[0])
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\n%s\n", match->message);

		if (match->blockmethod == CVARBAN_LOGONLY)
		{
			Com_Printf ("LOG: %s[%s] matched userinfoban: %s == %s\n", LOG_SERVER, Info_ValueForKey (userinfo, "name"), NET_AdrToString(adr), key, Info_ValueForKey (userinfo, key));
		}
		else
		{
			if (match->blockmethod == CVARBAN_BLACKHOLE)
				Blackhole (adr, true, sv_blackhole_mask->intvalue, BLACKHOLE_SILENT, "userinfovarban: %s == %s", key, Info_ValueForKey (userinfo, key));

			Com_DPrintf ("    userinfo ban %s matched\n", key);
			return;
		}
	}*/

#if KINGPIN
	// MH: at least v1.20 is needed for download support
	version = atoi(Info_ValueForKey(userinfo, "ver"));
	if (version < 120)
	{
		Com_DPrintf ("    old version\n");
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nYou need an updated version of Kingpin to connect to this server. Please install the v1.21 patch from www.kingpin.info\n");
		return;
	}
#endif

	pass = Info_ValueForKey (userinfo, "name");

	if (!pass[0] || StringIsWhitespace (pass))
	{
		Com_DPrintf ("    missing name\n");
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nPlease set your name before connecting.\n");
		return;
	}
#if !KINGPIN
	else if (NameColorFilterCheck (pass))
	{
		Com_DPrintf ("    color name filter check failed\n");
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nQuake 3 style colored names are not permitted on this server.\n");
		return;
	}
#endif
	else if (Info_KeyExists (userinfo, "ip"))
	{
		char	*p;
		p = Info_ValueForKey(userinfo, "ip");
		Com_Printf ("EXPLOIT: Client %s[%s] attempted to spoof IP address: %s\n", LOG_EXPLOIT|LOG_SERVER, Info_ValueForKey (userinfo, "name"), NET_AdrToString(adr), p);
		Blackhole (adr, true, sv_blackhole_mask->intvalue, BLACKHOLE_SILENT, "attempted to spoof ip '%s'", p);
		return;
	}

	pass = Info_ValueForKey (userinfo, "password");

#if KINGPIN
	if (sv_password->string[0])
#else
	if (sv_password->string[0] && strcmp(sv_password->string, "none"))
#endif
	{
		if (!pass[0])
		{
			Com_DPrintf ("    empty password\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nPassword required.\n");
			return;
		}
		else if (strcmp (pass, sv_password->string))
		{
			Com_DPrintf ("    bad password\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nInvalid password.\n");
			return;
		}
	}

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey (userinfo, "ip", NET_AdrToString(adr));

	// attractloop servers are ONLY for local clients
	// r1: demo serving anyone?
	/*if (sv.attractloop)
	{
		if (!NET_IsLocalAddress (adr))
		{
			Com_Printf ("Remote connect in attract loop.  Ignored.\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n");
			return;
		}
	}*/

	if (sv_reserved_password->string[0] && !strcmp (pass, sv_reserved_password->string))
	{
		reserved = 0;

		//r1: this prevents mod/admin dll from also checking password as some mods incorrectly
		//refuse if password cvar exists and no password is set on the server. by definition a
		//server with reserved slots should be public anyway.
		Info_RemoveKey (userinfo, "password");
	}
	else
	{
		reserved = sv_reserved_slots->intvalue;
	}

	//newcl = &temp;
//	memset (newcl, 0, sizeof(client_t));

	saved_var[0] = saved_val[0] = 0;

	// if there is already a slot for this ip, reuse it
	for (i=0,cl=svs.clients ; i < maxclients->intvalue; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;

		if (NET_CompareAdr (adr, &cl->netchan.remote_address))
		{
			//r1: !! fix nasty bug where non-disconnected clients (from dropped disconnect
			//packets) could be overwritten!
			if (cl->state != cs_zombie)
			{
				Com_DPrintf ("    client already found\n");

				//if we legitly get here, spoofed udp isn't possible (passed challenge) and client addr/port combo is exactly
				//the same, so we can assume its really a dropped/crashed client. i hope...

				//Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nPlayer '%s' is already connected from your address.\n", cl->name);
				//SV_BroadcastPrintf (PRINT_HIGH, "%s was dropped: ghost client", cl->name);
				Com_Printf ("Dropping %s, ghost reconnect\n", LOG_SERVER|LOG_DROP, cl->name);
#if KINGPIN
				// MH: indicate to the game that the player is reconnecting
				cl->edict->client->ping = -1;
#endif
				SV_DropClient (cl, false);
				//return;
			}

			//r1: not needed
			/*if (!NET_IsLocalAddress (adr) && (svs.realtime - cl->lastconnect) < ((int)sv_reconnect_limit->value * 1000))
			{
				Com_DPrintf ("%s:reconnect rejected : too soon\n", NET_AdrToString (adr));
				return;
			}*/
			Com_DPrintf ("%s:reconnect\n", NET_AdrToString (adr));

			//r1: clean up last client data
			SV_CleanClient (cl);

			if (cl->reconnect_var[0])
			{
				if (adr->port != cl->netchan.remote_address.port)
					Com_Printf ("WARNING: %s[%s] reconnected from a different port (%d -> %d)! Tried to use ratbot/proxy or has broken router?\n", LOG_SERVER|LOG_WARNING, Info_ValueForKey (userinfo, "name"), NET_AdrToString (adr), ShortSwap (cl->netchan.remote_address.port), ShortSwap(adr->port));

#if !KINGPIN
				if (cl->protocol != protocol)
					Com_Printf ("WARNING: %s[%s] reconnected using a different protocol (%d -> %d)! Why would that happen?\n", LOG_SERVER|LOG_WARNING, Info_ValueForKey (userinfo, "name"), NET_AdrToString (adr), cl->protocol, protocol);
#endif
			}

			strcpy (saved_var, cl->reconnect_var);
			strcpy (saved_val, cl->reconnect_value);

			newcl = cl;
			goto gotnewcl;
		}
	}

	// MH: if there is a client with the same name and no contact for 5+ seconds, reuse their slot (they could have dynamic IP)
	pass = Info_ValueForKey (userinfo, "name");
	for (i=0,cl=svs.clients ; i < maxclients->intvalue; i++,cl++)
	{
		if (cl->state <= cs_zombie)
			continue;

		if (!strcmp(pass, cl->name) && (curtime - cl->netchan.last_received) > 5000)
		{
			Com_DPrintf ("    client already found with same name\n");
			Com_Printf ("Dropping %s, ghost reconnect (name match)\n", LOG_SERVER|LOG_DROP, cl->name);
#if KINGPIN
			// MH: indicate to the game that the player is reconnecting
			cl->edict->client->ping = -1;
#endif
			SV_DropClient (cl, false);

			Com_DPrintf ("%s:reconnect\n", pass);

			SV_CleanClient (cl);

			newcl = cl;
			goto gotnewcl;
		}
	}

	// find a client slot
	newcl = NULL;
	for (i=0,cl=svs.clients ; i<(maxclients->intvalue - reserved); i++,cl++)
	{
		if (cl->state == cs_free)
		{
			newcl = cl;
			break;
		}
	}

	if (!newcl)
	{
		if (sv_reserved_slots->intvalue && !reserved)
		{
			Com_DPrintf ("    reserved slots full\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer and reserved slots are full.\n");
		}
		else
		{
			Com_DPrintf ("    server full\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is full.\n");
		
			//aiee... just look away now and pretend you never saw this code
			if (sv_redirect_address->string[0])
			{
				netchan_t	chan;
				byte		data[1024];
				byte		string[256];
				int			len, datalen;

				data[0] = svc_print;
				data[1] = PRINT_CHAT;

				datalen = 2;

				len = Com_sprintf ((char *)string, sizeof(string), "This server is full, redirecting you to %s\n", sv_redirect_address->string);
				memcpy (data + datalen, string, len + 1);
				datalen += len + 1;

				data[datalen++] = svc_stufftext;

				len = Com_sprintf ((char *)string, sizeof(string), "connect %s\n", sv_redirect_address->string);
				memcpy (data + datalen, string, len + 1);
				datalen += len + 1;

				Netchan_Setup (NS_SERVER, &chan, &net_from, protocol, qport, msglen);
				Netchan_OutOfBandPrint (NS_SERVER, adr, "client_connect");
				Netchan_Transmit (&chan, datalen, data);
			}
		}

		return;
	}

gotnewcl:	

	// MH: moved here to allow "ghost" checks first
	//r1: limit connections from a single IP
	previousclients = 0;
	for (i=0,cl=svs.clients ; i<maxclients->intvalue ; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;
		if (NET_CompareBaseAdr (adr, &cl->netchan.remote_address))
		{
			//r1: zombies are less dangerous
			if (cl->state == cs_zombie)
				previousclients++;
			else
				previousclients += 2;
		}
	}

	if (previousclients >= sv_iplimit->intvalue * 2)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nToo many connections from your host.\n");
		Com_DPrintf ("    too many connections\n");
		return;
	}

	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	
	sv_client = newcl;

#if KINGPIN
	if (newcl->messageListData || newcl->downloadFileName || newcl->download || newcl->lastlines)
#else
	if (newcl->messageListData || newcl->versionString || newcl->downloadFileName || newcl->download || newcl->lastlines)
#endif
	{
		Com_Printf ("WARNING: Client %d never got cleaned up, possible memory leak.\n", LOG_SERVER|LOG_WARNING, (int)(newcl - svs.clients));
		SV_CleanClient (newcl);
	}

	memset (newcl, 0, sizeof(*newcl));

	//r1: reconnect check
	if (saved_var[0] && saved_val[0])
	{
		strcpy (newcl->reconnect_value, saved_val);
		strcpy (newcl->reconnect_var, saved_var);
	}

	edictnum = (int)(newcl-svs.clients)+1;
	ent = EDICT_NUM(edictnum);
	newcl->edict = ent;
	newcl->challenge = challenge; // save challenge for checksumming

#if KINGPIN
	// MH: check if it is a patched client
	pass = Info_ValueForKey(userinfo, "ver");
	if (!strncmp(pass, "121p", 4))
		newcl->patched = atoi(pass + 4);
#endif

	reconnected = (!sv_force_reconnect->string[0] || saved_var[0] || NET_IsLANAddress(adr));

	if (reconnected)
	{
		// MH: log userinfo (maybe useful for debugging or stats)
		if (sv_log_connectinfo->intvalue)
			Com_Printf ("ConnectInfo: %s\n", LOG_SERVER, userinfo);

		// MH: handle name clashes
		SV_NameClash(newcl, userinfo);

		// MH: lookup country in GeoIP database
		Info_RemoveKey(userinfo, "country");
		if (mmdb.filename)
		{
			static const char *dbpath[] = { "country", "names", "en", NULL };
			int mmdb_error;
			MMDB_entry_data_s entry_data;
			struct sockaddr sa = { AF_INET, {0, 0, adr->ip[0], adr->ip[1], adr->ip[2], adr->ip[3]} };
			MMDB_lookup_result_s result = MMDB_lookup_sockaddr(&mmdb, &sa, &mmdb_error);
			if (result.found_entry && !MMDB_aget_value(&result.entry, &entry_data, dbpath) && entry_data.type == MMDB_DATA_TYPE_UTF8_STRING)
			{
				char country[32];
				if (entry_data.data_size > sizeof(country) - 1)
					entry_data.data_size = sizeof(country) - 1;
				memcpy(country, entry_data.utf8_string, entry_data.data_size);
				country[entry_data.data_size] = 0;
				Info_SetValueForKey(userinfo, "country", country);
			}
		}

		//if (!ge->edicts[0].client)
		//	Com_Error (ERR_DROP, "Missed a call to InitGame");
		// get the game a chance to reject this connection or modify the userinfo
		allowed = ge->ClientConnect (ent, userinfo);

		if (userinfo[MAX_INFO_STRING-1])
		{
			//probably already crashed by now but worth a try
			Com_Error (ERR_FATAL, "Game DLL overflowed userinfo string after ClientConnect");
		}

		if (!allowed)
		{
			if (Info_ValueForKey (userinfo, "rejmsg")[0]) 
				Netchan_OutOfBandPrint (NS_SERVER, adr, "print\n%s\nConnection refused.\n",  
					Info_ValueForKey (userinfo, "rejmsg"));
			else
				Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n" );
			Com_DPrintf ("    game rejected the connection.\n");
			return;
		}

		//did the game ruin our userinfo string?
		if (!userinfo[0])
		{
			Com_Printf ("GAME ERROR: Userinfo string corrupted after ClientConnect\n", LOG_SERVER|LOG_ERROR|LOG_GAMEDEBUG); 
			if (sv_gamedebug->intvalue > 1)
				Sys_DebugBreak ();
		}

		// MH: notify other players of the new connection
		if (sv_show_connections->intvalue)
		{
			char	buf[64];
			char	*name = Info_ValueForKey(userinfo, "name");
			char	*country = mmdb.filename ? Info_ValueForKey(userinfo, "country") : "";

			if (country[0])
				Com_sprintf(buf, sizeof(buf), "%s connected from %s\n", name, country);
			else
				Com_sprintf(buf, sizeof(buf), "%s connected\n", name);

			for (i=0,cl=svs.clients ; i<maxclients->intvalue ; i++,cl++)
			{
				if (PRINT_HIGH < cl->messagelevel || cl->state != cs_spawned)
					continue;

				MSG_BeginWriting (svc_print);
				MSG_WriteByte (PRINT_CHAT);
#if KINGPIN
				if (name[0] == ':' || name[0] == '>')
					MSG_WriteByte(' '); // add a space so that it isn't confused for normal chat or yellow text
#endif
				MSG_WriteString (buf);
				SV_AddMessage (cl, true);
			}
		}
	}

	newcl->protocol_version = version;

	//moved netchan to here so userinfo changes can see remote address
	Netchan_Setup (NS_SERVER, &newcl->netchan, adr, protocol, qport, msglen);

	// parse some info from the info strings
	strcpy (newcl->userinfo, userinfo);

	//r1: this fills in the fields of userinfo.
	SV_UpdateUserinfo (newcl, reconnected);

	//r1: netchan init was here

	// send the connect packet to the client
#if !KINGPIN
	// r1: note we could ideally send this twice but it prints unsightly message on original client.
	if (sv_downloadserver->string[0])
		Netchan_OutOfBandPrint (NS_SERVER, adr, "client_connect dlserver=%s", sv_downloadserver->string);
	else
#endif
		Netchan_OutOfBandPrint (NS_SERVER, adr, "client_connect");

	if (sv_connectmessage->modified)
	{
		ExpandNewLines (sv_connectmessage->string);
		if (strlen(sv_connectmessage->string) >= MAX_USABLEMSG-16)
		{
			Com_Printf ("WARNING: sv_connectmessage string is too long!\n", LOG_SERVER|LOG_WARNING);
			Cvar_Set ("sv_connectmessage", "");
		}
		sv_connectmessage->modified = false;
	}

	if (reconnected)
	{
		//only show message on reconnect
		if (sv_connectmessage->string[0])
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\n%s\n", sv_connectmessage->string);
	}

	newcl->protocol = protocol;
	newcl->state = cs_connected;

	newcl->messageListData = Z_TagMalloc (sizeof(messagelist_t) * MAX_MESSAGES_PER_LIST, TAGMALLOC_CL_MESSAGES);
	memset (newcl->messageListData, 0, sizeof(messagelist_t) * MAX_MESSAGES_PER_LIST);

	newcl->msgListEnd = newcl->msgListStart = newcl->messageListData;
	newcl->msgListStart->data = NULL;

	newcl->lastlines = Z_TagMalloc (sizeof(entity_state_t) * MAX_EDICTS, TAGMALLOC_CL_BASELINES);
	//memset (newcl->lastlines, 0, sizeof(entity_state_t) * MAX_EDICTS);

	// MH: allocate client_entities now if it was delayed to reduce memory usage
	if (!svs.client_entities)
		svs.client_entities = Z_TagMalloc (sizeof(entity_state_t)*svs.num_client_entities, TAGMALLOC_CL_ENTS);

	//r1: per client baselines are now used
	//SV_CreateBaseline (newcl);
	
	//r1: concept of datagram buffer no longer exists
	//SZ_Init (&newcl->datagram, newcl->datagram_buf, sizeof(newcl->datagram_buf) );
	//newcl->datagram.allowoverflow = true;

	//r1ch: give them 5 secs to respond to the client_connect. since client_connect isn't
	//sent via netchan (its connectionless therefore unreliable), a dropped client_connect
	//will force a client to wait for the entire timeout->value before being allowed to try
	//and reconnect.
	//newcl->lastmessage = svs.realtime;	// don't timeout

	newcl->lastmessage = svs.realtime - ((timeout->intvalue - 5) * 1000);

	// MH: set default packetdup
	newcl->netchan.packetdup = sv_packetdup->intvalue;
	if (newcl->netchan.packetdup > sv_max_packetdup->intvalue)
		newcl->netchan.packetdup = sv_max_packetdup->intvalue;

	// MH: disable idle mode
	if (g_idle->intvalue)
		Cvar_ForceSet("g_idle", "0");

#ifdef _WIN32
	// MH: handle auto priority
	if (!strcmp(win_priority->string, "auto") && !win_priority->intvalue)
		win_priority->changed(win_priority, "auto", win_priority->string);
#endif
}

static int Rcon_Validate (void)
{
	if (rcon_password->string[0] && !strcmp (Cmd_Argv(1), rcon_password->string) )
		return 1;
	else if (lrcon_password->string[0] && !strcmp (Cmd_Argv(1), lrcon_password->string))
		return 2;

	return 0;
}

uint32 CalcMask (int32 bits)
{
	return 0xFFFFFFFF << (32 - bits);
}

void Blackhole (netadr_t *from, qboolean isAutomatic, int mask, int method, const char *fmt, ...)
{
	blackhole_t *temp;
	va_list		argptr;

	if (isAutomatic && !sv_blackholes->intvalue)
		return;

	temp = &blackholes;

	while (temp->next)
		temp = temp->next;

	temp->next = Z_TagMalloc(sizeof(blackhole_t), TAGMALLOC_BLACKHOLE);
	temp = temp->next;

	temp->next = NULL;

	temp->ip = *(uint32 *)from->ip;
	temp->mask = NET_htonl (CalcMask(mask));
	temp->method = method;

	temp->ratelimit.period = 1000;

	va_start (argptr,fmt);
	vsnprintf (temp->reason, sizeof(temp->reason)-1, fmt, argptr);
	va_end (argptr);

	//terminate
	temp->reason[sizeof(temp->reason)-1] = 0;

	// MH: reason can be empty
	if (sv.state)
		Com_Printf (temp->reason[0] ? "Added %s/%d to blackholes for %s.\n" : "Added %s/%d to blackholes.\n", // MH: reason is now optional
			LOG_SERVER|LOG_EXPLOIT, NET_inet_ntoa (temp->ip), mask, temp->reason);
}

qboolean UnBlackhole (int index)
{
	blackhole_t *temp, *last;
	int i;

	i = index;

	last = temp = &blackholes;
	while (temp->next)
	{
		last = temp;
		temp = temp->next;
		i--;
		if (i == 0)
			break;
		else if (i < 0)
			return false;
	}

	// if list ended before we could get to index
	if (i > 0)
		return false;

	// just copy the next over, don't care if it's null
	last->next = temp->next;

	Z_Free (temp);

	return true;
}

static const char *FindPlayer (netadr_t *from)
{
	int			i;
	client_t	*cl;

	// MH: fix for killserver via rcon
	if (!svs.initialized)
		return "";

	for (i=0, cl=svs.clients ; i<maxclients->intvalue ; i++,cl++)
	{
		//FIXME: do we want packets from zombies still?
		if (cl->state == cs_free)
			continue;

		if (!NET_CompareAdr (from, &cl->netchan.remote_address)) // MH: include port in check, for multiple clients with same IP
			continue;

		return cl->name;
	}

	return "";
}

/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
static void SVC_RemoteCommand (void)
{
	//static int last_rcon_time = 0;
	int		i, j;
	char	remaining[2048];

	if (sv_rcon_ratelimit->intvalue)
	{
		//only bad passwords get rate limited
		if (RateLimited (&svs.ratelimit_badrcon, sv_rcon_ratelimit->intvalue))
		{
			Com_DPrintf ("SVC_RemoteCommand: Dropped rcon request from %s\n", NET_AdrToString (&net_from));
			return;
		}
	}

	i = Rcon_Validate ();

	if (!i)
		RateSample (&svs.ratelimit_badrcon);

	// MH: if they're a connected client then send the reply via their reliable channel
	for (j = 0; j < maxclients->intvalue; j++)
		if (svs.clients[j].state == cs_spawned && NET_CompareAdr(&net_from, &svs.clients[j].netchan.remote_address))
			break;
	Com_BeginRedirect(j < maxclients->intvalue ? RD_CLIENTNUM | j : RD_PACKET, sv_outputbuf, j < maxclients->intvalue ? 32 : sv_rcon_buffsize->intvalue, SV_FlushRedirect);

	if (!i)
	{
		Com_Printf ("Bad rcon_password.\n", LOG_GENERAL);
		Com_EndRedirect (true);
		// MH: don't log password or command
		Com_Printf ("Bad rcon from %s[%s]\n", LOG_SERVER|LOG_ERROR, NET_AdrToString (&net_from), FindPlayer(&net_from));
	}
	else
	{
		qboolean	endRedir;

		remaining[0] = 0;
		endRedir = true;

		//hack to allow clients to send rcon set commands properly
		if (!Q_stricmp (Cmd_Argv(2), "set"))
		{
			char		setvar[2048];
			qboolean	serverinfo = false;
			int			params;
			int			j;

			setvar[0] = 0;
			params = Cmd_Argc();

			if (!Q_stricmp (Cmd_Argv(params-1), "s"))
			{
				serverinfo = true;
				params--;
			}

			for (j=4 ; j<params ; j++)
			{
				strcat (setvar, Cmd_Argv(j) );
				if (j+1 != params)
					strcat (setvar, " ");
			}

			Com_sprintf (remaining, sizeof(remaining), "set %s \"%s\"%s", Cmd_Argv(3), setvar, serverinfo ? " s" : "");
		}
		//FIXME: This is a nice idea, but currently redirected output blocks until full buffer occurs, making it useless.
		/*else if (!Q_stricmp (Cmd_Argv(2), "monitor"))
		{
			Sys_ConsoleOutput ("Console monitor request from ");
			Sys_ConsoleOutput (NET_AdrToString (&net_from));
			Sys_ConsoleOutput ("\n");
			if (!Q_stricmp (Cmd_Argv(3), "on"))
			{
				Com_Printf ("Console monitor enabled.\n", LOG_SERVER);
				return;
			}
			else
			{
				Com_Printf ("Console monitor disabled.\n", LOG_SERVER);
				Com_EndRedirect (true);
				return;
			}
		}*/
		else
		{
			// MH: preserve quotes in command
			int p = 0;
			for (i = 2; i < Cmd_Argc(); i++)
			{
				if (strchr(Cmd_Argv(i), ' '))
					p += Com_sprintf(remaining + p, sizeof(remaining) - p, "\"%s\" ", Cmd_Argv(i));
				else
					p += Com_sprintf(remaining + p, sizeof(remaining) - p, "%s ", Cmd_Argv(i));
				if (p == sizeof(remaining) - 1)
					break;
			}
		}

		if (strlen(remaining) == sizeof(remaining)-1)
		{
			Com_Printf ("Rcon string length exceeded, discarded.\n", LOG_SERVER|LOG_WARNING);
			remaining[0] = 0;
		}

		//r1: limited rcon support
		if (i == 2)
		{
			linkednamelist_t	*lrcon;
			size_t				clen;

			i = 0;

			//len = strlen(remaining);

			if (strchr (remaining, ';'))
			{
				Com_Printf ("You may not use multiple commands in a single rcon command.\n", LOG_SERVER);
			}
			else if (strchr (remaining, '$'))
			{
				Com_Printf ("Variable expansion is not allowed via lrcon.\n", LOG_SERVER);
			}
			else
			{			
				for (lrcon = lrconcmds.next; lrcon; lrcon = lrcon->next)
				{
					clen = strlen (lrcon->name);

					if (!strncmp (remaining, lrcon->name, clen))
					{
						i = 1;
						break;
					}
				}

				if (!i)
					Com_Printf ("Rcon command '%s' is not allowed.\n", LOG_SERVER, remaining);
			}
		}

		if (i)
		{
			Cmd_ExecuteString (remaining);
			Com_EndRedirect (true);
			Com_Printf ("Rcon from %s[%s]: %s\n", LOG_SERVER, NET_AdrToString (&net_from), FindPlayer(&net_from), remaining);
		}
		else
		{
			Com_EndRedirect (true);
			Com_Printf ("Bad limited rcon from %s[%s]: %s\n", LOG_SERVER, NET_AdrToString (&net_from), FindPlayer(&net_from), remaining);
		}
	}

	//check for remote kill
	if (!svs.initialized)
		Com_Error (ERR_HARD, "server killed via rcon");
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
// MH: separated from SV_ConnectionlessPacket so that it can be used on Gamespy packets too
static qboolean CheckBlackholes (qboolean reply)
{
	netblock_t	*whitehole = &blackhole_exceptions;
	blackhole_t *blackhole = &blackholes;
	uint32		network_ip;

	network_ip = *(uint32 *)net_from.ip;

	//r1: whiteholes (thanks WORM!)
	while (whitehole->next)
	{
		whitehole = whitehole->next;
		if ((network_ip & whitehole->mask) == (whitehole->ip & whitehole->mask))
		{
			return false;
		}
	}

	//r1: ignore packets if IP is blackholed for abuse
	while (blackhole->next)
	{
		blackhole = blackhole->next;
		if ((network_ip & blackhole->mask) == (blackhole->ip & blackhole->mask))
		{
			//do rate limiting in case there is some long-ish reason (MH: don't send any in reply to GS packets)
			if (blackhole->method == BLACKHOLE_MESSAGE && reply)
			{
				RateSample (&blackhole->ratelimit);
				if (!RateLimited (&blackhole->ratelimit, 2))
					Netchan_OutOfBandPrint (NS_SERVER, &net_from, "print\n%s\n", blackhole->reason);
			}
			return true;
		}
	}

	return false;
}

static void SV_ConnectionlessPacket (void)
{
	char		*s;
	char		*c;

	if (CheckBlackholes(true))
		return;

	//r1: should never happen, don't even bother trying to parse it
	if (net_message.cursize > 544)
	{
		Com_DPrintf ("dropped %d byte connectionless packet from %s\n", net_message.cursize, NET_AdrToString (&net_from));
		return;
	}

	//r1: make sure we never talk to ourselves
	if (NET_IsLocalAddress (&net_from) && !NET_IsLocalHost(&net_from) && ShortSwap(net_from.port) == server_port)
	{
		Com_DPrintf ("dropped %d byte connectionless packet from self! (spoofing attack?)\n", net_message.cursize);
		return;
	}

	MSG_BeginReading (&net_message);
	MSG_ReadLong (&net_message);		// skip the -1 marker

	s = MSG_ReadStringLine (&net_message);

	if (sv_format_string_hack->intvalue == 2)
	{
		char	*p = s;
		while ((p = strchr (p, '%')))
			p[0] = ' ';
	}

	Cmd_TokenizeString (s, false);

	c = Cmd_Argv(0);
	Com_DPrintf ("Packet %s : %s\n", NET_AdrToString(&net_from), c);

	if (!strcmp(c, "ping"))
		SVC_Ping ();
	else if (!strcmp(c,"status"))
		SVC_Status ();
	else if (!strcmp(c,"info"))
		SVC_Info ();
	else if (!strcmp(c,"getchallenge"))
		SVC_GetChallenge ();
	else if (!strcmp(c,"connect"))
		SVC_DirectConnect ();
	else if (!strcmp(c, "rcon"))
		SVC_RemoteCommand ();
	else if (!strcmp(c, "ack"))
		SVC_Ack ();
	else
		Com_DPrintf ("bad connectionless packet from %s:\n%s\n"
		, NET_AdrToString (&net_from), s);
}


//============================================================================

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
static void SV_CalcPings(void)
{
	int			i;
	client_t	*cl;

	for (i = 0; i < maxclients->intvalue; i++)
	{
		cl = &svs.clients[i];
		if (cl->state != cs_spawned)
			continue;

		// MH: just copy the already calculated ping
		cl->edict->client->ping = cl->ping;
	}
}

/*
===================
SV_GiveMsec

Every few frames, gives all clients an allotment of milliseconds
for their command moves.  If they exceed it, assume cheating.
===================
*/
static void SV_GiveMsec (void)
{
	int			i;
	client_t	*cl;

	int			diff;

	if (sv.framenum & 15)
		return;

	for (i=0 ; i<maxclients->intvalue ; i++)
	{
		cl = &svs.clients[i];

		if (cl->state <= cs_zombie )
			continue;

		//hasn't sent a movement packet, could be irc bot for example
		if (!cl->moved)
			continue;

		if (cl->state == cs_spawned)
		{
			// MH: doing things a bit differently here
			if (cl->initialRealTime)
			{
				diff = cl->totalMsecUsed - (cl->netchan.last_received - cl->initialRealTime);
				if (diff > 0)
				{
					if (cl->commandMsecOverflowCount < 0)
						cl->commandMsecOverflowCount = 0; // no accumulated time allowance
					else
						cl->commandMsecOverflowCount *= 0.97f; // allow a bit of leeway
				}
				cl->commandMsecOverflowCount += diff;

				if (abs(diff) > 100)
					Com_DPrintf ("%s has diff %d (%d %d) %.3f\n", cl->name, diff, cl->totalMsecUsed, (cl->netchan.last_received - cl->initialRealTime), cl->commandMsecOverflowCount);

				if (sv_enforcetime->intvalue && sv_enforcetime_kick->value && cl->commandMsecOverflowCount > sv_enforcetime_kick->value * 1000 && !(cl->edict->svflags & SVF_NOCLIENT))
				{
					SV_KickClient (cl, "irregular timing (speed cheat?)", "You were kicked from the game for having irregular timing. This could be caused by excessive lag or other network conditions.\n");
					continue;
				}
			}

			cl->totalMsecUsed = 0;
			cl->initialRealTime = cl->netchan.last_received;
		}

		cl->commandMsec = sv_msecs->intvalue;		// 1600 + some slop
	}
}


#if KINGPIN
// MH: Gamespy "secure" coding, taken from GSMSALG 0.3.3 by Luigi Auriemma
byte gsvalfunc(int reg)
{
	if (reg < 26) return(reg + 'A');
	if (reg < 52) return(reg + 'G');
	if (reg < 62) return(reg - 4);
	if (reg == 62) return('+');
	if (reg == 63) return('/');
	return(0);
}

byte *gsseckey(byte *dst, byte *src, byte *key)
{
	int    i, size, keysz;
	byte   enctmp[256], tmp[66], x, y, z, a, b, *p;

	size = strlen(src);
	if ((size < 1) || (size > 65)) {
		dst[0] = 0;
		return(dst);
	}
	keysz = strlen(key);

	for (i = 0; i < 256; i++) {
		enctmp[i] = i;
	}

	a = 0;
	for (i = 0; i < 256; i++) {
		a += enctmp[i] + key[i % keysz];
		x = enctmp[a];
		enctmp[a] = enctmp[i];
		enctmp[i] = x;
	}

	a = 0;
	b = 0;
	for (i = 0; src[i]; i++) {
		a += src[i] + 1;
		x = enctmp[a];
		b += x;
		y = enctmp[b];
		enctmp[b] = x;
		enctmp[a] = y;
		tmp[i] = src[i] ^ enctmp[(x + y) & 0xff];
	}
	for (size = i; size % 3; size++) {
		tmp[size] = 0;
	}

	p = dst;
	for (i = 0; i < size; i += 3) {
		x = tmp[i];
		y = tmp[i + 1];
		z = tmp[i + 2];
		*p++ = gsvalfunc(x >> 2);
		*p++ = gsvalfunc(((x & 3) << 4) | (y >> 4));
		*p++ = gsvalfunc(((y & 15) << 2) | (z >> 6));
		*p++ = gsvalfunc(z & 63);
	}
	*p = 0;

	return(dst);
}

// MH: process a Gamespy packet
static void SV_GamespyPacket(void)
{
	// GamespyLite and most master servers can handle large packets, so not bothering to split them here
	static int	qid;
	char		*s, buf[8192];
	int			i, count, len, master;
	qboolean	status;

	if (sv_hidestatus->intvalue)
		return;

	if (CheckBlackholes(false))
		return;

	if (net_message.cursize > 40)
	{
		Com_DPrintf("Dropped %d byte GS packet from %s\n", net_message.cursize, NET_AdrToString(&net_from));
		return;
	}

	MSG_BeginReading(&net_message);
	s = MSG_ReadStringLine(&net_message);

	Com_DPrintf("GS Packet %s : %s\n", NET_AdrToString(&net_from), s);

	if (s[0] != '\\')
		return;

	master = FromMaster();

	RateSample(&svs.ratelimit_status);
	if (master < 0 && RateLimited(&svs.ratelimit_status, sv_ratelimit_status->intvalue))
	{
		Com_DPrintf("Dropped GS request from %s\n", NET_AdrToString(&net_from));
		return;
	}

	len = 0;

	status = !!strstr(s, "\\status\\");

	if (status || strstr(s, "\\basic\\"))
		len += Com_sprintf(buf + len, sizeof(buf) - len, "\\gamename\\kingpin\\gamever\\kpded " VERSION "\\location\\%s", Cvar_Get("location", "0", 0)->string);

	if (status || strstr(s, "\\info\\"))
	{
		for (count = i = 0; i < maxclients->intvalue; i++)
			if (svs.clients[i].state >= cs_spawning)
				count++;

		len += Com_sprintf(buf + len, sizeof(buf) - len, "\\hostname\\%s\\hostport\\%d\\mapname\\%s\\gametype\\%s\\numplayers\\%d\\maxplayers\\%d\\gamemode\\openplaying",
			HostnameString(), server_port, sv.name, Cvar_VariableString("gamename"), count, maxclients->intvalue - sv_reserved_slots->intvalue);
	}

	if (status || strstr(s, "\\rules\\"))
	{
		char *rules = StatusString();

		// exclude conflicting and duplicate info
		Info_RemoveKey(rules, "gamename");
		Info_RemoveKey(rules, "hostname");
		Info_RemoveKey(rules, "mapname");

		len += Com_sprintf(buf + len, sizeof(buf) - len, "%s", rules);
	}

	if (!sv_hideplayers->intvalue && (status || strstr(s, "\\players\\")))
	{
		for (count = i = 0; i < maxclients->intvalue; i++)
			if (svs.clients[i].state >= cs_spawning)
			{
				len += Com_sprintf(buf + len, sizeof(buf) - len, "\\player_%d\\%s\\frags_%d\\%d\\ping_%d\\%d\\deaths_%d\\%d",
					count, svs.clients[i].name, count, svs.clients[i].edict->client->ps.stats[STAT_FRAGS], count, svs.clients[i].ping, count, svs.clients[i].edict->client->ps.stats[STAT_DEPOSITED]);
				if (g_features->intvalue & GMF_CLIENTTEAM)
					len += Com_sprintf(buf + len, sizeof(buf) - len, "\\team_%d\\%d", count, svs.clients[i].edict->client->team);
				count++;
				if (len > sizeof(buf) - 140)
					break;
			}
	}

	s = strstr(s, "\\secure\\");
	if (s)
	{
		char auth[89];
		char *e = strchr(s + 8, '\\');
		if (e)
			*e = 0;
		gsseckey(auth, s + 8, "QFWxY2"); // "QFWxY2" = Kingpin's key
		len += Com_sprintf(buf + len, sizeof(buf) - len, "\\validate\\%s", auth);
	}

	if (len)
	{
		len += Com_sprintf(buf + len, sizeof(buf) - len, "\\final\\\\queryid\\%u.1", ++qid);
		NET_SendPacket(NS_SERVER_GS, len, buf, &net_from);

		if (master >= 0 && !master_gs[master])
		{
			// mark the master server as Gamespy
			master_gs[master] = true;
			if (public_server->intvalue == 1)
				Com_DPrintf("Master %s is Gamespy compatible, not sending Quake2 heartbeats\n", NET_AdrToString(&master_adr[master]));
		}
	}
}
#endif

/*
=================
SV_ReadPackets
=================
*/
static void SV_ReadPackets (void)
{
	int			i, j;
	client_t	*cl;
	uint16		qport;

	//Com_Printf ("ReadPackets\n");
	for (;;)
	{
		j = NET_GetPacket (NS_SERVER, &net_from, &net_message);

		if (!j)
			break;

		if (j == -2)
			continue;

		if (j == -1)
		{
			// check for packets from connected clients
			for (i=0, cl=svs.clients ; i<maxclients->intvalue ; i++,cl++)
			{
				if (cl->state == cs_free)
					continue;

				// r1: workaround for broken linux kernel
				if (net_from.port == 0)
				{
					if (!NET_CompareBaseAdr (&net_from, &cl->netchan.remote_address))
						continue;
				}
				else
				{
					if (!NET_CompareAdr (&net_from, &cl->netchan.remote_address))
						continue;
				}

				// r1: only drop if we haven't seen a packet for a bit to prevent spoofed ICMPs from kicking people
				if (cl->state == cs_spawned)
				{
					if (cl->lastmessage > svs.realtime - 1500)
						continue;
				}
				else if (cl->state > cs_zombie)
				{
					// r1: longer delay if they are still loading
					if (cl->lastmessage > svs.realtime - 10000)
						continue;
				}

				if (cl->state > cs_zombie)
					SV_KickClient (cl, "connection reset by peer", NULL);

				SV_CleanClient (cl);
				cl->state = cs_free;	// don't bother with zombie state
				break;
			}
			continue;
		}

		// check for connectionless packet (0xffffffff) first
		if (*(int *)net_message_buffer == -1)
		{
			SV_ConnectionlessPacket ();
			continue;
		}

		// read the qport out of the message so we can fix up
		// stupid address translating routers
		//MSG_BeginReading (&net_message);
		//MSG_ReadLong (&net_message);		// sequence number
		//MSG_ReadLong (&net_message);		// sequence number
		
		//MSG_ReadShort (&net_message);

#if KINGPIN
		// MH: make sure it's big enough
		if (net_message.cursize < 10)
			continue;
#endif

		qport = *(uint16 *)(net_message_buffer + 8);

		// check for packets from connected clients
		for (i=0, cl=svs.clients ; i<maxclients->intvalue ; i++,cl++)
		{
			//FIXME: do we want packets from zombies still?
			if (cl->state == cs_free)
				continue;

			if (!NET_CompareBaseAdr (&net_from, &cl->netchan.remote_address))
				continue;

			//qport shit
#if KINGPIN
			if (true)
#else
			if (cl->protocol == PROTOCOL_ORIGINAL)
#endif
			{
				//compare short from original q2
				if (cl->netchan.qport != qport)
					continue;
			}
			else
			{
				//compare byte in newer r1q2, older r1q2 clients get qport zeroed on svc_directconnect
				if (cl->netchan.qport)
				{
					if (cl->netchan.qport != (qport & 0xFF))
						continue;
				}
				else if (cl->netchan.remote_address.port != net_from.port)
					continue;
			}

			if (cl->netchan.remote_address.port != net_from.port)
			{
				qboolean	fixedup;
				client_t	*test;

				fixedup = false;

				//verify user isn't misusing qport
				for (test = svs.clients; test < svs.clients + maxclients->intvalue; test++)
				{
					if (test->state <= cs_zombie)
						continue;

					if (test != cl && NET_CompareAdr (&test->netchan.remote_address, &net_from))
					{
						fixedup = true;
						Com_Printf ("SV_ReadPackets: bad qport for client %d (%s[%s]), matched %s (%s)\n", LOG_SERVER|LOG_NOTICE, i, test->name, NET_AdrToString (&test->netchan.remote_address), NET_AdrToString (&cl->netchan.remote_address), cl->name);
						SV_ClientPrintf (test, PRINT_HIGH, "Warning, server found another client (%s) from your IP, please check your qport is set properly.\n", NET_AdrToString (&cl->netchan.remote_address));
						cl = test;
						break;
					}
				}

				if (!fixedup)
				{
					if (cl->state == cs_zombie)
					{
						Com_Printf ("SV_ReadPackets: Got a translated port for client %d (zombie) [%d->%d], freeing client (broken NAT router reconnect?)\n", LOG_SERVER|LOG_NOTICE, i, (uint16)(ShortSwap(cl->netchan.remote_address.port)), (uint16)(ShortSwap(net_from.port)));
						SV_CleanClient (cl);
						cl->state = cs_free;
						continue;
					}
					else
					{
						Com_Printf ("SV_ReadPackets: fixing up a translated port for client %d (%s) [%d->%d]\n", LOG_SERVER|LOG_NOTICE, i, cl->name, (uint16)(ShortSwap(cl->netchan.remote_address.port)), (uint16)(ShortSwap(net_from.port)));
						cl->netchan.remote_address.port = net_from.port;
					}
				}
			}

			//they overflowed, but maybe not disconnected yet. ignore
			//any further commands.
			if (cl->notes & NOTE_OVERFLOWED)
				continue;

			if (Netchan_Process(&cl->netchan, &net_message))
			{	// this is a valid, sequenced packet, so process it
				if (cl->state != cs_zombie)
				{
					cl->lastmessage = svs.realtime;	// don't timeout

					if (!(sv.demofile && sv.state == ss_demo))
						SV_ExecuteClientMessage (cl);
					cl->packetCount++;

					// MH: check if unreliable layout message was received or lost
					if (cl->layout_unreliable && cl->netchan.incoming_acknowledged >= cl->layout_unreliable)
					{
						if (cl->netchan.incoming_acknowledged > cl->layout_unreliable)
							cl->layout[0] = 0; // lost so clear stored layout
						cl->layout_unreliable = 0;
					}

					//r1: send a reply immediately if the client is connecting
					if ((cl->state == cs_connected || cl->state == cs_spawning) && cl->msgListStart->next)
					{
						SV_WriteReliableMessages (cl, cl->netchan.message.buffsize);
						Netchan_Transmit (&cl->netchan, 0, NULL);
					}
				}
			}
			break;
		}
		
		
		/* r1: a little pointless?
		if (i != game.maxclients)
			continue;
		*/
	}

#if KINGPIN
	// MH: process Gamespy packets
	for (;;)
	{
		j = NET_GetPacket(NS_SERVER_GS, &net_from, &net_message);

		if (!j)
			break;

		if (j == 1)
			SV_GamespyPacket();
	}
#endif
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->value
seconds, drop the conneciton.  Server frames are used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
static void SV_CheckTimeouts (void)
{
	int		i;
	client_t	*cl;
	int			droppoint;
	int			zombiepoint;
#if KINGPIN
	int			dlrate = 0;
#endif

	droppoint = svs.realtime - 1000*timeout->intvalue;
	zombiepoint = svs.realtime - 1000*zombietime->intvalue;

	for (i=0,cl=svs.clients ; i<maxclients->intvalue ; i++,cl++)
	{
		// message times may be wrong across a changelevel
		if (cl->lastmessage > svs.realtime)
			cl->lastmessage = svs.realtime;

		if (cl->state == cs_zombie && cl->lastmessage < zombiepoint)
		{
			Com_DPrintf ("Going from cs_zombie to cs_free for client %d\n", i);
			SV_CleanClient (cl);
			cl->state = cs_free;	// can now be reused
			continue;
		}

		if (cl->state >= cs_connected)
		{
			// MH: don't start counting FPS until they are in the game
			if (!cl->lastfpscheck || cl->state != cs_spawned)
			{
				cl->packetCount = 0;
				cl->lastfpscheck = curtime;
			}
			else if ((curtime - cl->lastfpscheck) >= 5000)
			{
				// MH: using the wall clock (instead of server time) for more accurate FPS measurement
				cl->fps = (int)((float)cl->packetCount * 1000 / (int)(curtime - cl->lastfpscheck));

				//r1ch: fps spam protection
				// MH: modified to check/correct cl_maxfps before kicking if that fails to fix the problem
				if (sv_fpsflood->intvalue && cl->fps > sv_fpsflood->intvalue)
				{
					if (!cl->cl_maxfps)
					{
						MSG_BeginWriting (svc_stufftext);
						MSG_WriteString ("cmd \177c \177fps $cl_maxfps\n");
						SV_AddMessage (cl, true);
					}
					else
						SV_KickClient (cl, va("too many packets/sec (%d)", cl->fps), va("You were kicked from the game for sending more than %d packets per second.\n", sv_fpsflood->intvalue));
				}
				else if (sv_minpps->intvalue && cl->lastmessage > (svs.realtime - 500) && cl->fps < sv_minpps->intvalue)
				{
					if (!cl->cl_maxfps)
					{
						MSG_BeginWriting (svc_stufftext);
						MSG_WriteString ("cmd \177c \177fps $cl_maxfps\n");
						SV_AddMessage (cl, true);
					}
					else
						SV_KickClient (cl, va ("not enough packets/sec (%d)", cl->fps), va("You were kicked from the game for sending fewer than %d packets per second.\n", sv_minpps->intvalue));
				}
				else
					cl->cl_maxfps = 0;

				cl->packetCount = 0;
				cl->lastfpscheck = curtime;
			}

			if (cl->notes & NOTE_DROPME)
			{
				SV_DropClient (cl, true);
				return;
			}

			if (cl->state == cs_spawned)
			{
				cl->idletime++;

				//icky test
				if (sv_predict_on_lag->intvalue && (cl->lastmessage < svs.realtime - 200) && cl->moved)
				{
					int old = cl->lastcmd.msec;
					Com_DPrintf ("Lag predicting %s (lastMsg %d)\n", cl->name, svs.realtime - cl->lastmessage);
					cl->lastcmd.msec = 100;
					//SV_ClientThink (cl, cl->lastcmd);
					ge->ClientThink (cl->edict, &cl->lastcmd);
					cl->lastcmd.msec = old;
				}

				if (sv_idlekick->intvalue && cl->idletime >= sv_idlekick->intvalue * 10)
				{
					SV_KickClient (cl, "idling", "You have been disconnected due to inactivity.\n");
					continue;
				}

				// MH: send name clash message
				if ((cl->notes & NOTE_NAMECLASH) && cl->moved)
				{
					SV_ClientPrintf (cl, PRINT_HIGH, "Another player on the server is already using your name\nYou are now: %s\n", cl->name);
					cl->notes &= ~NOTE_NAMECLASH;
				}

#if KINGPIN
				// MH: send pending bad model message
				if (cl->badmodel[0] && cl->moved)
				{
					if (strchr(cl->badmodel, '/'))
						SV_ClientPrintf(cl, PRINT_HIGH, "This server does not have the \"%s\" skins, replaced with \"%s\"\n", cl->badmodel, cl->skin);
					else
						SV_ClientPrintf(cl, PRINT_HIGH, "This server does not have the %s model\n", cl->badmodel);
					cl->badmodel[0] = 0;
				}
#endif
			}

			if (cl->lastmessage < droppoint)
			{
				//r1: only message if they spawned (less spam plz)
				if (cl->state == cs_spawned && cl->name[0])
					SV_BroadcastPrintf (PRINT_HIGH, "%s timed out\n", cl->name);
				Com_Printf ("Dropping %s, timed out.\n", LOG_SERVER|LOG_DROP, cl->name);
				SV_DropClient (cl, true);
				//r1: do use zombie state. it's possible client is broken on send
				//but can receive, send a disconnect so they know they disconnected.
				//SV_CleanClient (cl);
				//cl->state = cs_free;	// don't bother with zombie state
			}

#if KINGPIN
			// MH: update download speed if bandwidth is limited
			if (cl->download && sv_bandwidth->intvalue > 0)
			{
				int rate;
				if (!dlrate)
					dlrate = GetDownloadRate();
				if (cl->patched)
					rate = dlrate;
				else
					rate = dlrate > 200 ? 200 : dlrate;
				if (cl->downloadrate != rate && (cl->patched || (cl->cl_maxfps && cl->cl_maxfps != -1)))
				{
					cl->downloadrate = rate;
					MSG_BeginWriting (svc_stufftext);
					MSG_WriteString (va(cl->patched ? "\177p dlr %d\n" : "set cl_maxfps %d\n", rate));
					SV_AddMessage (cl, true);
				}
				// give download tokens
				cl->downloadtokens += rate / 10.f;
				if (cl->downloadtokens > rate)
					cl->downloadtokens = (float)rate;
			}
#endif
		}
	}
}

/*
================
SV_PrepWorldFrame

This has to be done before the world logic, because
player processing happens outside RunWorldFrame
================
*/
static void SV_PrepWorldFrame (void)
{
	edict_t	*ent;
	int		i;

	for (i=0 ; i<ge->num_edicts ; i++)
	{
		ent = EDICT_NUM(i);

		// events only last for a single message
		ent->s.event = 0;
	}
}

/*
=================
SV_RunGameFrame
=================
*/
static void SV_RunGameFrame (void)
{
#ifndef DEDICATED_ONLY
	if (host_speeds->intvalue)
		time_before_game = Sys_Milliseconds ();
#endif

	// we always need to bump framenum, even if we
	// don't run the world, otherwise the delta
	// compression can get confused when a client
	// has the "current" frame
	sv.framenum++;
#if KINGPIN
	sv.time = sv.framenum * 100;
#else
	sv.time = sv.framenum * (1000 / sv_fps->intvalue);
#endif

#if !KINGPIN
	sv_tracecount = 0;
#endif

	// don't run if paused
	if (!sv_paused->intvalue || maxclients->intvalue > 1)
	{
		ge->RunFrame ();

		if (MSG_GetLength())
		{
			Com_Printf ("GAME ERROR: The Game DLL wrote data to the message buffer, but did not call gi.unicast or gi.multicast before finishing this frame.\nData: %s\n", LOG_ERROR|LOG_GAMEDEBUG|LOG_SERVER, MakePrintable (MSG_GetData(), MSG_GetLength()));
			if (sv_gamedebug->intvalue > 1)
				Sys_DebugBreak ();
			MSG_Clear ();
		}

		// never get more than one tic behind
		if (sv.time < svs.realtime)
		{
			if (sv_showclamp->intvalue && sv.framenum > 1) // MH: 1st frame is always clamped
				Com_Printf ("sv highclamp: %u < %d = %d\n", LOG_SERVER, sv.time, svs.realtime, svs.realtime - sv.time);
			svs.realtime = sv.time;
		}
	}

#ifndef DEDICATED_ONLY
	if (host_speeds->intvalue)
		time_after_game = Sys_Milliseconds ();
#endif
}

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/

static void Master_Heartbeat (void)
{
	const char	*string;
	int			i;

	if (!dedicated->intvalue)
		return;		// only dedicated servers send heartbeats

	if (!public_server->intvalue)
		return;		// a private dedicated game

	if (curtime - svs.last_heartbeat < HEARTBEAT_SECONDS*1000) // MH: using wall clock (not server time)
		return;		// not time to send yet

	svs.last_heartbeat = curtime; // MH: using wall clock (not server time)

	// MH: refresh global master server IP if there are no connected clients
	if (sv_global_master->intvalue && SV_CountPlayers() == 0)
		NET_StringToAdr (GLOBAL_MASTER, &master_adr[0]);

	// MH: don't generate status string until needed below
	string = NULL;

	// send to group master
	for (i=0 ; i<MAX_MASTERS ; i++)
	{
		if (master_adr[i].port)
		{
			Com_Printf ("Sending heartbeat to %s\n", LOG_SERVER|LOG_NOTICE, NET_AdrToString (&master_adr[i]));
#if KINGPIN
			// MH: send a Gamespy heartbeat
			if (public_server->intvalue < 3)
			{
				char buf[64];
				Com_sprintf(buf, sizeof(buf), "\\heartbeat\\%d\\gamename\\kingpin", server_port - 10);
				NET_SendPacket (NS_SERVER_GS, strlen(buf), buf, &master_adr[i]);
				if (master_gs[i] || public_server->intvalue == 2)
					continue;
			}
#endif
			// send the same string that we would give for a status OOB command
			if (!string)
				string = SV_StatusString();
			Netchan_OutOfBandPrint (NS_SERVER, &master_adr[i], "heartbeat\n%s", string);
		}
	}
}

static void SV_RunPmoves (int msec)
{
	client_t	*cl;

	if (!msec)
		return;
	
	for (cl = svs.clients; cl < svs.clients + maxclients->intvalue; cl++)
	{
		if (cl->state < cs_spawned || !cl->moved)
			continue;

		if (msec == -1 && cl->current_move.elapsed < cl->current_move.msec)
		{
			cl->current_move.elapsed = cl->current_move.msec;
			FastVectorCopy (cl->current_move.origin_end, cl->edict->s.origin);
			SV_LinkEdict (cl->edict);
			continue;
		}

		if (cl->current_move.elapsed < cl->current_move.msec)
		{
			float	scale;
			vec3_t	move;

			cl->current_move.elapsed += msec;

			if (cl->current_move.elapsed > cl->current_move.msec)
				cl->current_move.elapsed = cl->current_move.msec;

			scale = (float)cl->current_move.elapsed / (float)cl->current_move.msec;

			VectorSubtract (cl->current_move.origin_end, cl->current_move.origin_start, move);

			VectorScale (move, scale, move);

			VectorAdd (cl->current_move.origin_start, move, cl->edict->s.origin);
			SV_LinkEdict (cl->edict);
		}
	}
}

// MH: activate auto-idle mode if the server is empty
static void SV_CheckAutoIdle (void)
{
	int		i;
	client_t	*cl;

	if (!sv_autoidle->intvalue || g_idle->intvalue)
		return;

	for (i=0,cl=svs.clients ; i<maxclients->intvalue ; i++,cl++)
	{
		if (cl->state)
			return;
	}

	Cvar_ForceSet("g_idle", "1");
}

// MH: minimize memory usage when the server is empty
static void SV_MinimizeMemory (void)
{
	int			i;
	client_t	*cl;

	if (!sv_minimize_memory->intvalue || !svs.client_entities)
		return;

	for (i=0,cl=svs.clients ; i<maxclients->intvalue ; i++,cl++)
	{
		if (cl->state)
			return;
	}

	Z_Free (svs.client_entities);
	svs.client_entities = NULL;

#if KINGPIN
	// clear download cache
	ClearCachedDownloads();
#endif
}

/*
==================
SV_Frame

==================
*/
void SV_Frame (int msec)
{
	static int was_idle; // MH: ignore msec when coming out of idle mode

#ifndef DEDICATED_ONLY
	time_before_game = time_after_game = 0;
#endif

	// if server is not active, do nothing
	if (!svs.initialized)
	{
		//DERP DERP only do this when not running a god damn client!!
		if (dedicated->intvalue)
		{
#ifdef _WIN32
			// MH: release mutex before sleeping
			Sys_ReleaseConsoleMutex();
#endif
			Sys_Sleep (100); // MH: raised from 1ms
#ifdef _WIN32
			// MH: retake mutex
			Sys_AcquireConsoleMutex();
#endif
		}
		return;
	}

	// MH: ignore msec when coming out of idle mode
	if (was_idle)
		msec = 0;
	was_idle = g_idle->intvalue;

	// MH: time stops in idle mode
	if (g_idle->intvalue)
		svs.realtime = sv.time - 1;
	else
		svs.realtime += msec;

	// keep the random time dependent
	//rand ();

#if !KINGPIN
	if (sv_interpolated_pmove->intvalue)
		SV_RunPmoves (msec);
#endif

	// get packets from clients
	SV_ReadPackets ();

	// move autonomous things around if enough time has passed
	if (!sv_timedemo->intvalue && svs.realtime < sv.time)
	{
		// never let the time get too far off
#if KINGPIN
		if (sv.time - svs.realtime > 100)
#else
		if (sv.time - svs.realtime > (1000 / sv_fps->intvalue))
#endif
		{
			if (sv_showclamp->intvalue && sv.framenum > 0) // MH: 1st frame is always clamped
				Com_Printf ("sv lowclamp: %u - %d = %d\n", LOG_SERVER, sv.time, svs.realtime, sv.time - svs.realtime);
#if KINGPIN
			svs.realtime = sv.time - 100;
#else
			svs.realtime = sv.time - (1000 / sv_fps->intvalue);
#endif
		}

#if !KINGPIN
		//r1: send extra packets now for player position updates
		SV_SendPlayerUpdates (sv.time - svs.realtime);
#endif

		//r1: execute commands now
		if (dedicated->intvalue)
		{
			Cbuf_Execute();
			if (!svs.initialized)
				return;
		}

		// MH: do heartbeats now in idle mode because they won't be done below
		if (g_idle->intvalue)
			Master_Heartbeat ();

#ifdef _WIN32
		// MH: release mutex before sleeping
		Sys_ReleaseConsoleMutex();
#endif

		// MH: sleep until next heartbeat in idle mode
		if (g_idle->intvalue)
			NET_Sleep (public_server->intvalue ? svs.last_heartbeat + HEARTBEAT_SECONDS*1000 - curtime : INT_MAX);
		else
			NET_Sleep (sv.time - svs.realtime);

#ifdef _WIN32
		// MH: retake mutex
		Sys_AcquireConsoleMutex();
#endif

		return;
	}

	// MH: measure frame delay
	if (sv.framenum > 0)
	{
		int d = svs.realtime - sv.time;
#if KINGPIN
		int i = (sv.framenum / 10) % 30;
		if (!(sv.framenum % 10))
#else
		int i = (sv.framenum / sv_fps->intvalue) % 30;
		if (!(sv.framenum % sv_fps->intvalue))
#endif
			sv.frame_delays[i] = 0;
		if (sv.frame_delays[i] < d)
			sv.frame_delays[i] = d;
	}

#if !KINGPIN
	//force all moves to be current so that clients don't interpolate behind time
	if (sv_interpolated_pmove->intvalue)
		SV_RunPmoves (-1);
#endif

	//Com_Printf ("** game tick (%d drift)**\n", LOG_GENERAL, sv.time - svs.realtime);

	//r1: execute commands now
	Cbuf_Execute();

	//may have executed some kind of quit
	if (!svs.initialized)
		return;

	// update ping based on the last known frame from all clients
	SV_CalcPings();

	// give the clients some timeslices
	SV_GiveMsec ();

	// let everything in the world think and move
	SV_RunGameFrame ();

	// check timeouts
	SV_CheckTimeouts ();

	// send messages back to the clients that had packets read this frame
	SV_SendClientMessages ();

	// save the entire world state if recording a serverdemo
	SV_RecordDemoMessage ();

	// send a heartbeat to the master if needed
	Master_Heartbeat ();

	// clear teleport flags, etc for next frame
	SV_PrepWorldFrame ();

	// MH: check auto-idle mode
	SV_CheckAutoIdle ();

	// MH: minimize memory usage
	SV_MinimizeMemory ();

	//have to check this here for possible listen servers loading DLLs and stuff
	//during server execution
#ifndef DEDICATED_ONLY
	//check the server is running proper Q2 physics model
	//if (!Sys_CheckFPUStatus ())
	//	Com_Error (ERR_FATAL, "FPU control word is not set as expected, Quake II physics model will break.");
#endif
}

//============================================================================

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
static void Master_Shutdown (void)
{
	int			i;

	//r1: server died before cvars had a chance to init!
	if (!dedicated)
		return;

	if (!dedicated->intvalue)
		return;		// only dedicated servers send heartbeats

	if (public_server && !public_server->intvalue)
		return;		// a private dedicated game

	// send to group master
	for (i=0 ; i<MAX_MASTERS ; i++)
		if (master_adr[i].port)
		{
			Com_Printf ("Sending shutdown to %s\n", LOG_SERVER|LOG_NOTICE, NET_AdrToString (&master_adr[i]));
			Netchan_OutOfBandPrint (NS_SERVER, &master_adr[i], "shutdown");
		}
}

//============================================================================




void UserinfoBanDrop (const char *key, const banmatch_t *ban, const char *result)
{
	if (ban->message[0])
		SV_ClientPrintf (sv_client, PRINT_HIGH, "%s\n", ban->message);

	if (ban->blockmethod == CVARBAN_BLACKHOLE)
		Blackhole (&sv_client->netchan.remote_address, true, sv_blackhole_mask->intvalue, BLACKHOLE_SILENT, "userinfo: %s = %s", key, result);

	Com_Printf ("Dropped client %s, banned userinfo: %s = %s\n", LOG_SERVER|LOG_DROP, sv_client->name, key, result);

	SV_DropClient (sv_client, (ban->blockmethod != CVARBAN_BLACKHOLE));
}

qboolean SV_UserInfoBanned (client_t *cl)
{
	char				key[MAX_INFO_KEY];
	const banmatch_t	*match;

	key[0] = 0;
	while ((match = SV_CheckUserinfoBans (cl->userinfo, key)))
	{
		switch (match->blockmethod)
		{
			case CVARBAN_MESSAGE:
				SV_ClientPrintf (cl, PRINT_HIGH, "%s\n", match->message);
				break;
			case CVARBAN_STUFF:
				MSG_BeginWriting (svc_stufftext);
				MSG_WriteString (va ("%s\n",match->message));
				SV_AddMessage (cl, true);
				break;
			case CVARBAN_LOGONLY:
				Com_Printf ("LOG: %s[%s] userinfo check: %s = %s\n", LOG_SERVER, cl->name, NET_AdrToString (&cl->netchan.remote_address), key, Info_ValueForKey (cl->userinfo, key));
				break;
			case CVARBAN_EXEC:
				Cbuf_AddText (match->message);
				Cbuf_AddText ("\n");
				break;
			default:
				UserinfoBanDrop (key, match, Info_ValueForKey (cl->userinfo, key));
				return true;
		}
	}

	return false;
}

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
void SV_UserinfoChanged (client_t *cl)
{
	char				*val;

	if (cl->state < cs_connected)
		Com_Printf ("WARNING: SV_UserinfoChanged for unconnected client %s[%s]!!\n", LOG_SERVER|LOG_WARNING, cl->name, NET_AdrToString (&cl->netchan.remote_address));

	if (!cl->userinfo[0])
		Com_Printf ("WARNING: SV_UserinfoChanged for %s[%s] with empty userinfo!\n", LOG_SERVER|LOG_WARNING, cl->name, NET_AdrToString (&cl->netchan.remote_address));

	//r1ch: ban anyone trying to use the end-of-message-in-string exploit
	if (strchr (cl->userinfo, '\xFF'))
	{
		char		*ptr;
		const char	*p;
		ptr = strchr (cl->userinfo, '\xFF');
		ptr -= 8;
		if (ptr < cl->userinfo)
			ptr = cl->userinfo;
		p = MakePrintable (ptr, 0);
		Com_Printf ("EXPLOIT: Client %s[%s] supplied userinfo string containing 0xFF: %s\n", LOG_EXPLOIT|LOG_SERVER, cl->name, NET_AdrToString (&cl->netchan.remote_address), p);
		Blackhole (&cl->netchan.remote_address, true, sv_blackhole_mask->intvalue, BLACKHOLE_SILENT, "0xFF in userinfo (%.32s)", p);
		SV_KickClient (cl, "illegal userinfo", NULL);
		return;
	}

	if (!SV_UserinfoValidate (cl->userinfo))
	{
		//Com_Printf ("WARNING: SV_UserinfoValidate failed for %s[%s] (%s)\n", LOG_SERVER|LOG_WARNING, cl->name, NET_AdrToString (&cl->netchan.remote_address), MakePrintable (cl->userinfo));
		Com_Printf ("EXPLOIT: Client %s[%s] supplied an illegal userinfo string: %s\n", LOG_EXPLOIT|LOG_SERVER, cl->name, NET_AdrToString (&cl->netchan.remote_address), MakePrintable (cl->userinfo, 0));
		Blackhole (&cl->netchan.remote_address, true, sv_blackhole_mask->intvalue, BLACKHOLE_SILENT, "illegal userinfo string");
		SV_KickClient (cl, "illegal userinfo", NULL);
		return;
	}

	if (sv_strict_userinfo_check->intvalue)
	{
		if (!Info_CheckBytes (cl->userinfo))
		{
			Com_Printf ("WARNING: Illegal userinfo bytes from %s.\n", LOG_SERVER|LOG_WARNING, cl->name);
			SV_KickClient (cl, "illegal userinfo string", "Userinfo contains illegal bytes. Please disable any color-names and similar features.\n");
			return;
		}
	}

	if (sv_format_string_hack->intvalue)
	{
		char	*p = cl->userinfo;
		while ((p = strchr (p, '%')))
			p[0] = ' ';
	}

	//r1ch: allow filtering of stupid "fun" names etc.
	if (sv_filter_userinfo->intvalue)
		StripHighBits(cl->userinfo, (int)sv_filter_userinfo->intvalue == 2);

	val = Info_ValueForKey (cl->userinfo, "name");

	//they tried to set name to something bad
#if KINGPIN
	if (!val[0] || StringIsWhitespace(val))
#else
	if (!val[0] || NameColorFilterCheck (val) || StringIsWhitespace(val))
#endif
	{
		Com_Printf ("WARNING: Invalid name change to '%s' from %s[%s].\n", LOG_SERVER|LOG_WARNING, MakePrintable(val, 0), cl->name, NET_AdrToString(&cl->netchan.remote_address)); 
		SV_ClientPrintf (cl, PRINT_HIGH, "Invalid name '%s'\n", val);
		if (cl->name[0])
		{
			//force them back to old name
			MSG_BeginWriting (svc_stufftext);
			MSG_WriteString (va("set name \"%s\"\n", cl->name));
			SV_AddMessage (cl, true);
			Info_SetValueForKey (cl->userinfo, "name", cl->name);
		}
		else
		{
			//no old name, bye
			Com_DPrintf ("dropping %s for malformed userinfo (%s)\n", cl->name, cl->userinfo);
			SV_KickClient (cl, "bad userinfo", NULL);
			return;
		}
	}

	if (Info_KeyExists (cl->userinfo, "ip"))
	{
		Com_Printf ("Dropping %s for attempted IP spoof (%s)\n", LOG_SERVER|LOG_DROP, cl->name, cl->userinfo);
		SV_KickClient (cl, "bad userinfo", NULL);
		return;
	}

	if (SV_UserInfoBanned (cl))
		return;

	//this actually fills in the client fields
	SV_UpdateUserinfo (cl, true);
}

static void SV_UpdateWindowTitle (cvar_t *cvar, char *old, char *newvalue)
{
	if (dedicated->intvalue)
	{
		char buff[512];
#if KINGPIN
		Com_sprintf (buff, sizeof(buff), "%s - kpded " VERSION " (port %d)", newvalue, server_port);
#else
		Com_sprintf (buff, sizeof(buff), "%s - R1Q2/kpded " VERSION " (port %d)", newvalue, server_port);
#endif

		//for win32 this will set window titlebar text
		//for linux this currently does nothing
		Sys_SetWindowText (buff);
	}
}

static void _password_changed (cvar_t *var, char *oldvalue, char *newvalue)
{
	if (!newvalue[0] || !strcmp (newvalue, "none"))
		Cvar_FullSet ("needpass", "0", CVAR_SERVERINFO);
	else
		Cvar_FullSet ("needpass", "1", CVAR_SERVERINFO);
}

static void _rcon_buffsize_changed (cvar_t *var, char *oldvalue, char *newvalue)
{
	if (var->intvalue > SV_OUTPUTBUF_LENGTH)
	{
		Cvar_SetValue (var->name, SV_OUTPUTBUF_LENGTH);
	}
	else if (var->intvalue < 256)
	{
		Cvar_Set (var->name, "256");
	}
}

// MH: check that idle mode can be enabled
static void _g_idle_changed (cvar_t *var, char *oldvalue, char *newvalue)
{
	if (var->intvalue)
	{
		// idle mode is only allowed on an empty server
		if (SV_CountPlayers())
		{
			if (sv_gamedebug->intvalue)
				Com_Printf ("GAME WARNING: Attempt to activate g_idle while server is not empty.\n", LOG_SERVER|LOG_WARNING|LOG_GAMEDEBUG);
			if (sv_gamedebug->intvalue > 1)
				Sys_DebugBreak ();

			Cvar_ForceSet (var->name, "0");
		}
		else
		{
			// make any zombies timeout immediately
			int		i;
			client_t	*cl;
			for (i=0, cl=svs.clients; i<maxclients->intvalue; i++, cl++)
				cl->lastmessage = 0;

			Com_Printf("Entering idle mode\n", LOG_SERVER|LOG_NOTICE);
		}
	}
}

// MH: open GeoIP database
static void _geoipdb_changed (cvar_t *var, char *oldvalue, char *newvalue)
{
	if (mmdb.filename)
		MMDB_close(&mmdb);
	if (newvalue[0] && ((g_features->intvalue & GMF_WANT_COUNTRY) || sv_show_connections->intvalue))
	{
		int status = MMDB_open(newvalue, MMDB_MODE_MMAP, &mmdb);
		if (!status)
		{
			// get the entire database into memory to avoid lookup delays
			volatile int c;
			int a;
			for (c = a = 0; a < mmdb.file_size; a += 4096)
				c += mmdb.file_content[a];
			(void)c;
			Com_Printf("GeoIP database loaded\n", LOG_SERVER|LOG_NOTICE);
		}
		else
			Com_Printf("GeoIP database failed to load (%d)\n", LOG_SERVER|LOG_WARNING, status);
	}
}

/*
===============
SV_Init

Only called at quake2.exe startup, not for each game
===============
*/

void SV_Init (void)
{
	SV_InitOperatorCommands	();

	rcon_password = Cvar_Get ("rcon_password", "", 0);
	lrcon_password = Cvar_Get ("lrcon_password", "", 0);
	lrcon_password->help = "Limited rcon password for use with lrcon commands. Default empty.\n";

	Cvar_Get ("skill", "1", 0);

	//r1: default 1
	Cvar_Get ("deathmatch", "1", CVAR_LATCH);
	Cvar_Get ("coop", "0", CVAR_LATCH);
#if KINGPIN
	Cvar_Get ("dmflags", "784", CVAR_SERVERINFO | CVAR_ARCHIVE);
#else
	Cvar_Get ("dmflags", va("%i", DF_INSTANT_ITEMS), CVAR_SERVERINFO);
#endif
	Cvar_Get ("fraglimit", "0", CVAR_SERVERINFO);
	Cvar_Get ("timelimit", "0", CVAR_SERVERINFO);
#if KINGPIN
//	Cvar_Get ("cashlimit", "0", CVAR_SERVERINFO);
#endif
	Cvar_Get ("cheats", "0", CVAR_SERVERINFO|CVAR_LATCH);
#if KINGPIN
	Cvar_Get ("protocol", va("%i", PROTOCOL_ORIGINAL), CVAR_SERVERINFO | CVAR_NOSET);
#else
	Cvar_Get ("protocol", va("%i", PROTOCOL_R1Q2), CVAR_NOSET);
#endif

	//r1: default 8
	maxclients = Cvar_Get ("maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH);

#if KINGPIN
	hostname = Cvar_Get ("hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE);
#else
	hostname = Cvar_Get ("hostname", "Unnamed R1Q2 Server", CVAR_SERVERINFO | CVAR_ARCHIVE);
#endif

	//r1: update window title on the fly
	hostname->changed = SV_UpdateWindowTitle;
	hostname->changed (hostname, hostname->string, hostname->string);

	//r1: dropped to 90 from 125
	timeout = Cvar_Get ("timeout", "90", 0);
	timeout->help = "Number of seconds after which a client is dropped if they have not sent any data. Default 90.\n";

	//r1: bumped to 3 from 2
	zombietime = Cvar_Get ("zombietime", "3", 0);
	zombietime->help = "Number of seconds during which packets from a recently disconnected client are ignored. Default 3.\n";

	sv_showclamp = Cvar_Get ("showclamp", "0", 0);

	sv_paused = Cvar_Get ("paused", "0", 0);
	sv_timedemo = Cvar_Get ("timedemo", "0", 0);

	//r1: default 1
	sv_enforcetime = Cvar_Get ("sv_enforcetime", "1", 0);
	sv_enforcetime->help = "Enforce time movements to prevent speed hacking. Default 1.\n0: Disabled\n1: Enabled, prevent excess movement (and kick depending on sv_enforcetime_kick)\n";
	// MH: a separate kick option is needed for Kingpin because most mods reset sv_enforcetime to 1
	sv_enforcetime_kick = Cvar_Get ("sv_enforcetime_kick", "0", 0);
	sv_enforcetime_kick->help = "Kick clients that exceed this many seconds of time offset. Default 0.\n0: Disabled\n1+: Enabled (increase value to reduce false positives from lag)\n";

#ifndef NO_SERVER
#if KINGPIN
	allow_download = Cvar_Get ("allow_download", "1", CVAR_ARCHIVE);
#else
	allow_download = Cvar_Get ("allow_download", "0", CVAR_ARCHIVE);
#endif
	allow_download_players  = Cvar_Get ("allow_download_players", "1", CVAR_ARCHIVE);
	allow_download_models = Cvar_Get ("allow_download_models", "1", CVAR_ARCHIVE);
	allow_download_sounds = Cvar_Get ("allow_download_sounds", "1", CVAR_ARCHIVE);
	allow_download_maps	  = Cvar_Get ("allow_download_maps", "1", CVAR_ARCHIVE);
	allow_download_pics	  = Cvar_Get ("allow_download_pics", "1", CVAR_ARCHIVE);
#if KINGPIN
	allow_download_paks = Cvar_Get ("allow_download_paks", "1", 0);
	allow_download_paks->help = "Allow downloading of PAK files in the game directory (not main). Default 1.\n";
#else
	allow_download_textures = Cvar_Get ("allow_download_textures", "1", 0);
	allow_download_others = Cvar_Get ("allow_download_others", "0", 0);
#endif
#endif

	sv_noreload = Cvar_Get ("sv_noreload", "0", 0);

#if !KINGPIN
	sv_airaccelerate = Cvar_Get("sv_airaccelerate", "0", CVAR_LATCH);
#endif

	public_server = Cvar_Get ("public", "1", 0);
#if KINGPIN
	public_server->help = "Send information about this server to master servers, which will cause the server to be shown in server browsers. See also 'setmaster' command. Default 1.\n0: Disabled\n1: Auto-enable both heartbeats\n2: Enable Gamespy heartbeats\n3: Enable Quake2 heartbeats\n";
#else
	public_server->help = "If set, sends information about this server to master servers which will cause the server to be shown in server browsers. See also 'setmaster' command. Default 1.\n";
#endif

	//r1: not needed
	//sv_reconnect_limit = Cvar_Get ("sv_reconnect_limit", "3", CVAR_ARCHIVE);

	SZ_Init (&net_message, net_message_buffer, sizeof(net_message_buffer));

	//r1: lock server (prevent new connections)
	sv_locked = Cvar_Get ("sv_locked", "0", 0);
	sv_locked->help = "Prevent new players from connecting. Default 0.\n";

	//r1: restart server on this map if the _GAME_ dll dies
	//    (only via a ERR_GAME drop of course)
	sv_restartmap = Cvar_Get ("sv_restartmap", "", 0);
	sv_restartmap->help = "If set, restart the server by changing to this map if the server crashes from a non-fatal error. Default empty.\n";

	//r1: server-side password protection (why id put this into the game dll
	//    i will never figure out)
#if KINGPIN
	// MH: using the traditional "password" setting (no point having 2 password settings)
	sv_password = Cvar_Get ("password", "", 0);
#else
	sv_password = Cvar_Get ("sv_password", "", 0);
	sv_password->changed = _password_changed;
#endif
	sv_password->help = "Password required to connect to the server. Default empty.\n";

#if !KINGPIN
	//r1: text filtering: 1 = strip low bits, 2 = low+hi
	sv_filter_q3names = Cvar_Get ("sv_filter_q3names", "0", 0);
	sv_filter_q3names->help = "Disallow names that use Quake III style coloring. Default 0.\n";
#endif

	sv_filter_userinfo = Cvar_Get ("sv_filter_userinfo", "0", 0);
	sv_filter_userinfo->help = "Filter client userinfo. Default 0.\n0: No filtering\n1: Strip low ASCII values.\n2: Strip low+high ASCII values.\n";

	sv_filter_stringcmds = Cvar_Get ("sv_filter_stringcmds", "0", 0);
	sv_filter_stringcmds->help = "Filter client commands. Default 0.\n0: No filtering\n1: Strip low ASCII values.\n2: Strip low+high ASCII values.\n";

	//r1: enable blocking of clients that attempt to attack server/clients
	sv_blackholes = Cvar_Get ("sv_blackholes", "1", 0);
	sv_blackholes->help = "Allow automatic blackholing (IP blocking) by various features. Default 1.\n";

	//r1: allow clients to use cl_nodelta (increases server bw usage)
#if KINGPIN
	sv_allownodelta = Cvar_Get ("sv_allownodelta", "0", 0);
	sv_allownodelta->help = "Allow clients to use cl_nodelta (disables delta state). Not using delta state results in much greater bandwidth usage. Default 0.\n";
#else
	sv_allownodelta = Cvar_Get ("sv_allownodelta", "1", 0);
	sv_allownodelta->help = "Allow clients to use cl_nodelta (disables delta state). Not using delta state results in much greater bandwidth usage. Default 1.\n";
#endif

#if !KINGPIN
	//r1: option to block q2ace due to hacked versions
	sv_deny_q2ace = Cvar_Get ("sv_deny_q2ace", "0", 0);
	if (sv_deny_q2ace->intvalue)
		Com_Printf ("WARNING: sv_deny_q2ace is deprecated and will be removed in a future build.\n", LOG_SERVER|LOG_WARNING);
#endif

	//r1: limit connections per ip address (stop zombie dos/flood)
	sv_iplimit = Cvar_Get ("sv_iplimit", "3", 0);
	sv_iplimit->help = "Maximum number of connections allowed from a single IP. Default 3.\n";

	//r1: message to send to connecting clients via CONNECTIONLESS print immediately
	//    after client connects. \n is expanded to new line.
	sv_connectmessage = Cvar_Get ("sv_connectmessage", "", 0);
	sv_connectmessage->help = "Message to show to clients after they connect (prior to auto downloading and entering the game). Default empty.\n";

	//r1: nocheat visibility check support (cpu intensive -- warning)
	sv_nc_visibilitycheck = Cvar_Get ("sv_nc_visibilitycheck", "0", 0);
	sv_nc_visibilitycheck->help = "Attempt to calculate player visibility server-side to thwart wall-hacks and other cheats. Default 0.\n";

	sv_nc_clientsonly = Cvar_Get ("sv_nc_clientsonly", "1", 0);
	sv_nc_clientsonly->help = "Only apply sv_nc_visibilitycheck checking to other players. Default 1.\n";

#if !KINGPIN
	//r1: http dl server
	sv_downloadserver = Cvar_Get ("sv_downloadserver", "", 0);
	sv_downloadserver->help = "URL to a location where clients can download game content over HTTP. Default empty.\n";
#endif

	//r1: max allowed file size for autodownloading (bytes)
#if KINGPIN
	sv_max_download_size = Cvar_Get ("sv_max_download_size", "0", 0);
	sv_max_download_size->help = "Maximum file size in bytes that a client may attempt to auto download. Default 0 (no limit).\n";
#else
	sv_max_download_size = Cvar_Get ("sv_max_download_size", "8388608", 0);
	sv_max_download_size->help = "Maximum file size in bytes that a client may attempt to auto download. Default 8388608 (8MB).\n";
#endif

	//r1: max backup packets to allow from lagged clients (id.default=20)
	sv_max_netdrop = Cvar_Get ("sv_max_netdrop", "20", 0);
	sv_max_netdrop->help = "Maximum number of movements to replay from lagged clients. Lower this to limit 'warping' effects. Default 20.\n";

	//r1: don't respond to status requests
	sv_hidestatus = Cvar_Get ("sv_hidestatus", "0", 0);
	sv_hidestatus->help = "Don't respond at all to requests for information from server browsers. Default 0.\n";

	//r1: hide player info from status requests
	sv_hideplayers = Cvar_Get ("sv_hideplayers", "0", 0);
	sv_hideplayers->help = "Hide information about who is playing from server browsers. Default 0.\n";

	//r1: kick high fps users flooding packets
	// MH: modified to check cl_maxfps before kicking
	sv_fpsflood = Cvar_Get ("sv_maxpps", "0", 0); // MH: renamed from "sv_fpsflood"
	sv_fpsflood->help = "Limit users to sending this many packets/sec (tied to the client's cl_maxfps cvar) and kick if they persistently exceed it. 0 means no limit. Default 0.\n";

	//r1: randomize starting framenum to thwart map timers
	sv_randomframe = Cvar_Get ("sv_randomframe", "0", 0);
	sv_randomframe->help = "Randomize the server framenumber on starting to prevent clients from being able to tell how long the map has been running. Default 0.\n";

	//r1: msecs to give clients
	sv_msecs = Cvar_Get ("sv_msecs", "1800", 0);
	sv_msecs->help = "Milliseconds of movement to allow per client per 16 frames. You shouldn't need to change this unless you know why. Default 1800.\n";

#if !KINGPIN
	//r1: nocheat parser shit
	sv_nc_kick = Cvar_Get ("sv_nc_kick", "0", 0);
	if (sv_nc_kick->intvalue)
		Com_Printf ("WARNING: sv_nc_kick is deprecated and will be removed in a future build.\n", LOG_SERVER|LOG_WARNING);

	sv_nc_announce = Cvar_Get ("sv_nc_announce", "0", 0);
	if (sv_nc_announce->intvalue)
		Com_Printf ("WARNING: sv_nc_announce is deprecated and will be removed in a future build.\n", LOG_SERVER|LOG_WARNING);

	sv_filter_nocheat_spam = Cvar_Get ("sv_filter_nocheat_spam", "0", 0);
	if (sv_filter_nocheat_spam->intvalue)
		Com_Printf ("WARNING: sv_filter_nocheat_spam is deprecated and will be removed in a future build.\n", LOG_SERVER|LOG_WARNING);
#endif

	//r1: crash on game errors with int3
	sv_gamedebug = Cvar_Get ("sv_gamedebug", "0", 0);
	sv_gamedebug->help = "Show warnings and optionally cause breakpoints if the Game DLL does something in an incorrect or dangerous way. Useful for mod developers. Default 0.\n1: Show warnings only.\n2: Show warnings and break on severe errors.\n3: Show warnings and break on severe and normal errors.\n4+: Show warnings and break on all errors.\n";

	//r1: reload game dll on next map change (resets to 0 after)
	sv_recycle = Cvar_Get ("sv_recycle", "0", 0);
	sv_recycle->help = "Reload the Game DLL on the next map load. Default 0.\n"; // MH: removed "For mod developers only"

	//r1: track server uptime in serverinfo?
	sv_uptime = Cvar_Get ("sv_uptime", "1", 0); // MH: changed default from 0 to 1
	sv_uptime->help = "Display the server uptime statistics in the info response shown to server browsers. Default 1.\n";

#if !KINGPIN
	//r1: allow strafe jumping at high fps values (Requires hacked client (!!!))
	sv_strafejump_hack = Cvar_Get ("sv_strafejump_hack", "1", CVAR_LATCH);
	sv_strafejump_hack->help = "Allow strafe jumping at any FPS value. Default 0.\n0: Standard Q2 strafe jumping allowed\n1: Allow compatible clients to strafe jump at any FPS\n2: Allow all clients to strafe jump at any FPS (causes prediction errors on non-compatible clients)\n";
#endif

	//r1: reserved slots (set 'rp' userinfo)
	sv_reserved_password = Cvar_Get ("sv_reserved_password", "", 0);
	sv_reserved_password->help = "Password required to access a reserved player slot. Clients should set their 'password' cvar to this. Default empty.\n";

	sv_reserved_slots = Cvar_Get ("sv_reserved_slots", "0", CVAR_LATCH);
	sv_reserved_slots->help = "Number of reserved player slots. Default 0.\n";

	//r1: allow use of 'map' command after game dll is loaded?
#if KINGPIN
	sv_allow_map = Cvar_Get ("sv_allow_map", "1", 0);
	sv_allow_map->help = "Allow use of the 'map' command to change levels. The 'gamemap' command should be used to change levels as 'map' will force the Game DLL to unload and reload, losing any internal state. Default 1.\n";
#else
	sv_allow_map = Cvar_Get ("sv_allow_map", "0", 0);
	sv_allow_map->help = "Allow use of the 'map' command to change levels. The 'gamemap' command should be used to change levels as 'map' will force the Game DLL to unload and reload, losing any internal state. Default 0.\n";
#endif

	//r1: allow unconnected clients to execute game commands (default enabled in id!)
	sv_allow_unconnected_cmds = Cvar_Get ("sv_allow_unconnected_cmds", "0", 0);
	sv_allow_unconnected_cmds->help = "Allow players who aren't in game to send commands (eg players who are map downloading). Since the Game DLL has no knowledge of the player until they are in-game, this can be dangerous if enabled. Default 0.\n";

	//r1: validate userinfo strings explicitly according to info key rules?
	sv_strict_userinfo_check = Cvar_Get ("sv_strict_userinfo_check", "1", 0);
	sv_strict_userinfo_check->help = "Force client userinfo to conform to the userinfo specifications. This will prevent colored names amongst other things. Default 1.\n";

#if !KINGPIN
	//r1: method to calculate pings. 0 = disable entirely, 1 = average, 2 = min
	sv_calcpings_method = Cvar_Get ("sv_calcpings_method", "1", 0);
	sv_calcpings_method->help = "Method used to calculate displayed pings. Default 1.\n0: Disable ping calculations completely\n1: Average of packets (default Q2 method)\n2: Best of packets (increased accuracy under normal network conditions)\n";
#endif

	//r1: disallow mod to set serverinfo via Cvar_Get?
	sv_no_game_serverinfo = Cvar_Get ("sv_no_game_serverinfo", "0", 0);
	sv_no_game_serverinfo->help = "Disallow the Game DLL to set serverinfo cvars (serverinfo shows up on server browsers). Some mods set too much serverinfo, causing info string exceeded errors. Must be set before the mod loads to be of any effect. Default 0.\n";

	//r1: send message on download failure/success
	sv_mapdownload_denied_message = Cvar_Get ("sv_mapdownload_denied_message", "", 0);
	sv_mapdownload_denied_message->help = "Message to sent to clients when they are refused a map download. Default empty.\n";

	sv_mapdownload_ok_message = Cvar_Get ("sv_mapdownload_ok_message", "", 0);
	sv_mapdownload_ok_message->help = "Message to send to clients when they commence a map download. Default empty.\n";

#if KINGPIN
	// MH:
	sv_pakdownload_message = Cvar_Get ("sv_pakdownload_message", "", 0);
	sv_pakdownload_message->help = "Message to send to clients when they commence a PAK download. Default empty.\n";
#endif

#if !KINGPIN
	//r1: failsafe against buggy traces
	sv_max_traces_per_frame = Cvar_Get ("sv_max_traces_per_frame", "10000", 0);
	sv_max_traces_per_frame->help = "Maximum amount of path traces permitted by the Game DLL per frame (100ms). Some mods get into infinite trace loops so this counter is a protection against that. Default 10000.\n";
#endif

	//r1: rate limiting for status requests to prevent udp spoof DoS
	sv_ratelimit_status = Cvar_Get ("sv_ratelimit_status", "15", 0);
	sv_ratelimit_status->help = "Maximum number of status requests to reply to per second.\n";

#if !KINGPIN
	//r1: allow new SVF_ ent flags? some mods mistakenly extend SVF_ for their own purposes.
	sv_new_entflags = Cvar_Get ("sv_new_entflags", "0", 0);
	sv_new_entflags->help = "Allow use of new R1Q2-specific server entity flags. Only enable this if instructed to do so by the mod you are running. Default 0.\n";
#endif

	//r1: validate playerskins passed to us by the mod? if not, we could end up sending
	//illegal names (eg "name\hack/$$") which the client would issue as "download hack/$$"
	//resulting in a command expansion attack.
	sv_validate_playerskins = Cvar_Get ("sv_validate_playerskins", "1", 0);
#if KINGPIN
	sv_validate_playerskins->help = "Perform strict checks on playerskins passed to the engine by the mod. When enabled, this ensures all skins are in the form model/skin. Default 1.\n";
#else
	sv_validate_playerskins->help = "Perform strict checks on playerskins passed to the engine by the mod. Some (almost all) mods do not validate skin names from the client and malformed skins can cause problems if broadcast to other clients. When enabled, this ensures all skins are in the form model/skin. Default 1.\n";
#endif

	//r1: seconds before idle kick
	sv_idlekick = Cvar_Get ("sv_idlekick", "0", 0);
	sv_idlekick->help = "Seconds before kicking idle players. 0 means no limit.\n";

	//r1: don't send ents that are marked !inuse?
	sv_entity_inuse_hack = Cvar_Get ("sv_entity_inuse_hack", "0", 0);
	sv_entity_inuse_hack->help = "Save network bandwidth by not sending entities that are marked as no longer in use. This only applies to buggy mods that do not mark entities as unused when they are no longer in use. Note that some mods may have problems with this if set to 1. Default 0.\n";

	//r1: force reconnect?
	sv_force_reconnect = Cvar_Get ("sv_force_reconnect", "", 0);
	sv_force_reconnect->help = "Force a quick reconnect to this specified IP/hostname. Useful as an anti-proxy check, specify your full IP:PORT. Default empty.\n";

	//r1: refuse downloads if this many players connected
	sv_download_refuselimit = Cvar_Get ("sv_download_refuselimit", "0", 0);
	sv_download_refuselimit->help = "Refuse auto downloading if the number of players on the server is equal or higher than this value. 0 means no limit. Default 0.\n";

	sv_download_drop_file = Cvar_Get ("sv_download_drop_file", "", 0);
	sv_download_drop_file->help = "If a player attempts to download this file, they are kicked from the server. Used to prevent players from trying to auto download huge mods. Default empty.\n";

	sv_download_drop_message = Cvar_Get ("sv_download_drop_message", "This mod requires client-side files, please visit the mod homepage to download them.", 0);
	sv_download_drop_message->help = "Message to show to users who are kicked for trying to download the file specified by sv_download_drop_file.\n";

	//r1: default blackhole mask
	sv_blackhole_mask = Cvar_Get ("sv_blackhole_mask", "32", 0);
	sv_blackhole_mask->help = "Network mask to use for automatically added blackholes. Common values: 24 means 1.2.3.*, 16 means 1.2.*.*, etc. Default 32.\n";

	//r1: what to do on unrequested cvars
	sv_badcvarcheck = Cvar_Get ("sv_badcvarcheck", "1", 0);
	sv_badcvarcheck->help = "Action to take on receiving an illegal cvarcheck response (illegal meaning 'completely invalid', not 'fails the check condition').\n0: Console warning only\n1: Kick player\n2: Kick and blackhole player.\n";

	//r1: control rcon buffer size
	sv_rcon_buffsize = Cvar_Get ("sv_rcon_buffsize", "1384", 0);
	sv_rcon_buffsize->changed = _rcon_buffsize_changed;
	sv_rcon_buffsize->help = "Amount of bytes the rcon buffer holds before flushing. You should not change this unless you know what you are doing. Default 1384.\n";

	//r1: show output of rcon in console?
	sv_rcon_showoutput = Cvar_Get ("sv_rcon_showoutput", "0", 0);
	sv_rcon_showoutput->help = "Display output of rcon commands in the server console. Default 0.\n";

	sv_rcon_ratelimit = Cvar_Get ("sv_rcon_ratelimit", "2", 0);
	sv_rcon_ratelimit->help = "Number of rcon attempts to accept per second. 0 to disable rate limiting. Default 2.\n";

	//r1: broadcast name changes?
	sv_show_name_changes = Cvar_Get ("sv_show_name_changes", "0", 0);
	sv_show_name_changes->help = "Broadcast player name changes to all players on the server. Default 0.\n";

	// MH: log connecting client userinfo
	sv_log_connectinfo = Cvar_Get ("sv_log_connectinfo", "0", 0);
	sv_log_connectinfo->help = "Log connecting client userinfo. Default 0.\n";

	// MH: handle name clashes?
	sv_unique_names = Cvar_Get ("sv_unique_names", "1", 0);
	sv_unique_names->help = "Modify the name (append a number) of a player that tries to use the same name as another player on the server. Default 1.\n";

	// MH: broadcast connecting players?
	sv_show_connections = Cvar_Get ("sv_show_connections", "0", 0);
	sv_show_connections->help = "Broadcast connecting players to all players on the server. Default 0.\n";

#if !KINGPIN
	//r1: delta optimz (small non-r1q2 client breakage on 2)
	sv_optimize_deltas = Cvar_Get ("sv_optimize_deltas", "1", 0);
	sv_optimize_deltas->help = "Optimize network bandwidth by not sending the view angles back to the client.\n0: Disabled\n1: Enabled for protocol 35 clients\n2: Enabled for all clients\n";
#endif

	//r1: enhanced setplayer code?
	sv_enhanced_setplayer = Cvar_Get ("sv_enhanced_setplayer", "0", 0);
	sv_enhanced_setplayer->help = "Allow use of partial names in the kick, dumpuser, etc commands. Default 0.\n";

	//r1: test lag stuff
	sv_predict_on_lag = Cvar_Get ("sv_predict_on_lag", "0", 0);
	sv_predict_on_lag->help = "Try to predict movement of lagged players to avoid frozen/warping players (experimental). Default 0.\n";

	sv_format_string_hack = Cvar_Get ("sv_format_string_hack", "0", 0);
	sv_format_string_hack->help = "Remove %% from user-supplied strings to mitigate format string vulnerabilities in the Game DLL. Default 0.\n";

#if !KINGPIN
	sv_func_plat_hack = Cvar_Get ("sv_func_entities_hack", "0", 0);
	sv_func_plat_hack->help = "Use an ugly hack to change global sounds from moving func_plat (lifts) and func_door (doors) into local sounds for those near the entity only. Will also move func_plat sounds to the top of the platform. Default 0.\n";
#endif

	sv_max_packetdup = Cvar_Get ("sv_max_packetdup", "1", 0); // MH: changed default from 0 to 1
	sv_max_packetdup->help = "Maximum number of duplicate packets a client can request for when they have packet loss. Each duplicate causes the client to consume more bandwidth. Default 1.\n";

	// MH: default packetdup
	sv_packetdup = Cvar_Get ("sv_packetdup", "0", 0);
	sv_packetdup->help = "Default number of duplicate packets sent to a client suffering packet loss, if they have rate set to at least 15000. Default 0.\n";

	sv_redirect_address = Cvar_Get ("sv_redirect_address", "", 0);
	sv_redirect_address->help = "Address to redirect clients to if the server is full. Can be a hostname or IP. Default empty.\n";

#if !KINGPIN
	sv_fps = Cvar_Get ("sv_fps", "10", CVAR_LATCH);
	sv_fps->help = "FPS to run server at. Do not touch unless you know what you're doing. Default 10.\n";

	sv_max_player_updates = Cvar_Get ("sv_max_player_updates", "2", 0);
	sv_max_player_updates->help = "Maximum number of extra player updates to send in between game frames. Default 2.\n";
#endif

	sv_minpps = Cvar_Get ("sv_minpps", "0", 0);
	sv_minpps->help = "Minimum number of packets/sec a client is required to send before they are kicked. Default 0.\n";

	sv_disconnect_hack = Cvar_Get ("sv_disconnect_hack", "1", 0);
	sv_disconnect_hack->help = "Turn svc_disconnect and stuffcmd disconnect into a proper disconnect for buggy mods. Default 1.\n";

#if !KINGPIN
	sv_interpolated_pmove = Cvar_Get ("sv_interpolated_pmove", "0", 0);
	sv_interpolated_pmove->help = "Interpolate player movements server-side that have a duration of this value or higher to help compensate for low packet rates. Be sure you fully understand how it works before enabling; see the R1Q2 readme. Default 0.\n";
#endif

	sv_global_master = Cvar_Get ("sv_global_master", "1", 0);
#if KINGPIN
	sv_global_master->help = "Report to the master server at kingpin.info. Default 1.\n";
#else
	sv_global_master->help = "Report to the global Q2 master at q2servers.com. Default 1.\n";
#endif

#if !KINGPIN
	sv_disallow_download_sprites_hack = Cvar_Get ("sv_disallow_download_sprites_hack", "1", 0);
	sv_disallow_download_sprites_hack->help = "Disallow downloads of sprites (.sp2) to protocol 34 clients. 3.20 and other clients do not fetch linked skins on sprites, which may cause a crash when trying to render them with missing skins. Default 1.\n";
#endif

#if KINGPIN
	sv_features = Cvar_Get ("sv_features", va("%d", GMF_CLIENTPOV | GMF_WANT_ALL_DISCONNECTS | GMF_CLIENTTEAM | GMF_CLIENTNOENTS | GMF_WANT_COUNTRY), CVAR_NOSET);
#else
	sv_features = Cvar_Get ("sv_features", va("%d", GMF_CLIENTNUM | GMF_WANT_ALL_DISCONNECTS | GMF_PROPERINUSE | GMF_WANT_COUNTRY), CVAR_NOSET);
#endif
	sv_features->help = "Read-only bitmask of extended server features for the Game DLL. Do not modify.\n";

	g_features = Cvar_Get ("g_features", "0", CVAR_NOSET);
	g_features->help = "Read-only bitmask of extended game features for the server. Do not modify.\n";

	// MH: disable fov zooming
	sv_no_zoom = Cvar_Get ("sv_no_zoom", "0", 0);
	sv_no_zoom->help = "Disable zooming-in by setting fov below 90. Default 0.\n";

	// MH: idle mode that suspends the game when a server is empty for minimal CPU usage
	g_idle = Cvar_Get ("g_idle", "0", CVAR_NOSET);
	g_idle->changed = _g_idle_changed;

	// MH: automatically enable idle mode when the server is empty
	sv_autoidle = Cvar_Get ("sv_autoidle", "0", 0);
	sv_autoidle->help = "Suspend the game when the server is empty. Default 0.\n";

	// MH: minimize memory usage option
	sv_minimize_memory = Cvar_Get ("sv_minimize_memory", "0", 0);
	sv_minimize_memory->help = "Minimize memory usage when the server is empty. Default 0.\n";

	// MH: GeoIP database
	sv_geoipdb = Cvar_Get ("sv_geoipdb", Sys_FileLength("GeoLite2-Country.mmdb") > 0 ? "GeoLite2-Country.mmdb" : "", 0);
	sv_geoipdb->changed = _geoipdb_changed;
	sv_geoipdb->help = "The MaxMind GeoIP2 database to use for player country lookups. The database can be downloaded from http://dev.maxmind.com/geoip/geoip2/geolite2/. Default \"GeoLite2-Country.mmdb\" if it exists, otherwise empty (disabled).\n";

#if KINGPIN
	// MH: allow models that are not on the server
	sv_allow_any_model = Cvar_Get ("sv_allow_any_model", "0", 0);
	sv_allow_any_model->help = "Allow players to use models that the server does not have. Default 0.\n";

	// MH: compression level
	sv_compress = Cvar_Get ("sv_compress", "1", 0);
	sv_compress->help = "Compression level to use when sending compressed game state data to patched clients. Default 1.\n0: Disabled\n1-9: Lowest/fastest to Highest/slowest\n";

	// MH: compression level for downloads
	sv_compress_downloads = Cvar_Get ("sv_compress_downloads", "1", 0);
	sv_compress_downloads->help = "Compression level to use when sending compressed files to patched clients. Default 1.\n0: Disabled\n1-9: Lowest/fastest to Highest/slowest\n";

	// MH: pre-cache downloads to the system file cache
	sv_download_precache = Cvar_Get ("sv_download_precache", "0", 0);
	sv_download_precache->help = "Pre-cache downloads to the system file cache before sending them to avoid I/O delays. Default 0.\n";

	// MH: upstream bandwidth available
	sv_bandwidth = Cvar_Get ("sv_bandwidth", "0", 0);
	sv_bandwidth->help = "The upstream bandwidth available in KB/s. This determines how fast downloads can be. Default 0 (no limit).\n";

	// MH: underwater sound
	sv_underwater_sound = Cvar_Get ("sv_underwater_sound", "0", 0);
	sv_underwater_sound->help = "Play an underwater sound to players that are under water. Default 0.\n";
#endif
}

/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
static void SV_FinalMessage (const char *message, qboolean reconnect)
{
	int			i;
	client_t	*cl;
	
	SZ_Clear (&net_message);
	MSG_Clear();
	MSG_BeginWriting (svc_print);
	MSG_WriteByte (PRINT_HIGH);
	MSG_WriteString (message);
	MSG_EndWriting (&net_message);

	if (reconnect)
		SZ_WriteByte (&net_message, svc_reconnect);
	else
		SZ_WriteByte (&net_message, svc_disconnect);

	// send it twice
	// stagger the packets to crutch operating system limited buffers

	for (i=0, cl = svs.clients ; i<maxclients->intvalue ; i++, cl++)
		if (cl->state >= cs_connected)
		{
			Netchan_Transmit (&cl->netchan, net_message.cursize
			, net_message_buffer);
		}

	for (i=0, cl = svs.clients ; i<maxclients->intvalue ; i++, cl++)
		if (cl->state >= cs_connected)
		{
			Netchan_Transmit (&cl->netchan, net_message.cursize
			, net_message_buffer);
		}
}



/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown (char *finalmsg, qboolean reconnect, qboolean crashing)
{
	if (svs.clients)
		SV_FinalMessage (finalmsg, reconnect);

	Master_Shutdown ();

	if (!crashing || dbg_unload->intvalue)
		SV_ShutdownGameProgs ();

	// free current level
	if (sv.demofile)
		fclose (sv.demofile);
	memset (&sv, 0, sizeof(sv));
	Com_SetServerState (sv.state);

	// free server static data
	if (svs.clients)
	{
		// MH: free additional client data
		int i;
		for (i=0; i<maxclients->intvalue; i++)
		{
			if (svs.clients[i].state)
				SV_CleanClient(svs.clients + i);
		}

		Z_Free (svs.clients);
	}

	if (svs.client_entities)
		Z_Free (svs.client_entities);

	if (svs.demofile)
		fclose (svs.demofile);

	if (q2_initialized)
	{
		Cvar_ForceSet ("$mapname", "");
		Cvar_ForceSet ("$game", "");
	}

	memset (&svs, 0, sizeof(svs));

	MSG_Clear();

	// MH: close GeoIP database
	if (mmdb.filename)
		MMDB_close(&mmdb);
}

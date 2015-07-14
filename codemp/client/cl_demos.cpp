// Copyright (C) 2006 Sjoerd van der Berg
//
// cl_demos.c -- Enhanced demo player and server demo recorder

#include "client.h"
#include "../server/server.h"
#include "cl_demos.h"
#include "../qcommon/game_version.h"

#define DEMOLISTSIZE 1024

typedef struct {
	char demoName[ MAX_OSPATH ];
	char projectName[ MAX_OSPATH ];
} demoListEntry_t;

static demoListEntry_t	demoList[DEMOLISTSIZE];
static int				demoListIndex, demoListCount;
static demo_t			demo;
static byte				demoBuffer[128*1024];
static entityState_t	demoNullEntityState;
static playerState_t	demoNullPlayerState;
static qboolean			demoFirstPack;
static qboolean			demoPrecaching = qfalse;
static qboolean			demoCommandSmoothing = qfalse;
static int				demoNextNum = 0, demoCurrentNum = 0;
static clientConnection_t demoCutClc;
static clientActive_t	demoCutCl;

static const char *demoHeader = JK_VERSION " Demo";

static void demoFrameAddString( demoString_t *string, int num, const char *newString) {
	int			dataLeft, len;
	int			i;
	char		cache[sizeof(string->data)];

	if (!newString[0]) {
		string->offsets[num] = 0;
		return;
	}
	len = strlen( newString ) + 1;
	dataLeft = sizeof(string->data) - string->used;
	if (len <= dataLeft) {
		Com_Memcpy( string->data + string->used, newString, len );
		string->offsets[num] = string->used;
		string->used += len;
		return;
	}
	cache[0] = 0;
	string->used = 1;
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		const char * s;
		s = (i == num) ? newString : string->data + string->offsets[i];
		if (!s[0]) {
			string->offsets[i] = 0;
			continue;
		}
		dataLeft = sizeof(cache) - string->used;
		len = strlen( s ) + 1;
		if ( len >= dataLeft) {
			Com_Printf( "Adding configstring %i with %s overflowed buffer", i, s );
			return;
		}
		Com_Memcpy( cache + string->used, s, len );
		string->offsets[i] = string->used;
		string->used += len;
	}
	Com_Memcpy( string->data, cache, string->used );
}

static void demoFrameUnpack( msg_t *msg, demoFrame_t *oldFrame, demoFrame_t *newFrame ) {
	int last;
	qboolean isDelta = MSG_ReadBits( msg, 1 ) ? qfalse : qtrue;
	if (!isDelta)
		oldFrame = 0;

	newFrame->serverTime = MSG_ReadLong( msg );
	/* Read config strings */
	newFrame->string.data[0] = 0;
	newFrame->string.used = 1;
	last = 0;
	/* Extract config strings */
	while ( 1 ) {
		int i, num = MSG_ReadShort( msg );
		if (!isDelta ) {
			for (i = last;i<num;i++)
				newFrame->string.offsets[i] = 0;
		} else {
			for (i = last;i<num;i++)
				demoFrameAddString( &newFrame->string, i, oldFrame->string.data + oldFrame->string.offsets[i] );
		}
		if (num < MAX_CONFIGSTRINGS) {
			demoFrameAddString( &newFrame->string, num, MSG_ReadBigString( msg ) );
		} else {
			break;
		}
		last = num + 1;
	}
    /* Extract player states */
	Com_Memset( newFrame->clientData, 0, sizeof( newFrame->clientData ));
	last = MSG_ReadByte( msg );
	while (last < MAX_CLIENTS) {
		playerState_t *oldPlayer, *newPlayer;
		playerState_t *oldVeh, *newVeh;
		newFrame->clientData[last] = 1;
		oldPlayer = isDelta && oldFrame->clientData[last] ? &oldFrame->clients[last] : &demoNullPlayerState;
		newPlayer = &newFrame->clients[last];
		MSG_ReadDeltaPlayerstate( msg, oldPlayer, newPlayer );
		if (newPlayer->m_iVehicleNum) {
			oldVeh = isDelta && oldFrame->clientData[last] ? &oldFrame->vehs[last] : &demoNullPlayerState;
			newVeh = &newFrame->vehs[last];
			MSG_ReadDeltaPlayerstate( msg, oldVeh, newVeh, qtrue );
		}
		last = MSG_ReadByte( msg );
	}
	/* Extract entity states */
	last = 0;
	while ( 1 ) {
		int i, num = MSG_ReadBits( msg, GENTITYNUM_BITS );
		entityState_t *oldEntity, *newEntity;	
		if ( isDelta ) {
			for (i = last;i<num;i++)
				if (oldFrame->entities[i].number == i)
					newFrame->entities[i] = oldFrame->entities[i];
				else
					newFrame->entities[i].number = MAX_GENTITIES - 1;
		} else {
			for (i = last;i<num;i++)
				newFrame->entities[i].number = MAX_GENTITIES - 1;
		}
		if (num < MAX_GENTITIES - 1) {
			if (isDelta) {
				oldEntity = &oldFrame->entities[num];
				if (oldEntity->number != num)
					oldEntity = &demoNullEntityState;
			} else {
				oldEntity = &demoNullEntityState;
			}
			newEntity = &newFrame->entities[i];
			MSG_ReadDeltaEntity( msg, oldEntity, newEntity, num );
		} else
			break;
		last = num + 1;
	}
	/* Read the area mask */
	newFrame->areaUsed = MSG_ReadByte( msg );
	MSG_ReadData( msg, newFrame->areamask, newFrame->areaUsed );
	/* Read the command string data */
	newFrame->commandUsed = MSG_ReadLong( msg );
	MSG_ReadData( msg, newFrame->commandData, newFrame->commandUsed );
	/* Extract the entity baselines */
	while ( demoFirstPack ) {
		byte cmd = MSG_ReadByte( msg );
		if (cmd == svc_EOF)
			break;
		if (cmd == svc_baseline) {
			int num = MSG_ReadBits( msg, GENTITYNUM_BITS );
			if ( num < 0 || num >= MAX_GENTITIES ) {
				Com_Error( ERR_DROP, "Baseline number out of range: %i.\n", num );
			} else {
				entityState_t *newEntity = &newFrame->entityBaselines[num];
				MSG_ReadDeltaEntity( msg, &demoNullEntityState, newEntity, num );
			}
		} else {
			Com_Error( ERR_DROP, "Unknown block %d while unpacking demo frame.\n", cmd );
		}
	}
	demoFirstPack = qfalse;
}

static void demoFramePack( msg_t *msg, const demoFrame_t *newFrame, const demoFrame_t *oldFrame ) {
	int i;
	/* Full or delta frame marker */
	MSG_WriteBits( msg, oldFrame ? 0 : 1, 1 );
	MSG_WriteLong( msg, newFrame->serverTime );
	/* Add the config strings */
	for (i = 0;i<MAX_CONFIGSTRINGS;i++) {
		const char *oldString = !oldFrame ? "" : &oldFrame->string.data[oldFrame->string.offsets[i]];
		const char *newString = newFrame->string.data + newFrame->string.offsets[i];
		if (strcmp( oldString, newString)) {
			MSG_WriteShort( msg, i );
			MSG_WriteBigString( msg, newString );
		}
	}
	MSG_WriteShort( msg, MAX_CONFIGSTRINGS );
	/* Add the playerstates */
	for (i=0; i<MAX_CLIENTS; i++) {
		const playerState_t *oldPlayer, *newPlayer;
		const playerState_t *oldVeh, *newVeh;
		if (!newFrame->clientData[i])
			continue;
		oldPlayer = (!oldFrame || !oldFrame->clientData[i]) ? &demoNullPlayerState : &oldFrame->clients[i];
		newPlayer = &newFrame->clients[i];
		MSG_WriteByte( msg, i );
		MSG_WriteDeltaPlayerstate( msg, (playerState_t *)oldPlayer, (playerState_t *)newPlayer );
		if (newPlayer->m_iVehicleNum) {
			oldVeh = (!oldFrame || !oldFrame->clientData[i]) ? &demoNullPlayerState : &oldFrame->vehs[i];
			newVeh = &newFrame->vehs[i];
			MSG_WriteDeltaPlayerstate( msg, (playerState_t *)oldVeh, (playerState_t *)newVeh, qtrue );
		}
	}
	MSG_WriteByte( msg, MAX_CLIENTS );
	/* Add the entities */
	for (i=0; i<MAX_GENTITIES-1; i++) {
		const entityState_t *oldEntity, *newEntity;
		newEntity = &newFrame->entities[i];
		if (oldFrame) {
			oldEntity = &oldFrame->entities[i];
			if (oldEntity->number == (MAX_GENTITIES -1))
				oldEntity = 0;
		} else {
			oldEntity = 0;
		}
		if (newEntity->number != i || newEntity->number >= (MAX_GENTITIES -1)) {
			newEntity = 0;
		} else {
			if (!oldEntity) {
				oldEntity = &demoNullEntityState;
			}
		}
		MSG_WriteDeltaEntity( msg, (entityState_t *)oldEntity, (entityState_t *)newEntity, qtrue );
	}
	MSG_WriteBits( msg, (MAX_GENTITIES-1), GENTITYNUM_BITS );
	/* Add the area mask */
	MSG_WriteByte( msg, newFrame->areaUsed );
	MSG_WriteData( msg, newFrame->areamask, newFrame->areaUsed );
	/* Add the command string data */
	MSG_WriteLong( msg, newFrame->commandUsed );
	MSG_WriteData( msg, newFrame->commandData, newFrame->commandUsed );
	/* Add the entity baselines */
	if (demoFirstPack) {
		for (i=0; i<MAX_GENTITIES; i++) {
			const entityState_t *newEntity = &newFrame->entityBaselines[i];
			if ( !newEntity->number ) {
				continue;
			}
			MSG_WriteByte ( msg, svc_baseline );
			MSG_WriteDeltaEntity( msg, &demoNullEntityState, (entityState_t *)newEntity, qtrue );
		}
		MSG_WriteByte( msg, svc_EOF ); //using it for correct break in unpack
		demoFirstPack = qfalse;
	}
}

static void demoFrameInterpolate( demoFrame_t frames[], int frameCount, int index ) {
	int i;
	demoFrame_t *workFrame;

	workFrame = &frames[index % frameCount];
//	return;

	for (i=0; i<MAX_CLIENTS; i++) {
		entityState_t *workEntity;
		workEntity = &workFrame->entities[i];
		if (workEntity->number != ENTITYNUM_NONE && !workFrame->entityData[i]) {
			int m;
			demoFrame_t *prevFrame, *nextFrame;
			prevFrame = nextFrame = 0;
			for (m = index - 1; (m > (index - frameCount)) && m >0 ; m--) {
				demoFrame_t *testFrame = &frames[ m % frameCount];
				if ( !testFrame->entityData[i])
					continue;
				if (!testFrame->serverTime || testFrame->serverTime >= workFrame->serverTime)
					break;
				if ( testFrame->entities[i].number != i)
					break;
				if ( (testFrame->entities[i].eFlags ^ workEntity->eFlags) & EF_TELEPORT_BIT )
					break;
				prevFrame = testFrame;
				break;
			}
			for (m = index + 1; (m < (index + frameCount)); m++) {
				demoFrame_t *testFrame = &frames[ m % frameCount];
				if ( !testFrame->entityData[i])
					continue;
				if (!testFrame->serverTime || testFrame->serverTime <= workFrame->serverTime)
					break;
				if ( testFrame->entities[i].number != i)
					break;
				if ( (testFrame->entities[i].eFlags ^ workEntity->eFlags) & EF_TELEPORT_BIT )
					break;
				nextFrame = testFrame;
				break;
			}
			if (prevFrame && nextFrame) {
				const entityState_t *prevEntity = &prevFrame->entities[i];
				const entityState_t *nextEntity = &nextFrame->entities[i];
				float lerp;
				int posDelta;
				float posLerp;
				int	 prevTime, nextTime;
                  
				prevTime = prevFrame->serverTime;
				nextTime = nextFrame->serverTime;
				lerp = (workFrame->serverTime - prevTime) / (float)( nextTime - prevTime );
				posDelta = nextEntity->pos.trTime - prevEntity->pos.trTime;
				if ( posDelta ) {
					workEntity->pos.trTime = prevEntity->pos.trTime + posDelta * lerp;
					posLerp = (workEntity->pos.trTime - prevEntity->pos.trTime) / (float) posDelta;
				} else {
					posLerp = lerp;
				}
				LerpOrigin( prevEntity->pos.trBase, nextEntity->pos.trBase, workEntity->pos.trBase, posLerp );
				LerpOrigin( prevEntity->pos.trDelta, nextEntity->pos.trDelta, workEntity->pos.trDelta, posLerp );

				LerpAngles( prevEntity->apos.trBase, nextEntity->apos.trBase, workEntity->apos.trBase, lerp );
			}
		}
	}
}

qboolean demoCutConfigstringModified(clientActive_t *clCut) {
	char		*old, *s;
	int			i, index;
	char		*dup;
	gameState_t	oldGs;
	int			len;
	index = atoi(Cmd_Argv(1));
	if (index < 0 || index >= MAX_CONFIGSTRINGS) {
		Com_Printf("demoCutConfigstringModified: bad index %i", index);
		return qtrue;
	}
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom(2);
	old = clCut->gameState.stringData + clCut->gameState.stringOffsets[index];
	if (!strcmp(old, s)) {
		return qtrue; // unchanged
	}
	// build the new gameState_t
	oldGs = clCut->gameState;
	Com_Memset(&clCut->gameState, 0, sizeof(clCut->gameState));
	// leave the first 0 for uninitialized strings
	clCut->gameState.dataCount = 1;
	for (i = 0; i < MAX_CONFIGSTRINGS; i++) {
		if (i == index) {
			dup = s;
		} else {
			dup = oldGs.stringData + oldGs.stringOffsets[i];
		}
		if (!dup[0]) {
			continue; // leave with the default empty string
		}
		len = strlen(dup);
		if (len + 1 + clCut->gameState.dataCount > MAX_GAMESTATE_CHARS) {
			Com_Printf("MAX_GAMESTATE_CHARS exceeded");
			return qfalse;
		}
		// append it to the gameState string buffer
		clCut->gameState.stringOffsets[i] = clCut->gameState.dataCount;
		Com_Memcpy(clCut->gameState.stringData + clCut->gameState.dataCount, dup, len + 1);
		clCut->gameState.dataCount += len + 1;
	}
	return qtrue;
}

void demoCutWriteDemoHeader(fileHandle_t f, clientConnection_t *clcCut, clientActive_t *clCut) {
	byte			bufData[MAX_MSGLEN];
	msg_t			buf;
	int				i;
	int				len;
	entityState_t	*ent;
	entityState_t	nullstate;
	char			*s;
	// write out the gamestate message
	MSG_Init(&buf, bufData, sizeof(bufData));
	MSG_Bitstream(&buf);
	// NOTE, MRE: all server->client messages now acknowledge
	MSG_WriteLong(&buf, clcCut->reliableSequence);
	MSG_WriteByte(&buf, svc_gamestate);
	MSG_WriteLong(&buf, clcCut->serverCommandSequence);
	// configstrings
	for (i = 0; i < MAX_CONFIGSTRINGS; i++) {
		if (!clCut->gameState.stringOffsets[i]) {
			continue;
		}
		s = clCut->gameState.stringData + clCut->gameState.stringOffsets[i];
		MSG_WriteByte(&buf, svc_configstring);
		MSG_WriteShort(&buf, i);
		MSG_WriteBigString(&buf, s);
	}
	// baselines
	Com_Memset(&nullstate, 0, sizeof(nullstate));
	for (i = 0; i < MAX_GENTITIES ; i++) {
		ent = &clCut->entityBaselines[i];
		if ( !ent->number ) {
			continue;
		}
		MSG_WriteByte(&buf, svc_baseline);
		MSG_WriteDeltaEntity(&buf, &nullstate, ent, qtrue);
	}
	MSG_WriteByte(&buf, svc_EOF);
	// finished writing the gamestate stuff
	// write the client num
	MSG_WriteLong(&buf, clcCut->clientNum);
	// write the checksum feed
	MSG_WriteLong(&buf, clcCut->checksumFeed);
	// RMG stuff
	if ( clcCut->rmgHeightMapSize ) {
		// Height map
		MSG_WriteShort(&buf, (unsigned short)clcCut->rmgHeightMapSize);
		MSG_WriteBits(&buf, 0, 1 );
		MSG_WriteData(&buf, clcCut->rmgHeightMap, clcCut->rmgHeightMapSize);
		// Flatten map
		MSG_WriteShort(&buf, (unsigned short)clcCut->rmgHeightMapSize);
		MSG_WriteBits(&buf, 0, 1 );
		MSG_WriteData(&buf, clcCut->rmgFlattenMap, clcCut->rmgHeightMapSize);
		// Seed
		MSG_WriteLong (&buf, clcCut->rmgSeed);
		// Automap symbols
		MSG_WriteShort (&buf, (unsigned short)clcCut->rmgAutomapSymbolCount);
		for (i = 0; i < clcCut->rmgAutomapSymbolCount; i ++) {
			MSG_WriteByte(&buf, (unsigned char)clcCut->rmgAutomapSymbols[i].mType);
			MSG_WriteByte(&buf, (unsigned char)clcCut->rmgAutomapSymbols[i].mSide);
			MSG_WriteLong(&buf, (long)clcCut->rmgAutomapSymbols[i].mOrigin[0]);
			MSG_WriteLong(&buf, (long)clcCut->rmgAutomapSymbols[i].mOrigin[1]);
		}
	} else {
		MSG_WriteShort (&buf, 0);
	}
	// finished writing the client packet
	MSG_WriteByte(&buf, svc_EOF);
	// write it to the demo file
	len = LittleLong(clcCut->serverMessageSequence - 1);
	FS_Write(&len, 4, f);
	len = LittleLong(buf.cursize);
	FS_Write(&len, 4, f);
	FS_Write(buf.data, buf.cursize, f);
}

static void demoCutEmitPacketEntities(clSnapshot_t *from, clSnapshot_t *to, msg_t *msg, clientActive_t *clCut) {
	entityState_t *oldent, *newent;
	int oldindex, newindex;
	int oldnum, newnum;
	int from_num_entities;
	// generate the delta update
	if (!from) {
		from_num_entities = 0;
	} else {
		from_num_entities = from->numEntities;
	}
	newent = NULL;
	oldent = NULL;
	newindex = 0;
	oldindex = 0;
	while (newindex < to->numEntities || oldindex < from_num_entities) {
		if (newindex >= to->numEntities) {
			newnum = 9999;
		} else {
			newent = &clCut->parseEntities[(to->parseEntitiesNum+newindex) & (MAX_PARSE_ENTITIES-1)];
			newnum = newent->number;
		}
		if (oldindex >= from_num_entities) {
			oldnum = 9999;
		} else {
			oldent = &clCut->parseEntities[(from->parseEntitiesNum+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldent->number;
		}
		if (newnum == oldnum) {
			// delta update from old position
			// because the force parm is qfalse, this will not result
			// in any bytes being emited if the entity has not changed at all
			MSG_WriteDeltaEntity(msg, oldent, newent, qfalse );
			oldindex++;
			newindex++;
			continue;
		}
		if (newnum < oldnum) {
			// this is a new entity, send it from the baseline
			MSG_WriteDeltaEntity(msg, &clCut->entityBaselines[newnum], newent, qtrue);
			newindex++;
			continue;
		}
		if (newnum > oldnum) {
			// the old entity isn't present in the new message
			MSG_WriteDeltaEntity(msg, oldent, NULL, qtrue);
			oldindex++;
			continue;
		}
	}
	MSG_WriteBits(msg, (MAX_GENTITIES-1), GENTITYNUM_BITS);	// end of packetentities
}

void demoCutWriteDemoMessage(msg_t *msg, fileHandle_t f, clientConnection_t *clcCut) {
	int len;
	// write the packet sequence
	len = LittleLong(clcCut->serverMessageSequence);
	FS_Write(&len, 4, f);
	// skip the packet sequencing information
	len = LittleLong(msg->cursize);
	FS_Write(&len, 4, f);
	FS_Write(msg->data, msg->cursize, f);
}

void demoCutWriteDeltaSnapshot(int firstServerCommand, fileHandle_t f, qboolean forceNonDelta, clientConnection_t *clcCut, clientActive_t *clCut) {
	msg_t			msgImpl, *msg = &msgImpl;
	byte			msgData[MAX_MSGLEN];
	clSnapshot_t	*frame, *oldframe;
	int				lastframe = 0;
	int				snapFlags;
	MSG_Init(msg, msgData, sizeof(msgData));
	MSG_Bitstream(msg);
	MSG_WriteLong(msg, clcCut->reliableSequence);
	// copy over any commands
	for (int serverCommand = firstServerCommand; serverCommand <= clcCut->serverCommandSequence; serverCommand++) {
		char *command = clcCut->serverCommands[serverCommand & (MAX_RELIABLE_COMMANDS - 1)];
		MSG_WriteByte(msg, svc_serverCommand);
		MSG_WriteLong(msg, serverCommand/* + serverCommandOffset*/);
		MSG_WriteString(msg, command);
	}
	// this is the snapshot we are creating
	frame = &clCut->snap;
	if (clCut->snap.messageNum > 0 && !forceNonDelta) {
		lastframe = 1;
		oldframe = &clCut->snapshots[(clCut->snap.messageNum - 1) & PACKET_MASK]; // 1 frame previous
		if (!oldframe->valid) {
			// not yet set
			lastframe = 0;
			oldframe = NULL;
		}
	} else {
		lastframe = 0;
		oldframe = NULL;
	}
	MSG_WriteByte(msg, svc_snapshot);
	// send over the current server time so the client can drift
	// its view of time to try to match
	MSG_WriteLong(msg, frame->serverTime);
	// what we are delta'ing from
	MSG_WriteByte(msg, lastframe);
	snapFlags = frame->snapFlags;
	MSG_WriteByte(msg, snapFlags);
	// send over the areabits
	MSG_WriteByte(msg, sizeof(frame->areamask));
	MSG_WriteData(msg, frame->areamask, sizeof(frame->areamask));
	// delta encode the playerstate
	if (oldframe) {
#ifdef _ONEBIT_COMBO
		MSG_WriteDeltaPlayerstate(msg, &oldframe->ps, &frame->ps, frame->pDeltaOneBit, frame->pDeltaNumBit);
#else
		MSG_WriteDeltaPlayerstate(msg, &oldframe->ps, &frame->ps);
#endif
		if (frame->ps.m_iVehicleNum) {
		//then write the vehicle's playerstate too
			if (!oldframe->ps.m_iVehicleNum) {
			//if last frame didn't have vehicle, then the old vps isn't gonna delta
			//properly (because our vps on the client could be anything)
#ifdef _ONEBIT_COMBO
				MSG_WriteDeltaPlayerstate(msg, NULL, &frame->vps, NULL, NULL, qtrue);
#else
				MSG_WriteDeltaPlayerstate(msg, NULL, &frame->vps, qtrue);
#endif
			} else {
#ifdef _ONEBIT_COMBO
				MSG_WriteDeltaPlayerstate(msg, &oldframe->vps, &frame->vps, frame->pDeltaOneBitVeh, frame->pDeltaNumBitVeh, qtrue);
#else
				MSG_WriteDeltaPlayerstate(msg, &oldframe->vps, &frame->vps, qtrue);
#endif
			}
		}
	} else {
#ifdef _ONEBIT_COMBO
		MSG_WriteDeltaPlayerstate(msg, NULL, &frame->ps, NULL, NULL);
#else
		MSG_WriteDeltaPlayerstate(msg, NULL, &frame->ps);
#endif
		if (frame->ps.m_iVehicleNum) {
		//then write the vehicle's playerstate too
#ifdef _ONEBIT_COMBO
			MSG_WriteDeltaPlayerstate(msg, NULL, &frame->vps, NULL, NULL, qtrue);
#else
			MSG_WriteDeltaPlayerstate(msg, NULL, &frame->vps, qtrue);
#endif
		}
	}
	// delta encode the entities
	demoCutEmitPacketEntities(oldframe, frame, msg, clCut);
	MSG_WriteByte(msg, svc_EOF);
	demoCutWriteDemoMessage(msg, f, clcCut);
}

void demoCutParsePacketEntities(msg_t *msg, clSnapshot_t *oldSnap, clSnapshot_t *newSnap, clientActive_t *clCut) {
	/* The beast that is entity parsing */
	int			newnum;
	entityState_t	*oldstate, *newstate;
	int			oldindex = 0;
	int			oldnum;
	newSnap->parseEntitiesNum = clCut->parseEntitiesNum;
	newSnap->numEntities = 0;
	newnum = MSG_ReadBits(msg, GENTITYNUM_BITS);
	while (1) {
		// read the entity index number
		if (oldSnap && oldindex < oldSnap->numEntities) {
			oldstate = &clCut->parseEntities[(oldSnap->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		} else {
			oldstate = 0;
			oldnum = 99999;
		}
		newstate = &clCut->parseEntities[clCut->parseEntitiesNum];
		if (!oldstate && (newnum == (MAX_GENTITIES-1))) {
			break;
		} else if (oldnum < newnum) {
			*newstate = *oldstate;
			oldindex++;
		} else if (oldnum == newnum) {
			oldindex++;
			MSG_ReadDeltaEntity(msg, oldstate, newstate, newnum);
			newnum = MSG_ReadBits(msg, GENTITYNUM_BITS);
		} else if (oldnum > newnum) {
			MSG_ReadDeltaEntity(msg, &clCut->entityBaselines[newnum], newstate, newnum);
			newnum = MSG_ReadBits(msg, GENTITYNUM_BITS);
		}
		if (newstate->number == MAX_GENTITIES-1)
			continue;
		clCut->parseEntitiesNum++;
		clCut->parseEntitiesNum &= (MAX_PARSE_ENTITIES-1);
		newSnap->numEntities++;
	}
}

qboolean demoCutParseSnapshot(msg_t *msg, clientConnection_t *clcCut, clientActive_t *clCut) {
	int len;
	clSnapshot_t *oldSnap;
	clSnapshot_t newSnap;
	int deltaNum;
	int oldMessageNum;
	int i, packetNum;
	Com_Memset(&newSnap, 0, sizeof(newSnap));
	newSnap.serverCommandNum = clcCut->serverCommandSequence;
	newSnap.serverTime = MSG_ReadLong(msg);
	cl_paused->modified = qfalse;
	newSnap.messageNum = clcCut->serverMessageSequence;
	deltaNum = MSG_ReadByte(msg);
	newSnap.snapFlags = MSG_ReadByte(msg);
	if (!deltaNum) {
		newSnap.deltaNum = -1;
		newSnap.valid = qtrue;		// uncompressed frame
		oldSnap = NULL;
	} else {
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
		oldSnap = &clCut->snapshots[newSnap.deltaNum & PACKET_MASK];
		if (!oldSnap->valid) {
			// should never happen
			Com_Printf("Delta from invalid frame (not supposed to happen!).\n");
		} else if (oldSnap->messageNum != newSnap.deltaNum) {
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_Printf("Delta frame too old.\n");
		} else if (clCut->parseEntitiesNum - oldSnap->parseEntitiesNum > MAX_PARSE_ENTITIES-128) {
			Com_DPrintf("Delta parseEntitiesNum too old.\n");
		} else {
			newSnap.valid = qtrue;	// valid delta parse
		}
	}
	// read areamask
	len = MSG_ReadByte(msg);
	if(len > sizeof(newSnap.areamask)) {
		Com_Printf("demoCutParseSnapshot: Invalid size %d for areamask", len);
		return qfalse;
	}
	MSG_ReadData(msg, &newSnap.areamask, len);
	// read playerinfo
	MSG_ReadDeltaPlayerstate(msg, oldSnap ? &oldSnap->ps : NULL, &newSnap.ps);
	if(newSnap.ps.m_iVehicleNum)
		MSG_ReadDeltaPlayerstate(msg, oldSnap ? &oldSnap->vps : NULL, &newSnap.vps, qtrue);
	// read packet entities
	demoCutParsePacketEntities(msg, oldSnap, &newSnap, clCut);
	// if not valid, dump the entire thing now that it has
	// been properly read
	if (!newSnap.valid) {
		return qtrue;
	}
	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = clCut->snap.messageNum + 1;
	if (newSnap.messageNum - oldMessageNum >= PACKET_BACKUP) {
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP - 1);
	}
	for (;oldMessageNum < newSnap.messageNum; oldMessageNum++) {
		clCut->snapshots[oldMessageNum & PACKET_MASK].valid = qfalse;
	}
	// copy to the current good spot
	clCut->snap = newSnap;
	clCut->snap.ping = 999;
	// calculate ping time
	for (i = 0 ; i < PACKET_BACKUP ; i++) {
		packetNum = (clcCut->netchan.outgoingSequence-1-i) & PACKET_MASK;
		if (clCut->snap.ps.commandTime >= clCut->outPackets[packetNum].p_serverTime) {
			clCut->snap.ping = cls.realtime - clCut->outPackets[packetNum].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	clCut->snapshots[clCut->snap.messageNum & PACKET_MASK] = clCut->snap;
	clCut->newSnapshots = qtrue;
	return qtrue;
}

#include "../zlib/zlib.h"
void demoCutParseRMG(msg_t *msg, clientConnection_t *clcCut, clientActive_t *clCut) {
	int i;
	clcCut->rmgHeightMapSize = (unsigned short)MSG_ReadShort(msg);
	if (clcCut->rmgHeightMapSize == 0) {
		return;
	}
	z_stream zdata;
	int flatDataSize;
	unsigned char heightmap1[15000];
	// height map
	if (MSG_ReadBits(msg, 1)) {
		memset(&zdata, 0, sizeof(z_stream));
		inflateInit(&zdata/*, Z_SYNC_FLUSH*/);
		MSG_ReadData (msg, heightmap1, clcCut->rmgHeightMapSize);
		zdata.next_in = heightmap1;
		zdata.avail_in = clcCut->rmgHeightMapSize;
		zdata.next_out = (unsigned char*)clcCut->rmgHeightMap;
		zdata.avail_out = MAX_HEIGHTMAP_SIZE;
		inflate(&zdata,Z_SYNC_FLUSH);
		clcCut->rmgHeightMapSize = zdata.total_out;
		inflateEnd(&zdata);
	} else {
		MSG_ReadData(msg, (unsigned char*)clcCut->rmgHeightMap, clcCut->rmgHeightMapSize);
	}
	// Flatten map
	flatDataSize = MSG_ReadShort(msg);
	if (MSG_ReadBits(msg, 1)) {	
		// Read the flatten map
		memset(&zdata, 0, sizeof(z_stream));
		inflateInit(&zdata/*, Z_SYNC_FLUSH*/);
		MSG_ReadData ( msg, heightmap1, flatDataSize);
		zdata.next_in = heightmap1;
		zdata.avail_in = clcCut->rmgHeightMapSize;
		zdata.next_out = (unsigned char*)clcCut->rmgFlattenMap;
		zdata.avail_out = MAX_HEIGHTMAP_SIZE;
		inflate(&zdata, Z_SYNC_FLUSH);
		inflateEnd(&zdata);
	} else {
		MSG_ReadData(msg, (unsigned char*)clcCut->rmgFlattenMap, flatDataSize);
	}
	// Seed
	clcCut->rmgSeed = MSG_ReadLong(msg);
	// Automap symbols
	clcCut->rmgAutomapSymbolCount = (unsigned short)MSG_ReadShort(msg);
	for(i = 0; i < clcCut->rmgAutomapSymbolCount; i++) {
		clcCut->rmgAutomapSymbols[i].mType = (int)MSG_ReadByte(msg);
		clcCut->rmgAutomapSymbols[i].mSide = (int)MSG_ReadByte(msg);
		clcCut->rmgAutomapSymbols[i].mOrigin[0] = (float)MSG_ReadLong(msg);
		clcCut->rmgAutomapSymbols[i].mOrigin[1] = (float)MSG_ReadLong(msg);
	}
}

qboolean demoCutParseGamestate(msg_t *msg, clientConnection_t *clcCut, clientActive_t *clCut) {
	int				i;
	entityState_t	*es;
	int				newnum;
	entityState_t	nullstate;
	int				cmd;
	char			*s;
	clcCut->connectPacketCount = 0;
	Com_Memset(clCut, 0, sizeof(*clCut));
	clcCut->serverCommandSequence = MSG_ReadLong(msg);
	clCut->gameState.dataCount = 1;
	while (1) {
		cmd = MSG_ReadByte(msg);
		if (cmd == svc_EOF) {
			break;
		}
		if (cmd == svc_configstring) {
			int len, start;
			start = msg->readcount;
			i = MSG_ReadShort(msg);
			if (i < 0 || i >= MAX_CONFIGSTRINGS) {
				Com_Printf("configstring > MAX_CONFIGSTRINGS");
				return qfalse;
			}
			s = MSG_ReadBigString(msg);
			len = strlen(s);
			if (len + 1 + clCut->gameState.dataCount > MAX_GAMESTATE_CHARS) {
				Com_Printf("MAX_GAMESTATE_CHARS exceeded");
				return qfalse;
			}
			// append it to the gameState string buffer
			clCut->gameState.stringOffsets[i] = clCut->gameState.dataCount;
			Com_Memcpy(clCut->gameState.stringData + clCut->gameState.dataCount, s, len + 1);
			clCut->gameState.dataCount += len + 1;
		} else if (cmd == svc_baseline) {
			newnum = MSG_ReadBits(msg, GENTITYNUM_BITS);
			if (newnum < 0 || newnum >= MAX_GENTITIES) {
				Com_Printf("Baseline number out of range: %i", newnum);
				return qfalse;
			}
			Com_Memset(&nullstate, 0, sizeof(nullstate));
			es = &clCut->entityBaselines[newnum];
			MSG_ReadDeltaEntity(msg, &nullstate, es, newnum);
		} else {
			Com_Printf("demoCutParseGameState: bad command byte");
			return qfalse;
		}
	}
	clcCut->clientNum = MSG_ReadLong(msg);
	clcCut->checksumFeed = MSG_ReadLong(msg);
	// RMG stuff
	demoCutParseRMG(msg, clcCut, clCut);
	return qtrue;
}

void demoCutParseCommandString(msg_t *msg, clientConnection_t *clcCut) {
	int index;
	int seq = MSG_ReadLong(msg);
	char *s = MSG_ReadString(msg);
	if (clcCut->serverCommandSequence >= seq) {
		return;
	}
	clcCut->serverCommandSequence = seq;
	index = seq & (MAX_RELIABLE_COMMANDS-1);
	Q_strncpyz(clcCut->serverCommands[index], s, sizeof(clcCut->serverCommands[index]));
}

qboolean demoCut(const char *oldName, int startTime, int endTime) {
	fileHandle_t	oldHandle = 0;
	fileHandle_t	newHandle = 0;
	msg_t			oldMsg;
	byte			oldData[MAX_MSGLEN];
	int				oldSize;
	char			newName[MAX_OSPATH];
	int				buf;
	int				readGamestate = 0;
	int				i;
	demoPlay_t		*play = demo.play.handle;
	qboolean		ret = qfalse;
	int				framesSaved = 0;
	char			newGameDir[MAX_QPATH];
	char			next;
	if (!play) {
		Com_Printf("Demo cutting is allowed in mme mode only.\n");
		return qfalse;
	}
	startTime += play->startTime;
	endTime += play->startTime;
	oldSize = FS_FOpenFileRead(va("demos/%s.dm_26", oldName), &oldHandle, qtrue);
	if (!oldHandle) {
		Com_Printf("Failed to open %s for cutting.\n", oldName);
		return qfalse;
	}
	memset(&demoCutClc, 0, sizeof(demoCutClc));
	while (oldSize > 0) {
cutcontinue:
		MSG_Init(&oldMsg, oldData, sizeof(oldData));
		/* Read the sequence number */
		if (FS_Read(&demoCutClc.serverMessageSequence, 4, oldHandle) != 4)
			goto cuterror;
		demoCutClc.serverMessageSequence = LittleLong(demoCutClc.serverMessageSequence);
		oldSize -= 4;
		/* Read the message size */
		if (FS_Read(&oldMsg.cursize,4, oldHandle) != 4)
			goto cuterror;
		oldMsg.cursize = LittleLong(oldMsg.cursize);
		oldSize -= 4;
		/* Negative size signals end of demo */
		if (oldMsg.cursize < 0)
			break;
		if (oldMsg.cursize > oldMsg.maxsize) 
			goto cuterror;
		/* Read the actual message */
		if (FS_Read(oldMsg.data, oldMsg.cursize, oldHandle) != oldMsg.cursize)
			goto cuterror;
		oldSize -= oldMsg.cursize;
		// init the bitstream
		MSG_BeginReading(&oldMsg);
		// Skip the reliable sequence acknowledge number
		MSG_ReadLong(&oldMsg);
		//
		// parse the message
		//
		while (1) {
			byte cmd;
			if (oldMsg.readcount > oldMsg.cursize) {
				Com_Printf("Demo cutter, read past end of server message.\n");
				goto cuterror;
			}
            cmd = MSG_ReadByte(&oldMsg);
			if (cmd == svc_EOF) {
                break;
			}
			// skip all the gamestates until we reach needed
			if (readGamestate < demoCurrentNum) {
				if (cmd == svc_gamestate) {
					readGamestate++;
				}
				goto cutcontinue;
			}
			// other commands
			switch (cmd) {
			default:
				Com_Printf(S_COLOR_RED"ERROR: CL_ParseServerMessage: Illegible server message\n");
				goto cuterror;		
			case svc_nop:
				break;
			case svc_serverCommand:
				demoCutParseCommandString(&oldMsg, &demoCutClc);
				break;
			case svc_gamestate:
				if (readGamestate > demoCurrentNum) {
					Com_Printf("Warning: unexpected new gamestate, finishing cutting.\n");
					goto cutcomplete;
				}
				if (!demoCutParseGamestate(&oldMsg, &demoCutClc, &demoCutCl)) {
					goto cuterror;
				}
				Com_sprintf(newName, sizeof(newName), "demos/%s_cut.dm_26", oldName);
				newHandle = FS_FOpenFileWrite(newName);
				if (!newHandle) {
					Com_Printf("Failed to open %s for target cutting.\n", newName);
					return qfalse;
				}
				readGamestate++;
				break;
			case svc_snapshot:
				if (!demoCutParseSnapshot(&oldMsg, &demoCutClc, &demoCutCl)) {
					goto cuterror;
				}
				break;
			case svc_download:
				// read block number
				buf = MSG_ReadShort(&oldMsg);
				if (!buf)	//0 block, read file size
					MSG_ReadLong(&oldMsg);
				// read block size
				buf = MSG_ReadShort(&oldMsg);
				// read the data block
				for (;buf>0;buf--)
					MSG_ReadByte(&oldMsg);
				break;
			case svc_setgame:
				i = 0;
				while (i < MAX_QPATH) {
					next = MSG_ReadByte(&oldMsg);
					if(next) {
						newGameDir[i] = next;
					} else {
						break;
					}
					i++;
				}
				newGameDir[i] = 0;
				// But here we stop, and don't do more. If this goes horribly wrong sometime, you might have to go and actually do something with this.
				break;
			case svc_mapchange:
				// nothing to parse.
				break;
			}
		}
		int firstServerCommand = demoCutClc.lastExecutedServerCommand;
		// process any new server commands
		for (;demoCutClc.lastExecutedServerCommand < demoCutClc.serverCommandSequence; demoCutClc.lastExecutedServerCommand++) {
			char *command = demoCutClc.serverCommands[demoCutClc.lastExecutedServerCommand & (MAX_RELIABLE_COMMANDS - 1)];
			Cmd_TokenizeString(command);
			char *cmd = Cmd_Argv(0);
			if (cmd[0]) {
				firstServerCommand = demoCutClc.lastExecutedServerCommand;
			}
			if (!strcmp(cmd, "cs")) {
				if (!demoCutConfigstringModified(&demoCutCl)) {
					goto cuterror;
				}
			}
		}
		if (demoCutCl.snap.serverTime > endTime) {
			goto cutcomplete;
		} else if (framesSaved > 0) {
			/* this msg is in range, write it */
			if (framesSaved > max(10, demoCutCl.snap.messageNum - demoCutCl.snap.deltaNum)) {
				demoCutWriteDemoMessage(&oldMsg, newHandle, &demoCutClc);
			} else {
				demoCutWriteDeltaSnapshot(firstServerCommand, newHandle, qfalse, &demoCutClc, &demoCutCl);
			}
			framesSaved++;
		} else if (demoCutCl.snap.serverTime >= startTime) {
			demoCutWriteDemoHeader(newHandle, &demoCutClc, &demoCutCl);
			demoCutWriteDeltaSnapshot(firstServerCommand, newHandle, qtrue, &demoCutClc, &demoCutCl);
			// copy rest
			framesSaved = 1;
		}
	}
cutcomplete:
	if (newHandle) {
		buf = -1;
		FS_Write(&buf, 4, newHandle);
		FS_Write(&buf, 4, newHandle);
		ret = qtrue;
	}
cuterror:
	//remove previosly converted demo from the same cut
	if (newHandle) {
		memset(newName, 0, sizeof(newName));
		if (demoCurrentNum > 0)
			Com_sprintf(newName, sizeof(newName), "mmedemos/%s.%d_cut.mme", oldName, demoCurrentNum);
		else {
			Com_sprintf(newName, sizeof(newName), "mmedemos/%s_cut.mme", oldName);
		}
		if (FS_FileExists(newName))
			FS_FileErase(newName);
	}
	FS_FCloseFile(oldHandle);
	FS_FCloseFile(newHandle);
	return ret;
}

void demoConvert( const char *oldName, const char *newBaseName, qboolean smoothen ) {
	fileHandle_t	oldHandle = 0;
	fileHandle_t	newHandle = 0;
	int				temp;
	int				oldSize;
	int				msgSequence;
	msg_t			oldMsg;
	byte			oldData[ MAX_MSGLEN ];
	int				oldTime, nextTime, fullTime;
	int				clientNum;
	demoFrame_t		*workFrame;
	int				parseEntitiesNum = 0;
	demoConvert_t	*convert;
	char			bigConfigString[BIG_INFO_STRING];
	int				bigConfigNumber;
	const char		*s;
	clSnapshot_t	*oldSnap = 0;
	clSnapshot_t	*newSnap;
	int				levelCount = 0;
	char			newName[MAX_OSPATH];
	short			rmgDataSize = 0;
	short			flatDataSize = 0;
	short			automapCount = 0;
	int				i = 0;

	oldSize = FS_FOpenFileRead( oldName, &oldHandle, qtrue );
	if (!oldHandle) {
		Com_Printf("Failed to open %s for conversion.\n", oldName);
		return;
	}
	demoFirstPack = qtrue;
	/* Alloc some memory */
	convert = (demoConvert_t *)Z_Malloc( sizeof( demoConvert_t), TAG_GENERAL ); //what tag do we need?
	memset( convert, 0, sizeof(demoConvert_t));
	/* Initialize the first workframe's strings */
	while (oldSize > 0) {
		MSG_Init( &oldMsg, oldData, sizeof( oldData ) );
		/* Read the sequence number */
		if (FS_Read( &convert->messageNum, 4, oldHandle) != 4)
			goto conversionerror;
		convert->messageNum = LittleLong( convert->messageNum );
		oldSize -= 4;
		/* Read the message size */
		if (FS_Read( &oldMsg.cursize,4, oldHandle) != 4)
			goto conversionerror;
		oldSize -= 4;
		oldMsg.cursize = LittleLong( oldMsg.cursize );
		/* Negative size signals end of demo */
		if (oldMsg.cursize < 0)
			break;
		if ( oldMsg.cursize > oldMsg.maxsize ) 
			goto conversionerror;
		/* Read the actual message */
		if (FS_Read( oldMsg.data, oldMsg.cursize, oldHandle ) != oldMsg.cursize)
			goto conversionerror;
		oldSize -= oldMsg.cursize;
		// init the bitstream
		MSG_BeginReading( &oldMsg );
		// Skip the reliable sequence acknowledge number
		MSG_ReadLong( &oldMsg );
		//
		// parse the message
		//
		while ( 1 ) {
			byte cmd;
			if ( oldMsg.readcount > oldMsg.cursize ) {
				Com_Printf("Demo conversion, read past end of server message.\n");
				goto conversionerror;
			}
            cmd = MSG_ReadByte( &oldMsg );
			if ( cmd == svc_EOF) {
                break;
			}
			workFrame = &convert->frames[ convert->frameIndex % DEMOCONVERTFRAMES ];
			// other commands
			switch ( cmd ) {
			default:
				Com_Printf(S_COLOR_RED"ERROR: CL_ParseServerMessage: Illegible server message\n");
				goto conversionerror;
			case svc_nop:
				break;
			case svc_serverCommand:
				temp = MSG_ReadLong( &oldMsg );
				s = MSG_ReadString( &oldMsg );
				if (temp<=msgSequence)
					break;
//				Com_Printf( " server command %s\n", s );
				msgSequence = temp;
				Cmd_TokenizeString( s );
	
				if ( !Q_stricmp( Cmd_Argv(0), "bcs0" ) ) {
					bigConfigNumber = atoi( Cmd_Argv(1) );
					Q_strncpyz( bigConfigString, Cmd_Argv(2), sizeof( bigConfigString ));
					break;
				}
				if ( !Q_stricmp( Cmd_Argv(0), "bcs1" ) ) {
					Q_strcat( bigConfigString, sizeof( bigConfigString ), Cmd_Argv(2));
					break;
				}
				if ( !Q_stricmp( Cmd_Argv(0), "bcs2" ) ) {
					Q_strcat( bigConfigString, sizeof( bigConfigString ), Cmd_Argv(2));
					demoFrameAddString( &workFrame->string, bigConfigNumber, bigConfigString );
					break;
				}
				if ( !Q_stricmp( Cmd_Argv(0), "cs" ) ) {
					int num = atoi( Cmd_Argv(1) );
					s = Cmd_ArgsFrom( 2 );
					demoFrameAddString( &workFrame->string, num, Cmd_ArgsFrom( 2 ) );	
					break;
				}
				if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
					int len = strlen( s ) + 1;
					char *dst;
					if (workFrame->commandUsed + len + 1 > sizeof( workFrame->commandData)) {
						Com_Printf("Overflowed state command data.\n");
						goto conversionerror;
					}
					dst = workFrame->commandData + workFrame->commandUsed;
					*dst = clientNum;
					Com_Memcpy( dst+1, s, len );
					workFrame->commandUsed += len + 1;
				}
				break;
			case svc_gamestate:
				if (newHandle) {
					FS_FCloseFile( newHandle );
					newHandle = 0;
				}
				if (levelCount) {
					demoFirstPack = qtrue;
					Com_sprintf( newName, sizeof( newName ), "%s.%d.mme", newBaseName, levelCount );
				} else {
					Com_sprintf( newName, sizeof( newName ), "%s.mme", newBaseName );
				}
				fullTime = -1;
				clientNum = -1;
				oldTime = -1;
				Com_Memset( convert, 0, sizeof( *convert ));
				convert->frames[0].string.used = 1;
				levelCount++;
				newHandle = FS_FOpenFileWrite( newName );
				if (!newHandle) {
					Com_Printf("Failed to open %s for target conversion target.\n", newName);
					goto conversionerror;
				} else {
					FS_Write ( demoHeader, strlen( demoHeader ), newHandle );
				}
				Com_sprintf( newName, sizeof( newName ), "%s.txt", newBaseName );
				workFrame = &convert->frames[ convert->frameIndex % DEMOCONVERTFRAMES ];
				msgSequence = MSG_ReadLong( &oldMsg );
				while( 1 ) {
					cmd = MSG_ReadByte( &oldMsg );
					if (cmd == svc_EOF)
						break;
					if ( cmd == svc_configstring) {
						int		num;
						const char *s;
						num = MSG_ReadShort( &oldMsg );
						s = MSG_ReadBigString( &oldMsg );
						demoFrameAddString( &workFrame->string, num, s );
					} else if ( cmd == svc_baseline ) {
						int num = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
						if ( num < 0 || num >= MAX_GENTITIES ) {
							Com_Printf( "Baseline number out of range: %i.\n", num );
							goto conversionerror;
						}
						MSG_ReadDeltaEntity( &oldMsg, &demoNullEntityState, &convert->entityBaselines[num], num );
						memcpy(&workFrame->entityBaselines[num], &convert->entityBaselines[num], sizeof(entityState_t));
					} else {
						Com_Printf( "Unknown block while converting demo gamestate.\n" );
						goto conversionerror;
					}
				}
				clientNum = MSG_ReadLong( &oldMsg );
				/* Skip the checksum feed */
				MSG_ReadLong( &oldMsg );

				// RMG stuff
				rmgDataSize = MSG_ReadShort(&oldMsg);
				if( rmgDataSize != 0) {
					unsigned char dataBuffer[MAX_HEIGHTMAP_SIZE];
					// height map
					MSG_ReadBits(&oldMsg, 1);
					MSG_ReadData(&oldMsg, dataBuffer, rmgDataSize);

					// Flatten map
					flatDataSize = MSG_ReadShort(&oldMsg);
					MSG_ReadBits(&oldMsg, 1);
					MSG_ReadData(&oldMsg, dataBuffer, flatDataSize);

					// Seed
					MSG_ReadLong(&oldMsg);

					// Automap symbols
					automapCount = MSG_ReadShort(&oldMsg);
					for(i = 0; i < automapCount; i++) {
						MSG_ReadByte(&oldMsg);
						MSG_ReadByte(&oldMsg);
						MSG_ReadLong(&oldMsg);
						MSG_ReadLong(&oldMsg);
					}
				}
				break;
			case svc_snapshot:
				nextTime = MSG_ReadLong( &oldMsg );
				/* Delta number, not needed */
				newSnap = &convert->snapshots[convert->messageNum & PACKET_MASK];
				Com_Memset (newSnap, 0, sizeof(*newSnap));
				newSnap->deltaNum = MSG_ReadByte( &oldMsg );
				newSnap->messageNum = convert->messageNum;
				if (!newSnap->deltaNum) {
					newSnap->deltaNum = -1;
					newSnap->valid = qtrue;		// uncompressed frame
					oldSnap  = NULL;
				} else {
					newSnap->deltaNum = newSnap->messageNum - newSnap->deltaNum;
					oldSnap = &convert->snapshots[newSnap->deltaNum & PACKET_MASK];
					if (!oldSnap->valid) {
						Com_Printf( "Delta snapshot without base.\n" );
//						goto conversionerror;
					} else if (oldSnap->messageNum != newSnap->deltaNum) {
						// The frame that the server did the delta from
						// is too old, so we can't reconstruct it properly.
						Com_Printf ("Delta frame too old.\n");
					} else if ( parseEntitiesNum - oldSnap->parseEntitiesNum > MAX_PARSE_ENTITIES-128 ) {
						Com_Printf ("Delta parseEntitiesNum too old.\n");
					} else {
						newSnap->valid = qtrue;	// valid delta parse
					}
				}

				/* Snapflags, not needed */
				newSnap->snapFlags = MSG_ReadByte( &oldMsg );
				// read areamask
				workFrame->areaUsed = MSG_ReadByte( &oldMsg );
				MSG_ReadData( &oldMsg, workFrame->areamask, workFrame->areaUsed );
				if (clientNum <0 || clientNum >= MAX_CLIENTS) {
					Com_Printf("Got snapshot with invalid client.\n");
					goto conversionerror;
				}
				MSG_ReadDeltaPlayerstate( &oldMsg, oldSnap ? &oldSnap->ps : &demoNullPlayerState, &newSnap->ps );
				if( newSnap->ps.m_iVehicleNum)
					MSG_ReadDeltaPlayerstate( &oldMsg, oldSnap ? &oldSnap->vps : &demoNullPlayerState, &newSnap->vps, qtrue );
				/* Read the individual entities */
				newSnap->parseEntitiesNum = parseEntitiesNum;
				newSnap->numEntities = 0;
				Com_Memset( workFrame->entityData, 0, sizeof( workFrame->entityData ));

				/* The beast that is entity parsing */
				{
				int			newnum;
				entityState_t	*oldstate, *newstate;
				int			oldindex = 0;
				int			oldnum;
				newnum = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
				while ( 1 ) {
					// read the entity index number
					if (oldSnap && oldindex < oldSnap->numEntities) {
						oldstate = &convert->parseEntities[(oldSnap->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
						oldnum = oldstate->number;
					} else {
						oldstate = 0;
						oldnum = 99999;
					}
					newstate = &convert->parseEntities[parseEntitiesNum];
					if ( !oldstate && (newnum == (MAX_GENTITIES-1))) {
						break;
					} else if ( oldnum < newnum ) {
						*newstate = *oldstate;
						oldindex++;
					} else if (oldnum == newnum) {
						oldindex++;
						MSG_ReadDeltaEntity( &oldMsg, oldstate, newstate, newnum );
						if ( newstate->number != MAX_GENTITIES-1)
							workFrame->entityData[ newstate->number ] = 1;
						newnum = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
					} else if (oldnum > newnum) {
						MSG_ReadDeltaEntity( &oldMsg, &convert->entityBaselines[newnum], newstate , newnum );
						memcpy(&workFrame->entityBaselines[newnum], &convert->entityBaselines[newnum], sizeof(entityState_t));
						if ( newstate->number != MAX_GENTITIES-1)
							workFrame->entityData[ newstate->number ] = 1;
						newnum = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
					}
					if (newstate->number == MAX_GENTITIES-1)
						continue;
					parseEntitiesNum++;
					parseEntitiesNum &= (MAX_PARSE_ENTITIES-1);
					newSnap->numEntities++;
				}}
				/* Stop processing this further since it's an invalid snap due to lack of delta data */
				if (!newSnap->valid)
					break;

				/* Skipped snapshots will be set invalid in the circular buffer */
				if ( newSnap->messageNum - convert->lastMessageNum >= PACKET_BACKUP ) {
					convert->lastMessageNum = newSnap->messageNum - ( PACKET_BACKUP - 1 );
				}
				for ( ; convert->lastMessageNum < newSnap->messageNum ; convert->lastMessageNum++ ) {
					convert->snapshots[convert->lastMessageNum & PACKET_MASK].valid = qfalse;
				}
				convert->lastMessageNum = newSnap->messageNum + 1;

				/* compress the frame into the new format */
				if (nextTime > oldTime) {
					demoFrame_t *cleanFrame;
					int writeIndex;
					for (temp = 0;temp<newSnap->numEntities;temp++) {
						int p = (newSnap->parseEntitiesNum+temp) & (MAX_PARSE_ENTITIES-1);
						entityState_t *newState = &convert->parseEntities[p];
						workFrame->entities[newState->number] = *newState;
					}
					workFrame->clientData[clientNum] = 1;
					workFrame->clients[clientNum] = newSnap->ps;
					workFrame->vehs[clientNum] = newSnap->vps;
					workFrame->serverTime = nextTime;

					/* Which frame from the cache to save */
					writeIndex = convert->frameIndex - (DEMOCONVERTFRAMES/2);
					if (writeIndex >= 0) {
						const demoFrame_t *newFrame;
						msg_t writeMsg;
						// init the message
						MSG_Init( &writeMsg, demoBuffer, sizeof (demoBuffer));
						MSG_Clear( &writeMsg );
						MSG_Bitstream( &writeMsg );
						newFrame = &convert->frames[ writeIndex  % DEMOCONVERTFRAMES];
						if ( smoothen )
							demoFrameInterpolate( convert->frames, DEMOCONVERTFRAMES, writeIndex );
						if ( nextTime > fullTime || writeIndex <= 0 ) {
							/* Plan the next time for a full write */
							fullTime = nextTime + 2000;
							demoFramePack( &writeMsg, newFrame, 0 );
						} else {
							const demoFrame_t *oldFrame = &convert->frames[ ( writeIndex -1 ) % DEMOCONVERTFRAMES];
							demoFramePack( &writeMsg, newFrame, oldFrame );
						}
						/* Write away the new data in the msg queue */
						temp = LittleLong( writeMsg.cursize );
						FS_Write (&temp, 4, newHandle );
						FS_Write ( writeMsg.data , writeMsg.cursize, newHandle );
					}

					/* Clean up the upcoming frame for all new changes */
					convert->frameIndex++;
					cleanFrame = &convert->frames[ convert->frameIndex % DEMOCONVERTFRAMES];
					cleanFrame->serverTime = 0;
					for (temp = 0;temp<MAX_GENTITIES;temp++)
						cleanFrame->entities[temp].number = MAX_GENTITIES-1;
					Com_Memset( cleanFrame->clientData, 0, sizeof ( cleanFrame->clientData ));
					Com_Memcpy( cleanFrame->string.data, workFrame->string.data, workFrame->string.used );
					Com_Memcpy( cleanFrame->string.offsets, workFrame->string.offsets, sizeof( workFrame->string.offsets ));
					cleanFrame->string.used = workFrame->string.used;
					cleanFrame->commandUsed = 0;
					/* keep track of this last frame's time */
					oldTime = nextTime;
				}
				break;
			case svc_download:
				// read block number
				temp = MSG_ReadShort ( &oldMsg );
				if (!temp)	//0 block, read file size
					MSG_ReadLong( &oldMsg );
				// read block size
				temp = MSG_ReadShort ( &oldMsg );
				// read the data block
				for ( ;temp>0;temp--)
					MSG_ReadByte( &oldMsg );
				break;
			case svc_setgame:
				char newGameDir[MAX_QPATH];
				char next;
				i = 0;
				while (i < MAX_QPATH)
				{
					next = MSG_ReadByte(&oldMsg);
					if(next)
					{
						newGameDir[i] = next;
					}
					else
					{
						break;
					}
					i++;
				}
				newGameDir[i] = 0;

				// But here we stop, and don't do more. If this goes horribly wrong sometime, you might have to go and actually do something with this.
				break;
			case svc_mapchange:
				// nothing to parse.
				break;
			}
		}
	}
conversionerror:
	FS_FCloseFile( oldHandle );
	FS_FCloseFile( newHandle );
	Z_Free( convert );
	return;
}

void demoCommandSmoothingEnable(qboolean enable) {
	demoCommandSmoothing = enable;
}

qboolean demoCommandSmoothingState(void) {
	return demoCommandSmoothing;
}

static void demoPlayAddCommand( demoPlay_t *play, const char *cmd ) {
	int len = strlen ( cmd ) + 1;
	int index = (++play->commandCount) % DEMO_PLAY_CMDS;
	int freeWrite;
	/* First clear previous command */
	int nextStart = play->commandStart[ index ];
	if (play->commandFree > nextStart) {
		int freeNext = sizeof( play->commandData ) - play->commandFree;
		if ( len <= freeNext ) {
			freeWrite = play->commandFree;
			play->commandFree += len;
		} else {
			if ( len > nextStart )
					Com_Error( ERR_DROP, "Ran out of server command space");
			freeWrite = 0;
			play->commandFree = len;
		}
	} else {
		int left = nextStart - play->commandFree;
		if ( len > left )
			Com_Error( ERR_DROP, "Ran out of server command space");
		freeWrite = play->commandFree;
		play->commandFree = (play->commandFree + len) % sizeof( play->commandData );
	}
	play->commandStart[ index ] = freeWrite;
	Com_Memcpy( play->commandData + freeWrite, cmd, len );
}

static void demoPlaySynch( demoPlay_t *play, demoFrame_t *frame) {
	int i;
	int startCount = play->commandCount;
	int totalLen = 0;
	for (i = 0;i<MAX_CONFIGSTRINGS;i++) {
		char *oldString = cl.gameState.stringData + cl.gameState.stringOffsets[i];
		char *newString = frame->string.data + frame->string.offsets[i];
		if (!strcmp( oldString, newString ))
			continue;
		totalLen += strlen( newString );
		demoPlayAddCommand( play, va("cs %d \"%s\"", i, newString) );
	}
	/* Copy the new server commands */
	for (i=0;i<frame->commandUsed;) {
		char *line = frame->commandData + i;
		int len, cmdClient;

		cmdClient = *line++;
		len = strlen( line ) + 1;
		i += len + 1;
		if ( cmdClient != play->clientNum ) 
			continue;
		demoPlayAddCommand( play, line );
		totalLen += strlen( line );
	}
	if (play->commandCount - startCount > DEMO_PLAY_CMDS) {
		Com_Error( ERR_DROP, "frame added more than %d commands", DEMO_PLAY_CMDS);
	}
//	Com_Printf("Added %d commands, length %d\n", play->commandCount - startCount, totalLen );
}

static void demoPlayForwardFrame( demoPlay_t *play ) {
	int			blockSize;
	msg_t		msg;

	if (play->filePos + 4 > play->fileSize) {
		if (mme_demoAutoNext->integer && demoNextNum && !demoPrecaching) {
			CL_Disconnect_f();
		}
		if (mme_demoAutoQuit->integer && !demoPrecaching) {
			if (mme_demoAutoQuit->integer == 2)
				Cbuf_ExecuteText( EXEC_APPEND, "quit" );
			CL_Disconnect_f();
		}
		play->lastFrame = qtrue;
		return;
	}
	play->lastFrame = qfalse;
	play->filePos += 4;
	FS_Read( &blockSize, 4, play->fileHandle );
	blockSize = LittleLong( blockSize );
	FS_Read( demoBuffer, blockSize, play->fileHandle );
	play->filePos += blockSize;
	MSG_Init( &msg, demoBuffer, sizeof(demoBuffer) );
	MSG_BeginReading( &msg );
	msg.cursize = blockSize;
	if (demoCommandSmoothing) {
		play->frame = &play->storageFrame[(play->frameNumber + 1 + FRAME_BUF_SIZE) % FRAME_BUF_SIZE];
		play->nextFrame = &play->storageFrame[(play->frameNumber + 2 + FRAME_BUF_SIZE) % FRAME_BUF_SIZE];
	} else {
		demoFrame_t *copyFrame = play->frame;
		play->frame = play->nextFrame;
		play->nextFrame = copyFrame;
	}
	demoFrameUnpack( &msg, play->frame, play->nextFrame );
	play->frameNumber++;
}

static void demoPlaySetIndex( demoPlay_t *play, int index ) {
	int wantPos;
	if (index <0 || index >= play->fileIndexCount) {
		Com_Printf("demoFile index Out of range search\n");
		index = 0;
	}
	wantPos = play->fileIndex[index].pos;
#if 0
	if ( !play->fileHandle || play->filePos > wantPos) {
		trap_FS_FCloseFile( play->fileHandle );
		trap_FS_FOpenFile( play->fileName, &play->fileHandle, FS_READ );
		play->filePos = 0;
	}
	if (!play->fileHandle)
		Com_Error(0, "Failed to open demo file \n");
	while (play->filePos < wantPos) {
		int toRead = wantPos - play->filePos;
		if (toRead > sizeof(demoBuffer))
			toRead = sizeof(demoBuffer);
		play->filePos += toRead;
		trap_FS_Read( demoBuffer, toRead, play->fileHandle );
	}
#else
	FS_Seek( play->fileHandle, wantPos, FS_SEEK_SET );
	play->filePos = wantPos;
#endif

	if (demoCommandSmoothing)
		play->frameNumber = play->fileIndex[index].frame - 2;

	demoPlayForwardFrame( play );
	demoPlayForwardFrame( play );

	if (!demoCommandSmoothing)
		play->frameNumber = play->fileIndex[index].frame;
}
static int demoSeekTime = 0;
static int demoPlaySeek(demoPlay_t *play, int seekTime) {
	int i;
	demoSeekTime = seekTime;
	seekTime += play->startTime;
	if (seekTime < 0)
		seekTime = 0;
	if (seekTime < play->frame->serverTime || (seekTime > play->frame->serverTime + 1000)) {
		for (i=0;i<(play->fileIndexCount-1);i++) {
			if (play->fileIndex[i].time <= seekTime && 
				play->fileIndex[i+1].time > seekTime)
				goto foundit;
		}
		i = play->fileIndexCount-1;
foundit:
		demoPlaySetIndex(play, i);
	}
	while (!play->lastFrame && (demoCommandSmoothing ? (seekTime >= play->frame->serverTime) : (seekTime > play->nextFrame->serverTime))) {
		demoPlayForwardFrame(play);
	}
	return seekTime;
}
int demoLength(void) {
	demoPlay_t *play = demo.play.handle;
	if (!play)
		return 0;
	return play->endTime - play->startTime;
}
int demoTime(void) {
	demoPlay_t *play = demo.play.handle;
	if (!play)
		return 0;
	if (demoSeekTime < 0)
		demoSeekTime = 0;
	return demoSeekTime;
}
static int demoFindNext(const char *fileName) {
	int i;
	const int len = strlen(fileName);
	char name[MAX_OSPATH], seekName[MAX_OSPATH];
	fileHandle_t demoHandle = 0;
	qboolean tryAgain = qtrue;
	if (isdigit(fileName[len-1]) && ((fileName[len-2] == '.'))) {
		Com_sprintf(seekName, len-1, fileName);
		demoCurrentNum = fileName[len-1] - '0';
	} else if (isdigit(fileName[len-1]) && (isdigit(fileName[len-2]) && (fileName[len-3] == '.'))) {
		Com_sprintf(seekName, len-2, fileName);
		demoCurrentNum = (fileName[len-2] - '0')*10 + (fileName[len-1] - '0');
	} else {
		Com_sprintf(seekName, MAX_OSPATH, fileName);
		demoCurrentNum = demoNextNum;
	}
tryAgain:
	for (i = demoCurrentNum + 1; i < 99; i++) {
		Com_sprintf(name, MAX_OSPATH, "mmedemos/%s.%d.mme", seekName, i);
		FS_FOpenFileRead(name, &demoHandle, qtrue);
		if (FS_FileExists(name)) {
			Com_Printf("Next demo file: %s\n", name);
			return i;
		}
	}
	Com_sprintf(seekName, len, "%s", fileName);
	if (tryAgain) {
		tryAgain = qfalse;
		goto tryAgain;
	}
	return 0;
}

static demoPlay_t *demoPlayOpen( const char* fileName ) {
	demoPlay_t	*play;
	fileHandle_t fileHandle;
	int	fileSize, filePos;
	int i;

	msg_t msg;
	fileSize = FS_FOpenFileRead( fileName, &fileHandle, qtrue );
	if (fileHandle<=0) {
		Com_Printf("Failed to open demo file %s \n", fileName );
		return 0;
	}
	filePos = strlen( demoHeader );
	i = FS_Read( &demoBuffer, filePos, fileHandle );
	if ( i != filePos || Q_strncmp( (char *)demoBuffer, demoHeader, filePos )) {
		Com_Printf("demo file %s is wrong version\n", fileName );
		FS_FCloseFile( fileHandle );
		return 0;
	}
	play = (demoPlay_t *)Z_Malloc( sizeof( demoPlay_t ), TAG_GENERAL);//what tag do we need?
	memset( play, 0, sizeof(demoPlay_t) ); // In Q3MME the Z_Malloc doees a memset 0, it doesn't here though. So we gotta do it.
	Q_strncpyz( play->fileName, fileName, sizeof( play->fileName ));
	play->fileSize = fileSize;
	play->frame = &play->storageFrame[0];
	play->nextFrame = &play->storageFrame[1];
	for (i=0;i<DEMO_PLAY_CMDS;i++)
		play->commandStart[i] = i;
	play->commandFree = DEMO_PLAY_CMDS;

	for ( ; filePos<fileSize; ) {
		int blockSize, isFull, serverTime;
		FS_Read( &blockSize, 4, fileHandle );
		blockSize = LittleLong( blockSize );
		if (blockSize > sizeof(demoBuffer)) {
			Com_Printf( "Block too large to be read in.\n");
			goto errorreturn;
		}
		if ( blockSize + filePos > fileSize) {
			Com_Printf( "Block would read past the end of the file.\n");
			goto errorreturn;
		}
		FS_Read( demoBuffer, blockSize, fileHandle );
		MSG_Init( &msg, demoBuffer, sizeof(demoBuffer) );
		MSG_BeginReading( &msg );
		msg.cursize = blockSize;	
		isFull = MSG_ReadBits( &msg, 1 );
		serverTime = MSG_ReadLong( &msg );
		if (!play->startTime)
			play->startTime = serverTime;
		if (isFull) {
			if (play->fileIndexCount < DEMO_MAX_INDEX) {
				play->fileIndex[play->fileIndexCount].pos = filePos;
				play->fileIndex[play->fileIndexCount].frame = play->totalFrames;
				play->fileIndex[play->fileIndexCount].time = serverTime;
				play->fileIndexCount++;
			}
		}
		play->endTime = serverTime;
		filePos += 4 + blockSize;
		play->totalFrames++;
	}
	play->fileHandle = fileHandle;
	demoFirstPack = qtrue;
	demoPlaySetIndex( play, 0 );
	play->clientNum = -1;
	for( i=0;i<MAX_CLIENTS;i++)
		if (play->frame->clientData[i]) {
			play->clientNum = i;
			break;
		}
	demoNextNum = demoFindNext(mme_demoFileName->string);
	return play;
errorreturn:
	Z_Free( play );
	FS_FCloseFile( fileHandle );
	return 0;
}

static void demoPlayStop( demoPlay_t *play ) {
	FS_FCloseFile( play->fileHandle );
	if (mme_demoRemove->integer && FS_FileExists( play->fileName ))
		FS_FileErase( play->fileName );
	Z_Free( play );
}

qboolean demoGetDefaultState(int index, entityState_t *state) {
	demoPlay_t *play = demo.play.handle;
	if (index < 0 || index >= MAX_GENTITIES)
		return qfalse;
	if (!(play->frame->entityBaselines[index].eFlags & EF_PERMANENT))
		return qfalse;
	*state = play->frame->entityBaselines[index];
	return qtrue;
}

extern void CL_ConfigstringModified( void );
qboolean demoGetServerCommand( int cmdNumber ) {
	demoPlay_t *play = demo.play.handle;
	int index = cmdNumber % DEMO_PLAY_CMDS;
	const char *cmd;

	if (cmdNumber < play->commandCount - DEMO_PLAY_CMDS || cmdNumber > play->commandCount )
		return qfalse;
	
	Cmd_TokenizeString( play->commandData + play->commandStart[index] );

	cmd = Cmd_Argv( 0 );
	if ( !strcmp( cmd, "cs" ) ) {
		CL_ConfigstringModified();
	}
	return qtrue;
}

int demoSeek( int seekTime ) {
	return demoPlaySeek( demo.play.handle, seekTime );
}

void demoRenderFrame( stereoFrame_t stereo ) {
	VM_Call( cgvm, CG_DRAW_ACTIVE_FRAME, cls.realtime, stereo, 2 );	
}

void demoGetSnapshotNumber( int *snapNumber, int *serverTime ) {
	demoPlay_t *play = demo.play.handle;

	*snapNumber = play->frameNumber + (play->lastFrame ? 0 : 1);
	*serverTime = play->lastFrame ?  play->frame->serverTime : play->nextFrame->serverTime;
}

qboolean demoGetSnapshot( int snapNumber, snapshot_t *snap ) {
	demoPlay_t *play = demo.play.handle;
	demoFrame_t *frame;
	int i;

	if (demoCommandSmoothing && snapNumber < play->frameNumber - FRAME_BUF_SIZE + 2)
		return qfalse;
	if (!demoCommandSmoothing && snapNumber < play->frameNumber)
		return qfalse;
	if (snapNumber > play->frameNumber + 1)
		return qfalse;
	if (snapNumber == play->frameNumber + 1 && play->lastFrame)
		return qfalse;

	if (demoCommandSmoothing)
		frame = &play->storageFrame[snapNumber % FRAME_BUF_SIZE];
	else
		frame = snapNumber == play->frameNumber ? play->frame : play->nextFrame;

	demoPlaySynch( play, frame );
	snap->serverCommandSequence = play->commandCount;
	snap->serverTime = frame->serverTime;
	if (play->clientNum >=0 && play->clientNum < MAX_CLIENTS) {
		snap->ps = frame->clients[ play->clientNum ];
		snap->vps = frame->vehs[ play->clientNum ];
	} else {
		Com_Memset( &snap->ps, 0, sizeof(snap->ps) );
		Com_Memset( &snap->vps, 0, sizeof(snap->vps) );
	}

	snap->numEntities = 0;
	for (i=0;i<MAX_GENTITIES-1 && snap->numEntities < MAX_ENTITIES_IN_SNAPSHOT;i++) {
		if (frame->entities[i].number != i)
			continue;
		/* Skip your own entity if there ever comes server side recording */
		if (frame->entities[i].number == snap->ps.clientNum)
			continue;
		memcpy(&snap->entities[snap->numEntities++], &frame->entities[i], sizeof(entityState_t));
	}
	snap->snapFlags = 0;
	snap->ping = 0;
	snap->numServerCommands = 0;
	Com_Memcpy( snap->areamask, frame->areamask, frame->areaUsed );
	return qtrue;
}

void CL_DemoSetCGameTime( void ) {
	/* We never timeout */
	clc.lastPacketTime = cls.realtime;
}

void CL_DemoShutDown( void ) {
	if ( demo.play.handle ) {
		FS_FCloseFile( demo.play.handle->fileHandle );
		Z_Free( demo.play.handle );
		demo.play.handle = 0;
	}
}

void demoStop( void ) {
	if (demo.play.handle) {
		demoPlayStop( demo.play.handle );
	}
	Com_Memset( &demo.play, 0, sizeof( demo.play ));
}

static void demoPrecacheModel(char *str) {
	char skinName[MAX_QPATH], modelName[MAX_QPATH];
	char *p;
	qhandle_t skinID = 0;
	int i = 0;
	
	strcpy(modelName, str);
	if (strstr(modelName, ".glm")) {
		//Check to see if it has a custom skin attached.
		//see if it has a skin name
		p = Q_strrchr(modelName, '*');
		if (p) { //found a *, we should have a model name before it and a skin name after it.
			*p = 0; //terminate the modelName string at this point, then go ahead and parse to the next 0 for the skin.
			p++;
			while (p && *p) {
				skinName[i] = *p;
				i++;
				p++;
			}
			skinName[i] = 0;

			if (skinName[0]) { //got it, register the skin under the model path.
				char baseFolder[MAX_QPATH];

				strcpy(baseFolder, modelName);
				p = Q_strrchr(baseFolder, '/'); //go back to the first /, should be the path point

				if (p) { //got it.. terminate at the slash and register.
					char *useSkinName;
					*p = 0;
					if (strchr(skinName, '|')) {//three part skin
						useSkinName = va("%s/|%s", baseFolder, skinName);
					} else {
						useSkinName = va("%s/model_%s.skin", baseFolder, skinName);
					}
					skinID = re.RegisterSkin(useSkinName);
				}
			}
		}
	}
	re.RegisterModel( str );
}

static void demoPrecacheClient(char *str) {
	const char *v;
	int     team;
	char	modelName[MAX_QPATH];
	char	skinName[MAX_QPATH];
	char		*slash;
	qhandle_t torsoSkin;
	void	  *ghoul2;
	memset(&ghoul2, 0, sizeof(ghoul2));

	v = Info_ValueForKey(str, "t");
	team = atoi(v);
	v = Info_ValueForKey(str, "model");
	Q_strncpyz( modelName, v, sizeof( modelName ) );

	slash = strchr( modelName, '/' );
	if ( !slash ) {
		// modelName did not include a skin name
		Q_strncpyz( skinName, "default", sizeof( skinName ) );
	} else {
		Q_strncpyz( skinName, slash + 1, sizeof( skinName ) );
		// truncate modelName
		*slash = 0;
	}

	// fix for transparent custom skin parts
	if (strchr(skinName, '|')
		&& strstr(skinName,"head")
		&& strstr(skinName,"torso")
		&& strstr(skinName,"lower"))
	{//three part skin
		torsoSkin = re.RegisterSkin(va("models/players/%s/|%s", modelName, skinName));
	} else {
		if (team == TEAM_RED) {
			Q_strncpyz(skinName, "red", sizeof(skinName));
		} else if (team == TEAM_BLUE) {
			Q_strncpyz(skinName, "blue", sizeof(skinName));
		}
		torsoSkin = re.RegisterSkin(va("models/players/%s/model_%s.skin", modelName, skinName));
	}
	re.G2API_InitGhoul2Model((CGhoul2Info_v **)&ghoul2, va("models/players/%s/model.glm", modelName), 0, torsoSkin, 0, 0, 0);
}

static void demoPrecache( void ) {
	demoPlay_t *play = demo.play.handle;
	int latestSequence = 0, time = play->startTime;
	demoPlaySetIndex(play, 0);
	while (!play->lastFrame) {
		demoPlaySynch( play, play->frame );
		while (latestSequence < play->commandCount) {
			if (demoGetServerCommand(++latestSequence)) {
				if ( !Q_stricmp( Cmd_Argv(0), "cs" ) ) {
					int num = atoi( Cmd_Argv(1) );
					char *str = cl.gameState.stringData + cl.gameState.stringOffsets[num];
					if ( num >= CS_MODELS && num < CS_MODELS+MAX_MODELS ) {
						demoPrecacheModel(str);
					} else if ( num >= CS_SOUNDS && num < CS_SOUNDS+MAX_SOUNDS && (str[0] || str[1] == '$') ) {
						if ( str[0] != '*' ) S_StartSound(vec3_origin, 0, CHAN_AUTO, -1, S_RegisterSound( str ));
					} else if ( num >= CS_PLAYERS && num < CS_PLAYERS+MAX_CLIENTS ) {
						demoPrecacheClient(str);
					}	
				}
			}
		}
		time += 50;
		while (!play->lastFrame && time > play->nextFrame->serverTime)
			demoPlayForwardFrame(play);
	}
	S_Update();
	S_StopAllSounds();
	demoPlaySetIndex(play, 0);
}

qboolean demoPlay( const char *fileName ) {
	demo.play.handle = demoPlayOpen( fileName );
	if (demo.play.handle) {
		demoPlay_t *play = demo.play.handle;
		clc.demoplaying = qtrue;
		clc.newDemoPlayer = qtrue;
		clc.serverMessageSequence = 0;
		clc.lastExecutedServerCommand = 0;
		Com_Printf("Opened %s, which has %d seconds and %d frames\n", fileName, (play->endTime - play->startTime) / 1000, play->totalFrames );
		Con_Close();
		
		// wipe local client state
		CL_ClearState();
		cls.state = CA_LOADING;
		// Pump the loop, this may change gamestate!
		Com_EventLoop();
		// starting to load a map so we get out of full screen ui mode
		Cvar_Set("r_uiFullScreen", "0");
		// flush client memory and start loading stuff
		// this will also (re)load the UI
		// if this is a local client then only the client part of the hunk
		// will be cleared, note that this is done after the hunk mark has been set
		CL_FlushMemory();
		// initialize the CGame
		cls.cgameStarted = qtrue;
		// Create the gamestate
		Com_Memcpy( cl.gameState.stringOffsets, play->frame->string.offsets, sizeof( play->frame->string.offsets ));
		Com_Memcpy( cl.gameState.stringData, play->frame->string.data, play->frame->string.used );
		cl.gameState.dataCount = play->frame->string.used;
		demoPrecaching = qfalse;
		if (mme_demoPrecache->integer) {
			demoPrecaching = qtrue;
			demoPrecache();
			demoPrecaching = qfalse;
		}
		CL_InitCGame();
		cls.state = CA_ACTIVE;
		return qtrue;
	} else {
		return qfalse;
	}
}

void CL_MMEDemo_f( void ) {
	const char *cmd = Cmd_Argv( 1 );

	if (Cmd_Argc() == 2) {
		char mmeName[MAX_OSPATH];
		Com_sprintf (mmeName, MAX_OSPATH, "mmedemos/%s.mme", cmd);
		if (FS_FileExists( mmeName )) {
			Cvar_Set( "mme_demoFileName", cmd );
			demoPlay( mmeName );
		} else {
			Com_Printf("%s not found.\n", mmeName );
		}
		return;
	}
	if (!Q_stricmp( cmd, "convert")) {
		demoConvert( Cmd_Argv( 2 ), Cmd_Argv( 3 ), (qboolean)mme_demoSmoothen->integer );
	} else if (!Q_stricmp( cmd, "play")) {
		demoPlay( Cmd_Argv( 2 ) );
	} else {
		Com_Printf("That does not compute...%s\n", cmd );
	}
}

void CL_DemoList_f(void) {
	int len, i;
	char *buf;
	char word[MAX_OSPATH];
	int	index;
	qboolean readName;
	qboolean haveQuote;

	demoListCount = 0;
	demoListIndex = 0;
	haveQuote = qfalse;

	if (Cmd_Argc() < 2) {
		Com_Printf( "Usage demoList filename.\n");
		Com_Printf( "That file should have lines with demoname projectname.\n" );
		Com_Printf( "These will be played after each other.\n" );
	}
	if (!FS_FileExists( Cmd_Argv(1))) {
		Com_Printf( "Listfile %s doesn't exist\n", Cmd_Argv(1));
		return;
	}
	len = FS_ReadFile( Cmd_Argv(1), (void **)&buf);
	if (!buf) {
		Com_Printf("file %s couldn't be opened\n", Cmd_Argv(1));
		return;
	}
	i = 0;
	index = 0;
	readName = qtrue;
	while( i < len) {
		switch (buf[i]) {
		case '\r':
			break;
		case '"':
			if (!haveQuote) {
				haveQuote = qtrue;
				break;
			}
		case '\n':
		case ' ':
		case '\t':
			if (haveQuote && buf[i] != '"') {
				if (index < (sizeof(word)-1)) {
	              word[index++] = buf[i];  
				}
				break;
			}
			if (!index)
				break;
			haveQuote = qfalse;
			word[index++] = 0;
			if (readName) {
				Com_Memcpy( demoList[demoListCount].demoName, word, index );
				readName = qfalse;
			} else {
				if (demoListCount < DEMOLISTSIZE) {
					Com_Memcpy( demoList[demoListCount].projectName, word, index );
					demoListCount++;
				}
				readName = qtrue;
			}
			index = 0;
			break;
		default:
			if (index < (sizeof(word)-1)) {
              word[index++] = buf[i];  
			}
			break;
		}
		i++;
	}
	/* Handle a final line if any */
	if (!readName && index && demoListCount < DEMOLISTSIZE) {
		word[index++] = 0;
		Com_Memcpy( demoList[demoListCount].projectName, word, index );
		demoListCount++;
	}

	FS_FreeFile ( buf );
	demoListIndex = 0;
}

void CL_DemoListNext_f(void) {
	if ( demoListIndex < demoListCount ) {
		const demoListEntry_t *entry = &demoList[demoListIndex++];
		Cvar_Set( "mme_demoStartProject", entry->projectName );
		Com_Printf( "Starting demo %s with project %s\n",
			entry->demoName, entry->projectName );
		Cmd_ExecuteString( va( "demo \"%s\"\n", entry->demoName ));
	} else if (demoListCount) {
		Com_Printf( "DemoList:Finished playing %d demos\n", demoListCount );
		demoListCount = 0;
		demoListIndex = 0;
		if ( mme_demoListQuit->integer )
			Cbuf_ExecuteText( EXEC_APPEND, "quit" );
	}
	if (mme_demoAutoNext->integer && demoNextNum) {
		char demoName[MAX_OSPATH];
		if (demoNextNum == 1) {
			Com_sprintf(demoName, MAX_OSPATH, "%s.1", mme_demoFileName->string);
		} else if (demoNextNum < 10) {
			Com_sprintf(demoName, strlen(mme_demoFileName->string)-1, mme_demoFileName->string);
			strcat(demoName, va(".%d", demoNextNum));
		} else if (demoNextNum < 100) {
			Com_sprintf(demoName, strlen(mme_demoFileName->string)-2, mme_demoFileName->string);
			strcat(demoName, va(".%d", demoNextNum));
		}
		Cmd_ExecuteString(va("mmeDemo \"%s\"\n", demoName));
	}
}

void CL_DemoCut_f(void) {
	double startTime, endTime;
	char demoName[MAX_OSPATH];
	if (Cmd_Argc() < 3) {
		Com_Printf("Usage: demoCut start end (in seconds)\n");
		return;
	}
	startTime = atof(Cmd_Argv(1));
	endTime = atof(Cmd_Argv(2));
	if (endTime <= startTime) {
		Com_Printf("invalid range: %.3f >= %.3f\n", startTime, endTime);
		return;
	}
	if (endTime - startTime < 0.05) {
		Com_Printf("invalid range: less than 50 milliseconds is not allowed\n");
		return;
	}
	/* convert to msec */
	startTime *= 1000;
	endTime *= 1000;
	Com_sprintf(demoName, MAX_OSPATH, mme_demoFileName->string);
	if (demoCut(demoName, startTime, endTime)) {
		Com_Printf(S_COLOR_GREEN"Demo %s got successfully cut\n", mme_demoFileName->string);
	} else {
		Com_Printf(S_COLOR_RED"Demo %s has failed to get cut or cut with errors\n", mme_demoFileName->string);
	}
}
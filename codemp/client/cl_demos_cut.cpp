#include "client.h"
#include "cl_demos.h"

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
	char			*ext;
	if (!play) {
		Com_Printf("Demo cutting is allowed in mme mode only.\n");
		return qfalse;
	}
	startTime += play->startTime;
	endTime += play->startTime;
	ext = Cvar_FindVar("mme_demoExt")->string;
	if (!*ext)
		ext = ".dm_26";
	oldSize = FS_FOpenFileRead(va("demos/%s%s", oldName, ext), &oldHandle, qtrue);
	if (!oldHandle) {
		Com_Printf("Failed to open %s for cutting.\n", oldName);
		return qfalse;
	}
	memset(&demo.cut.Clc, 0, sizeof(demo.cut.Clc));
	Com_SetLoadingMsg("Cutting the demo...");
	while (oldSize > 0) {
cutcontinue:
		MSG_Init(&oldMsg, oldData, sizeof(oldData));
		/* Read the sequence number */
		if (FS_Read(&demo.cut.Clc.serverMessageSequence, 4, oldHandle) != 4)
			goto cuterror;
		demo.cut.Clc.serverMessageSequence = LittleLong(demo.cut.Clc.serverMessageSequence);
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
			if (readGamestate < demo.currentNum) {
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
				demoCutParseCommandString(&oldMsg, &demo.cut.Clc);
				break;
			case svc_gamestate:
				if (readGamestate > demo.currentNum) {
					Com_Printf("Warning: unexpected new gamestate, finishing cutting.\n");
					goto cutcomplete;
				}
				if (!demoCutParseGamestate(&oldMsg, &demo.cut.Clc, &demo.cut.Cl)) {
					goto cuterror;
				}
				Com_sprintf(newName, sizeof(newName), "demos/%s_cut%s", oldName, ext);
				newHandle = FS_FOpenFileWrite(newName);
				if (!newHandle) {
					Com_Printf("Failed to open %s for target cutting.\n", newName);
					return qfalse;
				}
				readGamestate++;
				break;
			case svc_snapshot:
				if (!demoCutParseSnapshot(&oldMsg, &demo.cut.Clc, &demo.cut.Cl)) {
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
		int firstServerCommand = demo.cut.Clc.lastExecutedServerCommand;
		// process any new server commands
		for (;demo.cut.Clc.lastExecutedServerCommand < demo.cut.Clc.serverCommandSequence; demo.cut.Clc.lastExecutedServerCommand++) {
			char *command = demo.cut.Clc.serverCommands[demo.cut.Clc.lastExecutedServerCommand & (MAX_RELIABLE_COMMANDS - 1)];
			Cmd_TokenizeString(command);
			char *cmd = Cmd_Argv(0);
			if (cmd[0]) {
				firstServerCommand = demo.cut.Clc.lastExecutedServerCommand;
			}
			if (!strcmp(cmd, "cs")) {
				if (!demoCutConfigstringModified(&demo.cut.Cl)) {
					goto cuterror;
				}
			}
		}
		if (demo.cut.Cl.snap.serverTime > endTime) {
			goto cutcomplete;
		} else if (framesSaved > 0) {
			/* this msg is in range, write it */
			if (framesSaved > max(10, demo.cut.Cl.snap.messageNum - demo.cut.Cl.snap.deltaNum)) {
				demoCutWriteDemoMessage(&oldMsg, newHandle, &demo.cut.Clc);
			} else {
				demoCutWriteDeltaSnapshot(firstServerCommand, newHandle, qfalse, &demo.cut.Clc, &demo.cut.Cl);
			}
			framesSaved++;
		} else if (demo.cut.Cl.snap.serverTime >= startTime) {
			demoCutWriteDemoHeader(newHandle, &demo.cut.Clc, &demo.cut.Cl);
			demoCutWriteDeltaSnapshot(firstServerCommand, newHandle, qtrue, &demo.cut.Clc, &demo.cut.Cl);
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
		if (demo.currentNum > 0) {
			Com_sprintf(newName, sizeof(newName), "mmedemos/%s.%d_cut.mme", oldName, demo.currentNum);
		} else {
			Com_sprintf(newName, sizeof(newName), "mmedemos/%s_cut.mme", oldName);
		}
		if (FS_FileExists(newName))
			FS_FileErase(newName);
	}
	FS_FCloseFile(oldHandle);
	FS_FCloseFile(newHandle);
	return ret;
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
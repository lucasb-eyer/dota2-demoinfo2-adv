//====== Copyright (c) 2012, Valve Corporation, All rights reserved. ========//
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//===========================================================================//

#include <algorithm>
#include <stdarg.h>
#include <cstdio>
#include "demofile.h"
#include "demofiledumpdemson.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/descriptor.pb.h>

#include "generated_proto/usermessages.pb.h"
#include "generated_proto/ai_activity.pb.h"
#include "generated_proto/dota_modifiers.pb.h"
#include "generated_proto/dota_commonmessages.pb.h"
#include "generated_proto/dota_usermessages.pb.h"

#include <string>
#include <sstream>

// helper method to deal with unicode fÜn. Source: http://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c
std::string escapeJsonString(const std::string& input) {
    std::ostringstream ss;
    //for (auto iter = input.cbegin(); iter != input.cend(); iter++) {
    //C++98/03:
    for (std::string::const_iterator iter = input.begin(); iter != input.end(); iter++) {
        switch (*iter) {
            case '\\': ss << "\\\\"; break;
            case '"': ss << "\\\""; break;
            case '/': ss << "\\/"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default: ss << *iter; break;
        }
    }
    return ss.str();
}

void fatal_errorf( const char* fmt, ... )
{
    va_list  vlist;
    char buf[ 1024 ];

    va_start( vlist, fmt);
    vsnprintf( buf, sizeof( buf ), fmt, vlist );
	buf[ sizeof( buf ) - 1 ] = 0;
    va_end( vlist );

    fprintf( stderr, "\nERROR: %s\n", buf );
    exit( -1 );
}

bool CDemoFileDump::Open( const char *filename )
{
	if ( !m_demofile.Open( filename ) )
	{
		fprintf( stderr, "Couldn't open '%s'\n", filename );
		return false;
	}

	return true;
}

void CDemoFileDump::MsgPrintf( const ::google::protobuf::Message& msg, int size, const char *fmt, ... )
{
	va_list vlist;
	const std::string& TypeName = msg.GetTypeName();

#ifdef OUTPUT_ORIGINAL
	// Print the message type and size
	printf( "---- %s (%d bytes) -----------------\n", TypeName.c_str(), size );

	va_start( vlist, fmt);
	vprintf( fmt, vlist );
	va_end( vlist );
#endif
}

// Our demson format expects the output to be a json object per line.
// For this to hold true, we need to "fix" all newline characters in strings.
// When you want to make an apple pie... again ;)
std::string replace(const std::string& s, const std::string& from, const std::string& to, std::string::size_type start = 0)
{
	std::string result = s;

	while((start = result.find(from, start)) != std::string::npos) {
		result.replace(start, 1, "\\n");
		start += 2;
	}

	return result;
}

// How I miss currying!
std::string escape(const std::string& s, std::string::size_type start = 0)
{
	return replace(s, "\n", "\\n", start);
}

template<typename Msg>
void PrintMessageDemson( const Msg& msg, int tick,  bool endline = true );

// Prints out the given field of the msg in json-style, i.e. as a quoted string, integer,
// boolean, ...
// Only works for primitive types, but calls PrintMessageDemson for message types to recurse.
// If the field is a repeated field, this only prints the i-th entry.
template<typename Msg>
void PrintPrimitiveDemson( const Msg& msg, const google::protobuf::FieldDescriptor* field, int tick, int i = 0)
{
	using namespace google::protobuf;
	const Reflection *r = msg.GetReflection();

	// Gah this is just horrible, any idea on how to merge those two?
	if(field->is_repeated()) {
		switch(field->cpp_type()) {
		case FieldDescriptor::CPPTYPE_INT32:  printf("%d", r->GetRepeatedInt32(msg, field, i)); break;
		case FieldDescriptor::CPPTYPE_INT64:  printf("%lld", r->GetRepeatedInt64(msg, field, i)); break;
		case FieldDescriptor::CPPTYPE_UINT32: printf("%u", r->GetRepeatedUInt32(msg, field, i)); break;
		case FieldDescriptor::CPPTYPE_UINT64: printf("%llu", r->GetRepeatedUInt64(msg, field, i)); break;
		case FieldDescriptor::CPPTYPE_DOUBLE: printf("%g", r->GetRepeatedDouble(msg, field, i)); break;
		case FieldDescriptor::CPPTYPE_FLOAT:  printf("%f", r->GetRepeatedFloat(msg, field, i)); break;
		case FieldDescriptor::CPPTYPE_BOOL:   printf("%s", r->GetRepeatedBool(msg, field, i) ? "true" : "false"); break;
		case FieldDescriptor::CPPTYPE_STRING: printf("\"%s\"", escape(r->GetRepeatedString(msg, field, i)).c_str()); break;
		case FieldDescriptor::CPPTYPE_ENUM:   printf("\"%s\"", r->GetRepeatedEnum(msg, field, i)->name().c_str()); break;
		case FieldDescriptor::CPPTYPE_MESSAGE: PrintMessageDemson(r->GetRepeatedMessage(msg, field, i), tick, false); break;
		}
	} else {
		switch(field->cpp_type()) {
		case FieldDescriptor::CPPTYPE_INT32:  printf("%d", r->GetInt32(msg, field)); break;
		case FieldDescriptor::CPPTYPE_INT64:  printf("%lld", r->GetInt64(msg, field)); break;
		case FieldDescriptor::CPPTYPE_UINT32: printf("%u", r->GetUInt32(msg, field)); break;
		case FieldDescriptor::CPPTYPE_UINT64: printf("%llu", r->GetUInt64(msg, field)); break;
		case FieldDescriptor::CPPTYPE_DOUBLE: printf("%g", r->GetDouble(msg, field)); break;
		case FieldDescriptor::CPPTYPE_FLOAT:  printf("%f", r->GetFloat(msg, field)); break;
		case FieldDescriptor::CPPTYPE_BOOL:   printf("%s", r->GetBool(msg, field) ? "true" : "false"); break;
		case FieldDescriptor::CPPTYPE_STRING: printf("\"%s\"", escape(r->GetString(msg, field)).c_str()); break;
		case FieldDescriptor::CPPTYPE_ENUM:   printf("\"%s\"", r->GetEnum(msg, field)->name().c_str()); break;
		case FieldDescriptor::CPPTYPE_MESSAGE: PrintMessageDemson(r->GetMessage(msg, field), tick, false); break;
		}
	}
}

// Prints out a whole protobuf message in demson format using reflection
// to get all fields and their values.
template<typename Msg>
void PrintMessageDemson( const Msg& msg, int tick, bool endline )
{
	using namespace google::protobuf;
	// TODO: get UnknownFieldSet!
	printf("{\"demsontype\": \"%s\"", msg.GetTypeName().c_str());
	printf(", \"tick\": %d", tick); //tick all the things

	const Reflection *r = msg.GetReflection();
	std::vector<const FieldDescriptor*> fields;
	r->ListFields(msg, &fields);
	for(std::vector<const FieldDescriptor*>::iterator iField = fields.begin() ; iField != fields.end() ; iField++) {
		const FieldDescriptor* field = *iField;
		printf(", \"%s\": ", field->name().c_str());
		if(field->is_repeated()) {
			printf("[");
			for(int i = 0 ; i < r->FieldSize(msg, field) ; ++i) {
				PrintPrimitiveDemson(msg, field, tick, i);
				if(i + 1 != r->FieldSize(msg, field))
					printf(", ");
			}
			printf("]");
		} else {
			PrintPrimitiveDemson(msg, field, tick);
		}
	}
	printf("}");
	if(endline)
		printf("\n");
}

template < class T, int msgType >
void PrintUserMessage( CDemoFileDump& Demo, const void *parseBuffer, int BufferSize, int tick )
{
	T msg;

	if( msg.ParseFromArray( parseBuffer, BufferSize ) )
	{
		Demo.MsgPrintf( msg, BufferSize, "%s", msg.DebugString().c_str() );
#ifdef OUTPUT_AUTODEMSON_USER
		PrintMessageDemson( msg, tick );
#endif
	}
}

//TODO: see enum DOTA_CHAT_MESSAGE in dota_usermessages.proto:120 for what-the-hell-is-going-on
#ifdef OUTPUT_ChatEvent
template<>
void PrintUserMessage<CDOTAUserMsg_ChatEvent, DOTA_UM_ChatEvent>( CDemoFileDump& Demo, const void *parseBuffer, int BufferSize, int tick )
{
	CDOTAUserMsg_ChatEvent msg;

	if( !msg.ParseFromArray( parseBuffer, BufferSize ) )
		return;

    //TODO: game time / tick count?
	printf( "{\"demsontype\": \"chatevent\", "
        "\"type\": %d, "
        "\"value\": %d, "
        "\"playerid_1\": %d, "
        "\"playerid_2\": %d, "
        "\"playerid_3\": %d, "
        "\"playerid_4\": %d, "
        "\"playerid_5\": %d, "
        "\"playerid_6\": %d, "
        "\"tick\": %d "
        "}\n", msg.type(), msg.value(), msg.playerid_1(),
        msg.playerid_2(), msg.playerid_3(), msg.playerid_4(),
        msg.playerid_5(), msg.playerid_6(), tick);
}
#endif

#ifdef OUTPUT_UnitEvent
//TODO: proper output?
//the announcer sound bits (-80 seconds = 'pre_game' and -3 seconds = 'game_start').
//useful for synchronization (timestamp with replay game time).
template<>
void PrintUserMessage<CDOTAUserMsg_UnitEvent, DOTA_UM_UnitEvent>( CDemoFileDump& Demo, const void *parseBuffer, int BufferSize, int tick )
{
	CDOTAUserMsg_UnitEvent msg;

	if ( !msg.ParseFromArray( parseBuffer, BufferSize ) )
		return;

    //Demo.MsgPrintf( msg, BufferSize, "%s", msg.DebugString().c_str() );

    //parsing this output in python is sadly faster for me than cpping it
    PrintMessageDemson( msg, tick );
}
#endif

#ifdef OUTPUT_LocationPing
template<>
void PrintUserMessage<CDOTAUserMsg_LocationPing, DOTA_UM_LocationPing>( CDemoFileDump& Demo, const void *parseBuffer, int BufferSize, int tick )
{
	CDOTAUserMsg_LocationPing msg;

	if( !msg.ParseFromArray( parseBuffer, BufferSize ) )
		return;

    //TODO: escape for json?
	printf( "{\"demsontype\": \"LocationPing\", \"player\": %d, \"x\": %d, \"y\": %d, \"target\": %d, \"direct_ping\": %s }\n", msg.player_id(), msg.location_ping().x(), msg.location_ping().y(), msg.location_ping().target(), msg.location_ping().direct_ping() ? "True" : "False" );
}
#endif

void CDemoFileDump::DumpUserMessage( const void *parseBuffer, int BufferSize, int tick )
{
	CSVCMsg_UserMessage userMessage;

	if( userMessage.ParseFromArray( parseBuffer, BufferSize ) )
	{
		int Cmd = userMessage.msg_type();
		int SizeUM = userMessage.msg_data().size();
		const void *parseBufferUM = &userMessage.msg_data()[ 0 ];

		switch( Cmd )
		{
#define HANDLE_UserMsg( _x )			case UM_ ## _x: PrintUserMessage< CUserMsg_ ## _x, UM_ ## _x >( *this, parseBufferUM, SizeUM, tick ); break
#define HANDLE_DOTA_UserMsg( _x )		case DOTA_UM_ ## _x: PrintUserMessage< CDOTAUserMsg_ ## _x, DOTA_UM_ ## _x >( *this, parseBufferUM, SizeUM, tick ); break

		default:
			fprintf( stderr, "WARNING. DumpUserMessage(): Unknown user message %d.\n", Cmd );
			break;

		HANDLE_UserMsg( AchievementEvent );            	// 1,
		HANDLE_UserMsg( CloseCaption );                 // 2,
		//$ HANDLE_UserMsg( CloseCaptionDirect );       // 3,
		HANDLE_UserMsg( CurrentTimescale );             // 4,
		HANDLE_UserMsg( DesiredTimescale );             // 5,
		HANDLE_UserMsg( Fade );                         // 6,
		HANDLE_UserMsg( GameTitle );                    // 7,
		HANDLE_UserMsg( Geiger );                       // 8,
		HANDLE_UserMsg( HintText );                     // 9,
		HANDLE_UserMsg( HudMsg );                       // 10,
		HANDLE_UserMsg( HudText );                      // 11,
		HANDLE_UserMsg( KeyHintText );                  // 12,
		HANDLE_UserMsg( MessageText );                  // 13,
		HANDLE_UserMsg( RequestState );                 // 14,
		HANDLE_UserMsg( ResetHUD );                     // 15,
		HANDLE_UserMsg( Rumble );                       // 16,
		HANDLE_UserMsg( SayText );                      // 17,
		HANDLE_UserMsg( SayText2 );                     // 18,
		HANDLE_UserMsg( SayTextChannel );               // 19,
		HANDLE_UserMsg( Shake );                        // 20,
		HANDLE_UserMsg( ShakeDir );                     // 21,
		HANDLE_UserMsg( StatsCrawlMsg );                // 22,
		HANDLE_UserMsg( StatsSkipState );               // 23,
		HANDLE_UserMsg( TextMsg );                      // 24,
		HANDLE_UserMsg( Tilt );                         // 25,
		HANDLE_UserMsg( Train );                        // 26,
		HANDLE_UserMsg( VGUIMenu );                     // 27,
		HANDLE_UserMsg( VoiceMask );                    // 28,
		HANDLE_UserMsg( VoiceSubtitle );                // 29,
		HANDLE_UserMsg( SendAudio );                    // 30,

		//$ HANDLE_DOTA_UserMsg( AddUnitToSelection );  // 64,
		HANDLE_DOTA_UserMsg( AIDebugLine );             // 65,
		HANDLE_DOTA_UserMsg( ChatEvent );               // 66, TODO: document
		HANDLE_DOTA_UserMsg( CombatHeroPositions );     // 67, TODO: y u no happen?
		HANDLE_DOTA_UserMsg( CombatLogData );           // 68,
		//$ HANDLE_DOTA_UserMsg( CombatLogName );       // 69,
		HANDLE_DOTA_UserMsg( CombatLogShowDeath );      // 70,
		HANDLE_DOTA_UserMsg( CreateLinearProjectile );  // 71,
		HANDLE_DOTA_UserMsg( DestroyLinearProjectile ); // 72,
		HANDLE_DOTA_UserMsg( DodgeTrackingProjectiles );// 73,
		HANDLE_DOTA_UserMsg( GlobalLightColor );        // 74,
		HANDLE_DOTA_UserMsg( GlobalLightDirection );    // 75,
		HANDLE_DOTA_UserMsg( InvalidCommand );          // 76,
		HANDLE_DOTA_UserMsg( LocationPing );            // 77, TODO: Got x,y,tgt. See dota_commonmessages.proto
		HANDLE_DOTA_UserMsg( MapLine );                 // 78,
		HANDLE_DOTA_UserMsg( MiniKillCamInfo );         // 79, TODO: maybe interesting?
		HANDLE_DOTA_UserMsg( MinimapDebugPoint );       // 80,
		HANDLE_DOTA_UserMsg( MinimapEvent );            // 81, TODO: Maybe? Has x,y!
		HANDLE_DOTA_UserMsg( NevermoreRequiem );        // 82,
		HANDLE_DOTA_UserMsg( OverheadEvent );           // 83, TODO: maybe interesting? miss, poison, xp, ...
		HANDLE_DOTA_UserMsg( SetNextAutobuyItem );      // 84,
		HANDLE_DOTA_UserMsg( SharedCooldown );          // 85,
		HANDLE_DOTA_UserMsg( SpectatorPlayerClick );    // 86, TODO: y u no interesting?
		HANDLE_DOTA_UserMsg( TutorialTipInfo );         // 87,
		HANDLE_DOTA_UserMsg( UnitEvent );               // 88, TODO: Maybe use GESTURE or SPEECH?
		HANDLE_DOTA_UserMsg( ParticleManager );         // 89,
		HANDLE_DOTA_UserMsg( BotChat );                 // 90,
		HANDLE_DOTA_UserMsg( HudError );                // 91,
		HANDLE_DOTA_UserMsg( ItemPurchased );           // 92, TODO: Maybe not DOTA but real $$
		HANDLE_DOTA_UserMsg( Ping );                    // 93
		// TODO: and 94 is DOTA_UM_ItemFound but it's only the items at the end.

#undef HANDLE_UserMsg
#undef HANDLE_DOTA_UserMsg
		}
	}
}

template < class T, int msgType >
void PrintNetMessage( CDemoFileDump& Demo, const void *parseBuffer, int BufferSize, int tick )
{
	T msg;

	if( msg.ParseFromArray( parseBuffer, BufferSize ) )
	{
		if( msgType == svc_GameEventList )
		{
			Demo.m_GameEventList.CopyFrom( msg );
		}

		Demo.MsgPrintf( msg, BufferSize, "%s", msg.DebugString().c_str() );
#ifdef OUTPUT_AUTODEMSON_NET
		PrintMessageDemson( msg, tick );
#endif
	}
}

#ifndef OUTPUT_RAWDATA_IN_DEMSON
// These messages cause problems in the json due to their raw data in strings.
template<> void PrintNetMessage< CSVCMsg_CreateStringTable, svc_CreateStringTable >( CDemoFileDump&, const void *, int, int ) { }
template<> void PrintNetMessage< CSVCMsg_UpdateStringTable, svc_UpdateStringTable >( CDemoFileDump&, const void *, int, int ) { }
template<> void PrintNetMessage< CSVCMsg_PacketEntities, svc_PacketEntities >( CDemoFileDump&, const void *, int, int ) { }
template<> void PrintNetMessage< CSVCMsg_TempEntities, svc_TempEntities >( CDemoFileDump&, const void *, int, int ) { }
#endif

template <>
void PrintNetMessage< CSVCMsg_UserMessage, svc_UserMessage >( CDemoFileDump& Demo, const void *parseBuffer, int BufferSize, int tick )
{
	Demo.DumpUserMessage( parseBuffer, BufferSize, tick );
}

#ifdef OUTPUT_GameEvent
template <>
void PrintNetMessage< CSVCMsg_GameEvent, svc_GameEvent >( CDemoFileDump& Demo, const void *parseBuffer, int BufferSize, int tick )
{
	CSVCMsg_GameEvent msg;

	if( msg.ParseFromArray( parseBuffer, BufferSize ) )
	{
		int iDescriptor;

		for( iDescriptor = 0; iDescriptor < Demo.m_GameEventList.descriptors().size(); iDescriptor++ )
		{
			const CSVCMsg_GameEventList::descriptor_t& Descriptor = Demo.m_GameEventList.descriptors( iDescriptor );

			if( Descriptor.eventid() == msg.eventid() )
				break;
		}

		if( iDescriptor == Demo.m_GameEventList.descriptors().size() )
		{
#ifdef OUTPUT_ORIGINAL
			printf( "%s", msg.DebugString().c_str() );
#endif
		}
		else
		{
			int numKeys = msg.keys().size();
			const CSVCMsg_GameEventList::descriptor_t& Descriptor = Demo.m_GameEventList.descriptors( iDescriptor );

            //TODO: game time / tick count?
			printf( "{\"demsontype\": \"gameevent\", \"evname\": \"%s\", \"evid\": %d, \"evname2\": \"%s\", \"tick\": %d",
                escapeJsonString(Descriptor.name()).c_str(), msg.eventid(),
				msg.has_event_name() ? escapeJsonString(msg.event_name()).c_str() : "", tick );

			for( int i = 0; i < numKeys; i++ )
			{
				const CSVCMsg_GameEventList::key_t& Key = Descriptor.keys( i );
				const CSVCMsg_GameEvent::key_t& KeyValue = msg.keys( i );

				printf(", \"%s\": ", escapeJsonString(Key.name()).c_str() );

				if( KeyValue.has_val_string() )
					printf( "\"%s\"", escapeJsonString(KeyValue.val_string()).c_str() );
				if( KeyValue.has_val_float() )
					printf( "%f", KeyValue.val_float() );
				if( KeyValue.has_val_long() )
					printf( "%d", KeyValue.val_long() );
				if( KeyValue.has_val_short() )
					printf( "%d", KeyValue.val_short() );
				if( KeyValue.has_val_byte() )
					printf( "%d", KeyValue.val_byte() );
				if( KeyValue.has_val_bool() )
					printf( "%s", KeyValue.val_bool() ? "true" : "false" );
				if( KeyValue.has_val_uint64() )
					printf( "%lld", KeyValue.val_uint64() );
			}
			printf("}\n");
		}
	}
}
#endif

static std::string GetNetMsgName( int Cmd )
{
	if( NET_Messages_IsValid( Cmd ) )
	{
		return NET_Messages_Name( ( NET_Messages )Cmd );
	}
	else if( SVC_Messages_IsValid( Cmd ) )
	{
		return SVC_Messages_Name( ( SVC_Messages )Cmd );
	}

	assert( 0 );
	return "NETMSG_???";
}

void CDemoFileDump::DumpDemoPacket( const std::string& buf, int tick )
{
	size_t index = 0;

	while( index < buf.size() )
	{
		int Cmd = ReadVarInt32( buf, index );
		uint32 Size = ReadVarInt32( buf, index );

		if( index + Size > buf.size() )
		{
			const std::string& strName = GetNetMsgName( Cmd );

			fatal_errorf( "buf.ReadBytes() failed. Cmd:%d '%s' \n", Cmd, strName.c_str() );
		}

		switch( Cmd )
		{
#define HANDLE_NetMsg( _x )		case net_ ## _x: PrintNetMessage< CNETMsg_ ## _x, net_ ## _x >( *this, &buf[ index ], Size, tick ); break
#define HANDLE_SvcMsg( _x )		case svc_ ## _x: PrintNetMessage< CSVCMsg_ ## _x, svc_ ## _x >( *this, &buf[ index ], Size, tick ); break

		default:
			fprintf( stderr, "WARNING. DumpUserMessage(): Unknown netmessage %d.\n", Cmd );
			break;

		HANDLE_NetMsg( NOP );            	// 0
		HANDLE_NetMsg( Disconnect );        // 1
		HANDLE_NetMsg( File );              // 2
		HANDLE_NetMsg( SplitScreenUser );   // 3
		HANDLE_NetMsg( Tick );              // 4
		HANDLE_NetMsg( StringCmd );         // 5
		HANDLE_NetMsg( SetConVar );         // 6
		HANDLE_NetMsg( SignonState );       // 7
		HANDLE_SvcMsg( ServerInfo );        // 8
		HANDLE_SvcMsg( SendTable );         // 9
		HANDLE_SvcMsg( ClassInfo );         // 10
		HANDLE_SvcMsg( SetPause );          // 11
		HANDLE_SvcMsg( CreateStringTable ); // 12
		HANDLE_SvcMsg( UpdateStringTable ); // 13
		HANDLE_SvcMsg( VoiceInit );         // 14
		HANDLE_SvcMsg( VoiceData );         // 15
		HANDLE_SvcMsg( Print );             // 16
		HANDLE_SvcMsg( Sounds );            // 17
		HANDLE_SvcMsg( SetView );           // 18
		HANDLE_SvcMsg( FixAngle );          // 19
		HANDLE_SvcMsg( CrosshairAngle );    // 20
		HANDLE_SvcMsg( BSPDecal );          // 21
		HANDLE_SvcMsg( SplitScreen );       // 22
		HANDLE_SvcMsg( UserMessage );       // 23
		//$ HANDLE_SvcMsg( EntityMessage ); // 24
		HANDLE_SvcMsg( GameEvent );         // 25
		HANDLE_SvcMsg( PacketEntities );    // 26 TODO: If only we understood entity_data...
		HANDLE_SvcMsg( TempEntities );      // 27
		HANDLE_SvcMsg( Prefetch );          // 28
		HANDLE_SvcMsg( Menu );              // 29
		HANDLE_SvcMsg( GameEventList );     // 30 TODO: check that out. See netmessages.proto
		HANDLE_SvcMsg( GetCvarValue );      // 31
		// TODO: what about CSVCMsgList_GameEvents and CSVCMsgList_UserMessages ?

#undef HANDLE_SvcMsg
#undef HANDLE_NetMsg
		}

		index += Size;
	}
}

static const std::string interesting_tables[] = {"CombatLogNames", "userinfo"};

// w00t w00t
template <typename TN, typename TH, size_t N>
static bool in(TN needle, TH (&haystack)[N])
{
	return std::find(haystack, haystack + N, needle) != haystack + N;
}

// string tables, to which //TODO events refer
static bool DumpDemoStringTable( CDemoFileDump& Demo, const CDemoStringTables& StringTables )
{
	for( int i = 0; i < StringTables.tables().size(); i++ )
	{
		const CDemoStringTables::table_t& Table = StringTables.tables( i );

		bool bIsActiveModifiersTable = !strcmp( Table.table_name().c_str(), "ActiveModifiers" );
		bool bIsUserInfo = !strcmp( Table.table_name().c_str(), "userinfo" );

		// skip over noninteresting stringtables, just mention them
		if( !in(Table.table_name(), interesting_tables) ) {
			//TODO: only output if debug flag? (stringtable_ignored to find/highlight those entries better)
            #ifdef OUTPUT_StringTableIgnored
			printf("{\"demsontype\": \"stringtable_ignored\", \"tablename\": \"%s\"}\n", escapeJsonString(Table.table_name()).c_str());
            #endif
			continue;
		}

        // skip the utterly broken userinfo string table we don't even need
        if ( !strcmp( Table.table_name().c_str(), "userinfo" ) ) {
            continue;
        }

		printf("{\"demsontype\": \"stringtable\", \"tablename\": \"%s\", \"stringtable\": [", escapeJsonString(Table.table_name()).c_str());

		for( int itemid = 0; itemid < Table.items().size(); itemid++ )
		{
			const CDemoStringTables::items_t& Item = Table.items( itemid );

			if( bIsActiveModifiersTable )
			{
				CDOTAModifierBuffTableEntry Entry;

				if( Entry.ParseFromString( Item.data() ) )
				{
					std::string EntryStr = Entry.DebugString();
					printf( "\"%s\"", escapeJsonString(EntryStr).c_str() );
				}
			}
			//TODO: this does not work - nothing is printed if the size is not ignored!
			//if we ignore the size, the data might be (will be) massively wrong
			//has the code changed enough, that this struct is out of date?
			//
			// The problem here is much worse. player_info_s contains "bool" and "int" members.
			// The size (in bytes) of those is not fixed by the standard and actual implementations
			// really do differ (32 vs 64 bit, linux vs windows, ...). The struct is not "packed"
			// either, meaning compilers are free to realign (even shuffle? unsure.) members
			// as they feel. Seeing they provided a VC++ project, we need to find out what exactly
			// VC++ generates and modify the struct to fit that on gcc too, by using fixed-size
			// types and possibly packing.
			//
			// While the above is right, this is "undone" by struct padding which aligns the
			// elements to start at memory adresses which are multiple of 4, effectively padding
			// the 1-byte bools with 3 bytes. Thus in the end, two differences work together to
			// give the same end result... 1+1=0
			//
			// So the problem is the data is 140 bytes big while the struct has size 144 - both on win/lin.
			//
			// Wait... do we even need this data? Probably only player name and id, which are early
			// in the struct and thus likely to be correct.
			else if( bIsUserInfo && Item.data().size() > 0 )
			{
				const player_info_s *pPlayerInfo = ( const player_info_s * )&Item.data()[ 0 ];

				printf("{\"xuid\":%lld, \"name\": \"%s\", \"userID\": %d, \"guid\": \"%s\", \"friendsID\":%d, \"friendsName\": \"%s\", \"fakeplayer\": %s, \"ishltv\": %s, \"filesDownloaded\":%d}",
					pPlayerInfo->xuid, escapeJsonString(pPlayerInfo->name).c_str(), pPlayerInfo->userID, escapeJsonString(pPlayerInfo->guid).c_str(), pPlayerInfo->friendsID,
					escapeJsonString(pPlayerInfo->friendsName).c_str(), pPlayerInfo->fakeplayer ? "true" : "false", pPlayerInfo->ishltv ? "true" : "false", pPlayerInfo->filesDownloaded );
			}
			else
			{
				printf("\"%s\"", escapeJsonString(Item.str()).c_str());
			}

			// sep the entries with commas
			if(itemid < Table.items().size()-1)
			{
				printf(", ");
			}
		}
		printf("]}\n");
	}

	return true;
}

void CDemoFileDump::PrintDemoHeader( EDemoCommands DemoCommand, int tick, int size, int uncompressed_size )
{
	const std::string& DemoCommandName = EDemoCommands_Name( DemoCommand );

#ifdef OUTPUT_ORIGINAL
	printf( "==== #%d: Tick:%d '%s' Size:%d UncompressedSize:%d ====\n",
		m_nFrameNumber, tick, DemoCommandName.c_str(), size, uncompressed_size );
#endif
}

template < class DEMCLASS >
void PrintDemoMessage( CDemoFileDump& Demo, bool bCompressed, int tick, int& size, int& uncompressed_size )
{
	DEMCLASS Msg;

	if( Demo.m_demofile.ReadMessage( &Msg, bCompressed, &size, &uncompressed_size ) )
	{
		Demo.PrintDemoHeader( Msg.GetType(), tick, size, uncompressed_size );

		Demo.MsgPrintf( Msg, size, "%s", Msg.DebugString().c_str() );
	}
}

template <>
void PrintDemoMessage<CDemoStringTables_t>( CDemoFileDump& Demo, bool bCompressed, int tick, int& size, int& uncompressed_size )
{
	CDemoStringTables_t Msg;

	if( Demo.m_demofile.ReadMessage( &Msg, bCompressed, &size, &uncompressed_size ) )
	{
		Demo.PrintDemoHeader( Msg.GetType(), tick, size, uncompressed_size );

		DumpDemoStringTable( Demo, Msg );
	}
}

template <>
void PrintDemoMessage<CDemoFileInfo_t>( CDemoFileDump& Demo, bool bCompressed, int tick, int& size, int& uncompressed_size )
{
	CDemoFileInfo_t Msg;

	if( Demo.m_demofile.ReadMessage( &Msg, bCompressed, &size, &uncompressed_size ) )
	{
		Demo.PrintDemoHeader( Msg.GetType(), tick, size, uncompressed_size );

		printf( "{\"demsontype\": \"fileinfo\", \"playback_time\": %f, \"playback_ticks\": %d, \"playback_frames\": %d}\n",
				Msg.playback_time(), Msg.playback_ticks(), Msg.playback_frames());
		const CGameInfo_CDotaGameInfo& gameinfo = Msg.game_info().dota();
		printf( "{\"demsontype\": \"gameinfo\", \"match_id\": %d, \"game_mode\": %d, \"game_winner\": %d, \"players\": [",
				gameinfo.match_id(), gameinfo.game_mode(), gameinfo.game_winner() );
		for( int iplayer = 0 ; iplayer < gameinfo.player_info_size() ; ++iplayer ) {
			const CGameInfo_CDotaGameInfo_CPlayerInfo& player = gameinfo.player_info(iplayer);
			printf( "{\"hero_name\": \"%s\", \"player_name\": \"%s\", \"is_fake_client\": %s}",
					escapeJsonString(player.hero_name()).c_str(), escapeJsonString(player.player_name()).c_str(), player.is_fake_client() ? "true" : "false" );
			if( iplayer < gameinfo.player_info_size()-1 )
				printf( ", " );
		}
		printf( "]}\n" );
	}
}

void CDemoFileDump::DoDump()
{
	bool bStopReading = false;

	for( m_nFrameNumber = 0; !bStopReading; m_nFrameNumber++ ) //TODO HERE: really frameNumber?
	{
		int tick = 0;
		int size = 0;
		bool bCompressed;
		int uncompressed_size = 0;

		if( m_demofile.IsDone() )
			break;

		EDemoCommands DemoCommand = m_demofile.ReadMessageType( &tick, &bCompressed );

		switch( DemoCommand )
		{
#define HANDLE_DemoMsg( _x )	case DEM_ ## _x: PrintDemoMessage< CDemo ## _x ## _t >( *this, bCompressed, tick, size, uncompressed_size ); break

		HANDLE_DemoMsg( FileHeader );
		HANDLE_DemoMsg( FileInfo );
		HANDLE_DemoMsg( Stop );
		HANDLE_DemoMsg( SyncTick );
		HANDLE_DemoMsg( ConsoleCmd );
		HANDLE_DemoMsg( SendTables );
		HANDLE_DemoMsg( ClassInfo );
		HANDLE_DemoMsg( StringTables );
		HANDLE_DemoMsg( UserCmd );
		HANDLE_DemoMsg( CustomDataCallbacks );
		HANDLE_DemoMsg( CustomData );

#undef HANDLE_DemoMsg

		case DEM_FullPacket:
			{
				CDemoFullPacket_t FullPacket;

				if( m_demofile.ReadMessage( &FullPacket, bCompressed, &size, &uncompressed_size ) )
				{
					PrintDemoHeader( DemoCommand, tick, size, uncompressed_size );

					// Spew the stringtable
					DumpDemoStringTable( *this, FullPacket.string_table() );

					// Ok, now the packet.
					DumpDemoPacket( FullPacket.packet().data(), tick );
				}
			}
			break;

		case DEM_Packet:
		case DEM_SignonPacket:
			{
				CDemoPacket_t Packet;

				if( m_demofile.ReadMessage( &Packet, bCompressed, &size, &uncompressed_size ) )
				{
					PrintDemoHeader( DemoCommand, tick, size, uncompressed_size );

					DumpDemoPacket( Packet.data(), tick );
				}
			}
			break;

		default:
		case DEM_Error:
			bStopReading = true;
			fatal_errorf( "Shouldn't ever get this demo command?!? %d\n", DemoCommand );
			break;
		}
	}
}


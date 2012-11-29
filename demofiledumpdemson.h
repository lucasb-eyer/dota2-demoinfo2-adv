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

/*
 *	This file is built up in parallel to the original demofiledump, but the output is fixed 
 *	to be in .demson format, if the underlying files changes, it can be adjusted without 
 *	changing the provided files, hopefuly with minimal effort.
 */

#ifndef DEMOFILEDUMPDEMSON_H
#define DEMOFILEDUMPDEMSON_H

#include "demofile.h"
#include "generated_proto/netmessages.pb.h"

class CDemoFileDump
{
public:
	CDemoFileDump() : m_nFrameNumber( 0 ) {}
	~CDemoFileDump() {}

	bool Open( const char *filename ); 
	void DoDump();

//TODO: is this really needed here? Forward declarations?
//yes, those functions are used in non-class functions apparently
public:
	void DumpDemoPacket( const std::string& buf );
	void DumpUserMessage( const void *parseBuffer, int BufferSize );
	void PrintDemoHeader( EDemoCommands DemoCommand, int tick, int size, int uncompressed_size );
	void MsgPrintf( const ::google::protobuf::Message& msg, int size, const char *fmt, ... );

public:
	CDemoFile m_demofile;
	CSVCMsg_GameEventList m_GameEventList;

	int m_nFrameNumber;
};

#endif // DEMOFILEDUMPDEMSON_H


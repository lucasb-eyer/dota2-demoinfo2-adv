Compilation
===========

Make sure you have the following prerequisites installed:
- [protobuf (2.4.1)](http://code.google.com/p/protobuf)
    - The `protoc` command should be in your path.
    - You can run `PROTOC=/path/to/protoc make` if that's not the case.
- [snappy (1.0.5)](http://code.google.com/p/snappy)

You can then compile the project by simply running `make`. If anything is
in a non-standard location, you can add compiler options using a hack like

    CXX="g++ -I/more/include/paths -L/more/library/paths" make

Usage
=====

    ./demoinfo replay.dem > outfile.txt

You'll probably want to postprocess the output using a higher-level tool.

Output format
=============
Currently, each line of the file contains a json object. This object is either
a listof strings, in which case the object is the 'combatlog' string table, or
it is a dict, in which case it is a game event.

String table
------------
The string table only gets extended over time, meaning that what is the 3rd
entry at the beginning of the replay will stay the 3rd entry until the end.
This also implies the string table only grows.

Game event
----------
Each game event has a `evname` and a `evid` key whose value describes the type
of the game event. As of now, it seems this information is redundant. The
'evname2' key seems to be empty for all game events we can parse.

The remainder of the game event entries is dependent of the type of the event.
For the `dota_combatlog` event, all "names" are keys of the (full, upcoming)
`combatlog` string table. For other events, like `dota_chase_hero`, the `target`s
seem to be entity ids which we can't get a hold of yet.

Credit
======
Original software released by Valve, OSX port by Armin (mitsuhiko).

Original copyright from Valve:
<pre>
====== Copyright (c) 2012, Valve Corporation, All rights reserved. ========

 Redistribution and use in source and binary forms, with or without 
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice, 
 this list of conditions and the following disclaimer in the documentation 
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
</pre>
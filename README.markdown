Compilation
===========

Make sure you have the following prerequisites installed:
- [protobuf (2.4.1)](http://code.google.com/p/protobuf)
    - The `protoc` command should be in your path.
    - You can run `PROTOC=/path/to/protoc make` if that's not the case.
- [snappy (1.0.5)](http://code.google.com/p/snappy)

You can then compile the project by simply running `make`. (But read the
compile-time settings section beforehand.) If anything is in a non-standard
location, you can add compiler options using a hack like

    CXX="g++ -I/more/include/paths -L/more/library/paths" make

Compile-time settings
---------------------
There are some settings you can choose at compile-time in order to control
what is output. Yes, this would be friendlier if those were runtime arguments,
but life's not friendly. This allows for minimal changes to the demoinfo2 code
which Valve "promised" to update at some point. You do know what
[Valve promises](https://developer.valvesoftware.com/wiki/Valve_Time) are, do you?

Enable these settings by adding CXX flags:

    CXX="g++ -DOUTPUT_AUTODEMSON_USER -DOUTPUT_AUTODEMSON_NET" make

### OUTPUT\_ORIGINAL
Output whatever the original demoinfo2 code would have output.

### OUTPUT\_AUTODEMSON\_[USER,NET]
Output demson types generated automatically through reflection.

This is useful in that you get _everything_, i.e. all message and even "hidden"
fields, which are present in the data but not in the protobuf files.
(Those were mostly added after the release of demoinfo2.)

This is useless in that you get tons of useless messages and many messages have
a less-than-optimal design. Disabling this gives you a hand-curated list of
demsontypes which also expose a friendly interface.

`USER` and `NET` are just two different categories of messages. The user
messages seem to be more related to the dota game while the net messages seem
to be related more to steam engine's generic internals, although they contain
some very dota-related stuff too.

#### OUTPUT\_RAWDATA\_IN\_DEMSON
Some `NET` messages contain raw data buffers. These are output as json strings
where non-printable characters are escaped as hex as in `\xFF`. This encoding
of raw data into strings causes problems with some loading libraries (namely,
python's `json` module). This options will just skip these messages.

TODO: We need to investigate these messages and find out what's hidden in the
raw data, as it's probably interesting.

### OUTPUT\_`some message type`
`some message type` is a string consisting of only the "useful" part of the
message name. For example, for a `CDOTAUserMsg_LocationPing`, the "useful"
part is `LocationPing` and thus the define is called `OUTPUT_LocationPing`.

Messages for which this is defined are _additionally_ output by a hand-crafted
json outputting routine, thus potentially being easier to handle.

Currently, the following ones exist:

    * `OUTPUT_LocationPing`
    * `OUTPUT_GameEvent`

Usage
=====

    ./demoinfo2 replay.dem > replay.rawdemson

You'll probably want to postprocess the output using a higher-level tool.
A tool for converting the "names" present in the combatlog gameevents into
actual strings (from the stringtables) is included in this repo since it is
tightly connected:

    python ids_to_names.py < replay.rawdemson > replay.demson

Note that only the combatlog gameevents are output by this script.

Output format
=============
Currently, each line of the file contains a json object. This object is always
a dict which contains at least the key `demsontype`. This key determines the
shape of the dict - it is its class if you like to think static typing.

demsontype
----------
Each json object has a key `demsontype`, which fully specifies what kind of
data is contained in this particular line. This can also be used for debug purposes.
(see `debug_ignored_stringtable`)

fileinfo
--------
`"demsontype": "fileinfo"`

Contains the game's length in seconds (`playback_time`), ticks (`playback_ticks`) and frames (`playback_frames`).

gameinfo
--------
`"demsontype": "gameinfo"`

Containsvery useful info about the game like the `match_id`.
* `game_mode`: TODO
* `game_winner`: TODO (wtf is 3? why not only 0/1 or so?)
* `players`: A list of information (dicts) about the players in the game.
    * `hero_name`
    * `player_name`: the nickname, not steam/server id.
    * `is_fake_client`: boolean. Didn't see any `true` yet.

String table
------------
`"demsontype": "stringtable"` or `"demsontype": "stringtable_ignored"`

The string table only gets extended over time, meaning that what is the 3rd
entry at the beginning of the replay will stay the 3rd entry until the end.
This also implies the string table only grows.

Currently only stringtables of type `"demsontype": "stringtable_combatlog"` are completely printed.

Game event
----------
`"demsontype": "gameevent"`

Each game event has a `evname` and a `evid` key whose value describes the type
of the game event. As of now, it seems this information is redundant. The
'evname2' key seems to be empty for all game events we can parse.

The remainder of the game event entries is dependent of the type of the event.
For the `dota_combatlog` event, all "names" are keys of the (full, upcoming)
`combatlog` string table. For other events, like `dota_chase_hero`, the `target`s
seem to be entity ids which we can't get a hold of yet.

### Combatlog game events
This might be one of the most interesting gameevents. It describes some interaction
between entities, like damaging, healing, stunning, ...

TODO: find out what exactly the different `name`s, `health` and `value` mean in each case.

#### type: 0
burst damage being dealt, as opposed to damage over time (more details need to be determined).
Includes spell damage. `value` is the effective damage dealt, i.e. after all resistances have been
computed. `health` seems to be the target's health left after the damage has been dealt.

#### type: 1
burst healing, as opposed to healing over time. Examples of this are lifeleech and magic stick.
No salve, no tango.

#### type: 2
seems to be spell damage/modifiers. Unsure

#### type: 3
same as above. What's the difference?

#### type: 4
Seems to be kills, including creeps and building kills. This seems to be the case because the health
is always 0. TODO: check this by watching the replay alongside the data.

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

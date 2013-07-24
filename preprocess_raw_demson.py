#! /usr/bin/env python
import json
import sys

from cgi import escape

# puts game/file info first, translated game events laters
# replaces ids in gameevents by strings (does not output string tables)
# replaces playerids/type in chateevents by player/activity strings

def is_combatlog_stringtable(ge):
    return ge['demsontype'] == 'stringtable_combatlog' or (ge['demsontype'] == 'stringtable' and ge['tablename'] == 'CombatLogNames')

def get_demsontype_item(demsontype, replay):
    """ picks out first object with a given demsontype string from the replay object array """
    return filter(lambda x: x["demsontype"] == demsontype, replay)[0]

## Fuck, I love python. Had to resist not to do this in one line!
replay = map(json.loads, sys.stdin) #load linewise json from stdin

# DEBUG: this was an attempt to defubar an invalid "json" output. Fixed in the cpp source
#replay = []
#for line in sys.stdin.readlines():
#    print line
#    replay.append(json.loads(line))

# Note: tr stands for translate.
#keys which need to be translated
to_tr_gameevent = ('attackername', 'inflictorname', 'sourcename', 'targetname', 'targetsourcename')
#No more assumptions on what is output last
tr_gameevent = filter(is_combatlog_stringtable, replay)[-1]['stringtable']  # The last (translation) replay combatlog stringtable.

#TODO: use these?
gameinfo = get_demsontype_item("gameinfo", replay)
fileinfo = get_demsontype_item("fileinfo", replay)

#print important stuff first
print(json.dumps(fileinfo))
print(json.dumps(gameinfo))

# this might be used to keep track of what exactly is ignored
ignored_set = set()

for entry in replay:
    if entry['demsontype'] == 'gameevent':
        if entry['evname'] == 'dota_combatlog':
            #translate each dictionary entry (if appropriate)
            translated_entry = {k: tr_gameevent[v] if k in to_tr_gameevent else v for k, v in entry.iteritems()}
            print(json.dumps(translated_entry))
        else:
            ignored_set.add(('gameevent', entry['evname']))
            pass
    elif entry['demsontype'] == 'chatevent':
        #TODO: translate
        print(json.dumps(entry))
    elif entry['demsontype'] == "gameinfo": #these two are already handled
        #print(json.dumps(entry))
        pass
    elif entry['demsontype'] == "fileinfo":
        #print(json.dumps(entry))
        pass
    else:
        ignored_set.add((entry['demsontype'], ''))
        pass #ignore any other entry

#show what we have missed?
#print(ignored_set)

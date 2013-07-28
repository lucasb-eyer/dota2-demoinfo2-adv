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
    elif entry['demsontype'] == 'CDOTAUserMsg_UnitEvent':
        if entry['msg_type'] == 'DOTA_UNIT_SPEECH_CLIENTSIDE_RULES':
            new_entry = {}
            new_entry['demsontype'] = 'matchtime' #TODO: this should be done in the cpp part?
            new_entry['tick'] = entry['tick']
            facts = entry['speech_match_on_client']['responsequery']['facts']
            matches = filter(lambda x: x["key"] == 30, facts) #facts that match our obscure key

            if len(matches) != 1: #not the droids we are looking for
                continue
            what = matches[0]["val_string"]

            if what == "pre_game":
                new_entry["match_time"] = -80
            elif what == "game_start":
                new_entry["match_time"] = -3
            else:
                ignored_set.add(('speech_clientside', what))
                continue

            print(json.dumps(new_entry))
        else:
            ignored_set.add((entry['demsontype'], entry['msg_type']))
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
print(ignored_set)

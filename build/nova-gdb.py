# -*- Mode: Python -*-
import gdb

print "Nova command set loading"

class Sc_printer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return 'prio %#2x (unfinished)'.format(self.val['prio'])

def make_nova_printer(val):
    if str (val.type()) == 'Sc':
        return Sc_printer(val)
    return None

print "Done"

# EOF

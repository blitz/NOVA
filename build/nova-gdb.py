# -*- Mode: Python -*-
import gdb
from time import time

class Attach_command(gdb.Command):
    def __init__(self):
        super(Attach_command, self).__init__("nova-attach", gdb.COMMAND_NONE)

    def invoke(self, arg, from_tty):
        gdb.execute("target remote localhost:1235")

class Benchstepi_command(gdb.Command):
    def __init__(self):
        super(Benchstepi_command, self).__init__("nova-stepi", gdb.COMMAND_NONE)

    def invoke(self, arg, from_tty):
        repeat = 15
        start = time()
        for i in range(repeat):
            gdb.execute("stepi")
        end = time()
        print("stepi took %f" % ((end - start) / repeat))

class Sc_printer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return 'prio %#2x (unfinished)' % self.val['prio']

def make_nova_printer(val):
    if str (val.type()) == 'Sc':
        return Sc_printer(val)
    return None

Attach_command()
Benchstepi_command()
gdb.pretty_printers.append(make_nova_printer)

print "Type 'nova-attach' to attach to a running Nova kernel."

# EOF

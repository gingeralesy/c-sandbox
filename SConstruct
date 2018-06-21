import sys

cflags = '-iquoteinclude -Wall'
ldflags = '-l:libpcre.a -lpthread -lm -Llib'
if sys.platform == "win32":
  ldflags += ' -LC:/msys64/mingw64/lib -lWs2_32'

env = Environment(CCFLAGS = cflags, LINKFLAGS = ldflags)
env.ParseConfig('pkg-config --cflags --libs ncursesw freetype2')

main_obj = env.Object('obj/main.o', source = [ 'src/main.c' ])

memory_obj = env.Object('obj/memory.o', source = [ 'memory/src/memory.c' ])
ncurs_obj = env.Object('obj/ncurs.o', source = [ 'ncurs/src/ncurs.c' ])
rbtree_obj = env.Object('obj/rbtree.o', source = [ 'rbtree/src/rbtree.c' ])
ticket_obj = env.Object('obj/ticket.o', source = [ 'ticket/src/ticket.c' ])

env.Depends(main_obj, [ ncurs_obj ])
env.Depends(ncurs_obj, [ memory_obj, ticket_obj ])
env.Depends(memory_obj, [ ticket_obj, rbtree_obj ])

sb_prog = env.Program('bin/sandbox', [ main_obj, ticket_obj, memory_obj, ncurs_obj, rbtree_obj ])
import sys

cflags = '-iquoteinclude -Wall'
ldflags = '-l:libpcre.a -lpthread -lm -Llib'
if sys.platform == "win32":
  ldflags += ' -LC:/msys64/mingw64/lib -lWs2_32'

if int(ARGUMENTS.get('debug', 0)):
  cflags += ' -D_SANDBOX_DEBUG'

project = ARGUMENTS.get('project', 'ncurs')

env = Environment(CCFLAGS = cflags, LINKFLAGS = ldflags)
if project == 'ncurs' or project == 'wifi':
  if project == 'wifi':
    env.Append(LINKFLAGS = ' -liw')
  env.ParseConfig('pkg-config --cflags --libs ncursesw')
elif project == 'freetype':
  env.ParseConfig('pkg-config --cflags --libs freetype2')

main_obj = env.Object('obj/main.o', source = [ 'src/' + project +'/main.c' ])

if project == 'freetype':
  freetype_obj = env.Object('obj/freetype.o', source = [ 'src/freetype/freetype.c' ])
elif project == 'ncurs':
  ncurs_obj = env.Object('obj/ncurs.o', source = [ 'src/ncurs/ncurs.c' ])
elif project == 'wifi':
  ncurs_obj = env.Object('obj/ncurs.o', source = [ 'src/ncurs/ncurs.c' ])
  wifi_obj = env.Object('obj/wifi.o', source = [ 'src/wifi/wifi.c' ])

hashmap_obj = env.Object('obj/hashmap.o', source = [ 'src/hashmap/hashmap.c' ])
memory_obj = env.Object('obj/memory.o', source = [ 'src/memory/memory.c' ])
objects_obj = env.Object('obj/objects.o', source = [ 'src/objects/objects.c' ])
rbtree_obj = env.Object('obj/rbtree.o', source = [ 'src/rbtree/rbtree.c' ])
ticket_obj = env.Object('obj/ticket.o', source = [ 'src/ticket/ticket.c' ])

if project == 'ncurs':
  sb_prog = env.Program('bin/sandbox', [ main_obj, hashmap_obj, ticket_obj, memory_obj, ncurs_obj, rbtree_obj ])
elif project == 'freetype':
  sb_prog = env.Program('bin/sandbox', [ main_obj, freetype_obj, hashmap_obj, ticket_obj, memory_obj, rbtree_obj ])
elif project == 'wifi':
  sb_prog = env.Program('bin/sandbox', [ main_obj, wifi_obj, ncurs_obj, hashmap_obj, ticket_obj, memory_obj, rbtree_obj ])
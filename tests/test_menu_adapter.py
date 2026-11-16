import ctypes as C
import json
import pathlib
import struct
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT=pathlib.Path(__file__).resolve().parents[1]
SRC=ROOT/'source/plugins/plugin_mono/source/cheat'
MOCK=r'''
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
struct proc {char name[32];int pid;} __attribute__((packed));
struct info {int pid;char name[40],path[64],title[16],content[64];} __attribute__((packed));
struct map {char name[32];uint64_t start,end,offset;uint16_t prot;} __attribute__((packed));
struct maps {struct map *maps;uint64_t count;} __attribute__((packed));
static unsigned char memory[4096];
static char title[16]="CUSA12345";
static int writes;
void mock_title(const char *s) {strncpy(title,s,15);title[15]=0;}
int mock_byte(unsigned i) {return memory[i];}
int mock_writes(void) {return writes;}
long orbis_syscall(long number,...) {
 va_list ap;va_start(ap,number);long rc=0;
 if(number==107) {
  struct proc *p=va_arg(ap,struct proc *);uint64_t *n=va_arg(ap,uint64_t *);
  if(p) {memset(p,0,sizeof(*p));strcpy(p->name,"eboot.bin");p->pid=42;}*n=1;
 } else if(number==109) {
  uint64_t pid=va_arg(ap,uint64_t),cmd=va_arg(ap,uint64_t);void *data=va_arg(ap,void *);
  if(pid!=42) rc=-1;
  else if(cmd==8) {struct info *p=data;memset(p,0,sizeof(*p));p->pid=42;strcpy(p->name,"eboot.bin");strcpy(p->title,title);}
  else if(cmd==4) {struct maps *m=data;if(m->maps) {memset(m->maps,0,sizeof(*m->maps));strcpy(m->maps->name,"executable");m->maps->start=0x400000;m->maps->end=0x401000;m->maps->prot=5;}m->count=1;}
  else rc=-1;
 } else if(number==108) {
  uint64_t pid=va_arg(ap,uint64_t),addr=va_arg(ap,uint64_t);void *data=va_arg(ap,void *);
  uint64_t n=va_arg(ap,uint64_t),write=va_arg(ap,uint64_t);
  if(pid!=42 || addr<0x400000 || addr+n>0x401000) rc=-1;
  else if(write) {memcpy(memory+addr-0x400000,data,n);++writes;}
  else memcpy(data,memory+addr-0x400000,n);
 } else rc=-1;
 va_end(ap);return rc;
}
'''
class MenuAdapterTests(unittest.TestCase):
 def test_live_identity_menu_toggle_restore_and_stale_session(self):
  with tempfile.TemporaryDirectory() as tmp:
   p=pathlib.Path(tmp);(p/'metadata/CUSA12345').mkdir(parents=True);(p/'cheats/json').mkdir(parents=True)
   (p/'metadata/CUSA12345/app_info').write_bytes(struct.pack('<i10s8s8s2x',42,b'CUSA12345',b'gd',b'01.00'))
   data={'name':'Demo','id':'CUSA12345','version':'01.00','process':'eboot.bin','mods':[{'name':'A < B & C','type':'checkbox','memory':[{'offset':'10','on':'AA','off':'00'}]}]}
   (p/'cheats/json/CUSA12345_01.00.json').write_text(json.dumps(data));(p/'mock.c').write_text(MOCK)
   so=p/'menu.so'
   subprocess.run(['cc','-shared','-fPIC','-O2','-Wall','-Wextra',f'-DCHEAT_ROOT="{p}/cheats"',f'-DBH_APPINFO_ROOT="{p}/metadata"',str(SRC/'engine.c'),str(SRC/'adapter.c'),str(SRC/'cJSON.c'),str(p/'mock.c'),'-lm','-o',str(so)],check=True)
   lib=C.CDLL(str(so));lib.bh_cheat_menu_xml.restype=C.c_char_p
   lib.bh_cheat_menu_press.argtypes=[C.c_char_p,C.c_void_p,C.c_size_t]
   lib.mock_title.argtypes=[C.c_char_p]
   xml=ET.fromstring(lib.bh_cheat_menu_xml());button=xml.find('.//button');self.assertEqual(button.get('title'),'[OFF] A < B & C')
   err=C.create_string_buffer(256);self.assertEqual(lib.bh_cheat_menu_press(b'bhcheat_0',err,256),1)
   self.assertEqual(lib.mock_byte(16),0xAA);self.assertIn(b'Enabled and verified',err.value)
   xml=ET.fromstring(lib.bh_cheat_menu_xml());self.assertTrue(xml.find('.//button').get('title').startswith('[ON]'))
   lib.bh_cheat_menu_press(b'bhcheat_0',err,256);self.assertEqual(lib.mock_byte(16),0)
   writes=lib.mock_writes();lib.mock_title(b'CUSA54321');lib.bh_cheat_menu_press(b'bhcheat_0',err,256)
   self.assertEqual(lib.mock_writes(),writes);self.assertIn(b'session changed',err.value)
if __name__=='__main__':unittest.main(verbosity=2)

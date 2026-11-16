import ctypes as C
import pathlib
import shutil
import struct
import subprocess
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
SRC=ROOT/'source/plugins/plugin_mono/source/fortnite_reset.c'
MOCK=r'''
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
struct proc {char name[32];int pid;} __attribute__((packed));
struct info {int pid;char name[40],path[64],title[16],content[64];} __attribute__((packed));
static int running,rename_count,fail_rename;
void mock_reset(int run,int fail) {running=run;rename_count=0;fail_rename=fail;}
int bh_test_rename(const char *src,const char *dst) {if(++rename_count==fail_rename) {errno=EIO;return -1;}return renameat(AT_FDCWD,src,AT_FDCWD,dst);}
long orbis_syscall(long number,...) {
 va_list ap;va_start(ap,number);long rc=0;
 if(number==107) {struct proc *p=va_arg(ap,struct proc*);uint64_t *n=va_arg(ap,uint64_t*);if(p) {memset(p,0,sizeof(*p));p->pid=42;strcpy(p->name,"eboot.bin");}*n=1;}
 else if(number==109) {uint64_t pid=va_arg(ap,uint64_t),cmd=va_arg(ap,uint64_t);struct info *p=va_arg(ap,struct info*);if(pid!=42 || cmd!=8)rc=-1;else {memset(p,0,sizeof(*p));p->pid=42;strcpy(p->title,running?"CUSA12345":"NPXS20001");}}
 else rc=-1;
 va_end(ap);return rc;
}
'''
def sfo(title='Fortnite',identity='CUSA12345'):
 keys=b'TITLE\0TITLE_ID\0';a=title.encode()+b'\0';b=identity.encode()+b'\0';keypos=52;datapos=(keypos+len(keys)+3)&~3
 header=struct.pack('<5I',0x46535000,0x101,keypos,datapos,2)
 entries=struct.pack('<HHIII',0,0x204,len(a),len(a),0)+struct.pack('<HHIII',6,0x204,len(b),len(b),len(a))
 return header+entries+keys+b'\0'*(datapos-keypos-len(keys))+a+b
class ResetTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.root=pathlib.Path(cls.tmp.name);(cls.root/'mock.c').write_text(MOCK)
  lib=cls.root/'reset.so'
  subprocess.run(['cc','-shared','-fPIC','-O2','-Wall','-Wextra',f'-DBH_APPMETA_ROOT="{cls.root}/appmeta"',f'-DBH_DOWNLOAD_ROOT="{cls.root}/download"','-Drename=bh_test_rename',str(SRC),str(cls.root/'mock.c'),'-o',str(lib)],check=True)
  cls.lib=C.CDLL(str(lib));cls.lib.bh_fortnite_reset.argtypes=[C.c_void_p,C.c_size_t];cls.lib.mock_reset.argtypes=[C.c_int,C.c_int]
 @classmethod
 def tearDownClass(cls):cls.tmp.cleanup()
 def setUp(self):
  for name in ['appmeta','download']:shutil.rmtree(self.root/name,ignore_errors=True)
  self.meta=self.root/'appmeta/CUSA12345';self.meta.mkdir(parents=True);(self.meta/'param.sfo').write_bytes(sfo())
  self.files=self.root/'download/CUSA12345';self.files.mkdir(parents=True)
  self.a=self.files/'download0.dat';self.b=self.files/'download0_information.dat';self.a.write_bytes(b'original-a');self.b.write_bytes(b'original-b')
  (self.files/'unrelated.dat').write_bytes(b'keep');self.err=C.create_string_buffer(2048);self.lib.mock_reset(0,0)
 def run_reset(self):return self.lib.bh_fortnite_reset(self.err,len(self.err))
 def unchanged(self):self.assertEqual(self.a.read_bytes(),b'original-a');self.assertEqual(self.b.read_bytes(),b'original-b')
 def test_moves_exact_pair_into_recovery_folder(self):
  self.assertEqual(self.run_reset(),0,self.err.value);self.assertFalse(self.a.exists());self.assertFalse(self.b.exists())
  backup=list(self.files.glob('.BalenciHEN-backup-*'));self.assertEqual(len(backup),1)
  self.assertEqual((backup[0]/self.a.name).read_bytes(),b'original-a');self.assertEqual((backup[0]/self.b.name).read_bytes(),b'original-b');self.assertEqual((self.files/'unrelated.dat').read_bytes(),b'keep')
 def test_missing_second_file_no_partial_reset(self):
  self.b.unlink();self.assertEqual(self.run_reset(),-1);self.assertEqual(self.a.read_bytes(),b'original-a');self.assertEqual(list(self.files.glob('.BalenciHEN*')),[])
 def test_running_game_refused(self):self.lib.mock_reset(1,0);self.assertEqual(self.run_reset(),-1);self.unchanged()
 def test_second_move_failure_restores_first(self):self.lib.mock_reset(0,2);self.assertEqual(self.run_reset(),-1);self.unchanged();self.assertEqual(list(self.files.glob('.BalenciHEN*')),[])
 def test_wrong_game_title_refused(self):(self.meta/'param.sfo').write_bytes(sfo(title='Another Game'));self.assertEqual(self.run_reset(),-1);self.unchanged()
 def test_identity_mismatch_refused(self):(self.meta/'param.sfo').write_bytes(sfo(identity='CUSA99999'));self.assertEqual(self.run_reset(),-1);self.unchanged()
 def test_file_symlink_refused(self):
  self.a.unlink();target=self.root/'target';target.write_bytes(b'keep');self.a.symlink_to(target);self.assertEqual(self.run_reset(),-1);self.assertEqual(target.read_bytes(),b'keep')
 def test_ambiguous_installations_refused(self):
  p=self.root/'appmeta/CUSA54321';p.mkdir();(p/'param.sfo').write_bytes(sfo(identity='CUSA54321'));self.assertEqual(self.run_reset(),-1);self.unchanged()
 def test_malformed_metadata_refused(self):(self.meta/'param.sfo').write_bytes(b'not a valid metadata file');self.assertEqual(self.run_reset(),-1);self.unchanged()
if __name__=='__main__':unittest.main(verbosity=2)

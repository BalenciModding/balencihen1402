import ctypes as C
import json
import pathlib
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
SRC=ROOT/'source/plugins/plugin_mono/source/cheat'
class EngineTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory()
  lib=pathlib.Path(cls.tmp.name)/'engine.so'
  subprocess.run(['cc','-shared','-fPIC','-O2','-Wall','-Wextra',str(SRC/'engine.c'),str(SRC/'cJSON.c'),'-lm','-o',str(lib)],check=True)
  cls.lib=C.CDLL(str(lib))
  cls.lib.bh_db_open.argtypes=[C.c_char_p]*4+[C.c_void_p,C.c_size_t];cls.lib.bh_db_open.restype=C.c_void_p
  cls.lib.bh_db_close.argtypes=[C.c_void_p]
  cls.lib.bh_db_supported.argtypes=[C.c_void_p,C.c_size_t]
  cls.lib.bh_db_enabled.argtypes=[C.c_void_p,C.c_size_t]
  cls.CB=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_uint64,C.POINTER(C.c_ubyte),C.c_size_t,C.c_int)
  cls.lib.bh_db_toggle.argtypes=[C.c_void_p,C.c_size_t,cls.CB,C.c_void_p,C.c_void_p,C.c_size_t]
 @classmethod
 def tearDownClass(cls):cls.tmp.cleanup()
 def setUp(self):
  self.mem=bytearray(256);self.err=C.create_string_buffer(256);self.writes=0;self.fail_at=None;self.db=None
  def rw(ctx,off,buf,n,write):
   if off+n>len(self.mem):return -1
   if write:
    self.writes+=1
    if self.fail_at and self.writes==self.fail_at:return -1
    self.mem[off:off+n]=bytes(buf[:n])
   else:C.memmove(buf,bytes(self.mem[off:off+n]),n)
   return 0
  self.rw=self.CB(rw)
 def tearDown(self):
  if self.db:self.lib.bh_db_close(self.db)
 def open(self,mods=None,version='01.00'):
  if mods is None:mods=[{'name':'Test <cheat>','type':'checkbox','memory':[{'offset':'10','on':'AABB','off':'0000'}]}]
  doc={'name':'Test','id':'CUSA12345','version':version,'process':'eboot.bin','mods':mods}
  self.db=self.lib.bh_db_open(json.dumps(doc).encode(),b'CUSA12345',b'01.00',b'eboot.bin',self.err,256)
  return self.db
 def toggle(self,i=0):return self.lib.bh_db_toggle(self.db,i,self.rw,None,self.err,256)
 def test_enable_disable_restores_snapshot(self):
  self.assertTrue(self.open());self.assertEqual(self.toggle(),0);self.assertEqual(self.mem[16:18],bytes.fromhex('AABB'));self.assertEqual(self.lib.bh_db_enabled(self.db,0),1)
  self.assertEqual(self.toggle(),0);self.assertEqual(self.mem[16:18],b'\0\0');self.assertEqual(self.lib.bh_db_enabled(self.db,0),0)
 def test_exact_version_required(self):self.assertFalse(self.open(version='01.01'));self.assertEqual(self.writes,0)
 def test_preflight_mismatch_makes_no_writes(self):
  self.open();self.mem[16]=1;self.assertEqual(self.toggle(),-1);self.assertEqual(self.writes,0)
 def test_disable_drift_does_not_overwrite(self):
  self.open();self.toggle();self.mem[16]=0xCC;before=self.writes;self.assertEqual(self.toggle(),-1);self.assertEqual(self.mem[16],0xCC);self.assertEqual(self.writes,before)
 def test_failed_second_write_rolls_back_first(self):
  self.open([{'name':'multi','type':'checkbox','memory':[{'offset':'10','on':'AA','off':'00'},{'offset':'20','on':'BB','off':'00'}]}]);self.fail_at=2
  self.assertEqual(self.toggle(),-1);self.assertEqual(self.mem,bytearray(256));self.assertEqual(self.lib.bh_db_enabled(self.db,0),0)
 def test_unsupported_section_and_button(self):
  self.open([{'name':'section','type':'checkbox','memory':[{'offset':'10','on':'AA','off':'00','section':1}]},{'name':'button','type':'button','memory':[{'offset':'20','on':'BB','off':'00'}]}])
  for i in range(2):self.assertEqual(self.lib.bh_db_supported(self.db,i),0);self.assertEqual(self.toggle(i),-1)
  self.assertEqual(self.writes,0)
 def test_bad_hex_unsupported(self):
  self.open([{'name':'bad','type':'checkbox','memory':[{'offset':'10','on':'XY','off':'00'}]}]);self.assertEqual(self.toggle(),-1);self.assertEqual(self.writes,0)
 def test_overlap_rejected(self):
  self.open([{'name':n,'type':'checkbox','memory':[{'offset':'10','on':'AA','off':'00'}]} for n in ['a','b']]);self.assertEqual(self.toggle(0),0);before=self.writes;self.assertEqual(self.toggle(1),-1);self.assertEqual(self.writes,before)
 def test_unmapped_address_rejected_before_write(self):
  self.open([{'name':'unmapped','type':'checkbox','memory':[{'offset':'400','on':'AA','off':'00'}]}]);self.assertEqual(self.toggle(),-1);self.assertEqual(self.writes,0)
if __name__=='__main__':unittest.main(verbosity=2)

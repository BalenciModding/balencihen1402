"""Upload the bundled JSON cheat snapshot to BalenciHEN on your LAN."""
import argparse
import ftplib
import getpass
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('ps4_ip');p.add_argument('--port',type=int,default=2121)
p.add_argument('--user',default='anonymous');p.add_argument('--ask-password',action='store_true')
a=p.parse_args();password=getpass.getpass('FTP password: ') if a.ask_password else 'anonymous'
source=Path(__file__).resolve().parent/'cheats'
with ftplib.FTP() as ftp:
 ftp.connect(a.ps4_ip,a.port,timeout=30);ftp.login(a.user,password)
 root='/data/BalenciHEN/cheats'
 for folder in ['/data/BalenciHEN',root,root+'/json']:
  try:ftp.mkd(folder)
  except ftplib.error_perm:
   ftp.cwd(folder)  # Existing directories are fine; other errors remain visible.
 files=sorted((source/'json').glob('*.json'))
 for index,file in enumerate(files,1):
  with file.open('rb') as stream:ftp.storbinary('STOR '+root+'/json/'+file.name,stream)
  if index%100==0 or index==len(files):print(f'Uploaded {index}/{len(files)} JSON files')
 for name in ['REPOSITORY_LICENSE','REPOSITORY_README.md','SOURCE.txt']:
  with (source/name).open('rb') as stream:ftp.storbinary('STOR '+root+'/'+name,stream)
print('Cheat database upload completed. No cheats were enabled.')

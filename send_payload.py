import argparse, socket
from pathlib import Path
p=argparse.ArgumentParser(description='Send a trusted raw userland .bin to BalenciHEN on your LAN')
p.add_argument('ps4_ip'); p.add_argument('payload'); a=p.parse_args()
b=Path(a.payload).read_bytes()
if not 5 <= len(b) <= 4*1024*1024 or b[0] != 0xe9:
    p.error('Expected raw position-independent .bin starting E9, 5 bytes to 4 MiB; ELF and PRX are not accepted')
with socket.create_connection((a.ps4_ip,9090),timeout=20) as s:
    s.sendall(b); s.shutdown(socket.SHUT_WR)
    while s.recv(1024): pass
print('Transfer completed. Verify payload behavior on the console; transfer alone does not prove execution.')

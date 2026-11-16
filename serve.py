from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
import socket
class Handler(SimpleHTTPRequestHandler):
    extensions_map = {**SimpleHTTPRequestHandler.extensions_map, '.manifest': 'text/cache-manifest', '.js': 'application/javascript', '.bin': 'application/octet-stream'}
    def __init__(self,*args,**kwargs):
        super().__init__(*args,directory=str(Path(__file__).resolve().parent/'site'),**kwargs)
if __name__ == '__main__':
    print('BalenciHEN local host: http://localhost:8088')
    print('On PS4 use http://YOUR-PC-LAN-IP:8088 (find IPv4 with ipconfig).')
    print('Same LAN required. Ctrl+C stops server.')
    try: ThreadingHTTPServer(('0.0.0.0',8088), Handler).serve_forever()
    except KeyboardInterrupt: pass


import socket, struct, sys

GRUPO, PUERTO = "224.1.1.1", 5007
MI_IP = sys.argv[1] if len(sys.argv) > 1 else "0.0.0.0"

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("", PUERTO))

mreq = struct.pack("4s4s", socket.inet_aton(GRUPO), socket.inet_aton(MI_IP))
s.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

print(f"Nodo {MI_IP} esperando mensajes del grupo {GRUPO}...")
while True:
    datos, origen = s.recvfrom(1024)
    print(f"Recibido de {origen}: {datos.decode()}")
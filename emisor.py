import socket, sys

GRUPO, PUERTO = "224.1.1.1", 5007
mensaje = sys.argv[1] if len(sys.argv) > 1 else "Hola grupo"
MI_IP = sys.argv[2] if len(sys.argv) > 2 else "0.0.0.0"

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
s.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
s.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(MI_IP))
s.sendto(mensaje.encode(), (GRUPO, PUERTO))
print(f"Enviado desde {MI_IP}: {mensaje}")
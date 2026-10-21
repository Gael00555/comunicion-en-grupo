# Comunicación en grupo (multicast UDP)

Grupo multicast `224.1.1.1`, puerto `5007`.

## Archivos
- `nodo.py`: miembro del grupo (receptor). Se ejecuta en cada nodo.
- `emisor.py`: envía un mensaje (número o texto) al grupo.

## Ejecución
Nodos (una terminal por nodo):
    python nodo.py <IP_local>

Emisor externo (cuarta terminal/computadora):
    python emisor.py "mensaje" <IP_local>

## Nota
Los 3 nodos y el emisor externo se simularon con terminales
en una sola computadora.

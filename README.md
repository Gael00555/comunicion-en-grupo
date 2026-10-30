# Sincronización de relojes – Algoritmo de Cristian (C# / ASP.NET Core)

Requisitos: .NET 8 SDK.

## Ejecución (4 terminales)

```bash
# Terminal 1 – Servidor de tiempo (http://localhost:5000)
cd TimeServer && dotnet run

# Terminales 2, 3 y 4 – Clientes (nombre, desfase inicial simulado en ms)
cd TimeClient
dotnet run -- Cliente1  5000
dotnet run -- Cliente2 -3000
dotnet run -- Cliente3  8000
```

Parámetros opcionales: `[rondas] [urlServidor]`.

## Algoritmo
1. T0 = hora del reloj lógico al enviar.
2. El servidor responde con Tserver (`GET /api/time`).
3. T1 = hora del reloj lógico al recibir; RTT = T1 − T0 (medido con Stopwatch).
4. Hora estimada = Tserver + RTT/2.
5. Corrección = estimada − hora al recibir; se suma al offset del `LogicalClock`.
6. El reloj del sistema operativo nunca se modifica.

El servidor simula latencia aleatoria (40–200 ms) repartida de forma simétrica,
lo que cumple la hipótesis de Cristian. Error máximo ≈ ±RTT/2.

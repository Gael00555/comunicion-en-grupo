using System.Diagnostics;
using System.Net.Http.Json;

namespace TimeClient;

public record TimeResponse(DateTime ServerUtcTime);

public record SyncResult(
    DateTime T0,            // instante de envío (reloj lógico del cliente)
    DateTime T1,            // instante de recepción (reloj lógico del cliente)
    TimeSpan Rtt,
    DateTime ServerTime,    // Tserver recibido
    DateTime Estimated,     // Tserver + RTT/2
    DateTime Before,        // hora del cliente justo al recibir, antes de ajustar
    TimeSpan Correction);   // Estimated - Before

public class CristianSynchronizer
{
    private readonly HttpClient _http;
    private readonly LogicalClock _clock;

    public CristianSynchronizer(HttpClient http, LogicalClock clock)
    {
        _http = http;
        _clock = clock;
    }

    public async Task<SyncResult> SyncAsync()
    {
        // 2. Registrar el instante de envío
        var t0 = _clock.Now;
        var sw = Stopwatch.StartNew();

        // 1. Consultar la hora del servidor (API REST)
        var response = await _http.GetFromJsonAsync<TimeResponse>("api/time")
                       ?? throw new InvalidOperationException("Respuesta vacía del servidor.");

        // 3. Registrar el instante de recepción
        sw.Stop();
        var t1 = _clock.Now;

        // 4. RTT (Stopwatch: más preciso que restar dos DateTime)
        var rtt = sw.Elapsed;

        // 5. Algoritmo de Cristian: T_cliente = T_servidor + RTT / 2
        var estimated = response.ServerUtcTime + rtt / 2;

        // 6. Ajustar el reloj lógico (no el del SO)
        var before = t1;
        var correction = estimated - before;
        _clock.Adjust(correction);

        return new SyncResult(t0, t1, rtt, response.ServerUtcTime, estimated, before, correction);
    }
}

using TimeClient;

// Uso: dotnet run -- <nombre> <desfaseInicialMs> [rondas] [urlServidor]
// Ej.:  dotnet run -- Cliente1 5000
string name        = args.Length > 0 ? args[0] : "Cliente1";
int skewMs         = args.Length > 1 ? int.Parse(args[1]) : 3000;
int rounds         = args.Length > 2 ? int.Parse(args[2]) : 5;
string serverUrl   = args.Length > 3 ? args[3] : "http://localhost:5000/";

var clock = new LogicalClock(TimeSpan.FromMilliseconds(skewMs));
using var http = new HttpClient { BaseAddress = new Uri(serverUrl) };
var sync = new CristianSynchronizer(http, clock);

const string F = "HH:mm:ss.fff";
Console.WriteLine($"=== {name} | desfase inicial simulado: {skewMs} ms ===\n");

for (int i = 1; i <= rounds; i++)
{
    try
    {
        var r = await sync.SyncAsync();

        Console.WriteLine($"[{name}] Sincronización #{i}");
        Console.WriteLine($"  T0 (envío)             : {r.T0.ToString(F)}");
        Console.WriteLine($"  T1 (recepción)         : {r.T1.ToString(F)}");
        Console.WriteLine($"  RTT                    : {r.Rtt.TotalMilliseconds:F2} ms");
        Console.WriteLine($"  Hora servidor          : {r.ServerTime.ToString(F)}");
        Console.WriteLine($"  Hora estimada (Cristian): {r.Estimated.ToString(F)}  (Tserver + RTT/2)");
        Console.WriteLine($"  Hora anterior          : {r.Before.ToString(F)}");
        Console.WriteLine($"  Hora sincronizada      : {clock.Now.ToString(F)}");
        Console.WriteLine($"  Diferencia (ajuste)    : {r.Correction.TotalMilliseconds:+0.00;-0.00} ms");
        Console.WriteLine($"  Precisión máx. (±RTT/2): ±{r.Rtt.TotalMilliseconds / 2:F2} ms\n");
    }
    catch (Exception ex)
    {
        Console.WriteLine($"[{name}] Error al sincronizar: {ex.Message}\n");
    }

    await Task.Delay(2000);
}

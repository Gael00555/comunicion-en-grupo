using Microsoft.AspNetCore.Mvc;

namespace TimeServer.Controllers;

public record TimeResponse(DateTime ServerUtcTime);

[ApiController]
[Route("api/[controller]")]
public class TimeController : ControllerBase
{
    private static readonly Random Rng = new();

    /// <summary>
    /// Devuelve la hora UTC del servidor (reloj de referencia).
    /// Para simular una red real, se introduce una latencia aleatoria repartida
    /// mitad antes de leer la hora y mitad después (red aproximadamente simétrica,
    /// que es la hipótesis del algoritmo de Cristian).
    /// </summary>
    [HttpGet]
    public async Task<ActionResult<TimeResponse>> Get([FromQuery] int minDelayMs = 40, [FromQuery] int maxDelayMs = 200)
    {
        int delay = Rng.Next(minDelayMs, maxDelayMs + 1);

        await Task.Delay(delay / 2);          // latencia de ida (simulada)
        var serverTime = DateTime.UtcNow;     // instante en que se lee el reloj
        await Task.Delay(delay - delay / 2);  // latencia de vuelta (simulada)

        return Ok(new TimeResponse(serverTime));
    }
}

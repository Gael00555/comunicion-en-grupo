namespace TimeClient;

/// <summary>
/// Reloj lógico de la aplicación. NO modifica el reloj del sistema operativo:
/// guarda un desfase (offset) que se suma a DateTime.UtcNow.
/// El desfase inicial simula un reloj físico desincronizado.
/// </summary>
public class LogicalClock
{
    private TimeSpan _offset;

    public LogicalClock(TimeSpan initialSkew) => _offset = initialSkew;

    public DateTime Now => DateTime.UtcNow + _offset;

    public TimeSpan Offset => _offset;

    public void Adjust(TimeSpan correction) => _offset += correction;
}

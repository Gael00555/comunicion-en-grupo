

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using namespace std::chrono;

static long long g_offset_ms = 0;  // Desfase simulado del reloj del maestro

long long ahora_ms() {
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() + g_offset_ms;
}

string formato_hora(long long ms) {
    time_t s = ms / 1000;
    struct tm tm_info;
    localtime_r(&s, &tm_info);
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", &tm_info);
    char out[48];
    snprintf(out, sizeof(out), "%s.%03d", buf, (int)(ms % 1000));
    return out;
}

bool enviar_linea(int fd, const string& msg) {
    string s = msg + "\n";
    size_t total = 0;
    while (total < s.size()) {
        ssize_t n = send(fd, s.c_str() + total, s.size() - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

bool recibir_linea(int fd, string& linea) {
    linea.clear();
    char c;
    while (true) {
        ssize_t n = recv(fd, &c, 1, 0);
        if (n <= 0) return false;
        if (c == '\n') return true;
        linea += c;
    }
}

struct Esclavo {
    int fd;
    string ip;
    long long diff;  // diferencia estimada respecto al maestro (ms)
    bool ok;         // respondió correctamente
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Uso: " << argv[0]
             << " <puerto> <num_esclavos> [umbral_ms=500] [offset_maestro_ms=0] [rondas=3]\n";
        return 1;
    }
    int puerto = atoi(argv[1]);
    int n_esclavos = atoi(argv[2]);
    long long umbral = argc > 3 ? atoll(argv[3]) : 500;
    g_offset_ms = argc > 4 ? atoll(argv[4]) : 0;
    int rondas = argc > 5 ? atoi(argv[5]) : 3;

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return 1; }
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(puerto);
    if (bind(srv, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(srv, n_esclavos) < 0) { perror("listen"); return 1; }

    cout << "[Maestro] Escuchando en el puerto " << puerto
         << ", esperando " << n_esclavos << " esclavo(s)...\n";

    vector<Esclavo> esclavos;
    while ((int)esclavos.size() < n_esclavos) {
        sockaddr_in cli{};
        socklen_t len = sizeof(cli);
        int fd = accept(srv, (sockaddr*)&cli, &len);
        if (fd < 0) { perror("accept"); continue; }
        string ip = inet_ntoa(cli.sin_addr);
        esclavos.push_back({fd, ip, 0, true});
        cout << "[Maestro] Esclavo #" << esclavos.size() << " conectado desde " << ip << "\n";
    }

    for (int r = 1; r <= rondas; r++) {
        cout << "\n================ RONDA " << r << " ================\n";
        cout << "[Maestro] Hora del maestro: " << formato_hora(ahora_ms()) << "\n";

        // --- 1 y 2: consultar a cada esclavo y compensar el RTT ---
        for (size_t i = 0; i < esclavos.size(); i++) {
            Esclavo& e = esclavos[i];
            if (!e.ok) continue;

            long long t0 = ahora_ms();
            if (!enviar_linea(e.fd, "GET_TIME")) { e.ok = false; continue; }
            string resp;
            if (!recibir_linea(e.fd, resp) || resp.rfind("TIME ", 0) != 0) { e.ok = false; continue; }
            long long t1 = ahora_ms();

            long long t_esclavo = atoll(resp.c_str() + 5);
            long long rtt = t1 - t0;
            long long t_esclavo_est = t_esclavo + rtt / 2;  // hora del esclavo en el instante t1
            e.diff = t_esclavo_est - t1;                    // respecto al maestro

            cout << "[Maestro] Esclavo #" << i + 1 << " (" << e.ip << "): hora="
                 << formato_hora(t_esclavo) << "  RTT=" << rtt << " ms"
                 << "  diferencia=" << e.diff << " ms\n";
        }

        // --- 4 y 5: descartar valores atípicos y promediar ---
        long long suma = 0;  // el maestro aporta diferencia 0
        int contador = 1;
        for (auto& e : esclavos) {
            if (!e.ok) continue;
            if (llabs(e.diff) <= umbral) {
                suma += e.diff;
                contador++;
            } else {
                cout << "[Maestro] Diferencia de " << e.ip << " (" << e.diff
                     << " ms) supera el umbral de " << umbral << " ms: se excluye del promedio\n";
            }
        }
        double promedio = (double)suma / contador;
        cout << "[Maestro] Diferencia promedio = " << promedio << " ms (sobre "
             << contador << " reloj(es))\n";

        // --- 6: enviar ajustes ---
        for (size_t i = 0; i < esclavos.size(); i++) {
            Esclavo& e = esclavos[i];
            if (!e.ok) continue;
            long long ajuste = llround(promedio - (double)e.diff);
            if (!enviar_linea(e.fd, "ADJUST " + to_string(ajuste))) { e.ok = false; continue; }
            cout << "[Maestro] Ajuste enviado a esclavo #" << i + 1 << ": " << ajuste << " ms\n";
        }
        long long ajuste_maestro = llround(promedio);
        g_offset_ms += ajuste_maestro;
        cout << "[Maestro] Ajuste propio: " << ajuste_maestro
             << " ms -> nueva hora: " << formato_hora(ahora_ms()) << "\n";

        if (r < rondas) this_thread::sleep_for(seconds(3));
    }

    for (auto& e : esclavos) {
        enviar_linea(e.fd, "BYE");
        close(e.fd);
    }
    close(srv);
    cout << "\n[Maestro] Sincronización finalizada.\n";
    return 0;
}


// El offset inicial permite simular un reloj desajustado (adelantado si es
// positivo, atrasado si es negativo) aunque se ejecute todo en una sola máquina.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

using namespace std;
using namespace std::chrono;

static long long g_offset_ms = 0;  // Desfase del reloj lógico del esclavo

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

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Uso: " << argv[0] << " <ip_maestro> <puerto> [offset_inicial_ms=0]\n";
        return 1;
    }
    string ip = argv[1];
    int puerto = atoi(argv[2]);
    g_offset_ms = argc > 3 ? atoll(argv[3]) : 0;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(puerto);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        cerr << "Dirección IP inválida\n";
        return 1;
    }
    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("connect"); return 1; }

    cout << "[Esclavo] Conectado al maestro " << ip << ":" << puerto << "\n";
    cout << "[Esclavo] Hora inicial: " << formato_hora(ahora_ms())
         << " (offset simulado " << g_offset_ms << " ms)\n";

    string msg;
    while (recibir_linea(fd, msg)) {
        if (msg == "GET_TIME") {
            long long t = ahora_ms();
            enviar_linea(fd, "TIME " + to_string(t));
            cout << "[Esclavo] Hora enviada al maestro: " << formato_hora(t) << "\n";
        } else if (msg.rfind("ADJUST ", 0) == 0) {
            long long delta = atoll(msg.c_str() + 7);
            g_offset_ms += delta;  // ajuste relativo (no se retrocede el reloj físico)
            cout << "[Esclavo] Ajuste recibido: " << delta << " ms -> nueva hora: "
                 << formato_hora(ahora_ms()) << "\n";
        } else if (msg == "BYE") {
            break;
        }
    }

    close(fd);
    cout << "[Esclavo] Finalizado.\n";
    return 0;
}

#include <iostream>
#include <fstream>
#include <iomanip>
#include <random>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <numeric>

#ifdef _WIN32
#include <direct.h>
#define CREAR_DIRECTORIO(x) _mkdir(x)
#define DIRECTORIO_ACTUAL _getcwd
#else
#include <unistd.h>
#include <sys/stat.h>
#define CREAR_DIRECTORIO(x) mkdir(x, 0755)
#define DIRECTORIO_ACTUAL getcwd
#endif

using namespace std;

const int NUM_PUNTOS = 100;
const vector<int> DIMENSIONES = {10, 50, 100, 500, 1000, 2000, 5000};

string directorioActual() {
    char buffer[4096];
    if (DIRECTORIO_ACTUAL(buffer, sizeof(buffer)) != NULL) return string(buffer);
    return string(".");
}

mt19937 gen(random_device{}());
uniform_real_distribution<double> dis(0.0, 1.0);

vector<double> generarPunto(int dimensiones){
    vector<double> punto(dimensiones);
    for (int i = 0; i < dimensiones; ++i) {
        punto[i] = dis(gen);
    }
    return punto;
}

vector<vector<double>> generarConjunto(int n, int dimensiones){
    vector<vector<double>> puntos;
    puntos.reserve(n);
    for (int i = 0; i < n; ++i) {
        puntos.push_back(generarPunto(dimensiones));
    }
    return puntos;
}

double distancia(vector<double>& a,vector<double>& b){
    double suma = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        double delta = b[i] - a[i];
        suma += delta * delta;
    }
    return sqrt(suma);
}

vector<double> distanciasEntrePares(vector<vector<double>>& puntos){
    size_t n = puntos.size();
    vector<double> distancias;
    distancias.reserve(n * (n - 1) / 2);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            distancias.push_back(distancia(puntos[i], puntos[j]));
        }
    }
    return distancias;
}

void guardarCSV(vector<double>& distancias, const string& archivo){
    ofstream out(archivo);
    if (!out) {
        cerr << "Error: no se pudo escribir " << archivo << endl;
        return;
    }
    out << "distancia\n";
    out << fixed << setprecision(6);
    for (double d : distancias) {
        out << d << "\n";
    }
}

void imprimirResumen(int d,vector<double>& dist) {
    double suma  = accumulate(dist.begin(), dist.end(), 0.0);
    double media = suma / dist.size();

    double dmin = *min_element(dist.begin(), dist.end());
    double dmax = *max_element(dist.begin(), dist.end());

    cout << fixed << setprecision(4);
    cout << setw(6)  << d
         << setw(10) << dist.size()
         << setw(10) << dmin
         << setw(10) << dmax
         << setw(10) << media
         << setw(10) << (dmax - dmin)
         << setw(12) << (dmax - dmin) / dmin
         << endl;
}

int main(int argc, char *argv[]){
    CREAR_DIRECTORIO("data");
    CREAR_DIRECTORIO("figuras");
    cout << "Directorio de trabajo: " << directorioActual() << endl;
    cout << "Los CSV se guardan en: " << directorioActual()
         << "/data" << endl << endl;

    cout << setw(6)  << "dim"
         << setw(10) << "pares"
         << setw(10) << "min" 
         << setw(10) << "max"
         << setw(10) << "media" 
         << setw(10) << "ancho"
         << setw(12) << "contraste"
         << endl; 
    cout << string(68, '-') << endl;

    for (int d : DIMENSIONES) {
        vector<vector<double>> puntos = generarConjunto(NUM_PUNTOS, d);
        vector<double> dist = distanciasEntrePares(puntos);
        guardarCSV(dist, "data/distancias_d" + to_string(d) + ".csv");
        imprimirResumen(d, dist);
    }

    cout << endl << "Archivos generados en "
         << directorioActual() << "/data" << endl;

    return 0;
}

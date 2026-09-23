#include "Octree.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace std;

string ARCHIVO = "puntos1.xyz";
int N = 100;
Point A(0, 0, 0);
int RADIO = 25;	
bool MOSTRAR_ARBOL = false;

vector<Point> leerXYZ(string archivo) {
    vector<Point> v;
    ifstream in(archivo.c_str());
    if (!in) {
        cout << "Error: no se encontro " << archivo << endl;
        return v;
    }
    string linea;
    while (getline(in, linea)) {
        if (linea.empty()) continue;
        istringstream ss(linea);
        double x, y, z;
        if (ss >> x >> y >> z)
            v.push_back(Point((int)llround(x), (int)llround(y), (int)llround(z)));
    }
    return v;
}

int main() {
    vector<Point> datos = leerXYZ(ARCHIVO);
    if (datos.empty()) return 1;

    int minx = datos[0].x, miny = datos[0].y, minz = datos[0].z;
    int maxx = minx, maxy = miny, maxz = minz;
    for (size_t i = 1; i < datos.size(); ++i) {
        minx = min(minx, datos[i].x); maxx = max(maxx, datos[i].x);
        miny = min(miny, datos[i].y); maxy = max(maxy, datos[i].y);
        minz = min(minz, datos[i].z); maxz = max(maxz, datos[i].z);
    }
    int extension = max(max(maxx - minx, maxy - miny), maxz - minz) + 1;
    int lado = 1;
    while (lado < extension) lado *= 2;

    Octree arbol(Point(minx, miny, minz), (double)lado, N);
    for (size_t i = 0; i < datos.size(); ++i) arbol.insert(datos[i]);

    Point bl = arbol.bottomLeftRaiz();
    Point X = arbol.find_closest(A, RADIO);

    cout << fixed << setprecision(2);
    cout << "bottomLeft raiz = (" << bl.x << ", " << bl.y << ", " << bl.z << ")" << endl;
    cout << "h raiz          = " << arbol.ladoRaiz() << endl;

    if (X == NULL_POINT) {
        cout << "X               = NULL" << endl;
        cout << "h del nodo de X = NULL" << endl;
    } else {
        cout << "X               = (" << X.x << ", " << X.y << ", " << X.z << ")" << endl;
        cout << "h del nodo de X = " << arbol.hDeX() << endl;
    }

    if (MOSTRAR_ARBOL) {
        cout << endl;
        arbol.imprimir(2);
    }

    return 0;
}

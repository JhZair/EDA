#include "Octree.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

using namespace std;

Octree::Octree() {
    for (int i = 0; i < 8; ++i) children[i] = NULL;
    bottomLeft = Point(0, 0, 0);
    h = 0.0;
    nPoints = 0;
    capacidad = 100;
    hoja = true;
    ultimoH = -1.0;
}

Octree::Octree(const Point& bl, double lado, int cap) {
    for (int i = 0; i < 8; ++i) children[i] = NULL;
    bottomLeft = bl;
    h = lado;
    nPoints = 0;
    capacidad = cap;
    hoja = true;
    ultimoH = -1.0;
}

Octree::~Octree() {
    for (int i = 0; i < 8; ++i) delete children[i];
}

int Octree::octante(const Point& p) {
    double cx = bottomLeft.x + h / 2.0;
    double cy = bottomLeft.y + h / 2.0;
    double cz = bottomLeft.z + h / 2.0;
    int i = 0;
    if (p.x >= cx) i |= 1;
    if (p.y >= cy) i |= 2;
    if (p.z >= cz) i |= 4;
    return i;
}

Point Octree::bottomLeftHijo(int i) {
    double m = h / 2.0;
    int bx = bottomLeft.x + ((i & 1) ? (int)m : 0);
    int by = bottomLeft.y + ((i & 2) ? (int)m : 0);
    int bz = bottomLeft.z + ((i & 4) ? (int)m : 0);
    return Point(bx, by, bz);
}

void Octree::subdividir() {
    for (int i = 0; i < 8; ++i)
        children[i] = new Octree(bottomLeftHijo(i), h / 2.0, capacidad);
    hoja = false;
    for (size_t k = 0; k < points.size(); ++k) {
        int i = octante(points[k]);
        children[i]->insert(points[k]);
    }
    points.clear();
}

void Octree::insert(const Point& p) {
    if (p.x < bottomLeft.x || p.x > bottomLeft.x + h) return;
    if (p.y < bottomLeft.y || p.y > bottomLeft.y + h) return;
    if (p.z < bottomLeft.z || p.z > bottomLeft.z + h) return;

    nPoints++;

    if (!hoja) {
        children[octante(p)]->insert(p);
        return;
    }

    points.push_back(p);

    if ((int)points.size() > capacidad && h > 1.0) {
        subdividir();
    }
}

bool Octree::exist(const Point& p) {
    if (p.x < bottomLeft.x || p.x > bottomLeft.x + h) return false;
    if (p.y < bottomLeft.y || p.y > bottomLeft.y + h) return false;
    if (p.z < bottomLeft.z || p.z > bottomLeft.z + h) return false;

    if (!hoja) return children[octante(p)]->exist(p);

    for (size_t k = 0; k < points.size(); ++k)
        if (points[k] == p) return true;
    return false;
}

double Octree::distanciaAlCubo(const Point& p) {
    double dx = 0.0, dy = 0.0, dz = 0.0;
    if (p.x < bottomLeft.x) dx = bottomLeft.x - p.x;
    else if (p.x > bottomLeft.x + h) dx = p.x - (bottomLeft.x + h);
    if (p.y < bottomLeft.y) dy = bottomLeft.y - p.y;
    else if (p.y > bottomLeft.y + h) dy = p.y - (bottomLeft.y + h);
    if (p.z < bottomLeft.z) dz = bottomLeft.z - p.z;
    else if (p.z > bottomLeft.z + h) dz = p.z - (bottomLeft.z + h);
    return sqrt(dx * dx + dy * dy + dz * dz);
}

void Octree::buscarCercano(const Point& p, double radio,
                           Point& mejor, double& mejorDist, double& hMejor) {
    if (nPoints == 0) return;
    if (distanciaAlCubo(p) > min(radio, mejorDist)) return;

    if (hoja) {
        for (size_t k = 0; k < points.size(); ++k) {
            double dx = points[k].x - p.x;
            double dy = points[k].y - p.y;
            double dz = points[k].z - p.z;
            double d = sqrt(dx * dx + dy * dy + dz * dz);
            if (d == 0.0) continue;
            if (d <= radio && d < mejorDist) {
                mejorDist = d;
                mejor = points[k];
                hMejor = h;
            }
        }
        return;
    }

    int orden[8];
    for (int i = 0; i < 8; ++i) orden[i] = i;
    int primero = octante(p);
    swap(orden[0], orden[primero]);

    for (int k = 0; k < 8; ++k)
        children[orden[k]]->buscarCercano(p, radio, mejor, mejorDist, hMejor);
}

Point Octree::find_closest(const Point& p, int radius) {
    Point mejor = NULL_POINT;
    double mejorDist = 1e18;
    double hMejor = -1.0;
    buscarCercano(p, (double)radius, mejor, mejorDist, hMejor);
    ultimoH = hMejor;
    return mejor;
}

double Octree::hDeX() { return ultimoH; }
Point Octree::bottomLeftRaiz() { return bottomLeft; }
double Octree::ladoRaiz() { return h; }
int Octree::totalPuntos() { return nPoints; }

int Octree::contarHojas() {
    if (hoja) return 1;
    int s = 0;
    for (int i = 0; i < 8; ++i) s += children[i]->contarHojas();
    return s;
}

int Octree::altura() {
    if (hoja) return 1;
    int m = 0;
    for (int i = 0; i < 8; ++i) m = max(m, children[i]->altura());
    return 1 + m;
}

void Octree::imprimirRec(int nivel, int maxProfundidad, int indice) {
    for (int i = 0; i < nivel; ++i) cout << "   ";
    if (indice >= 0) cout << "[" << indice << "] ";
    cout << "bl=(" << bottomLeft.x << "," << bottomLeft.y << "," << bottomLeft.z
         << ") h=" << h << " n=" << nPoints;
    if (hoja) cout << " HOJA(" << points.size() << ")";
    cout << endl;

    if (hoja || nivel >= maxProfundidad) return;
    for (int i = 0; i < 8; ++i)
        if (children[i]->totalPuntos() > 0)
            children[i]->imprimirRec(nivel + 1, maxProfundidad, i);
}

void Octree::imprimir(int maxProfundidad) {
    imprimirRec(0, maxProfundidad, -1);
}

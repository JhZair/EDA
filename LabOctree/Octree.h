#ifndef OCTREE_H
#define OCTREE_H

#include <vector>

struct Point {
    int x;
    int y;
    int z;

    Point() : x(0), y(0), z(0) {}
    Point(int a, int b, int c) : x(a), y(b), z(c) {}

    bool operator==(const Point& o) {
        return x == o.x && y == o.y && z == o.z;
    }
};

const Point NULL_POINT(-2147483647, -2147483647, -2147483647);

class Octree {
private:
    Octree* children[8];
    std::vector<Point> points;
    Point bottomLeft;
    double h;
    int nPoints;
    int capacidad;
    bool hoja;
    double ultimoH;

    int octante(const Point& p);
    Point bottomLeftHijo(int i);
    void subdividir();
    double distanciaAlCubo(const Point& p);
    void buscarCercano(const Point& p, double radio,
                       Point& mejor, double& mejorDist, double& hMejor);
    void imprimirRec(int nivel, int maxProfundidad, int indice);

public:
    Octree();
    Octree(const Point& bl, double lado, int cap);
    ~Octree();

    bool exist(const Point& p);
    void insert(const Point& p);
    Point find_closest(const Point& p, int radius);

    double hDeX();
    Point bottomLeftRaiz();
    double ladoRaiz();
    int totalPuntos();
    int contarHojas();
    int altura();
    void imprimir(int maxProfundidad);
};

#endif

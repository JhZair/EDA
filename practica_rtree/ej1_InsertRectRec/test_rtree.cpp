// ===========================================================================
//  Banco de pruebas para practicar las funciones del RTree del examen.
//
//  Compilar:  g++ -std=c++14 -o test RTree.cpp verificador.cpp test_rtree.cpp
//  Ejecutar:  ./test
//
//  Inserta rectangulos uno por uno y despues de CADA insercion verifica las
//  propiedades (1) a (6) del paper. Si tu implementacion rompe alguna, te
//  dice cual y en que insercion.
// ===========================================================================
#include "RTree.h"
#include <iostream>
#include <vector>
using namespace std;

static vector<pair<int,int>> Poligono(int xmin, int ymin, int xmax, int ymax)
{
	vector<pair<int,int>> p;
	p.push_back(make_pair(xmin, ymin));
	p.push_back(make_pair(xmax, ymax));
	return p;
}

struct Caso { int xmin, ymin, xmax, ymax; };

int main()
{
	Caso datos[] = {
		{  0,  0, 10, 10}, {  5,  5, 15, 15}, { 50,  0, 60, 10},
		{ 55,  5, 65, 15}, { 20, 40, 30, 50}, {  0, 10, 10, 20},
		{ 25, 45, 35, 55}, { 10,  0, 20, 10}, { 70, 70, 80, 80},
		{ 72, 65, 78, 75}, { 40, 20, 45, 25}, { 90, 90, 95, 95},
		{  3,  3,  7,  7}, { 60, 30, 70, 40}, { 15, 60, 25, 70},
	};
	int n = sizeof(datos) / sizeof(datos[0]);

	RTree arbol;
	int fallos = 0;

	cout << "=== Insertando " << n << " rectangulos ===\n";
	for (int i = 0; i < n; ++i)
	{
		int mn[2] = { datos[i].xmin, datos[i].ymin };
		int mx[2] = { datos[i].xmax, datos[i].ymax };
		vector<pair<int,int>> pol = Poligono(datos[i].xmin, datos[i].ymin,
		                                     datos[i].xmax, datos[i].ymax);
		arbol.Insert(mn, mx, pol);

		string err = arbol.CheckTree();
		if (!err.empty())
		{
			cout << "  FALLO tras insertar el #" << i + 1 << ": " << err << "\n";
			fallos++;
			if (fallos >= 3) { cout << "  (se detiene tras 3 fallos)\n"; break; }
		}
	}

	int guardados = arbol.Count();
	cout << "\nregistros esperados : " << n << "\n";
	cout << "registros contados  : " << guardados
	     << (guardados == n ? "   OK" : "   <-- NO COINCIDE") << "\n";
	cout << "altura del arbol    : " << arbol.Altura() << "\n";
	cout << "propiedades (1)-(6) : "
	     << (arbol.CheckTree().empty() ? "OK" : arbol.CheckTree()) << "\n";

	cout << "\n=== Borrando 3 rectangulos ===\n";
	int aBorrar[] = { 0, 4, 8 };
	for (int k = 0; k < 3; ++k)
	{
		int i = aBorrar[k];
		int mn[2] = { datos[i].xmin, datos[i].ymin };
		int mx[2] = { datos[i].xmax, datos[i].ymax };
		vector<pair<int,int>> pol = Poligono(datos[i].xmin, datos[i].ymin,
		                                     datos[i].xmax, datos[i].ymax);
		arbol.Remove(mn, mx, pol);

		string err = arbol.CheckTree();
		cout << "  borrado #" << i + 1 << ": "
		     << (err.empty() ? "arbol valido" : err) << "\n";
	}

	cout << "\nregistros tras borrar: " << arbol.Count()
	     << (arbol.Count() == n - 3 ? "   OK" : "   <-- NO COINCIDE") << "\n";

	const long long FIRMA_CORRECTA = 129889876L;
	long long firma = arbol.Firma();
	cout << "firma del arbol      : " << firma;
	if (firma == FIRMA_CORRECTA) cout << "   OK (igual a la implementacion correcta)\n";
	else cout << "   <-- DISTINTA (la correcta es " << FIRMA_CORRECTA << ")\n";

	bool todoBien = fallos == 0 && arbol.CheckTree().empty()
	             && arbol.Count() == n - 3 && firma == FIRMA_CORRECTA;
	cout << "\n>>> " << (todoBien ? "TODO CORRECTO" : "HAY ERRORES, revisa arriba") << "\n";

	return 0;
}

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>

using namespace std;

struct Point{
	vector<int> coords;
	int dimensiones = 1;
	Point(int dim){
		dimensiones = dim;
		for(int i=0;i<dimensiones;i++){
			coords.push_back(0);
		}
	}
};

int distEuclid(Point& a, Point& b){
	if(a.dimensiones != b.dimensiones)return 0;
	int sum = 0;
	for(int i=0;i<a.dimensiones;i++){
		sum+=pow((b.coords[i])-(a.coords[i]),2);
	}
	return sqrt(sum);
}

Point llenarPuntos(string archivo, int dim){
	std::ifstream archivo(archivo);
	
	if (!archivo.is_open()) {
		std::cout << "No se pudo abrir el archivo." << std::endl;
		return 1;
	}
	
	std::string linea;
	
	// Opcional: saltar la primera línea si es el encabezado
	std::getline(archivo, linea);
	
	Point punto;
	int dimensiones=0;
	while (std::getline(archivo, linea)) {
		dimensiones++;
		std::stringstream ss(linea);
		std::string columna1;
		
		std::getline(ss, columna1, ',');
		
		punto.coords.push_back(columna1);
		std::cout << "Col 1: " << columna1 << endl;
	}
	punto.dimensiones = dimensiones;
	
	archivo.close();
	return punto;
}
	
int main() {
	
	return 0;
}


//int main(int argc, char *argv[]) {
//	
//	return 0;
//}

// qué otras distacias hay? (2 más)
//

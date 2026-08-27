#include <iostream>
#include <random>
#include <vector>
#include <cmath>

using namespace std;

vector<double> GenerarCoords(int dimensiones){
	vector<double> Puntos;
	std::random_device rd;  
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0.0, 1.0);
	for (int n = 0; n < dimensiones; ++n){
		Puntos.push_back(dis(gen));
//		std::cout << Puntos[n] << ' '; //ver puntos
	}
	return Puntos;
}
void llenarPuntos(vector<vector<double>>& p, int dimensiones){
	for(int i=0;i<100;i++){
		vector<double> Punto = GenerarCoords(dimensiones);
		p.push_back(Punto);
	}
}
double distancia(vector<double>& a, vector<double>& b){
	if(a.size() != b.size())return -1;
	double sum = 0;
	for(int i=0;i<a.size();i++){
		sum+=pow(b[i]-a[i],2);
	}
	return sqrt(sum);
}
void distanciasEntrePuntos(vector<vector<double>>& puntos, vector<double>& distancias){
	for(int i = 0;i<puntos.size();i++){
		for(int j = 0;j<puntos.size();j++){
			double t = distancia(puntos[i],puntos[j]);
			if(t==0) continue;
			distancias.push_back(t);
			cout << t << endl;
		}
	}
}
int main(int argc, char *argv[]) {
	vector<vector<double>> Puntos10;
	llenarPuntos(Puntos10, 10);
//	for(int i = 0;i<Puntos10.size();i++){
//		cout << "-----------------" << endl;
//		for(int j = 0;j<Puntos10[i].size();j++){
//			cout << Puntos10[i][j];
//		}
//	}
//	cout << Puntos10.size()<< endl;
	vector<double> distancias;
	distanciasEntrePuntos(Puntos10, distancias);
	
	cout << distancias.size() << endl;
	return 0;
}


//verificar la cantidad de bins para hacer el histograma
//pq deben ser 4950 distancias?

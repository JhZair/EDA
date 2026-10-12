// ============================================================================
//  R-tree 
//  Para Compilar:  g++ -std=c++17  rtree_guttman.cpp -o rtree
// ============================================================================

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

// N�mero de dimensiones 
constexpr int DIM = 2;

const double INF = numeric_limits<double>::infinity();

// ---------------------------------------------------------------------------
//  Secci�n 2: I = (I0, I1, ..., In-1), donde cada Ii es un intervalo
//  cerrado [a, b] que describe la extensi�n del objeto en la dimensi�n i.
// ---------------------------------------------------------------------------
struct Rect {
	array<double, DIM> lo;   // l�mite inferior en cada dimensi�n
	array<double, DIM> hi;   // l�mite superior en cada dimensi�n
	
	double area() const {
		double a = 1.0;
		for (int d = 0; d < DIM; d++) a *= hi[d] - lo[d];
		return a;
	}
	
	bool overlaps(const Rect& o) const {
		for (int d = 0; d < DIM; d++)
			if (lo[d] > o.hi[d] || o.lo[d] > hi[d]) return false;
		return true;
	}
	
	bool operator==(const Rect& o) const { return lo == o.lo && hi == o.hi; }
	
	// Cada intervalo [a, b] debe tener extremos finitos y cumplir a <= b.
	// Se exige isfinite porque NaN e infinito rompen el algoritmo:
	//  - toda comparaci�n con NaN da false, as� que "lo > hi" no lo detecta;
	//  - NaN != NaN, as� que FL2 nunca encontrar�a el registro para borrarlo;
	//  - inf - inf = NaN, lo que corrompe �reas y separaciones.
	bool isValid() const {
		for (int d = 0; d < DIM; d++)
			if (!isfinite(lo[d]) || !isfinite(hi[d]) || lo[d] > hi[d])
				return false;
		return true;
	}
};

// Rect�ngulo m�s peque�o que contiene a 'a' y a 'b'
Rect combine(const Rect& a, const Rect& b) {
	Rect r;
	for (int d = 0; d < DIM; d++) {
		r.lo[d] = min(a.lo[d], b.lo[d]);
		r.hi[d] = max(a.hi[d], b.hi[d]);
	}
	return r;
}

// Cu�nto debe crecer el �rea de 'r' para incluir a 'add'
double enlargement(const Rect& r, const Rect& add) {
	return combine(r, add).area() - r.area();
}

ostream& operator<<(ostream& os, const Rect& r) {
	os << "[(";
	for (int d = 0; d < DIM; d++) os << r.lo[d] << (d + 1 < DIM ? "," : "");
	os << ")-(";
	for (int d = 0; d < DIM; d++) os << r.hi[d] << (d + 1 < DIM ? "," : "");
	return os << ")]";
}

// ---------------------------------------------------------------------------
//  Secci�n 2: entradas y nodos
//    - Entrada de hoja:    (I, tuple-identifier)
//    - Entrada interna:    (I, child-pointer)
// ---------------------------------------------------------------------------
struct Node;

struct Entry {
	Rect  I;
	Node* child;   // child-pointer      (solo en nodos internos)
	int   tuple;        // tuple-identifier   (solo en hojas)
};

struct Node {
	explicit Node(int lvl) : level(lvl) {}
	int level;                    // 0 = hoja
	vector<Entry> entries;
	bool isLeaf() const { return level == 0; }
};

// Rect�ngulo que encierra todas las entradas de un nodo
Rect coveringRect(const Node* n) {
	Rect r = n->entries[0].I;
	for (const auto& e : n->entries) r = combine(r, e.I);
	return r;
}

// Rect�ngulo que encierra un grupo de entradas
Rect coveringRect(const vector<Entry>& g) {
	Rect r = g[0].I;
	for (const auto& e : g) r = combine(r, e.I);
	return r;
}

// Algoritmos de divisi�n de nodos de la secci�n 3.5
enum class SplitAlgorithm { Exhaustive, Quadratic, Linear };

// ===========================================================================
//  R-tree
//
//  Propiedades (secci�n 2):
//   (1) Todo nodo hoja tiene entre m y M registros, salvo que sea la ra�z.
//   (2) Cada registro (I, tuple) de una hoja: I es el menor rect�ngulo que
//       contiene al objeto.
//   (3) Todo nodo interno tiene entre m y M hijos, salvo que sea la ra�z.
//   (4) Cada entrada (I, child) de un nodo interno: I es el menor rect�ngulo
//       que contiene a los rect�ngulos del nodo hijo.
//   (5) La ra�z tiene al menos dos hijos, salvo que sea hoja.
//   (6) Todas las hojas est�n en el mismo nivel.
// ===========================================================================
class RTree {
public:
	RTree(int maxEntries, int minEntries, SplitAlgorithm splitAlg)
		: M(maxEntries), m(minEntries), alg(splitAlg), root(new Node(0)) {
		// Secci�n 2: m <= M/2. Adem�s se exige m >= 2: con m = 1 un nodo
		// no ra�z podr�a tener un solo hijo, y D4 (que acorta el �rbol una
		// sola vez) no bastar�a para mantener la propiedad (5).
		if (m < 2 || m > M / 2)
			throw invalid_argument("se requiere 2 <= m <= M/2");
		// El split exhaustivo eval�a ~2^M particiones por cada split.
		// Con M = 20 son ~1 mill�n por split: en clase conviene M <= 8.
		if (alg == SplitAlgorithm::Exhaustive && M > 20)
			throw invalid_argument("el algoritmo exhaustivo solo es viable con M peque�o");
	}
	~RTree() { destroySubtree(root); }
	RTree(const RTree&) = delete;
	RTree& operator=(const RTree&) = delete;
	
	// ----------------------------------------------------------------------
	//  3.1 Search: encontrar todos los registros cuyo rect�ngulo se
	//  superpone con el rect�ngulo de b�squeda S.
	// ----------------------------------------------------------------------
	vector<int> search(const Rect& S) const {
		// Una consulta con NaN "se superpondr�a" con todo (las comparaciones
		// con NaN dan false), as� que tambi�n se valida S.
		if (!S.isValid())
			throw invalid_argument("rectangulo de consulta invalido");
		vector<int> result;
		search(root, S, result);
		return result;
	}
	
	// ----------------------------------------------------------------------
	//  3.2 Insert: insertar un nuevo registro (I, tuple)
	// ----------------------------------------------------------------------
	void insert(const Rect& I, int tuple) {
		if (!I.isValid())
			throw invalid_argument("rectangulo invalido: se requieren coordenadas finitas y lo <= hi");
		insertEntry(Entry{I, nullptr, tuple}, 0);
	}
	
	// ----------------------------------------------------------------------
	//  3.3 Delete: eliminar el registro (I, tuple)
	//  I debe ser exactamente el mismo rect�ngulo con que se insert�.
	// ----------------------------------------------------------------------
	bool remove(const Rect& I, int tuple) {
		Entry E{I, nullptr, tuple};
		
		// D1. [Encontrar el nodo que contiene el registro]
		//     Invocar FindLeaf para ubicar la hoja L que contiene a E.
		//     Detenerse si no se encontr� el registro.
		vector<Node*> path;
		if (!findLeaf(root, E, path)) return false;
		Node* L = path.back();
		
		// D2. [Eliminar el registro] Quitar E de L.
		for (size_t i = 0; i < L->entries.size(); i++) {
			if (L->entries[i].I == E.I && L->entries[i].tuple == E.tuple) {
				L->entries.erase(L->entries.begin() + i);
				break;
			}
		}
		
		// D3. [Propagar cambios] Invocar CondenseTree pasando L.
		condenseTree(path);
		
		// D4. [Acortar el �rbol] Si la ra�z tiene un solo hijo despu�s de
		//     ajustar el �rbol, hacer que ese hijo sea la nueva ra�z.
		if (!root->isLeaf() && root->entries.size() == 1) {
			Node* oldRoot = root;
			root = root->entries[0].child;
			delete oldRoot;
		}
		return true;
	}
	
	// ----------------------------------------------------------------------
	//  3.4 Update: si cambia el rect�ngulo de un objeto, su registro se
	//  elimina, se actualiza y se vuelve a insertar.
	// ----------------------------------------------------------------------
	bool update(const Rect& oldI, const Rect& newI, int tuple) {
		if (!newI.isValid())
			throw invalid_argument("rectangulo invalido: se requieren coordenadas finitas y lo <= hi");
		if (!remove(oldI, tuple)) return false;
		insert(newI, tuple);
		return true;
	}
	
	int height() const { return root->level + 1; }
	
	// Cantidad de registros almacenados
	int size() const { return countRecords(root); }
	
	// Verifica las propiedades (1), (3), (4), (5) y (6) de la secci�n 2.
	// La (2) depende de los datos del usuario y no se puede verificar aqu�.
	// Devuelve "" si se cumplen, o la descripci�n de la primera violaci�n.
	string checkProperties() const { return checkNode(root, true); }
	
	void print() const {
		cout << "(altura = " << height() << ")\n";
		print(root, 0);
	}
	
private:
		const int M;               // m�ximo de entradas por nodo
		const int m;               // m�nimo de entradas por nodo (m <= M/2)
		const SplitAlgorithm alg;  // algoritmo de split a usar
		Node* root;
		
		// ======================================================================
		//  3.1 Search
		// ======================================================================
		void search(const Node* T, const Rect& S, vector<int>& out) const {
			if (!T->isLeaf()) {
				// S1. [Buscar en sub�rboles] Si T no es hoja, revisar cada
				//     entrada E para ver si E.I se superpone con S. Para cada
				//     entrada que se superpone, invocar Search en el sub�rbol
				//     cuya ra�z es apuntada por E.child.
				for (const auto& E : T->entries)
					if (E.I.overlaps(S)) search(E.child, S, out);
			} else {
				// S2. [Buscar en el nodo hoja] Si T es hoja, revisar todas las
				//     entradas E para ver si E.I se superpone con S. Si es as�,
				//     E es un registro que cumple la condici�n.
				for (const auto& E : T->entries)
					if (E.I.overlaps(S)) out.push_back(E.tuple);
			}
		}
		
		// ======================================================================
		//  3.2 Insert
		//  'level' indica el nivel del nodo donde debe ir la entrada:
		//  0 para registros nuevos; mayor que 0 al reinsertar en CT6.
		// ======================================================================
		void insertEntry(const Entry& E, int level) {
			// I1. [Encontrar posici�n para el nuevo registro]
			//     Invocar ChooseLeaf para seleccionar la hoja L donde poner E.
			vector<Node*> path = chooseLeaf(E.I, level);
			Node* L = path.back();
			
			// I2. [Agregar el registro al nodo hoja]
			//     Si L tiene espacio, instalar E. Si no, invocar SplitNode para
			//     obtener L y LL, que contienen a E y a todas las entradas de L.
			L->entries.push_back(E);
			Node* LL = nullptr;
			if ((int)L->entries.size() > M)
				LL = splitNode(L);
			
			// I3. [Propagar cambios hacia arriba]
			//     Invocar AdjustTree con L, y tambi�n con LL si hubo split.
			Node* rootSplit = adjustTree(path, LL);
			
			// I4. [Hacer crecer el �rbol en altura]
			//     Si la propagaci�n hizo que la ra�z se dividiera, crear una
			//     nueva ra�z cuyos hijos son los dos nodos resultantes.
			if (rootSplit) {
				Node* newRoot = new Node(root->level + 1);
				newRoot->entries.push_back(Entry{coveringRect(root), root, -1});
				newRoot->entries.push_back(Entry{coveringRect(rootSplit), rootSplit, -1});
				root = newRoot;
			}
		}
		
		// ----------------------------------------------------------------------
		//  ChooseLeaf: selecciona la hoja donde colocar una nueva entrada.
		//  Devuelve el camino desde la ra�z hasta el nodo elegido.
		// ----------------------------------------------------------------------
		vector<Node*> chooseLeaf(const Rect& I, int level) const {
			// CL1. [Inicializar] N = ra�z.
			Node* N = root;
			vector<Node*> path{N};
			
			// CL2. [Verificar hoja] Si N es hoja (o est� en el nivel pedido),
			//      devolver N.
			while (N->level > level) {
				// CL3. [Elegir sub�rbol] Sea F la entrada de N cuyo rect�ngulo
				//      F.I necesita crecer menos para incluir I. Los empates se
				//      resuelven eligiendo la entrada con rect�ngulo de menor �rea.
				const Entry* F = nullptr;
				double bestEnl = INF, bestArea = INF;
				for (const auto& e : N->entries) {
					double enl  = enlargement(e.I, I);
					double area = e.I.area();
					if (enl < bestEnl || (enl == bestEnl && area < bestArea)) {
						F = &e;
						bestEnl = enl;
						bestArea = area;
					}
				}
				// CL4. [Descender hasta llegar a una hoja] N = F.child;
				//      repetir desde CL2.
				N = F->child;
				path.push_back(N);
			}
			return path;
		}
		
		// ----------------------------------------------------------------------
		//  AdjustTree: sube desde la hoja L hasta la ra�z, ajustando los
		//  rect�ngulos y propagando los splits. Devuelve el nodo hermano de la
		//  ra�z si la ra�z se dividi�, o nullptr.
		// ----------------------------------------------------------------------
		Node* adjustTree(const vector<Node*>& path, Node* LL) {
			// AT1. [Inicializar] N = L. Si L se dividi�, NN = LL.
			int i = (int)path.size() - 1;
			Node* N  = path[i];
			Node* NN = LL;
			
			// AT2. [Verificar si termin�] Si N es la ra�z, detenerse.
			while (N != root) {
				// AT3. [Ajustar el rect�ngulo en la entrada del padre]
				//      Sea P el padre de N y E_N la entrada de N en P. Ajustar
				//      E_N.I para que encierre exactamente a todos los
				//      rect�ngulos de N.
				Node* P = path[i - 1];
				entryFor(P, N).I = coveringRect(N);
				
				// AT4. [Propagar el split hacia arriba]
				//      Si N tiene un compa�ero NN de un split anterior, crear
				//      una entrada E_NN con E_NN.child = NN y E_NN.I encerrando
				//      a todos los rect�ngulos de NN. Agregar E_NN a P si hay
				//      espacio; si no, invocar SplitNode para obtener P y PP.
				Node* PP = nullptr;
				if (NN) {
					P->entries.push_back(Entry{coveringRect(NN), NN, -1});
					if ((int)P->entries.size() > M)
						PP = splitNode(P);
				}
				
				// AT5. [Subir al siguiente nivel] N = P, y NN = PP si hubo
				//      split. Repetir desde AT2.
				N = P;
				NN = PP;
				i--;
			}
			return NN;
		}
		
		// ======================================================================
		//  3.3 Delete
		// ======================================================================
		
		// ----------------------------------------------------------------------
		//  FindLeaf: encuentra la hoja que contiene el registro E.
		//  Deja en 'path' el camino desde la ra�z hasta esa hoja.
		// ----------------------------------------------------------------------
		bool findLeaf(Node* T, const Entry& E, vector<Node*>& path) {
			path.push_back(T);
			if (!T->isLeaf()) {
				// FL1. [Buscar en sub�rboles] Si T no es hoja, revisar cada
				//      entrada F de T para ver si F.I se superpone con E.I. Para
				//      cada una, invocar FindLeaf en el sub�rbol apuntado por
				//      F.child hasta encontrar E o revisar todas las entradas.
				for (const auto& F : T->entries)
					if (F.I.overlaps(E.I) && findLeaf(F.child, E, path))
						return true;
			} else {
				// FL2. [Buscar en el nodo hoja] Si T es hoja, revisar cada
				//      entrada para ver si coincide con E. Si se encuentra,
				//      devolver T.
				for (const auto& e : T->entries)
					if (e.I == E.I && e.tuple == E.tuple)
						return true;
			}
			path.pop_back();
			return false;
		}
		
		// ----------------------------------------------------------------------
		//  CondenseTree: dada la hoja L de la que se elimin� una entrada,
		//  elimina los nodos que quedaron con pocas entradas y reinserta sus
		//  entradas. Ajusta los rect�ngulos en el camino hacia la ra�z.
		// ----------------------------------------------------------------------
		void condenseTree(const vector<Node*>& path) {
			// CT1. [Inicializar] N = L. Q (conjunto de nodos eliminados) = vac�o.
			int i = (int)path.size() - 1;
			Node* N = path[i];
			vector<Node*> Q;
			
			// CT2. [Encontrar la entrada en el padre] Si N es la ra�z, ir a CT6.
			//      Si no, sea P el padre de N y E_N la entrada de N en P.
			while (N != root) {
				Node* P = path[i - 1];
				
				if ((int)N->entries.size() < m) {
					// CT3. [Eliminar nodo con pocas entradas] Si N tiene menos
					//      de m entradas, quitar E_N de P y agregar N a Q.
					for (size_t k = 0; k < P->entries.size(); k++) {
						if (P->entries[k].child == N) {
							P->entries.erase(P->entries.begin() + k);
							break;
						}
					}
					Q.push_back(N);
				} else {
					// CT4. [Ajustar el rect�ngulo] Si N no fue eliminado,
					//      ajustar E_N.I para que contenga exactamente a todas
					//      las entradas de N.
					entryFor(P, N).I = coveringRect(N);
				}
				
				// CT5. [Subir un nivel] N = P; repetir desde CT2.
				N = P;
				i--;
			}
			
			// CT6. [Reinsertar entradas hu�rfanas] Reinsertar todas las entradas
			//      de los nodos en Q. Las entradas de hojas eliminadas se
			//      reinsertan en hojas como en Insert; las de nodos de niveles
			//      superiores deben ponerse m�s arriba en el �rbol, de modo que
			//      las hojas de sus sub�rboles queden al mismo nivel que las
			//      hojas del �rbol principal.
			for (Node* q : Q) {
				for (const auto& e : q->entries)
					insertEntry(e, q->level);
				delete q;   // solo el nodo; sus sub�rboles fueron reinsertados
			}
		}
		
		// ======================================================================
		//  3.5 Node Splitting
		//  Divide un nodo con M+1 entradas en dos nodos: N se queda con el
		//  grupo 1 y se devuelve un nodo nuevo con el grupo 2.
		// ======================================================================
		Node* splitNode(Node* N) {
			vector<Entry> E = N->entries;
			vector<Entry> G1, G2;
			
			switch (alg) {
			case SplitAlgorithm::Exhaustive: exhaustiveSplit(E, G1, G2); break;
			case SplitAlgorithm::Quadratic:  splitWithSeeds(E, G1, G2, false); break;
			case SplitAlgorithm::Linear:     splitWithSeeds(E, G1, G2, true);  break;
			}
			
			N->entries = G1;
			Node* NN = new Node(N->level);
			NN->entries = G2;
			return NN;
		}
		
		// ----------------------------------------------------------------------
		//  3.5.1 Exhaustive Algorithm
		//  Generar todas las agrupaciones posibles y elegir la que minimiza el
		//  �rea total de los dos rect�ngulos. El n�mero de agrupaciones es
		//  aproximadamente 2^(M-1), por lo que solo es pr�ctico con M peque�o.
		// ----------------------------------------------------------------------
		void exhaustiveSplit(const vector<Entry>& E, vector<Entry>& G1, vector<Entry>& G2) {
			int n = (int)E.size();
			double bestArea = INF;
			unsigned bestMask = 0;
			
			// Bit k de 'mask' en 1 => la entrada k va al grupo 2.
			// La entrada 0 siempre va al grupo 1 (mask par), as� cada
			// partici�n se eval�a una sola vez.
			for (unsigned mask = 0; mask < (1u << n); mask += 2) {
				vector<Entry> A, B;
				for (int k = 0; k < n; k++)
					((mask >> k) & 1 ? B : A).push_back(E[k]);
				if ((int)A.size() < m || (int)B.size() < m) continue;
				
				double total = coveringRect(A).area() + coveringRect(B).area();
				if (total < bestArea) {
					bestArea = total;
					bestMask = mask;
				}
			}
			
			for (int k = 0; k < n; k++)
				((bestMask >> k) & 1 ? G2 : G1).push_back(E[k]);
		}
		
		// ----------------------------------------------------------------------
		//  3.5.2 Quadratic Split  y  3.5.3 Linear Split
		//  Ambos comparten los pasos QS1-QS3. Seg�n el paper, el algoritmo
		//  lineal es id�ntico al cuadr�tico salvo dos cosas:
		//    - usa LinearPickSeeds en lugar de PickSeeds, y
		//    - su PickNext elige cualquier entrada restante.
		//  linear = false -> Quadratic Split;  linear = true -> Linear Split
		// ----------------------------------------------------------------------
		void splitWithSeeds(vector<Entry> E, vector<Entry>& G1, vector<Entry>& G2, bool linear) {
			// QS1. [Elegir la primera entrada de cada grupo] Aplicar PickSeeds
			//      para elegir dos entradas que ser�n las primeras de cada
			//      grupo. Asignar cada una a un grupo.
			int s1, s2;
			if (linear) linearPickSeeds(E, s1, s2);
			else        pickSeeds(E, s1, s2);
			
			// Invariante: las dos semillas son entradas distintas. Ambas
			// funciones lo garantizan por construcci�n (comparan �ndices).
			assert(s1 != s2);
			
			G1.push_back(E[s1]);
			G2.push_back(E[s2]);
			Rect R1 = E[s1].I, R2 = E[s2].I;
			E.erase(E.begin() + max(s1, s2));
			E.erase(E.begin() + min(s1, s2));
			
			while (true) {
				// QS2. [Verificar si termin�] Si todas las entradas fueron
				//      asignadas, detenerse. Si un grupo tiene tan pocas
				//      entradas que todas las restantes deben asign�rsele para
				//      que llegue al m�nimo m, asignarlas y detenerse.
				if (E.empty()) break;
				if ((int)(G1.size() + E.size()) <= m) {
					G1.insert(G1.end(), E.begin(), E.end());
					break;
				}
				if ((int)(G2.size() + E.size()) <= m) {
					G2.insert(G2.end(), E.begin(), E.end());
					break;
				}
				// QS3. [Seleccionar entrada a asignar] Invocar PickNext para
				//      elegir la siguiente entrada. Agregarla al grupo cuyo
				//      rect�ngulo necesite crecer menos para incluirla. Los
				//      empates se resuelven agreg�ndola al grupo de menor �rea,
				//      luego al de menos entradas, luego a cualquiera.
				//      Repetir desde QS2.
				int k = linear ? linearPickNext(E) : pickNext(E, R1, R2);
				Entry e = E[k];
				E.erase(E.begin() + k);
				
				double d1 = enlargement(R1, e.I);
				double d2 = enlargement(R2, e.I);
				bool toG1;
				if (d1 != d2)
					toG1 = d1 < d2;
				else if (R1.area() != R2.area())   
					toG1 = R1.area() < R2.area();
				else                               
					toG1 = G1.size() <= G2.size();
				
				if (toG1) { G1.push_back(e); R1 = combine(R1, e.I); }
				else      { G2.push_back(e); R2 = combine(R2, e.I); }
			}
		}
		
		// ----------------------------------------------------------------------
		//  PickSeeds (cuadr�tico): elegir las dos entradas que ser�n las
		//  primeras de cada grupo.
		// ----------------------------------------------------------------------
		void pickSeeds(const vector<Entry>& E, int& s1, int& s2) const {
			s1 = 0; s2 = 1;   // valor inicial defensivo (E siempre tiene M+1 >= 5 entradas)
			// PS1. [Calcular la ineficiencia de agrupar entradas] Para cada par
			//      de entradas E1 y E2, construir el rect�ngulo J que incluye a
			//      E1.I y E2.I. Calcular d = area(J) - area(E1.I) - area(E2.I).
			// PS2. [Elegir el par m�s derrochador] Elegir el par con mayor d.
			double worst = -INF;
			for (int i = 0; i < (int)E.size(); i++) {
				for (int j = i + 1; j < (int)E.size(); j++) {
					double d = combine(E[i].I, E[j].I).area() - E[i].I.area() - E[j].I.area();
					if (d > worst) { worst = d; s1 = i; s2 = j; }
				}
			}
		}
		
		// ----------------------------------------------------------------------
		//  PickNext (cuadr�tico): elegir una entrada restante para asignarla.
		// ----------------------------------------------------------------------
		int pickNext(const vector<Entry>& E, const Rect& R1, const Rect& R2) const {
			// PN1. [Determinar el costo de poner cada entrada en cada grupo]
			//      Para cada entrada E no asignada, calcular d1 = crecimiento
			//      del rect�ngulo del grupo 1 para incluir E.I; y d2 igual
			//      para el grupo 2.
			// PN2. [Encontrar la entrada con mayor preferencia por un grupo]
			//      Elegir la entrada con la m�xima diferencia entre d1 y d2.
			int best = 0;
			double maxDiff = -INF;
			for (int i = 0; i < (int)E.size(); i++) {
				double d1 = enlargement(R1, E[i].I);
				double d2 = enlargement(R2, E[i].I);
				double diff = d1 > d2 ? d1 - d2 : d2 - d1;
				if (diff > maxDiff) { maxDiff = diff; best = i; }
			}
			return best;
		}
		
		// ----------------------------------------------------------------------
		//  3.5.3 LinearPickSeeds
		// ----------------------------------------------------------------------
		void linearPickSeeds(const vector<Entry>& E, int& s1, int& s2) const {
			s1 = 0; s2 = 1;   // valor inicial defensivo (E siempre tiene M+1 >= 5 entradas)
			int n = (int)E.size();
			double bestSep = -INF;
			
			for (int d = 0; d < DIM; d++) {
				// LPS1. [Encontrar rect�ngulos extremos en todas las dimensiones]
				//       En cada dimensi�n, encontrar la entrada cuyo rect�ngulo
				//       tiene el lado inferior m�s alto y la que tiene el lado
				//       superior m�s bajo. Registrar la separaci�n.
				int highestLow = 0;
				for (int i = 1; i < n; i++)
					if (E[i].I.lo[d] > E[highestLow].I.lo[d]) highestLow = i;
				
				int lowestHigh = (highestLow == 0) ? 1 : 0;
				for (int i = 0; i < n; i++)
					if (i != highestLow && E[i].I.hi[d] < E[lowestHigh].I.hi[d])
						lowestHigh = i;
				
				double separation = E[highestLow].I.lo[d] - E[lowestHigh].I.hi[d];
				
				// LPS2. [Ajustar por la forma del conjunto de rect�ngulos]
				//       Normalizar las separaciones dividi�ndolas por el ancho
				//       del conjunto completo en la dimensi�n correspondiente.
				double minLo = INF, maxHi = -INF;
				for (const auto& e : E) {
					minLo = min(minLo, e.I.lo[d]);
					maxHi = max(maxHi, e.I.hi[d]);
				}
				double width = maxHi - minLo;
				double normalized = (width > 0) ? separation / width : 0.0;
				
				// LPS3. [Seleccionar el par m�s extremo] Elegir el par con la
				//       mayor separaci�n normalizada en cualquier dimensi�n.
				if (normalized > bestSep) {
					bestSep = normalized;
					s1 = highestLow;
					s2 = lowestHigh;
				}
			}
		}
		
		// 3.5.3: en el algoritmo lineal, PickNext simplemente elige cualquiera
		// de las entradas restantes.
		int linearPickNext(const vector<Entry>&) const { return 0; }
		
		// ======================================================================
		//  Utilidades
		// ======================================================================
		
		// Entrada de P que apunta al nodo hijo N  (E_N en el paper)
		static Entry& entryFor(Node* P, Node* N) {
			for (auto& e : P->entries)
				if (e.child == N) return e;
			throw logic_error("el nodo no es hijo de P");
		}
		
		int countRecords(const Node* n) const {
			if (n->isLeaf()) return (int)n->entries.size();
			int total = 0;
			for (const auto& e : n->entries) total += countRecords(e.child);
			return total;
		}
		
		string checkNode(const Node* n, bool isRoot) const {
			int k = (int)n->entries.size();
			if (k > M)
				return "un nodo tiene mas de M entradas";
			if (!isRoot && k < m)
				return "propiedad (1)/(3): un nodo no raiz tiene menos de m entradas";
			if (isRoot && !n->isLeaf() && k < 2)
				return "propiedad (5): la raiz interna tiene menos de dos hijos";
			for (const auto& e : n->entries) {
				if (n->isLeaf()) {
					if (e.child) return "una entrada de hoja tiene puntero a hijo";
					continue;
				}
				if (!e.child) return "una entrada interna no tiene hijo";
				// Los niveles bajan de a uno hasta 0 => todas las hojas al mismo nivel
				if (e.child->level != n->level - 1)
					return "propiedad (6): las hojas no estan todas al mismo nivel";
				if (e.child->entries.empty())
					return "un nodo hijo esta vacio";
				if (!(e.I == coveringRect(e.child)))
					return "propiedad (4): el rectangulo de una entrada no es el minimo que cubre a su hijo";
				string err = checkNode(e.child, false);
				if (!err.empty()) return err;
			}
			return "";
		}
		
		static void destroySubtree(Node* n) {
			if (!n->isLeaf())
				for (auto& e : n->entries) destroySubtree(e.child);
			delete n;
		}
		
		void print(const Node* n, int depth) const {
			string indent(depth * 4, ' ');
			cout << indent << (n->isLeaf() ? "Hoja" : "Nodo interno")
				<< " (nivel " << n->level << ", "
				<< n->entries.size() << " entradas)\n";
			for (const auto& e : n->entries) {
				cout << indent << "  " << e.I;
				if (n->isLeaf()) {
					cout << "  tuple=" << e.tuple << "\n";
				} else {
					cout << "\n";
					print(e.child, depth + 1);
				}
			}
		}
};

// ===========================================================================
//  Ejemplo de uso
// ===========================================================================
void printResult(const vector<int>& ids) {
	cout << "tuples: ";
	for (int id : ids) cout << id << " ";
	cout << "\n";
}

int main() {
	// Datos de ejemplo: 12 rect�ngulos en el cuadrado [0,10] x [0,10]
	vector<Rect> data = {
		{{0, 0}, {2, 2}},   {{1, 1}, {3, 3}},   {{5, 5}, {6, 6}},     //  0  1  2
		{{8, 8}, {9, 9}},   {{2, 5}, {3, 6}},   {{6, 1}, {7, 2}},     //  3  4  5
		{{4, 4}, {5, 5}},   {{9, 0}, {10, 1}},  {{0, 8}, {1, 9}},     //  6  7  8
		{{3, 8}, {4, 9}},   {{7, 6}, {8, 7}},   {{5, 9}, {6, 10}},    //  9 10 11
	};
	
	// 1. Crear el �rbol e insertar
	//    M = 4, m = 2, split cuadr�tico (el m�s usado en implementaciones;
	//    en sus experimentos Guttman concluy� que el lineal es igual de bueno)
	RTree tree(4, 2, SplitAlgorithm::Quadratic);
	for (int i = 0; i < (int)data.size(); i++)
		tree.insert(data[i], i);
	
	cout << "=== Arbol tras insertar " << data.size() << " rectangulos ";
	tree.print();
	
	// 2. Buscar
	Rect S = {{0, 0}, {4, 4}};
	cout << "\n=== Search " << S << " ===\n";
	printResult(tree.search(S));
	
	// 3. Borrar
	cout << "\n=== Delete de tuples 0, 1 y 6 ===\n";
	tree.remove(data[0], 0);
	tree.remove(data[1], 1);
	tree.remove(data[6], 6);
	tree.print();
	
	cout << "\n=== Search " << S << " tras eliminar ===\n";
	printResult(tree.search(S));
	
	// 4. Comparaci�n de los tres algoritmos de split (secci�n 3.5)
	const char* names[] = {"Exhaustive", "Quadratic", "Linear"};
	SplitAlgorithm algs[] = {SplitAlgorithm::Exhaustive,
		SplitAlgorithm::Quadratic,
		SplitAlgorithm::Linear};
	for (int a = 0; a < 3; a++) {
		RTree t(4, 2, algs[a]);
		for (int i = 0; i < (int)data.size(); i++) t.insert(data[i], i);
		cout << "\n=== Split " << names[a] << " ";
		t.print();
	}
}

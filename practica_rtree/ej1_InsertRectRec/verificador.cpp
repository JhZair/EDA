#include "RTree.h"

static bool MismoRect(const Rect& a, const Rect& b)
{
	for (int i = 0; i < 2; ++i)
		if (a.m_min[i] != b.m_min[i] || a.m_max[i] != b.m_max[i]) return false;
	return true;
}

static string Txt(const Rect& r)
{
	ostringstream os;
	os << "(" << r.m_min[0] << "," << r.m_min[1] << ")-("
	   << r.m_max[0] << "," << r.m_max[1] << ")";
	return os.str();
}

string RTree::CheckNode(Node* a_node, bool a_isRoot)
{
	ostringstream err;

	if (a_node == NULL) return "hay un puntero a hijo en NULL";

	if (a_node->m_count > MAXNODES)
	{
		err << "un nodo de nivel " << a_node->m_level << " tiene "
		    << a_node->m_count << " entradas (max " << MAXNODES << ")";
		return err.str();
	}

	if (!a_isRoot && a_node->m_count < MINNODES)
	{
		err << "propiedad (1)/(3): un nodo no raiz de nivel " << a_node->m_level
		    << " tiene " << a_node->m_count << " entradas (min " << MINNODES << ")";
		return err.str();
	}

	if (a_isRoot && a_node->IsInternalNode() && a_node->m_count < 2)
	{
		err << "propiedad (5): la raiz interna tiene " << a_node->m_count << " hijo(s)";
		return err.str();
	}

	if (a_node->IsLeaf()) return "";

	for (int i = 0; i < a_node->m_count; ++i)
	{
		Node* hijo = a_node->m_branch[i].m_child;

		if (hijo == NULL) return "una entrada interna no tiene hijo";

		if (hijo->m_level != a_node->m_level - 1)
		{
			err << "propiedad (6): un hijo del nivel " << a_node->m_level
			    << " esta en nivel " << hijo->m_level
			    << " (deberia ser " << a_node->m_level - 1 << ")";
			return err.str();
		}

		if (hijo->m_count == 0) return "un nodo hijo esta vacio";

		Rect debido = NodeCover(hijo);
		if (!MismoRect(a_node->m_branch[i].m_rect, debido))
		{
			err << "propiedad (4): el MBR guardado " << Txt(a_node->m_branch[i].m_rect)
			    << " no coincide con el real " << Txt(debido)
			    << " (nivel " << a_node->m_level << ", rama " << i << ")";
			return err.str();
		}

		string sub = CheckNode(hijo, false);
		if (!sub.empty()) return sub;
	}

	return "";
}

string RTree::CheckTree()
{
	if (m_root == NULL) return "la raiz es NULL";
	return CheckNode(m_root, true);
}

int RTree::AlturaRec(Node* a_node)
{
	if (a_node->IsLeaf()) return 1;
	int mx = 0;
	for (int i = 0; i < a_node->m_count; ++i)
	{
		int h = AlturaRec(a_node->m_branch[i].m_child);
		if (h > mx) mx = h;
	}
	return 1 + mx;
}

int RTree::Altura() { return AlturaRec(m_root); }

void RTree::DibujarRec(Node* a_node, int a_depth)
{
	string sangria(a_depth * 3, ' ');
	cout << sangria << (a_node->IsLeaf() ? "Hoja" : "Interno")
	     << " nivel=" << a_node->m_level
	     << " entradas=" << a_node->m_count << "\n";
	for (int i = 0; i < a_node->m_count; ++i)
	{
		cout << sangria << "  " << Txt(a_node->m_branch[i].m_rect) << "\n";
		if (!a_node->IsLeaf()) DibujarRec(a_node->m_branch[i].m_child, a_depth + 1);
	}
}

void RTree::Dibujar() { DibujarRec(m_root, 0); }

// Resume la forma del arbol en un numero. Dos arboles con la misma firma
// tienen la misma estructura y los mismos MBR. Sirve para comparar tu
// implementacion contra la correcta.
long long RTree::FirmaRec(Node* a_node)
{
	 long long f = a_node->m_level * 31 + a_node->m_count * 7;
	for (int i = 0; i < a_node->m_count; ++i)
	{
		Rect* r = &a_node->m_branch[i].m_rect;
		f = f * 131 + r->m_min[0] * 3 + r->m_min[1] * 5
		            + r->m_max[0] * 11 + r->m_max[1] * 13;
		f = f % 1000000007;
		if (!a_node->IsLeaf()) f = (f * 17 + FirmaRec(a_node->m_branch[i].m_child)) % 1000000007;
	}
	return f % 1000000007;
}

long long RTree::Firma() { return FirmaRec(m_root); }

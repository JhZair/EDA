/*
 * R*-tree Implementation
 * Based on: "The R*-tree: An Efficient and Robust Access Method for Points and Rectangles"
 * Beckmann, Kriegel, Schneider, Seeger — ACM SIGMOD 1990
 *
 * Nomenclature follows the original paper as closely as possible:
 *   - Insert, ChooseSubtree, Split, OverflowTreatment, ReInsert, CondenseTree, etc.
 *   - M  = maximum entries per node
 *   - m  = minimum entries per node  (≥ 2, ≤ M/2, paper recommends m = 0.4·M)
 *   - p  = reInsert count            (paper recommends p = 0.3·M)
 */

#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <limits>
#include <cmath>
#include <cassert>
#include <memory>
#include <numeric>
#include <iomanip>

// ─────────────────────────────────────────────────────────────────────────────
// Compile-time parameters
// ─────────────────────────────────────────────────────────────────────────────
static constexpr int DIMENSIONS = 2;   // number of spatial dimensions
static constexpr int M          = 4;   // max entries per node
static constexpr int m          = 2;   // min entries per node  (paper: 0.4·M → 1.6 ≈ 2)
static constexpr int p          = 1;   // reInsert entries      (paper: 0.3·M → 1.2 ≈ 1)

static_assert(m >= 2 && m <= M / 2, "m must satisfy 2 <= m <= M/2");
static_assert(p >= 1 && p < M - m + 1, "p must be in [1, M-m]");

// ─────────────────────────────────────────────────────────────────────────────
// Rectangle (Minimum Bounding Rectangle – MBR)
// ─────────────────────────────────────────────────────────────────────────────
using Coord = double;

struct Rectangle {
    std::array<Coord, DIMENSIONS> lo; // lower corner
    std::array<Coord, DIMENSIONS> hi; // upper corner

    Rectangle() {
        lo.fill( std::numeric_limits<Coord>::max());
        hi.fill(-std::numeric_limits<Coord>::max());
    }

    Rectangle(std::array<Coord, DIMENSIONS> lo_,
              std::array<Coord, DIMENSIONS> hi_)
        : lo(lo_), hi(hi_) {}

    // Area of the rectangle
    Coord area() const {
        Coord a = 1.0;
        for (int i = 0; i < DIMENSIONS; ++i)
            a *= std::max(Coord(0), hi[i] - lo[i]);
        return a;
    }

    // Margin = sum of lengths of edges (used in split criterion, §3.1)
    Coord margin() const {
        Coord s = 0;
        for (int i = 0; i < DIMENSIONS; ++i)
            s += hi[i] - lo[i];
        return s;
    }

    // Overlap between this and r
    Coord overlap(const Rectangle& r) const {
        Coord o = 1.0;
        for (int i = 0; i < DIMENSIONS; ++i) {
            Coord len = std::min(hi[i], r.hi[i]) - std::max(lo[i], r.lo[i]);
            if (len <= 0) return 0.0;
            o *= len;
        }
        return o;
    }

    // Area enlargement needed to include r
    Coord enlargement(const Rectangle& r) const {
        return combined(r).area() - area();
    }

    // MBR of this and r
    Rectangle combined(const Rectangle& r) const {
        Rectangle res;
        for (int i = 0; i < DIMENSIONS; ++i) {
            res.lo[i] = std::min(lo[i], r.lo[i]);
            res.hi[i] = std::max(hi[i], r.hi[i]);
        }
        return res;
    }

    // Center coordinate along axis i
    Coord center(int i) const { return (lo[i] + hi[i]) / 2.0; }

    bool contains(const Rectangle& r) const {
        for (int i = 0; i < DIMENSIONS; ++i)
            if (r.lo[i] < lo[i] || r.hi[i] > hi[i]) return false;
        return true;
    }

    bool intersects(const Rectangle& r) const {
        for (int i = 0; i < DIMENSIONS; ++i)
            if (lo[i] > r.hi[i] || hi[i] < r.lo[i]) return false;
        return true;
    }

    void print() const {
        std::cout << "[";
        for (int i = 0; i < DIMENSIONS; ++i) {
            if (i) std::cout << " x ";
            std::cout << "[" << lo[i] << "," << hi[i] << "]";
        }
        std::cout << "]";
    }
};

// Compute MBR of a collection of rectangles
Rectangle boundingBox(const std::vector<Rectangle>& rects) {
    Rectangle bb;
    for (auto& r : rects)
        bb = bb.combined(r);
    return bb;
}

// ─────────────────────────────────────────────────────────────────────────────
// Entry  (held inside a Node)
// ─────────────────────────────────────────────────────────────────────────────
struct Node; // forward declaration

struct Entry {
    Rectangle   mbr;
    int         dataId  = -1;          // valid only in leaf entries
    Node*       child   = nullptr;     // valid only in internal entries

    bool isLeaf() const { return child == nullptr; }
};

// ─────────────────────────────────────────────────────────────────────────────
// Node
// ─────────────────────────────────────────────────────────────────────────────
struct Node {
    bool              leaf    = true;
    int               level   = 0;   // 0 = leaf level (paper convention)
    Node*             parent  = nullptr;
    std::vector<Entry> entries;       // at most M entries

    int size() const { return static_cast<int>(entries.size()); }

    Rectangle mbr() const {
        Rectangle bb;
        for (auto& e : entries)
            bb = bb.combined(e.mbr);
        return bb;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// R*-tree
// ─────────────────────────────────────────────────────────────────────────────
class RStarTree {
public:
    RStarTree() {
        root_ = newNode(true, 0);
    }

    ~RStarTree() { destroySubtree(root_); }

    // ── Public interface ────────────────────────────────────────────────────

    // Insert a data rectangle with identifier dataId
    void Insert(const Rectangle& r, int dataId) {
        overflowLevel_.assign(maxLevel() + 2, false);
        Entry e;
        e.mbr    = r;
        e.dataId = dataId;
        Insert(e, 0 /*leaf level*/);
    }

    // Search: return dataIds whose MBR intersects the query rectangle W
    std::vector<int> Search(const Rectangle& W) const {
        std::vector<int> result;
        Search(root_, W, result);
        return result;
    }

    // Delete an entry (by dataId + exact MBR)
    bool Delete(const Rectangle& r, int dataId) {
        Node* leaf = FindLeaf(root_, r, dataId);
        if (!leaf) return false;

        // Remove entry from leaf
        auto& es = leaf->entries;
        es.erase(std::remove_if(es.begin(), es.end(),
            [&](const Entry& e){ return e.dataId == dataId; }), es.end());

        CondenseTree(leaf);

        // Shorten tree if root has one child and is not a leaf
        while (!root_->leaf && root_->size() == 1) {
            Node* oldRoot = root_;
            root_ = root_->entries[0].child;
            root_->parent = nullptr;
            root_->level  = 0; // re-root level will be recomputed on next insert
            oldRoot->entries.clear();
            delete oldRoot;
        }
        return true;
    }

    void printTree() const {
        std::cout << "\n=== R*-tree (M=" << M << ", m=" << m << ") ===\n";
        printSubtree(root_, 0);
        std::cout << "===========================\n";
    }

private:
    Node*              root_;
    std::vector<bool>  overflowLevel_; // one flag per level (for reInsert)

    // ── Helpers ─────────────────────────────────────────────────────────────

    Node* newNode(bool leaf, int level) {
        Node* n  = new Node;
        n->leaf  = leaf;
        n->level = level;
        return n;
    }

    void destroySubtree(Node* n) {
        if (!n) return;
        if (!n->leaf)
            for (auto& e : n->entries)
                destroySubtree(e.child);
        delete n;
    }

    int maxLevel() const {
        int lv = 0;
        Node* cur = root_;
        while (!cur->leaf) { cur = cur->entries[0].child; ++lv; }
        return lv;
    }

    // Find the child entry in parent that points to child node
    Entry* entryFor(Node* parent, Node* child) {
        for (auto& e : parent->entries)
            if (e.child == child) return &e;
        return nullptr;
    }

    // MODIFICAR AQUI (1)
    // ── Algorithm Insert (paper §3.2) ────────────────────────────────────────
    /*
     * I1. Invoke ChooseSubtree to select a leaf node L in which to place E.
 Node* ChooseSubtree(const Rectangle& r, int targetLevel)
     * I2. If L has room, install E.  Otherwise invoke OverflowTreatment.
     * I3. Propagate changes upward via AdjustTree.
     * I4. Grow tree taller if needed.
     */
    // MODIFICAR AQUI (1)
    void Insert(const Entry& e, int level) {
         ChooseSubtree(,level)



    }

    // ── Algorithm ChooseSubtree (paper §3.1) ─────────────────────────────────
    /*
     * CS1. Set N to the root.
     * CS2. If N is a leaf, return N.
     * CS3. If N's children are leaves:
     *        choose the entry whose rectangle needs the least overlap enlargement
     *        to include the new data rectangle. Resolve ties by choosing the entry
     *        whose rectangle needs the least area enlargement, then the smallest area.
     *      Else:
     *        choose the entry whose rectangle needs the least area enlargement.
     *        Ties → smallest area.
     * CS4. Set N to the child pointer of the chosen entry, repeat CS2.
     */
    Node* ChooseSubtree(const Rectangle& r, int targetLevel) {
        Node* N = root_;

        while (N->level != targetLevel) {
            bool childrenAreLeaves = (N->level == targetLevel + 1);

            if (childrenAreLeaves) {
                // Minimize overlap enlargement
                Coord   bestOverlapEnl  = std::numeric_limits<Coord>::max();
                Coord   bestAreaEnl     = std::numeric_limits<Coord>::max();
                Coord   bestArea        = std::numeric_limits<Coord>::max();
                Entry*  chosen          = nullptr;

                // Pre-collect MBRs for overlap computation
                std::vector<Rectangle> mbrs;
                for (auto& e : N->entries) mbrs.push_back(e.mbr);

                for (int i = 0; i < N->size(); ++i) {
                    Entry& e = N->entries[i];
                    Rectangle enlarged = e.mbr.combined(r);

                    // overlap before
                    Coord oBefore = 0;
                    for (int j = 0; j < N->size(); ++j)
                        if (j != i) oBefore += e.mbr.overlap(mbrs[j]);

                    // overlap after
                    Coord oAfter = 0;
                    for (int j = 0; j < N->size(); ++j)
                        if (j != i) oAfter += enlarged.overlap(mbrs[j]);

                    Coord overlapEnl = oAfter - oBefore;
                    Coord areaEnl    = e.mbr.enlargement(r);
                    Coord area       = e.mbr.area();

                    if (overlapEnl < bestOverlapEnl ||
                       (overlapEnl == bestOverlapEnl && areaEnl < bestAreaEnl) ||
                       (overlapEnl == bestOverlapEnl && areaEnl == bestAreaEnl && area < bestArea)) {
                        bestOverlapEnl = overlapEnl;
                        bestAreaEnl    = areaEnl;
                        bestArea       = area;
                        chosen         = &e;
                    }
                }
                N = chosen->child;
            } else {
                // Minimize area enlargement
                Coord  bestEnl  = std::numeric_limits<Coord>::max();
                Coord  bestArea = std::numeric_limits<Coord>::max();
                Entry* chosen   = nullptr;

                for (auto& e : N->entries) {
                    Coord enl  = e.mbr.enlargement(r);
                    Coord area = e.mbr.area();
                    if (enl < bestEnl || (enl == bestEnl && area < bestArea)) {
                        bestEnl  = enl;
                        bestArea = area;
                        chosen   = &e;
                    }
                }
                N = chosen->child;
            }
        }
        return N;
    }

    // ── Algorithm Split (paper §3.1) ─────────────────────────────────────────
    /*
     * S1. Invoke ChooseSplitAxis to determine the axis, perpendicular to which
     *     the split is performed.
     * S2. Invoke ChooseSplitIndex to determine the best distribution into two
     *     groups along the chosen axis.
     * S3. Distribute the entries and return the two resulting nodes.
     *
     * ChooseSplitAxis:
     *   For each axis:
     *     Sort entries by lower then upper value. For each of the M-2m+2
     *     distributions, compute S = sum of all margin values of the MBRs.
     *   Choose the axis with minimum S.
     *
     * ChooseSplitIndex (along chosen axis):
     *   Among the distributions, choose the one with minimum overlap.
     *   Ties → minimum area.
     */
    Node* Split(Node* N) {
        int    bestAxis  = -1;
        Coord  bestS     = std::numeric_limits<Coord>::max();

        // S1 — ChooseSplitAxis
        for (int axis = 0; axis < DIMENSIONS; ++axis) {
            Coord S = 0;
            // Sort by lower value
            for (int sortByUpper = 0; sortByUpper <= 1; ++sortByUpper) {
                std::vector<Entry> sorted = N->entries;
                std::sort(sorted.begin(), sorted.end(), [&](const Entry& a, const Entry& b){
                    return sortByUpper ? a.mbr.hi[axis] < b.mbr.hi[axis]
                                       : a.mbr.lo[axis] < b.mbr.lo[axis];
                });
                // Distributions k = 1 … M-2m+2
                for (int k = 1; k <= M - 2*m + 2; ++k) {
                    int split = m - 1 + k; // first group has split entries
                    std::vector<Rectangle> g1, g2;
                    for (int i = 0; i < split;         ++i) g1.push_back(sorted[i].mbr);
                    for (int i = split; i <= M;        ++i) g2.push_back(sorted[i].mbr);
                    S += boundingBox(g1).margin() + boundingBox(g2).margin();
                }
            }
            if (S < bestS) { bestS = S; bestAxis = axis; }
        }

        // S2 — ChooseSplitIndex along bestAxis
        // Sort by lower, then try upper as well — pick whichever gives the
        // chosen distribution (paper says sort by lower value first, then by upper)
        Coord  bestOverlap = std::numeric_limits<Coord>::max();
        Coord  bestArea    = std::numeric_limits<Coord>::max();
        int    bestK       = -1;
        bool   bestUpper   = false;

        for (int sortByUpper = 0; sortByUpper <= 1; ++sortByUpper) {
            std::vector<Entry> sorted = N->entries;
            std::sort(sorted.begin(), sorted.end(), [&](const Entry& a, const Entry& b){
                return sortByUpper ? a.mbr.hi[bestAxis] < b.mbr.hi[bestAxis]
                                   : a.mbr.lo[bestAxis] < b.mbr.lo[bestAxis];
            });

            for (int k = 1; k <= M - 2*m + 2; ++k) {
                int split = m - 1 + k;
                std::vector<Rectangle> g1, g2;
                for (int i = 0; i < split; ++i) g1.push_back(sorted[i].mbr);
                for (int i = split; i <= M; ++i) g2.push_back(sorted[i].mbr);

                Rectangle bb1 = boundingBox(g1);
                Rectangle bb2 = boundingBox(g2);
                Coord ov   = bb1.overlap(bb2);
                Coord area = bb1.area() + bb2.area();

                if (ov < bestOverlap || (ov == bestOverlap && area < bestArea)) {
                    bestOverlap = ov;
                    bestArea    = area;
                    bestK       = k;
                    bestUpper   = sortByUpper;
                }
            }
        }

        // S3 — Distribute
        std::vector<Entry> sorted = N->entries;
        std::sort(sorted.begin(), sorted.end(), [&](const Entry& a, const Entry& b){
            return bestUpper ? a.mbr.hi[bestAxis] < b.mbr.hi[bestAxis]
                             : a.mbr.lo[bestAxis] < b.mbr.lo[bestAxis];
        });

        int splitAt = m - 1 + bestK;

        Node* NN = newNode(N->leaf, N->level);
        N->entries.clear();

        for (int i = 0; i <= M; ++i) {
            Entry& e = sorted[i];
            if (i < splitAt) {
                N->entries.push_back(e);
                if (!e.isLeaf() && e.child) e.child->parent = N;
            } else {
                NN->entries.push_back(e);
                if (!e.isLeaf() && e.child) e.child->parent = NN;
            }
        }
        return NN;
    }

    // ── Algorithm OverflowTreatment (paper §3.2) ─────────────────────────────
    /*
     * OT1. If the level is not the root level AND this level has not been called
     *       before during insertion of one data rectangle:
     *         invoke ReInsert.
     *      Else:
     *         invoke Split.
     */
    // MODIFICAR AQUI (2)
    Node* OverflowTreatment(Node* N, int level) {


    }

    // ── Algorithm ReInsert (paper §3.2) ──────────────────────────────────────
    /*
     * RI1. For all M+1 entries of N, compute distance between their centers and
     *       the center of N's MBR.
     * RI2. Sort entries in decreasing order of distance.
     * RI3. Remove the first p entries from N and adjust N's MBR.
     * RI4. In the order of decreasing distance re-insert the removed entries.
     */
    // MODIFICAR AQUI (3)
    void ReInsert(Node* N, int level) {



    }

    // ── Algorithm AdjustTree (propagate MBR changes up; paper §3.2) ──────────
    /*
     * AT1. If N is root, stop.
     * AT2. Let P = parent of N. Update P's entry for N to have the tightest MBR.
     * AT3. If a split was performed, add NN to P. If P overflows → OverflowTreatment.
     * AT4. Move up: N = P, NN = result of split if any. Repeat AT1.
     */
    void AdjustTree(Node* N, Node* NN) {
        while (N != root_) {
            Node* P = N->parent;
            Entry* En = entryFor(P, N);
            En->mbr = N->mbr();

            if (NN) {
                Entry newEntry;
                newEntry.mbr   = NN->mbr();
                newEntry.child = NN;
                NN->parent     = P;
                P->entries.push_back(newEntry);
                NN = nullptr;

                if (P->size() > M) {
                    int lv = P->level;
                    NN = OverflowTreatment(P, lv);
                    // NN may be nullptr if ReInsert was called
                }
            }
            N = P;
        }

        // If root was split, create a new root
        if (NN) {
            Node* newRoot   = newNode(false, root_->level + 1);
            Entry e1, e2;
            e1.mbr    = root_->mbr();  e1.child = root_;
            e2.mbr    = NN->mbr();     e2.child = NN;
            newRoot->entries.push_back(e1);
            newRoot->entries.push_back(e2);
            root_->parent = newRoot;
            NN->parent    = newRoot;
            root_         = newRoot;
            // Recompute levels
            fixLevels(root_, root_->level);
        } else if (N == root_) {
            // Update root's own MBR (no-op for the root, but keep for clarity)
        }
    }

    void fixLevels(Node* n, int lv) {
        n->level = lv;
        if (!n->leaf)
            for (auto& e : n->entries)
                fixLevels(e.child, lv - 1);
    }

    // ── Algorithm Search (paper §2) ─────────────────────────────────────────
    /*
     * S1. Search subtrees: if N is not a leaf, check each entry E whose MBR
     *     intersects W; invoke Search on E's child.
     * S2. If N is a leaf, report all entries whose MBR intersects W.
     */
    void Search(Node* N, const Rectangle& W, std::vector<int>& result) const {
        if (N->leaf) {
            for (auto& e : N->entries)
                if (e.mbr.intersects(W))
                    result.push_back(e.dataId);
        } else {
            for (auto& e : N->entries)
                if (e.mbr.intersects(W))
                    Search(e.child, W, result);
        }
    }

    // ── Algorithm FindLeaf (for Delete) ─────────────────────────────────────
    Node* FindLeaf(Node* T, const Rectangle& r, int dataId) const {
        if (!T->leaf) {
            for (auto& e : T->entries) {
                if (e.mbr.contains(r) || e.mbr.intersects(r)) {
                    Node* res = FindLeaf(e.child, r, dataId);
                    if (res) return res;
                }
            }
            return nullptr;
        } else {
            for (auto& e : T->entries)
                if (e.dataId == dataId && e.mbr.lo == r.lo && e.mbr.hi == r.hi)
                    return T;
            return nullptr;
        }
    }

    // ── Algorithm CondenseTree (paper §3.3 / original R-tree §3.3 adapted) ──
    /*
     * CT1. Let N = the leaf. Set Q (set of eliminated nodes) = empty.
     * CT2. If N = root, go to CT6.
     * CT3. Let P = parent, En = entry in P pointing to N.
     * CT4. If N has fewer than m entries, delete En from P and add N to Q.
     * CT5. Else adjust En's MBR to tightly contain N's entries.
     * CT6. Set N = P; repeat from CT2.
     * CT6b. Re-insert all entries of nodes in Q.
     */
    void CondenseTree(Node* L) {
        Node* N = L;
        std::vector<Entry> toReinsert;

        while (N != root_) {
            Node* P  = N->parent;
            Entry* En = entryFor(P, N);

            if (N->size() < m) {
                // Collect N's entries for reinsertion
                for (auto& e : N->entries) {
                    if (!e.isLeaf() && e.child) e.child->parent = nullptr;
                    toReinsert.push_back(e);
                }
                // Remove En from P
                auto& pes = P->entries;
                pes.erase(std::remove_if(pes.begin(), pes.end(),
                    [&](const Entry& e){ return e.child == N; }), pes.end());
                delete N;
            } else {
                En->mbr = N->mbr();
            }
            N = P;
        }

        // Re-insert orphaned entries
        for (auto& e : toReinsert) {
            overflowLevel_.assign(maxLevel() + 2, false);
            int lv = e.isLeaf() ? 0 : (e.child ? e.child->level : 0);
            Insert(e, lv);
        }
    }

    // ── Pretty-print ────────────────────────────────────────────────────────
    void printSubtree(Node* n, int indent) const {
        std::string pad(indent * 3, ' ');
        if (n->leaf) {
            std::cout << pad << "[LEAF lv=" << n->level
                      << " sz=" << n->size() << "]\n";
            for (auto& e : n->entries) {
                std::cout << pad << "   id=" << e.dataId << " mbr=";
                e.mbr.print(); std::cout << "\n";
            }
        } else {
            std::cout << pad << "[INTERNAL lv=" << n->level
                      << " sz=" << n->size() << "] mbr=";
            n->mbr().print(); std::cout << "\n";
            for (auto& e : n->entries)
                printSubtree(e.child, indent + 1);
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Helper: make a point rectangle (degenerate rectangle)
// ─────────────────────────────────────────────────────────────────────────────
Rectangle makePoint(Coord x, Coord y) {
    return Rectangle({x, y}, {x, y});
}

Rectangle makeRect(Coord x1, Coord y1, Coord x2, Coord y2) {
    return Rectangle({x1, y1}, {x2, y2});
}

// ─────────────────────────────────────────────────────────────────────────────
// main() — pruebas de inserción, búsqueda y eliminación
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    std::cout << std::fixed << std::setprecision(1);

    RStarTree tree;

    // ── Insertar puntos/rectángulos ──────────────────────────────────────────
    std::cout << "=== Insertando entradas ===\n";

    // Puntos del plano 2D
    struct TestRecord { int id; Rectangle r; std::string desc; };

    std::vector<TestRecord> records = {
        {  1, makePoint( 2,  2), "P(2,2)"},
        {  2, makePoint( 5,  5), "P(5,5)"},
        {  3, makePoint( 8,  1), "P(8,1)"},
        {  4, makePoint( 1,  8), "P(1,8)"},
        {  5, makePoint( 9,  9), "P(9,9)"},
        {  6, makeRect ( 3,  3,  6,  6), "R([3,6]x[3,6])"},
        {  7, makePoint( 4,  7), "P(4,7)"},
        {  8, makePoint( 7,  4), "P(7,4)"},
        {  9, makeRect ( 0,  0,  2,  3), "R([0,2]x[0,3])"},
        { 10, makePoint( 6,  2), "P(6,2)"},
        { 11, makeRect ( 5,  8,  9, 10), "R([5,9]x[8,10])"},
        { 12, makePoint( 3,  0), "P(3,0)"},
        { 13, makePoint( 0,  6), "P(0,6)"},
        { 14, makeRect ( 7,  7, 10, 10), "R([7,10]x[7,10])"},
        { 15, makePoint(10,  5), "P(10,5)"},
    };

    for (auto& rec : records) {
        std::cout << "  Insert id=" << rec.id << "  " << rec.desc << "\n";
        tree.Insert(rec.r, rec.id);
    }

    tree.printTree();

    // ── Búsqueda por rango ───────────────────────────────────────────────────
    std::cout << "\n=== Búsqueda por rango ===\n";

    auto printSearch = [&](Rectangle W, const std::string& label) {
        std::cout << "Query " << label << " → ";
        W.print();
        std::cout << "\n  Resultados: ";
        auto res = tree.Search(W);
        std::sort(res.begin(), res.end());
        if (res.empty()) std::cout << "(vacío)";
        for (int id : res) std::cout << id << " ";
        std::cout << "\n";
    };

    printSearch(makeRect(0, 0, 10, 10), "Universo [0,10]x[0,10]");
    printSearch(makeRect(4, 4,  7,  7), "Centro   [4,7]x[4,7]");
    printSearch(makeRect(7, 7, 10, 10), "Esquina  [7,10]x[7,10]");
    printSearch(makeRect(0, 0,  1,  1), "Esquina  [0,1]x[0,1]");
    printSearch(makePoint(5,  5),       "Punto    (5,5)");

    // ── Eliminación ─────────────────────────────────────────────────────────
    std::cout << "\n=== Eliminación ===\n";
    for (int id : {3, 7, 14}) {
        auto& rec = records[id - 1];
        bool ok = tree.Delete(rec.r, rec.id);
        std::cout << "  Delete id=" << id << " (" << rec.desc << "): "
                  << (ok ? "OK" : "NO ENCONTRADO") << "\n";
    }

    tree.printTree();

    // Búsqueda tras eliminación
    std::cout << "\n=== Búsqueda tras eliminación ===\n";
    printSearch(makeRect(0, 0, 10, 10), "Universo [0,10]x[0,10]");

    return 0;
}

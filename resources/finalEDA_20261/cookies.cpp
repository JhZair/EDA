#include <vector>
#include <algorithm>
#include <iostream>
#include <set>
#include <bits/stdc++.h>
//#include <bits/stdc++.h>



using namespace std;


 float distanceE( float t, float r){
    return sqrt(t*t + r*r);
 }
 float med(vector<int> t){
     float sum=0;
    for(int r: t){
        sum+=r;
    }
    return (float)(sum/(float)t.size());

 }
int distributeCookies(vector<int>& cookies, int k) {
    float xsub = med(cookies);


    return 0;
}

int main() {
    vector<int> cookies1 = {8, 15, 10, 20, 8};
    int k1 = 2;
    cout << "Ejemplo 1: " << distributeCookies(cookies1, k1) << endl;  // Output: 31
    cout<< med(cookies1);

    /*
    vector<int> cookies2 = {6, 1, 3, 2, 2, 4, 1, 2};
    int k2 = 3;
    cout << "Ejemplo 2: " << distributeCookies(cookies2, k2) << endl;  // Output: 7

    vector<int> cookies3 = {7, 4, 5, 6, 3, 2};
    int k3 = 2;
    cout << "Caso de prueba adicional 1: " << distributeCookies(cookies3, k3) << endl;  // Output: 14
    // Explicación: Se puede dividir equitativamente como [7, 4, 2] y [6, 5, 3]. El max(13, 14) = 14

    vector<int> cookies4 = {10, 5, 8, 3, 7, 9, 2, 1};
    int k4 = 4;
    cout << "Caso de prueba adicional 2: " << distributeCookies(cookies4, k4) << endl;  // Output: 12

    vector<int> cookies5 = {1, 1, 1, 1, 1, 1, 1, 50};
    int k5 = 3;
    cout << "Caso de prueba adicional 3: " << distributeCookies(cookies5, k5) << endl;  // Output: 50
*/
    return 0;
}

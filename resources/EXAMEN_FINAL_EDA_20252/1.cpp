#include <iostream>
#include <vector>
#include <cmath>

using namespace std;

void bubble(vector<int>& nums){
	for(int i = 0;i < nums.size()-1;i++){
		for(int j = 1;j < nums.size();j++){
			if(nums[i]<nums[j]){
				int a = nums[i];
				nums[i] = nums[j];
				nums[j] = a;
			}
		}
	}
}

long long largestPerimeter(vector<int>& nums){
	int min_points = 3;
	int n = nums.size();
	int numsum = 0;
	vector<int> shield;
	
	if(n < min_points)
		return -1;
	
	for(int j=0;j < nums.size();j++){
		numsum+=nums[j];
	}
	return numsum;
}
int main() {
	vector<int> a = {5,5,5};
	bubble(a);
	cout << largestPerimeter(a) << endl;
	return 0;
}


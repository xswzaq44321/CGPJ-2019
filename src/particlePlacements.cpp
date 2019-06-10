#include <iostream>
#include <cstdlib>

/*

	The universe is [-5,5]x[-5,5]x[-5,5].

*/

bool centerBox(float x, float y, float z);
bool centerSphere(float x, float y, float z);

bool centerBox(float x, float y, float z){
	return ((x >= 4.0f && x <= 6.0f) && (y >= 4.0f && y <= 6.0f) && (z >= 4.0f && z <= 6.0f));
}

bool centerSphere(float x, float y, float z){
	float radius = 1.5f;
	float key = x * x + y * y + z * z;
	return (key < radius * radius * radius);
}
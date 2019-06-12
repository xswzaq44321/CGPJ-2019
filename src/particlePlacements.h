#include <iostream>
#include <cstdlib>

/*

	The universe is [-5,5]x[-5,5]x[-5,5].

*/

bool centerBox(float x, float y, float z);
bool centerSphere(float x, float y, float z);

bool centerBox(float x, float y, float z){
	return ((x >= 0.0f && x <= 10.0f) && (y >= 6.0f && y <= 10.0f) && (z >= 0.0f && z <= 10.0f));
}

bool centerSphere(float x, float y, float z){
	float radius = 2.0f;
	float key = (x - 5) * (x - 5) + (y - 5) * (y - 5) + (z - 5) * (z - 5);
	return (key < radius * radius * radius);
}
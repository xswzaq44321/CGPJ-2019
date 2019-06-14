#include <iostream>
#include <cstdlib>

/*

	The universe is [-5,5]x[-5,5]x[-5,5].

*/

bool centerBox(float x, float y, float z);
bool centerSphere(float x, float y, float z);

bool centerBox(float x, float y, float z){
	return ((x >= 2.0f && x <= 8.0f) && (y >= 7.0f && y <= 9.0f) && (z >= 2.0f && z <= 8.0f));
}

bool centerSphere(float x, float y, float z){
	float radius = 2.0f;
	float key = (x - 5) * (x - 5) + (y - 5) * (y - 5) + (z - 5) * (z - 5);
	return (key < radius * radius * radius);
}
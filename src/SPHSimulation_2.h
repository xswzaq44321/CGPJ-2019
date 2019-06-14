#include <cmath>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <functional>

#define PI 3.1415926f

class SystemState{
public:
	float* position;		// Particle Position
	float* vH;				// Half step velocity
	float* vF;				// Full step velocity
	float* acceleration;	//
	float* density;			// Particle density, only 1 value for each particle
	float* pressure;		// Pressure of each paritcle, only 1 value for each particle
	int particleNum;		//

	std::vector<std::vector<int>> nearParticleIndex;	// A list for near particle
};

class Params{
public:
	static constexpr float gravity = 9.8f;	// 
	static constexpr float Cs = 10.0f;		// Speed of sound
	static constexpr float Cij = 5.0f;		// Strength of interaction force
	static constexpr float k = 1.38672255f;	// Constant used to calculate interaction force
	static constexpr float viscosity = 0.1f;  // Viscosity == mu
	static constexpr float mass = 1.0f;		// Mass of each particle (Assume each particle is same)
	static constexpr float radii = 0.75f; 			// Radii (radius of affect)
	static constexpr float rDensity = 3.0f;		// Refrence Density

	static constexpr int steps = 5;				// Amount of steps used in Leap From Integration
	static constexpr float timeStep = 0.0075f;	// Time of each Half-Step
};

// Functions for initialization
void setInitial(std::function<int(float, float, float)> initialPosFunc, SystemState* s);
void setPosition(std::function<int(float, float, float)> initialPosFunc, SystemState* s);

// Functions for setting new position with Leap-Frog integrations. Requires the new acceleration.
void leapFrogStart(SystemState* s, double timeStep);
void leapFrogStep(SystemState* s, double timeStep);
void reflectCheck(SystemState* s);
void reflect(int axisIndex, float barrier, float* position, float* vF, float* vH);

// Functions for calculating accelerations
void getAcceleration(SystemState* s);
float getSqrDistance(float* position, int m, int n);
std::vector<float> getVecDistance(float* position, int m, int n);
void getPressureAcc(SystemState* s);
void getDensity(SystemState* s);
void getPressure(SystemState* s);
void getViscosityAcc(SystemState* s);
void getInteractiveAcc(SystemState* s);
float Force(float r);

void setInitial(std::function<int(float, float, float)> initialPosFunc, SystemState* s){
	setPosition(initialPosFunc, s);

	//s->position = new float[3 * s->particleNum];
	s->vH =  new float[3 * s->particleNum];
	s->vF =  new float[3 * s->particleNum];
	s->acceleration =  new float[3 * s->particleNum];
	s->density = new float[s->particleNum];
	s->pressure = new float[s->particleNum];

	for(int temp = 0;temp < s->particleNum;++temp){
		s->vH[3 * temp + 0] = 0.0f;
		s->vH[3 * temp + 1] = 0.0f;
		s->vH[3 * temp + 2] = 0.0f;
		s->vF[3 * temp + 0] = 0.0f;
		s->vF[3 * temp + 1] = 0.0f;
		s->vF[3 * temp + 2] = 0.0f;
		s->acceleration[3 * temp + 0] = 0.0f;
		s->acceleration[3 * temp + 0] = -Params::gravity;
		s->acceleration[3 * temp + 0] = 0.0f;
	}
}

void setPosition(std::function<int(float, float, float)> initialPosFunc, SystemState* s){
	int count = 0;
	float gap = (Params::radii) / 1.3f;
	for(float x = 0.0f;x < 10.0f;x += gap){
		for(float y = 0.0f;y < 10.0f;y += gap){
			for(float z = 0.0f;z < 10.0f;z += gap){
				if(initialPosFunc(x, y, z)){
					++count;
				}
			}
		}
	}
	s->particleNum = count;
	s->position = new float[count * 3];
	count = 0;

	for(float x = 0.0f;x < 10.0f;x += gap){
		for(float y = 0.0f;y < 10.0f;y += gap){
			for(float z = 0.0f;z < 10.0f;z += gap){
				if(initialPosFunc(x, y, z)){
					s->position[3 * count + 0] = x; // Initial X position
					s->position[3 * count + 1] = y; // Initial Y position
					s->position[3 * count + 2] = z; // Initial Z position
					++count;
				}
			}
		}
	}
}

void getAcceleration(SystemState* s){

	// Acceleration of gravity, base.
	for(int temp = 0;temp < s->particleNum;++temp){
		s->acceleration[3 * temp + 0] = 0.0f;
		s->acceleration[3 * temp + 1] = -(Params::gravity);//-(Params::gravity);
		s->acceleration[3 * temp + 2] = 0.0f;
	}
	//std::cout << "I can go here u fucker ass dick man1";
	// Initialize and setup list of neighbor particles
	// Only stores particle whose index bigger than itself
	float rp2;
	float hp2 = Params::radii * Params::radii;
	float* position = s->position;
	s->nearParticleIndex = std::vector<std::vector<int>>(s->particleNum);
	//std::cout << "I can go here u fucker ass dick man2";
	for(int temp = 0;temp < s->particleNum;++temp){
		for(int temp2 = temp + 1;temp2 < s->particleNum;++temp2){
			rp2 = getSqrDistance(position, temp, temp2);
			if(hp2 > rp2){
				s->nearParticleIndex[temp].push_back(temp2);
			}
		}
	}
	//std::cout << "I can go here u fucker ass dick man3";
	// Acceleration of pressure
	getPressureAcc(s);

	// Acceleration of viscosity
	getViscosityAcc(s);

	// Acceleration of particle interactive
	getInteractiveAcc(s);
}

float getSqrDistance(float* position, int m, int n){
	float dx = position[3 * n + 0] - position[3 * m + 0];
	float dy = position[3 * n + 1] - position[3 * m + 1];
	float dz = position[3 * n + 2] - position[3 * m + 2];

	float rp2 = dx * dx + dy * dy + dz * dz;
	return rp2;
}

std::vector<float> getVecDistance(float* position, int m, int n){
	std::vector<float> distance;// = new std::vector<float>();
	distance.push_back(position[3 * m + 0] - position[3 * n + 0]);
	distance.push_back(position[3 * m + 1] - position[3 * n + 1]);
	distance.push_back(position[3 * m + 2] - position[3 * n + 2]);

	return distance;
}

void getPressureAcc(SystemState* s){
	getDensity(s);
	getPressure(s);

	float* acceleration = s->acceleration;
	float* p = s->pressure;
	float* rho = s->density;
	float h = Params::radii;
	float hp2 = h * h;
	int particleNum = s->particleNum;

	float P, W, r, cbrtWeight;
	float C = -945 / (32 * PI * (h * h * h) * (h * h * h) * (h * h * h));

	for(int temp = 0;temp < particleNum;++temp){
		for(int temp2 : s->nearParticleIndex[temp]){
			P = -Params::mass * ((p[temp] / rho[temp]) + (p[temp2] / rho[temp2]));
			std::vector<float> R = getVecDistance(s->position, temp, temp2);
			cbrtWeight = hp2 - getSqrDistance(s->position, temp, temp2);
			W = C * cbrtWeight * cbrtWeight * cbrtWeight;

			acceleration[3 * temp + 0] += P * W * R[0];
			acceleration[3 * temp + 1] += P * W * R[1];
			acceleration[3 * temp + 2] += P * W * R[2];
			acceleration[3 * temp2 + 0] -= P * W * R[0];
			acceleration[3 * temp2 + 1] -= P * W * R[1];
			acceleration[3 * temp2 + 2] -= P * W * R[2];
		}
	}
}

void getDensity(SystemState* s){
	float h = Params::radii;
	float hp2 = h * h;
	float hp3 = hp2 * h;
	float BASE = 315 / (64 * PI * hp3);
	float C = 315 / (64 * PI * (hp3 * hp3 * hp3));

	float rp2, cbrtWeight;
	float* position = s->position;
	float* density = s->density;

	// Base density due to self
	for(int temp = 0;temp < s->particleNum;++temp){
		density[temp] = BASE;
	}

	// Addon densities due to nearby particles
	for(int temp = 0;temp < s->particleNum;++temp){
		for(int temp2 : s->nearParticleIndex[temp]){
			rp2 = getSqrDistance(position, temp, temp2);
			cbrtWeight = hp2 - rp2;
			density[temp] += C * cbrtWeight * cbrtWeight * cbrtWeight;
			density[temp2] += C * cbrtWeight * cbrtWeight * cbrtWeight;
		}
	}

	// Multiple by mass
	for(int temp = 0;temp < s->particleNum;++temp){
		density[temp] *= Params::mass;
	}

	// for(int temp = 0;temp < s->particleNum;++temp){
	// 	std::cout << "density[" << temp << "] = " << density[temp] << std::endl; 
	// }
}

void getPressure(SystemState* s){
	float* pressure = s->pressure;
	float* density = s->density;

	for (int temp = 0;temp < s->particleNum;++temp){
		pressure[temp] = Params::Cs * Params::Cs * (density[temp] - Params::rDensity);
	}
}

void getViscosityAcc(SystemState* s){
	float* v = s->vH;
	float* rho = s->density;
	float* position = s->position;
	float* a = s->acceleration;
	float h = Params::radii;
	float hp2 = h * h;
	float hp3 = hp2 * h;
	float Cv = Params::viscosity * Params::mass;
	float C = -945 / (32 * PI * hp3 * hp3 * hp3);

	float Vx, Vy, Vz, cbrtWeight, rp2, W;

	for(int temp = 0;temp < s->particleNum;++temp){
		for(int temp2 : s->nearParticleIndex[temp]){
			Vx = Cv * ((v[3 * temp2 + 0] - v[3 * temp + 0]) / (rho[temp] * rho[temp2]));
			Vy = Cv * ((v[3 * temp2 + 1] - v[3 * temp + 1]) / (rho[temp] * rho[temp2]));
			Vz = Cv * ((v[3 * temp2 + 2] - v[3 * temp + 2]) / (rho[temp] * rho[temp2]));
		
			rp2 = getSqrDistance(position, temp, temp2);
			cbrtWeight = hp2 - rp2;
			W = C * (3 * cbrtWeight * cbrtWeight - 4 * rp2 * cbrtWeight);

			a[3 * temp + 0] += Vx * W;
			a[3 * temp + 1] += Vy * W;
			a[3 * temp + 2] += Vz * W;
			a[3 * temp2 + 0] -= Vx * W;
			a[3 * temp2 + 1] -= Vy * W;
			a[3 * temp2 + 2] -= Vz * W;
		}
	}
}

void getInteractiveAcc(SystemState* s){
	float* position = s->position;
	float* a = s->acceleration;

	float C = -Params::Cij * Params::mass * Params::mass;
	std::vector<float> R;
	float r;

	for(int temp = 0;temp < s->particleNum;++temp){
		for(int temp2 : s->nearParticleIndex[temp]){
			R = getVecDistance(position, temp, temp2);
			r = sqrt(getSqrDistance(position, temp, temp2));

			a[3 * temp + 0] += C * Force(r) * R[0];
			a[3 * temp + 1] += C * Force(r) * R[1];
			a[3 * temp + 2] += C * Force(r) * R[2];
			a[3 * temp2 + 0] -= C * Force(r) * R[0];
			a[3 * temp2 + 1] -= C * Force(r) * R[1];
			a[3 * temp2 + 2] -= C * Force(r) * R[2];
		}
	}
}

float Force(float r){
	return r * cos(3 * PI / (2 * Params::k * Params::radii));
}

////////////////////////////////////////////
void leapFrogStart(SystemState* s, double timeStep){
	for(int temp = 0;temp < 3 * s->particleNum;++temp)
		s->vH[temp] = s->vF[temp] + s->acceleration[temp] * timeStep / 2;
	for(int temp = 0;temp < 3 * s->particleNum;++temp)
		s->vF[temp] += s->acceleration[temp] * timeStep;
	for(int temp = 0;temp < 3 * s->particleNum;++temp)
		s->position[temp] += s->vH[temp] * timeStep;

	reflectCheck(s);
}

void leapFrogStep(SystemState* s, double timeStep){
	for(int temp = 0;temp < 3 * s->particleNum;++temp)
		s->vH[temp] += s->acceleration[temp] * timeStep;
	for(int temp = 0;temp < 3 * s->particleNum;++temp)
		s->vF[temp] = s->vH[temp] + s->acceleration[temp] * timeStep / 2;
	for(int temp = 0;temp < 3 * s->particleNum;++temp)
		s->position[temp] += s->vH[temp] * timeStep;

	reflectCheck(s);
}

void reflectCheck(SystemState* s){
	// Boundary of the container
	const float XMIN = 0.0f;
	const float XMAX = 10.0f;
	const float YMIN = 0.0f;
	const float YMAX = 10.0f;
	const float ZMIN = 0.0f;
	const float ZMAX = 10.0f;

	// Pointers to the detecting particle
	float* vH = s->vH;
	float* vF = s->vF;
	float* position = s->position;

	for(int temp = 0;temp < s->particleNum;++temp, position += 3, vF += 3, vH += 3){
		if(position[0] < XMIN) reflect(0, XMIN, position, vF, vH);
		if(position[0] > XMAX) reflect(0, XMAX, position, vF, vH);
		if(position[1] < YMIN) reflect(1, YMIN, position, vF, vH);
		if(position[1] > YMAX) reflect(1, YMAX, position, vF, vH);
		if(position[2] < ZMIN) reflect(2, ZMIN, position, vF, vH);
		if(position[2] > ZMAX) reflect(2, ZMAX, position, vF, vH);
	}

}

void reflect(int axisIndex, float barrier, float* position, float* vF, float* vH){
	
	// Evergy lost rate when touched the wall
	const float LOSS = 0.45f;

	// Ignoring
	if (vF[axisIndex] == 0)
		return;

	// Scale Back, due to the energy consumed (so each axis has loss)
	float tBounce = (position[axisIndex] - barrier) / vF[axisIndex];
	position[0] -= vF[0] * LOSS * tBounce;
	position[1] -= vF[1] * LOSS * tBounce;
	position[2] -= vF[2] * LOSS * tBounce;

	// Reflection
	position[axisIndex] = 2 * barrier - position[axisIndex];
	vF[axisIndex] = -vF[axisIndex];
	vH[axisIndex] = -vH[axisIndex];

	// Apply velocity loss
	vH[0] *= (1 - LOSS);
	vH[1] *= (1 - LOSS);
	vH[2] *= (1 - LOSS);
	vF[0] *= (1 - LOSS);
	vF[1] *= (1 - LOSS);
	vF[2] *= (1 - LOSS);
}


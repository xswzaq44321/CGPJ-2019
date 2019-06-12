#include <iostream>
#include <cstdlib>
#include <functional>
#include <vector>
#include <cstring>
#include <cmath>

#define PI 3.14159265f
// When the makeSPHFile() function is called, will create a file that saves the position of all particles for every frame.

class SystemState{
public:
	int particleNum;
	float particleMass;
	float* density;			// Density for each particle (Only one value for each particle, the density within radii)
	float* position; 		// Position for each particle (e.g., position[0:2] for particle #0's x, y, z) [-5,5]x[-5,5]x[-5,5]
	float* vF;				// Velocity (Full Step) (e.g., vF[0:2] for particle #0's x, y, z)
	float* vH;				// Velocity (Half Step) (e.g., vH[0:2] for particle #0's x, y, z)
	float* acceleration; 	// Acceleration (e.g., a[0:2] for particle #0's x, y, z)
	std::vector<std::vector<int>> nearParticleIndex; // A vector to store each particle's neighbor's index
};

class Parameters{
public:
	std::string fileName;	// Name for output file
	int frame;				// Total frame to calculate
	int steps;				// Steps each frame, steps * timeStep = 1 time-intergral for a frame
	float radii;			//
	float timeStep;			// delta-t used in Leap-Frog
	float referenceDensity;	// 
	float bulkModules;		//
	float viscosity;		//
	float gravity;			//
};

void setInitial(std::function<int(float, float, float)> initialPosFunc, SystemState* s, Parameters* p, int particleNum, int steps, float timeStep, float radii, float referenceDensity, float bulkModules, float viscosity);
void setInitial(std::function<int(float, float, float)> initialPosFunc, SystemState* s, Parameters* p);
void setPosition(std::function<int(float, float, float)> initialPosFunc, SystemState* s, Parameters* p);
void getDensity(SystemState* s, Parameters* p);
void getAcceleration(SystemState* s, Parameters* p);
void leapFrogStart(SystemState* s, double timeStep);
void leapFrogStep(SystemState* s, double timeStep);
void reflectCheck(SystemState* s);
void reflect(int axisIndex, float barrier, float* position, float* vF, float* vH);

void setInitial(std::function<int(float, float, float)> initialPosFunc,
				SystemState* s,
				Parameters* p,
				float particleMass,
				int steps,
				float timeStep, 
				float radii, 
				float referenceDensity, 
				float bulkModules, 
				float viscosity){
	// Set the parameters
	s->particleMass = particleMass;
	p->steps = steps;
	p->radii = radii;
	p->timeStep = timeStep;
	p->referenceDensity = referenceDensity;
	p->bulkModules = bulkModules;
	p->viscosity = viscosity;
	p->gravity = 9.8f;

	setPosition(initialPosFunc, s, p);
	//s->position = new float[3 * s->particleNum];
	s->vF = new float[3 * s->particleNum];
	for(int temp = 0;temp < 3 * s->particleNum;++temp){
			s->vF[temp] = 0.0f;
	}
	s->vH = new float[3 * s->particleNum];
	s->density = new float[s->particleNum];
	s->acceleration = new float[3 * s->particleNum];
}

void setInitial(std::function<int(float, float, float)> initialPosFunc, SystemState* s, Parameters* p){
	s->particleMass = 1.0f;
	p->steps = 5;
	p->radii = 0.35f;
	p->timeStep = 0.0025f;
	p->referenceDensity = 1000.0f;
	p->bulkModules = 1000.0f;
	p->viscosity = 1000.0f;
	p->gravity = 9.8f;


	setPosition(initialPosFunc, s, p);
	s->vF = new float[3 * s->particleNum];
	for(int temp = 0;temp < 3 * s->particleNum;++temp)
		s->vF[temp] = 0.0f;
	s->vH = new float[3 * s->particleNum];
	s->density = new float[s->particleNum];
	s->acceleration = new float[3 * s->particleNum];
}

void setPosition(std::function<int(float, float, float)> initialPosFunc, SystemState* s, Parameters* p){
	int count = 0;
	float gap = p->radii / 1.3f;
	for(float x = 0.0f;x < 10.0f;x += gap){
		for(float y = 0.0f;y < 10.0f;y += gap){
			for(float z = 0.0f;z < 10.0f;z += gap){
				if(initialPosFunc(x, y, z)){
					//std::cout << "\nx = " << x << ", y = " << y << ", z = " << z << " ///" << ((x >= -1.5f && x <= 1.5f) && (y >= -1.5f && y <= 1.5f) && (z >= -1.5f && z <= 1.5f));
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
					s->position[3 * count + 0] = x;  // Initial X position
					s->position[3 * count + 1] = y; // Initial Y position
					s->position[3 * count + 2] = z; // Initial Z position
					++count;
				}
			}
		}
	}
}

// Compute density for each particle to use to calculate acceleration
void getDensity(SystemState *s, Parameters *p)
{
	//std::cout << "I denc!" << std::endl;
	int particleNum = s->particleNum;
	float* density = s->density;
	float* position = s->position;
	float r = p->radii;


	float rp2 = r * r;
	float rp8 = (rp2 * rp2) * (rp2 * rp2);
	const float C = 315.0f * s->particleMass / PI / rp8 / r / 64;
	const float BASE = 315.0 * s->particleMass / PI / rp2 / r;

	

	float dx, dy, dz, distSqrSum, temp3;

	memset(density, 0.0f, particleNum * sizeof(float)); // Initializing
	for(int temp = 0;temp < particleNum;++temp){
		density[temp] += BASE;
		for(int temp2 = temp + 1;temp2 < particleNum;++temp2){
			dx = position[3 * temp + 0] - position[3 * temp2 + 0];
			dy = position[3 * temp + 1] - position[3 * temp2 + 1];
			dz = position[3 * temp + 2] - position[3 * temp2 + 2];
			distSqrSum = dx * dx + dy * dy + dz * dz;
			temp3 = rp2 - distSqrSum;
			if(temp3 > 0){
				density[temp] += C * temp3 * temp3 * temp3;
				density[temp2] += C * temp3 * temp3 * temp3;
			}
		}
		// for(int neighbor : s->nearParticleIndex[temp])
		// 	std::cout << neighbor << " ";
		// std::cout << std::endl;
	}

	// for(int temp2 = 0;temp2 < s->particleNum;++temp2)
	// 		std::cout << s->vF[3 * temp2 + 0] << "\t" << s->vF[3 * temp2 + 1] << "\t" << s->vF[3 * temp2 + 2] << std::endl;
}

void getAcceleration(SystemState* s, Parameters* p){
	//std::cout << "I acc!" << std::endl;
	const float r = p->radii;
	const float rp2 = r * r;
	const float rho0 = p->referenceDensity;
	const float k = p->bulkModules;
	const float viscosity = p->viscosity;
	const float gravity = p->gravity;
	const float mass = s->particleMass;
	const float* density = s->density;
	const float* position = s->position;
	float* velocity = s->vF;
	float* acceleration = s->acceleration;
	int particleNum = s->particleNum;

	getDensity(s, p);
	// for(int temp2 = 0;temp2 < s->particleNum;++temp2)
	// 		std::cout << s->vF[3 * temp2 + 0] << "\t" << s->vF[3 * temp2 + 1] << "\t" << s->vF[3 * temp2 + 2] << std::endl;


	// Constants
	float C0 = mass * 945 / 24 / PI / ((rp2 * rp2) * r) / 32;
	float Cp = 15 * k;
	float Cv = -40 * viscosity;

	// Set gravity for each particle first
	for(int temp = 0;temp < particleNum;++temp){
		acceleration[3 * temp + 0] = 0.0f;
		acceleration[3 * temp + 1] = -gravity;
		acceleration[3 * temp + 2] = 0.0f;
	}

	float dx;
	float dy;
	float dz;
	float distSqrSum;
	float rho1;
	float rho2;
	float q;
	float u;
	float w0;
	float wp;
	float wv;
	float dvx;
	float dvy;
	float dvz;

	for(int temp = 0;temp < particleNum;++temp){
		rho1 = density[temp];
		for(int temp2 = temp + 1;temp2 < particleNum;++temp2){
			dx = position[3 * temp + 0] - position[3 * temp2 + 0];
			dy = position[3 * temp + 1] - position[3 * temp2 + 1];
			dz = position[3 * temp + 2] - position[3 * temp2 + 2];
			distSqrSum = dx * dx + dy * dy + dz * dz;
			if(rp2 > distSqrSum){
				rho2 = density[temp2];
				q = sqrt(distSqrSum) / r;
				u = 1.0f - q;
				w0 = C0 * u / rho1 / rho2;
				wp = w0 * Cp * (rho1 + rho2 - 2 * rho0) * u / q;
				wv = w0 * Cv;
				dvx = velocity[3 * temp + 0] - velocity[3 * temp2 + 0];
				dvy = velocity[3 * temp + 1] - velocity[3 * temp2 + 1];
				dvz = velocity[3 * temp + 2] - velocity[3 * temp2 + 2];
				acceleration[3 * temp + 0] += (wp * dx + wv * dvx);
				acceleration[3 * temp + 1] += (wp * dy + wv * dvy);
				acceleration[3 * temp + 2] += (wp * dz + wv * dvz);
				acceleration[3 * temp2 + 0] -= (wp * dx + wv * dvx);
				acceleration[3 * temp2 + 1] -= (wp * dy + wv * dvy);
				acceleration[3 * temp2 + 2] -= (wp * dz + wv * dvz);
			}
		}
	}

	// for(int temp = 0;temp < s->particleNum;++temp)
	// 	std::cout << "\ta-x = " << s->acceleration[3 * temp + 0] << 
	// 				 "\ta-y = " << s->acceleration[3 * temp + 1] << 
	// 				 "\ta-z = " << s->acceleration[3 * temp + 2] << std::endl;
}



// The following equations has no issue.

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
	//std::cout << "No I sudn't" << std::endl;
	//std::cout << "\n" << axisIndex << "\t" << barrier;
	const float LOSS = 0.25f;

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



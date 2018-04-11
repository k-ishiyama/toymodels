#include <iostream>
#include <cassert>
#include <chrono>
#include <random>

#include <src/lib/math/Math.hpp>

int main()
{
	using namespace math;

	const int seed = static_cast<int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	std::mt19937 rand_engine(seed);
	std::uniform_real_distribution<float> uniform_dist(-1e6f, 1e6f);

	float n[9];
	for (int i = 0; i < 9; ++i)
		n[i] = uniform_dist(rand_engine);
	Float3x3 mat33 = Float3x3(Float3(n[0], n[1], n[2]), Float3(n[3], n[4], n[5]), Float3(n[6], n[7], n[8]));
	assert(NearlyEqual(1.0f / Determinant(mat33), Determinant(Inverse(mat33))));

	Float3x3 inv_mat33 = Inverse(mat33);
	Float3x3 mat33B = mat33 * inv_mat33;

	return 0;
}
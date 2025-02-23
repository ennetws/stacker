#include "ShapeState.h"
#include <iostream>


double ShapeState::energy()
{
	double alpha = 1;
	return  deltaStackability * alpha - distortion * (1-alpha);
}

bool lessEnergy::operator()(ShapeState a, ShapeState b)
{
	return a.energy() < b.energy();
}

bool lessDistortion::operator()(ShapeState a, ShapeState b)
{
	return a.distortion < b.distortion;
}
#include "Improver.h"

#include "Offset.h"
#include "Primitive.h"
#include "Controller.h"
#include "Propagator.h"
#include "EditPath.h"

Improver::Improver( Offset *offset )
{
	activeOffset = offset;

	// Parameters
	NUM_EXPECTED_SOLUTION = 10;
	BB_TOLERANCE = 1.2;
	TARGET_STACKABILITY = 0.4;
	LOCAL_RADIUS = 1;
}

QSegMesh* Improver::activeObject()
{
	return activeOffset->activeObject();
}

Controller* Improver::ctrl()
{
	return (Controller*)activeObject()->ptr["controller"];
}

void Improver::setTargetStackability( double s )
{
	TARGET_STACKABILITY = s;
}

void Improver::setBBTolerance( double tol )
{
	BB_TOLERANCE = tol;
}

void Improver::setNumExpectedSolutions( int num )
{
	NUM_EXPECTED_SOLUTION = num;
}

void Improver::setLocalRadius( int R )
{
	LOCAL_RADIUS = R;
}

bool Improver::satisfyBBConstraint()
{
	bool result = true;

	Vec3d preBB = constraint_bbmax - constraint_bbmin;
	activeObject()->computeBoundingBox();
	Vec3d currBB = activeObject()->bbmax - activeObject()->bbmin;

	Vec3d diff = preBB - currBB;

	// Current BB is within expanded BB (with tolerence)
	if ( diff[0] < 0 || diff[1] < 0 || diff[2] < 0 )
		result = false;

	// debug
	//std::cout << "-----------------------------------\n"
	//	<<"The preBB size: (" << preBB <<")\n";	
	//std::cout << "The currBB size:(" << currBB <<")\n";
	//std::cout << "BB-satisfying: " << result <<std::endl;

	return result;
}

bool Improver::isUnique( ShapeState state, double threshold )
{
	// dissimilar to \solutions
	foreach(ShapeState ss, solutions){
		if (ctrl()->similarity(ss, state) < threshold)
			return false;
	}

	// dissimilar to used candidate solutions
	foreach(ShapeState ss, usedCandidateSolutions){
		if (ctrl()->similarity(ss, state) < threshold)
			return false;
	}

	// dissimilar to candidate solutions
	PQShapeStateLessEnergy candSolutionsCopy = candidateSolutions;
	while(!candSolutionsCopy.empty())
	{
		ShapeState ss = candSolutionsCopy.top();

		if (ctrl()->similarity(ss, state) < threshold)
			return false;

		candSolutionsCopy.pop();
	}

	return true;
}

void Improver::setPositionalConstriants( HotSpot& fixedHS )
{
	std::vector<HotSpot> fixedHotspots = activeOffset->getHotspots(fixedHS.side);

	foreach(HotSpot hs, fixedHotspots)
	{
		Primitive* prim = ctrl()->getPrimitive(hs.segmentID);

		// Make them fixed
		foreach(Point p, hs.rep)
			prim->addFixedPoint(p);

		// Set height constraint if \hs is height-defining
		// === To do
	}
}

void Improver::recordSolution(Point handleCenter, Vec3d localMove)
{
	activeOffset->computeStackability();
	double stackability = activeObject()->val["stackability"];

	ShapeState state = ctrl()->getShapeState();

	// \state has to be unique
//	if (!isUnique(state, 0)) return;

	// Properties
	state.deltaStackability = stackability - origStackability;
	state.distortion = ctrl()->getDistortion();

	EditPath path;
	path.center = handleCenter;
	path.move = localMove;
	path.value = state.energy();

	state.path = path;

	// Store the state
	candidateSolutions.push(state);	
}

QVector<double> Improver::getLocalScales( HotSpot& HS )
{
	QVector<double> scales;

	double stepSize = 0.1;

	double s = 0;
	for (int i = 1; i<= LOCAL_RADIUS; i++)
	{
		s += stepSize;
		scales.push_back(1 + s);
		scales.push_back(1 - s);
	}

	return scales;
}

QVector<Vec3d> Improver::getLocalMoves( HotSpot& HS )
{
	// Debug
//	std::cout << "Local radius = " << LOCAL_RADIUS << std::endl;

	QVector< Vec3d > result;

	// pos
	Vec3d hotPoint = HS.rep.first();

	// step
	Vec3d step = (constraint_bbmax - constraint_bbmin) / 20;

	// Horizontal moves
	if (HS.type == POINT_HOTSPOT)
	{
		Primitive* prim = ctrl()->getPrimitive(HS.segmentID);
		if (!(prim->primType == GCYLINDER && prim->symmPlanes.size() == 1))
		{
			double min_x = - step[0] * LOCAL_RADIUS;
			double max_x = - min_x;
			double min_y = - step[1] * LOCAL_RADIUS;
			double max_y = - min_y;

			for (double x = min_x; x <= max_x; x += step[0]){
				for (double y = min_y; y <= max_y; y += step[1]){
					{
						if(x == hotPoint.x() && y == hotPoint.y())
							continue;
						else
							result.push_back(Vec3d(x,y,0));
						
					}			
				}
			}
		}

	}
	else if ( HS.type == LINE_HOTSPOT)
	{
		// Move only perpendicular to the line
		Vec3d d = HS.rep.last() - HS.rep.first();
		Vec3d d1 = Vec3d(d.y(), - d.x(), 0);
		Vec3d d2 = Vec3d(-d.y(), d.x(), 0);
		d1.normalize();
		d2.normalize();

		for (int i = 1; i <= LOCAL_RADIUS; i++)
		{
			result.push_back(d1 * i * step.x());
			result.push_back(d2 * i * step.x());
		}
	}

	// Vertical moves
	if (!HS.defineHeight)
	{
		Vec3d delta(0.0); 
		delta[2] = (1 == HS.side)?  - step[2] : step[2];
		for (int i = 1; i <= LOCAL_RADIUS; i++)
			result.push_back(delta * i);
	}

//	std::cout << "# Moves = " << result.size() << std::endl;

	return result;
}

void Improver::deformNearPointLineHotspot( int side )
{
	// The first pair of hot spots
	HotSpot& freeHS = activeOffset->getHotspot(side, 0);
	HotSpot& fixedHS = activeOffset->getHotspot(-side, 0);
	Primitive* free_prim = ctrl()->getPrimitive(freeHS.segmentID);
	Primitive* fixed_prim = ctrl()->getPrimitive(fixedHS.segmentID);
	QVector<Point> free_handle = freeHS.rep;
	QVector<Point> fixed_hanble = fixedHS.rep;

	// Move the hotspot locally
	Propagator propagator(ctrl());
	QVector<Vec3d> Ts = getLocalMoves(freeHS);

	//// debug
	//Ts.clear();
	//Ts.push_back(Vec3d(0,0.2,0));

	foreach ( Vec3d T, Ts)
	{
		ctrl()->setPrimitivesFrozen(false);	// Clear flags
		setPositionalConstriants(fixedHS); // Fix one end

		// Move the other end
		if (freeHS.type == POINT_HOTSPOT)
		{
			free_prim->movePoint(free_handle.first(), T); 
			free_prim->addFixedPoint(free_handle.first() + T); 
		}
		else
		{
			free_prim->moveLineJoint(free_handle.first(), free_handle.last(), T, T);
			free_prim->addFixedPoint(free_handle.first() + T);
			free_prim->addFixedPoint(free_handle.last() + T);
		}

		// Fix the relation between hot segments then propagate the local modification
		propagator.regroupPair(free_prim->id, fixed_prim->id); 	
		propagator.execute(); 

		// Record the shape state
		if (freeHS.type == POINT_HOTSPOT)
			recordSolution(free_handle.first(), T);
		else
			recordSolution( (free_handle.first()+free_handle.last())/2, T);

		// Restore the shape state of current candidate
		ctrl()->setShapeState(currentCandidate);
	}
}

void Improver::deformNearRingHotspot( int side )
{
	// The first pair of hot spots
	HotSpot& freeHS = activeOffset->getHotspot(side, 0);
	HotSpot& fixedHS = activeOffset->getHotspot(-side, 0);
	Primitive* free_prim = ctrl()->getPrimitive(freeHS.segmentID);
	Primitive* fixed_prim = ctrl()->getPrimitive(fixedHS.segmentID);
	int free_cid = free_prim->detectHotCurve(freeHS.hotSamples);
	Point free_curve_center = free_prim->curveCenter(free_cid);

	// Scale the ring hot spot
	Propagator propagator(ctrl());
	QVector<double> scales = getLocalScales(freeHS);

	// debug
	//scales.clear();
	//scales.push_back(0.5);

	foreach (double scale, scales)
	{
		ctrl()->setPrimitivesFrozen(false);	// Clear flags
		setPositionalConstriants(fixedHS); // Fix one end

		// Scale the other end
		free_prim->scaleCurve(free_cid, scale);
		free_prim->addFixedCurve(free_cid);

		// Fix the relation between hot segments then propagate the local modification
		propagator.regroupPair(free_prim->id, fixed_prim->id); 
		propagator.execute(); 

//		if ( !satisfyBBConstraint() ) continue; // BB constraint is strict	

		// Record the shape state
		Vec3d delta;
		Point hotSample = freeHS.hotSamples[0];
		if (scale > 1)
			delta = hotSample - free_curve_center;
		else
			delta = free_curve_center - hotSample;

		recordSolution(hotSample, delta.normalized()/10);

		// Restore the shape state of current candidate
		ctrl()->setShapeState(currentCandidate);
	}
}

void Improver::deformNearHotspot( int side )
{
	switch (activeOffset->getHotspot(side, 0).type)
	{
	case POINT_HOTSPOT:
	case LINE_HOTSPOT:
		deformNearPointLineHotspot(side);
		break;
	case RING_HOTSPOT:
		deformNearRingHotspot(side);
		break;
	}
}

// === Main access
void Improver::execute(int level)
{
	// Clear
	solutions.clear();
	usedCandidateSolutions.clear();
	candidateSolutions = PQShapeStateLessEnergy();

	// The bounding box constraint is hard
	constraint_bbmin = activeObject()->bbmin * BB_TOLERANCE;
	constraint_bbmax = activeObject()->bbmax * BB_TOLERANCE;

	// The original stackability
	origStackability = activeOffset->computeStackability();

	// Push the current shape as the initial candidate solution
	ShapeState origState = ctrl()->getShapeState();
	candidateSolutions.push(origState);
	double currentStackability = 0;

// Timer
timer.restart();
	while( ( level>0 || level==IMPROVER_MAGIC_NUMBER )	// Suggest || Improve
		&& !candidateSolutions.empty())
	{
		// Set current
		currentCandidate = candidateSolutions.top();
		candidateSolutions.pop();
		ctrl()->setShapeState(currentCandidate);
		currentStackability = activeOffset->computeStackability();

		std::cout << "CurrStackability = " << currentStackability << "\n";

		// Solution or not
		if (currentStackability >= TARGET_STACKABILITY)
		{
			solutions.push_back(currentCandidate);
			//std::cout << solutions.size() << " solutions have been found. \n";
			continue;
		}

		// #solutions 	
		if (solutions.size() >= NUM_EXPECTED_SOLUTION) break;

		// Detect hot spots
		activeOffset->detectHotspots();
		if (activeOffset->upperHotSpots.empty() || activeOffset->lowerHotSpots.empty())
			std::cout << "\nWARNING: Hot spot detection failed.\n";

		// Local modification
		deformNearHotspot(1);
		deformNearHotspot(-1);

		// Decrease the suggesting level
		if (level != IMPROVER_MAGIC_NUMBER) level--;


		//std::cout << "One level: E = " << currentCandidate.energy() << ", currS = " << currentStackability;
		//std::cout << " #Cand = " << candidateSolutions.size() << std::endl;
	}

std::cout << "Total time = " <<(double)timer.elapsed()/60000 << " min\n";

	// Restore the original
	ctrl()->setShapeState(origState);
	std::cout << "Searching completed.\n" << std::endl;
}


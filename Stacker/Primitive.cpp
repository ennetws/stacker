#include "Primitive.h"
#include "SimpleDraw.h"

Primitive::Primitive( QSurfaceMesh* mesh, QString newId )
{
	m_mesh = mesh;

	id = newId;

	isSelected = false;
	isFrozen = false;
}

void Primitive::drawDebug()
{
	// Debug points
	foreach(Point p, debugPoints)	SimpleDraw::IdentifyPoint(p, 1,0,0);
	foreach(Point p, debugPoints2)	SimpleDraw::IdentifyPoint(p, 0,1,0);
	foreach(Point p, debugPoints3)	SimpleDraw::IdentifyPoint(p, 0,0,1);

	// Debug lines
	foreach(std::vector<Point> line, debugLines) SimpleDraw::IdentifyConnectedPoints(line, 1.0,0,0);
	foreach(std::vector<Point> line, debugLines2) SimpleDraw::IdentifyConnectedPoints(line, 0,1.0,0);
	foreach(std::vector<Point> line, debugLines3) SimpleDraw::IdentifyConnectedPoints(line, 0,0,1.0);

	foreach(std::vector<Vec3d> poly, debugPoly)		SimpleDraw::DrawPoly(poly, 1, 0, 0);
	foreach(std::vector<Vec3d> poly, debugPoly2)	SimpleDraw::DrawPoly(poly, 1, 0, 0);
}

Vec3d Primitive::centerPoint()
{
	Point centerPoint(0,0,0);

	std::vector<Point> pnts = points();
	foreach(Point p, pnts) centerPoint += p;
	return centerPoint /= pnts.size();
}

void Primitive::addFixedPoint( Point fp )
{
	if (!symmPlanes.empty() || !fixedPoints.empty())
		isFrozen = true;

	fixedPoints.push_back(fp);
}

uint Primitive::detectHotCurve( Vec3d hotSample )
{
	std::vector<Vec3d> samples;
	samples.push_back(hotSample);
	return this->detectHotCurve(samples);
}

PrimitiveState Primitive::getState()
{
	PrimitiveState state;
	state.geometry = getGeometryState();
	state.isFrozen = isFrozen;
	state.fixedPoints = fixedPoints;

	return state;
}

void Primitive::setState( PrimitiveState state)
{
	setGeometryState(state.geometry);

	isFrozen = state.isFrozen;
	fixedPoints = state.fixedPoints;
}

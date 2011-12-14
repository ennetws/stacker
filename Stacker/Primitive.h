#pragma once

#include "QSurfaceMesh.h"
#include "PrimativeParam.h"
#include "Joint.h"

class Primitive
{
public:
	Primitive(QSurfaceMesh* m_mesh, QString newId);

	// Fit primitive to the underlying QSurfaceMesh
	virtual void fit() = 0;

	// Deform the underlying geometry according to the \pre_state and current state
	virtual void deformMesh() = 0;
	virtual void deform( PrimitiveParam* params, bool isPermanent = false) = 0;

	// Visualize the primitive and potential actions
	virtual void draw() = 0;
	virtual	void drawNames(int name, bool isDrawParts = false) = 0;

	// Hot curves
	virtual uint detectHotCurve( std::vector< Vec3d > &hotSamples ) = 0;
	virtual void translateCurve( uint cid, Vec3d T, uint sid_respect ) = 0;

	// Reshaping
	virtual void translate( Vec3d &T ) {}
	virtual void moveCurveCenter( uint fid, Vec3d T) {}
	virtual void deformRespectToJoint( Vec3d joint, Vec3d p, Vec3d T) {}
	virtual bool excludePoints( std::vector< Vec3d >& pnts ) = 0;

	std::vector<Joint> joints;

	// Primitive coordinate system
	virtual std::vector<double> getCoordinate( Point v ) = 0;
	virtual Point fromCoordinate(std::vector<double> coords) = 0;

	// Helpful for debugging
	std::vector<Vec3d> debugPoints;
	std::vector< std::pair<Vec3d,Vec3d> > debugLines;
	std::vector< std::vector<Vec3d> > debugPoly;
	void drawDebug();

	virtual std::vector <Vec3d> points() = 0;
	virtual QSurfaceMesh getGeometry() = 0;
	virtual double volume() = 0;
	Vec3d centerPoint();

	QString id;
	bool isSelected;
	int selectedPartId;

	virtual Vec3d selectedPartPos() {return Vec3d(0,0,0);}
	virtual void reshapePart( Vec3d q ) {};

	QSurfaceMesh* getMesh(){ return m_mesh; }

	QSurfaceMesh*		m_mesh;			// The underlying geometry
	bool				isHot;			// Is this hot component?
	bool				isDirty;		// Has the underlying geometry been updated?
};

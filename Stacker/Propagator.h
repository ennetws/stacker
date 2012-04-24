#pragma once

#include <QVector>
#include <QString>

#include "ShapeState.h"
#include "ConstraintGraph.h"

class Controller;


class Propagator
{
public:
	Propagator( Controller* ctrl );

	// Regroup pair
	void regroupPair(QString id1, QString id2);

	// Propagation
	void execute();

	// Solve constraints
	void solveConstraints(QString target, QVector<ConstraintGraph::Edge> constraints);

private:
	Controller *		mCtrl;
	ConstraintGraph *	mGraph;
};
#include "GraphicsLibrary/Mesh/QSegMesh.h"

#include "HiddenViewer.h"

HiddenViewer::HiddenViewer( QWidget * parent ) : QGLViewer (parent)
{
	// Restrict the size of the window
	setFixedSize(200, 200);

	// No active scene when initializing
	this->_activeObject = NULL;

	// Initial render mode as HV_NONE
	mode = HV_NONE;
}

void HiddenViewer::init()
{
	setBackgroundColor(backColor = palette().color(QPalette::Window));

	// Lights
	setupLights();

	// Camera
	setupCamera();

	// Material
	float mat_ambient[] = {0.1745f, 0.01175f, 0.01175f, 1.0f};
	float mat_diffuse[] = {0.65f, 0.045f, 0.045f, 1.0f};
	float mat_specular[] = {0.09f, 0.09f, 0.09f, 1.0f};
	float high_shininess = 100;

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, high_shininess);

	camera()->setType(Camera::ORTHOGRAPHIC);
}

void HiddenViewer::setupCamera()
{
	camera()->setUpVector(Vec(0,0,1));
	camera()->setPosition(Vec(2,-2,2));
	camera()->lookAt(Vec());
}

void HiddenViewer::setupLights()
{
	GLfloat lightColor[] = {0.9f, 0.9f, 0.9f, 1.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
}

void HiddenViewer::draw()
{
	switch (mode)
	{
	case HV_NONE:
		break;
	case HV_DEPTH:
//		std::cout << "Hidden Viewer: DEPTH\n";
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		activeObject()->simpleDraw();
		break;
	case HV_FACEUNIQUE:
//		std::cout << "Hidden Viewer: FACEUNIQUE\n";
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		activeObject()->drawFacesUnique();
		break;
	}

	setMode(HV_NONE);
}

QSegMesh* HiddenViewer::activeObject()
{
	return _activeObject;
}

void HiddenViewer::setActiveObject( QSegMesh * changedObject )
{
	_activeObject = changedObject;
	this->updateGL();
}

void HiddenViewer::setMode( HVMode toMode )
{
	mode = toMode;
}

void* HiddenViewer::readBuffer( GLenum format, GLenum type )
{
	void * data = NULL;

	int w = this->width();
	int h = this->height();

	switch(format)
	{
	case GL_DEPTH_COMPONENT:
		data = new GLfloat[w*h];
		break;

	case GL_RGBA:
		data = new GLubyte[w*h*4];
		break;
	}

	glReadPixels(0, 0, w, h, format, type, data);

	return data;
}

void HiddenViewer::setResolution( int newRes )
{
	setFixedSize(newRes,newRes);
}

#include "Offset.h"


Offset::Offset( Scene *scene )
{
	activeScene = scene;
	isDirty = true;

	O_max = 0.;
}

Offset::~Offset()
{

}

std::vector< std::vector<double> > Offset::computeEnvelope( int direction )
{
	QSegMesh * activeObject = activeScene->activeObject();

	// Set camera
	activeScene->camera()->setType(Camera::ORTHOGRAPHIC);
	activeScene->camera()->setPosition(Vec(0,0, direction * activeObject->radius));
	activeScene->camera()->lookAt(Vec());	
	activeScene->camera()->setUpVector(Vec(1,0,0));
	activeScene->camera()->fitBoundingBox(Vec(activeObject->bbmin), Vec(activeObject->bbmax));

	// Save this new camera settings
	activeScene->camera()->addKeyFrameToPath(direction + 2);

	// Compute the envelop (z value)
	double zCamera = (activeScene->camera()->position()).z;

	activeScene->specialRenderMode = DEPTH;
	activeScene->updateGL(); // draw
	activeScene->specialRenderMode = REGULAR;

	GLfloat* depthBuffer = (GLfloat*)activeScene->readBuffer(GL_DEPTH_COMPONENT, GL_FLOAT);

	int w = activeScene->width();
	int h = activeScene->height();
	double zNear = activeScene->camera()->zNear();
	double zFar = activeScene->camera()->zFar();
	std::vector< std::vector<double> > envelop(h);

	for(int y = 0; y < h; y++)
	{
		envelop[y].resize(w);

		for(int x = 0;x < w; x++)
		{
			double zU = depthBuffer[(y*w) + x];
			if (zU == 1.0)
				envelop[y][x] = FLOAT_INFINITY;
			else
				envelop[y][x] = zCamera - direction * ( zU * zFar + (1-zU) * zNear );
		}
	}

	delete[] depthBuffer;

	return envelop;
}

void Offset::computeOffset()
{
	QSegMesh * activeObject = activeScene->activeObject();

	// Compute the height of the shape
	objectH = (activeObject->bbmax - activeObject->bbmin).z();

	// Save original camera settings
	activeScene->camera()->addKeyFrameToPath(0);

	// Compute the offset function
	upperEnvolope = computeEnvelope(1);
	lowerEnvolope = computeEnvelope(-1);
	offset = upperEnvolope; 
	int h = upperEnvolope.size();
	int w = upperEnvolope[0].size();
	std::vector<double> row_max;	

	for (int y = 0; y < h; y++){
		for (int x = 0; x < w; x++)
		{
			if (upperEnvolope[y][x]==DOUBLE_INFINITY | lowerEnvolope[y][(w-1)-x]==DOUBLE_INFINITY)
				offset[y][x] = 0.0; //out the shape domain
			else
				offset[y][x] = upperEnvolope[y][x] - lowerEnvolope[y][(w-1)-x]; //in the shape domain
		}

		row_max.push_back(*max_element(offset[y].begin(), offset[y].end()));
	}

	O_max = *max_element(row_max.begin(), row_max.end());
}

std::set<uint> Offset::verticesOnEnvelope( int direction )
{
	//return value
	std::set<uint> vertices;	
	QSegMesh * activeObject = activeScene->activeObject();	

	// restore camera
	activeScene->camera()->playPath(direction + 2);

	//rend the faces with unique color
	int w = activeScene->width();
	int h = activeScene->height();	

	activeScene->specialRenderMode = UNIQUE_FACES;
	activeScene->updateGL();
	activeScene->specialRenderMode = REGULAR;

	GLubyte* colormap = (GLubyte*)activeScene->readBuffer(GL_RGBA, GL_UNSIGNED_BYTE);

	//get the indices back
	for(int y = 0; y < h; y++){
		for(int x = 0; x < w; x++)	{

			uint indx = ((y*w)+x)*4;

			uint r = (uint)colormap[indx+0];
			uint g = (uint)colormap[indx+1];
			uint b = (uint)colormap[indx+2];
			uint a = (uint)colormap[indx+3];

			uint f_id = ((255-a)<<24) + (r<<16) + (g<<8) + b;

			if(f_id > 0 && f_id < (activeObject->n_faces() + 1))
			{
				std::vector<uint> cur_vertices = activeObject->vertexIndicesAroundFace(f_id - 1);
				vertices.insert(cur_vertices.begin(), cur_vertices.end());
			}
		}
	}

	delete colormap;
	return vertices;
}

void Offset::setOffsetColors( int direction, std::vector< std::vector<double> > &offset, double O_max )
{
	QSegMesh * activeObject = activeScene->activeObject();

	// Restore camera settings
	activeScene->camera()->playPath(direction + 2);

	int h = offset.size();
	int w = offset[0].size();
	uchar * rgb = new uchar[3];	

	// Get all the vertices on the current envelope 
	std::set<uint> vindices = verticesOnEnvelope(direction);

	// Assign each vertex with offset color
	for (std::set<uint>::iterator it = vindices.begin(); it!=vindices.end(); it++)
	{
		Point src = activeObject->getVertexPos(*it);
		Vec vpixel = activeScene->camera()->projectedCoordinatesOf(Vec(src));

		// For flipping
		int _x = (direction) == 1 ? vpixel.x : (w-1) - vpixel.x;

		int x = RANGED(0, _x, (w-1));
		int y = RANGED(0, (h-1)-vpixel.y, (h - 1));

		// Average color around pixel
		double vertexColor = offset[y][x] / O_max;

		ColorMap::jetColorMap(rgb, vertexColor, 0, 1);

		double r = rgb[0] / 255.0;
		double g = rgb[1] / 255.0;
		double b = rgb[2] / 255.0;

		activeObject->setVertexColor(*it, Color(r,g,b,1));
	}

	delete[] rgb;
}

void Offset::run()
{
	if(!activeScene) return;

	QSegMesh * activeObject = activeScene->activeObject();
	if (!activeObject->isReady)	return;



	// Compute upper and lower envelops and offset function
	computeOffset();

	// Assign each vertex with offset color
	setOffsetColors(1, offset, O_max);
	setOffsetColors(-1, offset, O_max);

	//// Save the offset function to an image
	saveOffsetAsImage("offset_function.png");

	// Restore original camera settings 
	activeScene->camera()->setType(Camera::PERSPECTIVE);
	activeScene->camera()->resetPath(0);

	activeScene->displayMessage(QString("O_max / objectH = %1").arg(O_max / objectH));
	activeScene->print(QString("Offset function computing has done!"));
}



void Offset::setDirty( bool dirty)
{
	isDirty = dirty;
}

double Offset::getMaxOffset()
{
	return O_max;
}

void Offset::saveOffsetAsImage( QString fileName )
{
	int h = offset.size();
	int w = offset[0].size();
	QImage offset_img(w, h, QImage::Format_ARGB32);

	uchar * rgb = new uchar[3];		
	for(int y = 0; y < h; y++){
		for(int x = 0; x < w; x++)	{
			ColorMap::jetColorMap(rgb, Max(0., offset[y][x] / O_max), 0., 1.);
			offset_img.setPixel(x, (h-1) - y, QColor::fromRgb(rgb[0],rgb[1],rgb[2]).rgba());			
		}
	}

	delete[] rgb;
	offset_img.save(fileName);
}

double Offset::getStackability()
{
	return 1 - O_max/objectH;
}

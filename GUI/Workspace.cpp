#include "Workspace.h"
#include <QVBoxLayout>
#include <QFileInfo>

Workspace::Workspace(QWidget *parent, Qt::WFlags flags)	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	ui.leftDockWidget->setLayout(new QVBoxLayout);
	ui.rightDockWidget->setLayout(new QVBoxLayout);

	sp = new StackerPanel();
	ui.leftDockWidget->layout()->addWidget(sp);

	wp = new WiresPanel();
	ui.rightDockWidget->layout()->addWidget(wp);

	dp = new DeformerPanel();
	ui.rightDockWidget->layout()->addWidget(dp);

	// Create MeshDoc, where stores all the meshes
	mDoc = new QMeshDoc();
	connect(ui.actionImportObject, SIGNAL(triggered()), mDoc, SLOT(importObject()));

	// Add new scene action
	connect(ui.actionNewScene, SIGNAL(triggered()), SLOT(addNewScene()));

	// Create new scene when we start by default
	sceneCount = 0;
	addNewScene();
}

Workspace::~Workspace()
{
	
}

void Workspace::addNewScene()
{
	Scene * newScene = new Scene;

	ui.sceneArea->addSubWindow(newScene);
	sceneCount++;

	newScene->showMaximized();
	newScene->setWindowTitle("Untitled");

	// Workspace window
	connect(newScene, SIGNAL(gotFocus(Scene*)), SLOT(setActiveScene(Scene*)));

	// MeshDoc
	connect(newScene, SIGNAL(objectDiscarded(QString)), mDoc, SLOT(deleteObject(QString)));
	
	// Stack panel
	connect(newScene, SIGNAL(gotFocus(Scene*)), sp, SLOT(setActiveScene(Scene*)));
	connect(newScene, SIGNAL(objectInserted()), sp, SLOT(updateActiveObject()));
	connect(newScene, SIGNAL(sceneClosed(Scene*)), sp, SLOT(setActiveScene(Scene*)));
	connect(sp, SIGNAL(printMessage(QString)), newScene, SLOT(print(QString)));
	connect(sp, SIGNAL(objectModified()), newScene, SLOT(updateActiveObject()));

	// Wires
	connect(newScene, SIGNAL(gotFocus(Scene*)), wp, SLOT(setActiveScene(Scene*)));
	connect(wp, SIGNAL(wiresFound(QVector<Wire>)), newScene, SLOT(setActiveWires(QVector<Wire>)));
	connect(wp, SIGNAL(wiresFound(QVector<Wire>)), newScene, SLOT(updateGL()));

	// Deformation
	connect(newScene, SIGNAL(gotFocus(Scene*)), dp, SLOT(setActiveScene(Scene*)));
	connect(dp, SIGNAL(deformerCreated(QFFD *)), newScene, SLOT(setActiveDeformer(QFFD *)));

	// Update stacker panel
	sp->setActiveScene(newScene);

	this->setActiveScene(newScene);
}

void Workspace::setActiveScene(Scene* scene)
{
	activeScene = scene;

	QString title = QString("%1 - %2")
		.arg(QFileInfo(QApplication::applicationFilePath()).baseName())
		.arg(scene->windowTitle());

	this->setWindowTitle(title);

	// Disconnect Mesh Doc with from all scenes
	foreach (QMdiSubWindow *window, ui.sceneArea->subWindowList()) {
		Scene *s = qobject_cast<Scene *>(window->widget());
		s->disconnect(mDoc);
		s->disconnect(ui.actionExportObject);
	}

	activeScene->connect(mDoc, SIGNAL(objectImported(QSegMesh*)), SLOT(setActiveObject(QSegMesh*)), Qt::UniqueConnection);
	activeScene->connect(ui.actionExportObject, SIGNAL(triggered()), SLOT(exportActiveObject()), Qt::UniqueConnection);
	activeScene->connect(mDoc, SIGNAL(printMessage(QString)), SLOT(print(QString)), Qt::UniqueConnection);

	mDoc->connect(activeScene, SIGNAL(exportActiveObject(QSegMesh*)), SLOT(exportObject(QSegMesh*)), Qt::UniqueConnection);
}

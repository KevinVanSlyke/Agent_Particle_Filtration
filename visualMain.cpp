//  Adam Sokolow
//  adam.sokolow@duke.edu
//  Dust simulation for Dr. Sen
//  Dated July 29 2005

/*	Edited by Kevin VanSlyke
kgvansly@buffalo.edu
Dated Jan 2 2016*/

#include <cmath>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>
#include <math.h>
#include <errno.h>
#include <time.h>

/*	direct.h for windows filesystem manip
	unistd.h and sys/stat.h for unix filesystem manip*/
//#include <direct.h>
#include <unistd.h>
#include <sys/stat.h>
#include "timer.h"
#include <GL/glut.h>

#include "world.h"
#include "parameterReader.h"

using namespace std;
	struct timespec before, after;
	struct timespec time_diff;
	double time_s;

bool zoom, offvisual = false;
int myXMax, myYMax, myTMax, myPMax;
world * myWorld;
int viewStartX = 0;
int viewStartY = 0;
int viewEndX = 500;
int viewEndY = 500;
int windowWidth = 1000;
int windowHeight = 1000;
int timeCount = 0;
int magnification = 1;
int running = 0;
int selectX1, selectY1, selectX2, selectY2;
int pad = 3;
int prev_w = 700; 
int prev_h = 700;
GLubyte * myWorldImage;

void myIdle();
void takestep();
void(*Functor)() = myIdle;
void init();
void display();
void reshape(int w, int h);
void mouse(int button, int state, int x, int y);
void setView(int Sx, int Sy, int Ex, int Ey);
void resetView();
GLubyte * updateWorldImage(int ** worldArray);
void initWorldImage();

struct Box {    /* pixel coordinates for mouse events */
	int left;
	int right;
	int top;
	int bottom;
} L_border, R_border, T_border, B_border, button_box[3], dustWindow;

int isInside(struct Box *box, int x, int y) {
	return box->left < x && box->right > x
		&& box->top < y && box->bottom > y;
}

void myIdle()
{
	glutPostRedisplay();
	myWorld->updateWorld();
}

void takestep()
{
	++timeCount;
	myWorld->takeStep();
	myWorld->updateWorld();
	myWorld->writingDust(); //tracking dust particles
	if(timeCount > myTMax)
	{
		glutDestroyWindow(glutGetWindow());
		/* Runs program for determined amount of time. */
		get_time(&after);
		diff(&before, &after, &time_diff);
		time_s = time_diff.tv_sec + (double)(time_diff.tv_nsec)/1.0e9;
		std::cout << "Total runtime is: " << time_s << " seconds." << std::endl;

		ofstream timeOptimization("timeOptimization.txt", ios::app);
		if (timeOptimization.is_open())
		{
			//writing: time of calculation, number of particles and length/width
			timeOptimization << "Total runtime is: " << time_s << " seconds." << std::endl;
			timeOptimization << "For " << myTMax << "timesteps." << std::endl;
			timeOptimization << "Simulation of " << myPMax << " particles in an area of:" << std::endl;
			timeOptimization << myXMax << " by " << myYMax << ". (x by y)" << std::endl;
			timeOptimization.close();
		}
		std::cout << "Simulation completed!" << std::endl;
		exit(0);
	}
	if (!offvisual)
		glutPostRedisplay();
}

void init()
{
	glClearColor(1.0, 195 / 255.0, 0.0, 0.0);
	glShadeModel(GL_FLAT);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	resetView();
}

void drawText(const string& str, int x, int y)
{
	glRasterPos2i(x, y);
	int len = str.find('\0');
	for (int i = 0; i < len; i++)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, str[i]);
}

void display(void)
{
	glRasterPos2i(0,0);  //Specifies the raster position for pixel operations. 

	myWorldImage = updateWorldImage(myWorld->getWorldArray());
	glDrawPixels(myXMax, myYMax, GL_RGB, GL_UNSIGNED_BYTE, myWorldImage);
	GLfloat xZoom = windowWidth/myXMax;
	GLfloat yZoom = windowHeight/myYMax;
	glPixelZoom(xZoom, yZoom);
//	glPixelZoom(1.0, 1.0); //zooming display region, original factor=1,1 

	glutSwapBuffers();         // swaps the buffers of the current window 
	glFlush();
}

void reshape(int w, int h)
{
	windowWidth = w;
	windowHeight = h;
	GLfloat xZoom = windowWidth/myXMax;
	GLfloat yZoom = windowHeight/myYMax;
	glPixelZoom(xZoom, yZoom);
	glViewport((GLint)-windowWidth/2, (GLint)-windowHeight/2, (GLsizei)windowWidth, (GLsizei)windowHeight);
}

void mouse(int button, int state, int x, int y)
{
	//
}

GLubyte * updateWorldImage(int ** worldArray)   // GLubyte * <declare "getWorldImage()" member> 
{
	GLubyte * worldImage = myWorldImage;
	if (viewStartX < viewEndX && viewStartY < viewEndY)
	{

		for (int c = 0; c < myXMax; c++)
		{
			for (int d = 0; d < myYMax; d++)
			{
				worldImage[3 * (myXMax * (c)+d) + 0] = (GLubyte)0;
				worldImage[3 * (myXMax * (c)+d) + 1] = (GLubyte)0;
				worldImage[3 * (myXMax * (c)+d) + 2] = (GLubyte)0;
			}
		}
		for (int c = viewStartX; c < viewEndX; c++) 
		{
			for (int d = viewStartY; d < viewEndY; d++) 
			{
				for (int m1 = 0; m1 < magnification; ++m1)
				{
					for (int m2 = 0; m2 < magnification; ++m2)
					{
						worldImage[3 * (myXMax * (magnification*(c - viewStartX) + m1) + magnification*(d - viewStartY) + m2) + 0] = (GLubyte)(worldArray[c][d] >= 0 ? (int)(1 * ((worldArray[c][d] + 2)*(worldArray[c][d] + 2)*(worldArray[c][d] + 2))) % 255 : 255);
						worldImage[3 * (myXMax * (magnification*(c - viewStartX) + m1) + magnification*(d - viewStartY) + m2) + 1] = (GLubyte)(worldArray[c][d] >= 0 ? (int)(100 * (worldArray[c][d] + 1) % 255) : 255);
						worldImage[3 * (myXMax * (magnification*(c - viewStartX) + m1) + magnification*(d - viewStartY) + m2) + 2] = (GLubyte)(worldArray[c][d] >= 0 ? (int)(200 * ((worldArray[c][d] + 2)*(worldArray[c][d] + 2))) % 255 : 255);
					}
				}
			}
		}
	}
	return worldImage;

}

void setView(int Sx, int Sy, int Ex, int Ey)
{
	viewStartX = Sx;
	viewStartY = Sy;
	viewEndX = Ex;
	viewEndY = Ey;
}

void resetView()
{
	viewStartX = 0;
	viewStartY = 0;
	viewEndX = myXMax;
	viewEndY = myYMax;
	magnification = 1;
}

void initWorldImage()
{
	myWorldImage = new GLubyte[myXMax * myYMax * 3];
}

// *******
// M A I N
// *******
int main(int argc, char *argv[])
{
	int trialID;
	get_time(&before);


	/////////////////////
	/* Parameters so far:

	World Dimensions: X, Y
	World Speeds: x, y

	Total Dust: n
	Size Range of Dust: 1 -> N

	Total timesteps: t
	Enable: merging, splitting, sticking
	Filter Parameters: Length, Width of Slits, Width of Gaps
	trialID
	*/
	int filter, xMax, yMax, xSpeed, ySpeed, totalGrains, minGrainSize, maxGrainSize, maxTime, sticking, splitting, merging, filterWidth, filterGap, FilterLength, filter2Width, filter2Gap, Filter2Length;
	bool enableSticking, enableSplitting, enableMerging;
	if (argc < 12)
	{
		std::cout << "Not enough parameters passed to main (Need 11, 14 or 17 integers after the executable name), trying to read from parameters.txt... " << std::endl;
		parameterReader *pR = new parameterReader();
		xMax = pR->getxMax();
		yMax = pR->getyMax();
		xSpeed = pR->getxSpeed();
		ySpeed = pR->getySpeed();
		totalGrains = pR->gettotalGrains();
		maxTime = pR->getMaxTime();
		enableSticking = pR->getSticking();
		enableSplitting = pR->getSplitting();
		enableMerging = pR->getMerging();
		minGrainSize = pR->getMinGrainSize();
		maxGrainSize = pR->getMaxGrainSize();
		filterWidth = pR->getFilterWidth();
		filterGap = pR->getFilterGap();
		FilterLength = pR->getFilterLength();
		filter2Width = pR->getFilter2Width();
		filter2Gap = pR->getFilter2Gap();
		Filter2Length = pR->getFilter2Length();
		trialID = pR->getTrialID();
	}
	else if(argc == 13)
	{
		xMax = atof(argv[1]);
		yMax = atof(argv[2]);
		xSpeed = atof(argv[3]);
		ySpeed = atof(argv[4]);
		totalGrains = atof(argv[5]);
		minGrainSize = atof(argv[6]);
		maxGrainSize = atof(argv[7]);
		maxTime = atof(argv[8]);
		sticking = atof(argv[9]);
		if (sticking == 1)
			enableSticking = true;
		else
			enableSticking = false;
		splitting = atof(argv[10]);
		if (splitting == 1)
			enableSplitting = true;
		else
			enableSplitting = false;
		merging = atof(argv[11]);
		if (merging == 1)
			enableMerging = true;
		else
			enableMerging = false;
		trialID = atof(argv[12]);
		filterWidth = -1;
		filterGap = -1;
		FilterLength = -1;
		filter2Width = -1;
		filter2Gap = -1;
		Filter2Length = -1;
		filter = 0;
	}
	else if(argc == 16)
	{
		xMax = atof(argv[1]);
		yMax = atof(argv[2]);
		xSpeed = atof(argv[3]);
		ySpeed = atof(argv[4]);
		totalGrains = atof(argv[5]);
		minGrainSize = atof(argv[6]);
		maxGrainSize = atof(argv[7]);
		maxTime = atof(argv[8]);
		sticking = atof(argv[9]);
		if (sticking == 1)
			enableSticking = true;
		else
			enableSticking = false;
		splitting = atof(argv[10]);
		if (splitting == 1)
			enableSplitting = true;
		else
			enableSplitting = false;
		merging = atof(argv[11]);
		if (merging == 1)
			enableMerging = true;
		else
			enableMerging = false;
		filterWidth = atof(argv[12]);
		filterGap = atof(argv[13]);
		FilterLength = atof(argv[14]);
		trialID = atof(argv[15]);
		filter2Width = -1;
		filter2Gap = -1;
		Filter2Length = -1;
		filter = 1;
	}
	else if(argc == 19)
	{
		xMax = atof(argv[1]);
		yMax = atof(argv[2]);
		xSpeed = atof(argv[3]);
		ySpeed = atof(argv[4]);
		totalGrains = atof(argv[5]);
		minGrainSize = atof(argv[6]);
		maxGrainSize = atof(argv[7]);
		maxTime = atof(argv[8]);
		sticking = atof(argv[9]);
		if (sticking == 1)
			enableSticking = true;
		else
			enableSticking = false;
		splitting = atof(argv[10]);
		if (splitting == 1)
			enableSplitting = true;
		else
			enableSplitting = false;
		merging = atof(argv[11]);
		if (merging == 1)
			enableMerging = true;
		else
			enableMerging = false;
		filterWidth = atof(argv[12]);
		filterGap = atof(argv[13]);
		FilterLength = atof(argv[14]);
		filter2Width = atof(argv[15]);
		filter2Gap = atof(argv[16]);
		Filter2Length = atof(argv[17]);
		trialID = atof(argv[18]);
		filter = 2;
	}
	myXMax = xMax;
	myYMax = yMax;
	myTMax = maxTime;
	myPMax = totalGrains;
	glutInit(&argc, argv);
	initWorldImage();
	
	myWorld = new world(xMax, yMax, xSpeed, ySpeed);
	/*Sets the simulation size in the dust_list object for use in dust_list routines*/
	myWorld->myList->setMaxXLoc(xMax);
	myWorld->myList->setMaxYLoc(yMax);
	//TODO: Make arbitrary for any number of filter lines appended to end of parameter text file.
	if (filterGap == (-1) && filter2Gap == (-1))
	{
		filter = 0;
		myWorld->populateWorld(totalGrains, minGrainSize, maxGrainSize);
		std::cout << "No Filter detected." << std::endl;
	}
	else if (filterGap != (-1) && filter2Gap == (-1))
	{
		filter = 1;
		myWorld->populateWorld(totalGrains, minGrainSize, maxGrainSize, filterGap, filterWidth, FilterLength);
		std::cout << "Filter gap: " << filterGap << std::endl;
	}
	else if (filterGap != (-1) && filter2Gap != (-1))
	{
		filter = 2;
		myWorld->populateWorld(totalGrains, minGrainSize, maxGrainSize, filterGap, filterWidth, FilterLength, filter2Gap, filter2Width, Filter2Length);
		std::cout << "Filter 1 gap: " << filterGap << std::endl;
		std::cout << "Filter 2 gap: " << filter2Gap << std::endl;
	}

	/*  Routine for creating folder, need to make alternate version for Linux file systems*/

	std::cout << "Total No. of Dust Grains =  " << myWorld->myList->getTotal() - filter << std::endl;
	std::cout << "X Max = " << myWorld->getMaxXSize() << ". Y Max = " << myWorld->getMaxYSize() << ". " << std::endl;
	//chdir("/gpfs/scratch/kgvansly/");
	//chdir("/projects/academic/sen/kgvansly/Dust_Data");
	chdir("/home/kevin/Dust_Data");
	std::ostringstream oFolder;
	oFolder << filter << "fltrs" << filterGap << "pr" << filterWidth << "fbr" << FilterLength << "fl"<< totalGrains << "ptcls" << minGrainSize << "-" << maxGrainSize << "dstr" << xMax << "x" << yMax << "y" << xSpeed << "vx" << ySpeed << "vy" << maxTime << "tm";
	std::string outputFolder = oFolder.str();

	if(mkdir(outputFolder.c_str(), S_IRWXU) == -1)
		std::cout << "Folder " << outputFolder << " already exists, entering..." << std::endl;
	else
		mkdir(outputFolder.c_str(), S_IRWXU);
	chdir(outputFolder.c_str());

	struct stat fileInfo;
	if (stat("parameters.txt", &fileInfo) == 0)
	{
		std::cout << "parameters.txt already exists, not writting" << std::endl;
	}
	else
	{
		FILE * parameterFile = fopen("parameters.txt", "a");
		// OUTPUT: dust, size, y-position, y-step, x-localSp, y-localSp
		fprintf(parameterFile, "%d %d \n%d %d \n%d \n%d %d \n%d \n%d %d %d \n%d %d %d \n%d %d %d", xMax, yMax, xSpeed, ySpeed, totalGrains, minGrainSize, maxGrainSize, maxTime, sticking, splitting, merging, filterWidth, filterGap, FilterLength, filter2Width, filter2Gap, Filter2Length);
		fclose(parameterFile);
	}

	std::ostringstream pFolder;
	pFolder << "Trial" << trialID;
	std::string processFolder = pFolder.str();
	mkdir(processFolder.c_str(), S_IRWXU);
	chdir(processFolder.c_str());
	/* End of folder creation routine */

	/* Statistics collection calls, might be better to move them to lower level objects. */
	myWorld->myList->dust_dstr();
	myWorld->myList->setFunctionality(enableSplitting, enableSticking, enableMerging);
	//Debug line
	//std::cout << "Dust distribution has been calculated." << std::endl;
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);

	glutCreateWindow("Dust Simulation");

	std::cout << "Drawing Window" << std::endl;

	init();
	glutDisplayFunc(display);

	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutIdleFunc(takestep);
	glutMainLoop();
}

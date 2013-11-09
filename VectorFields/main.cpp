//Adam Bodnar
//Scientific Visualization
// Project 3
// 3/29/13

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cassert>
#include <string>
#include <sstream>
#include <time.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glut.h>

using namespace std;

static const int VPD_DEFAULT = 800;
static float tick = 0;

static float mousex, mousey;
static int mode = 0;
static float zoom = 4;

static const int MENU_SYNTH = 0;
static const int MENU_OCEAN = 1;
static const int MENU_IN = 2;
static const int MENU_OUT = 3;

GLint wid;               /* GLUT window id; value asigned in main() and should stay constant */
GLint vpw = VPD_DEFAULT; /* viewport dimensions; change when window is resized (resize callback) */
GLint vph = VPD_DEFAULT;

GLint TickLoc;
GLint ResLoc;
GLint MouseLoc;
GLint ModeLoc;
GLint ZoomLoc;

GLuint ProgramHandle;   
GLint QuadVAO;

struct rgb {
	unsigned char r,g,b;
};

rgb* readPPM(string name, int& x, int& y){
	
	rgb *pixels;

	ifstream ifs ( name, ios::binary );
	if (!ifs)
	{
		cerr << "Can't open " << name << endl;
		exit(1);
	}
	char c;
	ifs >> c;
	assert(c=='P');
	ifs >> c;
	assert(c=='6');   // binary PPMs start with the magic code `P6'
	ifs >> x >> y;
	int i;
	ifs >> i;
	assert(i==255);  // all images we'll use have 255 color levels
	ifs.get(); // need to skip one more byte
	pixels = new rgb[x*y];
	ifs.read((char*)pixels,x*y*sizeof(rgb));

	return pixels;
}

float* readRaw(char* name){
	float* T = new float[1440*720*50];

	FILE* pFile = fopen ( name , "rb" );
	assert(pFile);
	fread((void*)(T), sizeof(float), 1440*720*50, pFile);

	return T;
}
int resolution_x, resolution_y;
rgb* pixels2D = readPPM("3.ppm",resolution_x, resolution_y);

void uploadRAW(GLuint texture, float* T, int n)
{
	glActiveTexture(GL_TEXTURE0+n);
	glBindTexture(GL_TEXTURE_3D,texture);
	glTexImage3D(GL_TEXTURE_3D,0,GL_R32F,1440,720,50,0,GL_RED,GL_FLOAT,T);
	glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}
void uploadRGB(GLuint texture, rgb* T, int n)
{
	glActiveTexture(GL_TEXTURE0+n);
	glBindTexture(GL_TEXTURE_2D,texture);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,VPD_DEFAULT,VPD_DEFAULT,0,GL_RGB,GL_UNSIGNED_BYTE,T);
	//glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,resolution_x,resolution_y,0,GL_RGB,GL_UNSIGNED_BYTE,pixels2D);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}

int TextureSetup ()
{
	rgb* tex = new rgb[VPD_DEFAULT*VPD_DEFAULT];
	float* U = readRaw("U.raw");
	float* V = readRaw("V.raw");
	assert(pixels2D);
	srand (time(NULL));
	for(int i=0; i< VPD_DEFAULT*VPD_DEFAULT; i++){
		char intensity = rand() % 265;
		//tex[i].r = tex[i].g = tex[i].b = intensity;
		tex[i].r = rand() % 265;
		tex[i].g = rand() % 265;
		tex[i].b = rand() % 265;
		//tex[i].g = tex[i].r = rand() % 265;
		//tex[i].b = 255;
	}
	GLuint TexHandle[3];  // texture handle
	glGenTextures(3,TexHandle);

	//uploadRGB(TexHandle[0], pixels2D, 0);
	uploadRGB(TexHandle[0], tex, 0);
	uploadRAW(TexHandle[1], U, 1);
	uploadRAW(TexHandle[2], V, 2);
	return 0;
}


string ReadFromFile ( const char *name )
{
	ifstream ifs(name);
	if (!ifs)
	{
		cout << "Can't open " << name << endl;
		exit(1);
	}
	stringstream sstr;
	sstr << ifs.rdbuf();
	return sstr.str();
}


void PrintInfoLog ( GLuint obj )
{
	int infologLength = 0;
	int maxLength;
 
	if(glIsShader(obj))
		glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
	else
		glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
 
	char* infoLog = new char[maxLength];
  
	if (glIsShader(obj))
		glGetShaderInfoLog(obj, maxLength, &infologLength, infoLog);
	else
		glGetProgramInfoLog(obj, maxLength, &infologLength, infoLog);
  
	if (infologLength > 0)
		cout << infoLog << endl;
}


void SetUpProgram()
{
	GLuint vid = glCreateShader(GL_VERTEX_SHADER);
	GLuint fid = glCreateShader(GL_FRAGMENT_SHADER);

	string VertexSrc = ReadFromFile("vertex.glsl");
	string FragmentSrc = ReadFromFile("fragment.glsl");

	const char *VS = VertexSrc.c_str();
	const char *FS = FragmentSrc.c_str();

	// set source of the shaders
	glShaderSource(vid,1,&VS,NULL);
	glShaderSource(fid,1,&FS,NULL);

	// compile
	glCompileShader(vid);
	glCompileShader(fid);

	// any problems?
	cout << "Vertex Shader Log: " << endl;
	PrintInfoLog(vid);
	cout << "Fragment Shader Log: " << endl;
	PrintInfoLog(fid);

	// create program
	ProgramHandle = glCreateProgram();

	// attach vertex and fragment shadets
	glAttachShader(ProgramHandle,vid);
	glAttachShader(ProgramHandle,fid);

	// link
	glLinkProgram(ProgramHandle);  

	// any problems?
	cout << "Linker Log: " << endl;
	PrintInfoLog(ProgramHandle);

	TickLoc = glGetUniformLocation(ProgramHandle,"tick");
	ResLoc = glGetUniformLocation(ProgramHandle,"res");
	MouseLoc = glGetUniformLocation(ProgramHandle,"mouse");
	ModeLoc = glGetUniformLocation(ProgramHandle,"mode");
	ZoomLoc = glGetUniformLocation(ProgramHandle,"zoom");
}


void setup_slices ( )
{
	GLfloat slicearray[] = 
	{
		1.0,1.0,0.0,
		1.0,-1.0,0.0,
		-1.0,1.0,0.0,

		1.0,-1.0,0.0,
		-1.0,-1.0,0.0,
		-1.0,1.0,0.0
		
	};
	
	// generate vertex array object handle
	GLuint vao;
	glGenVertexArrays(1,&vao);
	glBindVertexArray(vao);
	GLuint vbo;
	glGenBuffers(1,&vbo);

	glBindBuffer(GL_ARRAY_BUFFER,vbo);
	glBufferData(GL_ARRAY_BUFFER,2*3*3*sizeof(GLfloat),slicearray,GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindVertexArray(0);

	QuadVAO = vao;
}


void draw ( )
{
	/* ensure we're drawing to the correct GLUT window */
	glUniform1f(TickLoc,tick += 0.005);
	glUniform1f(ZoomLoc,zoom);
	glUniform1i(ResLoc,VPD_DEFAULT);
	glUniform1i(ModeLoc,mode);
	glUniform2f(MouseLoc,-mousex,mousey);
	glutSetWindow(wid);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// straightforward OpenGL settings
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glClearColor(0,0,0, 1.0);
	//glClearColor(1.0,1.0,1.0,1.0);
	glEnable(GL_DEPTH_TEST);
  
	/* clear the color buffers */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(ProgramHandle);

	//  set them up at startup and just use what you created here
	glBindVertexArray(QuadVAO);
	glDrawArrays(GL_TRIANGLES,0,2*3);
	glBindVertexArray(0);

	/* flush the pipeline */
	glFlush();

	/* look at our handiwork */
	glutSwapBuffers();
}


void keyboard(GLubyte key, GLint x, GLint y)
{
	switch(key) {
	case 27:  /* ESC */
				exit(0);
	default:  break;
	}
}

void animate(){
	glutPostRedisplay();
}

GLvoid button_motion(GLint mx, GLint my)
{
	mousex = mx*1.0/VPD_DEFAULT-0.5;
	mousey = my*1.0/VPD_DEFAULT-0.5;

	glutPostRedisplay();
}


GLvoid reshape(GLint sizex, GLint sizey)
{
	glutSetWindow(wid);

	vpw = sizex;
	vph = sizey;

	glViewport(0, 0, vpw, vph);
	glutReshapeWindow(vpw, vph);

	glutPostRedisplay();
}

void menu ( int value )
{
  switch(value)
    {
    case MENU_SYNTH:
      mode = 0;
      break;
    case MENU_OCEAN:
      mode = 1;
      break;
	case MENU_IN:
		zoom += 2;
	break;
	case MENU_OUT:
		zoom -= 2;
	break;
    }
}


GLint init_glut(GLint *argc, char **argv)
{
	GLint id;

	glutInit(argc,argv);

	/* size and placement hints to the window system */
	glutInitWindowSize(vpw, vph);
	glutInitWindowPosition(10,10);

	/* double buffered, RGB color mode */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	/* create a GLUT window (not drawn until glutMainLoop() is entered) */
	id = glutCreateWindow("Vector Field");    

	/* register callbacks */

	/* window size changes */
	glutReshapeFunc(reshape);

	/* keypress handling when the current window has input focus */
	glutKeyboardFunc(keyboard);
  
	/* mouse event handling */
	glutMouseFunc(NULL);           /* button press/release        */
	glutMotionFunc(button_motion);         /* mouse motion w/ button down */
	glutPassiveMotionFunc(NULL);           /* mouse motion with button up */

	/* window obscured/revealed event handler */
	glutVisibilityFunc(NULL);
	glutSpecialFunc(NULL);
	glutIdleFunc(animate);
	glutEntryFunc(NULL);
	glutDisplayFunc(draw);

	GLint menuID = glutCreateMenu(menu);
	glutAddMenuEntry("synthetic",MENU_SYNTH);
	glutAddMenuEntry("ocean",MENU_OCEAN);
	glutAddMenuEntry("zoom in",MENU_IN);
	glutAddMenuEntry("zoom out",MENU_OUT);
	glutSetMenu(menuID);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

	return id;
}

/* --------------------------------------------- */
/* --------------------------------------------- */
/* --------------------------------------------- */

GLint main(GLint argc, char **argv)
	{

	/* initialize GLUT: register callbacks, etc */
	wid = init_glut(&argc, argv);

	// initialize glew and check for OpenGL 4.0 support
	glewInit();
	if (glewIsSupported("GL_VERSION_4_0"))
		cout << "Ready for OpenGL 4.0" << endl;
	else 
	{
		cout << "OpenGL 4.0 not supported" << endl;;
		return 1;
	}

	// set up vertex/fragment programs

	SetUpProgram();
	setup_slices();
	TextureSetup();
	glutMainLoop();

	system("PAUSE");
	return 0;
	}


/* --------------------------------------------- */
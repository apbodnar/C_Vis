//Adam Bodnar
//Scientific Visualization
// Final Project
// 3/29/13

#define GLM_SWIZZLE
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

//global variable hell
static const int VPD_DEFAULT = 1000;
static int tick = 0;
static int NUM_LINES =1024;
static int NUM_SAMPLES = 2048; // +1 is intentional
static int WORK_GROUP_SIZE = 512;
static int animate = 1;
static float rev = 1.0;

static float mousex, mousey;
static int mode = 1;
static float zoom = 4;

glm::mat4 R = glm::mat4();
glm::mat4 R0 = glm::mat4();

glm::vec3 camDir = glm::vec3(0,0,-1);
glm::vec3 camPos = glm::vec3(0,0,30);

static const int MENU_SYNTH = 0;
static const int MENU_ANIMATE = 1;
static const int MENU_REVERSE = 2;

GLint wid;               
GLint vpw = VPD_DEFAULT; 
GLint vph = VPD_DEFAULT;

GLuint licSSBO , velSSBO;
GLuint licVAO;

GLint TickLoc;
GLint ModeLoc;
GLint AnimaLoc;
GLint RevLoc;
GLint ProjectionMatrixLoc;
GLint ModelViewMatrixLoc;

GLuint VFProgramHandle;
GLuint CProgramHandle; 
GLint PointVAO;

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
// recycled functions from project 3
void uploadTex(GLuint texture, float* T, int* size)
{
	glActiveTexture(GL_TEXTURE0);
	glUseProgram(CProgramHandle); // use fixed-function
    glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D,texture);
	glTexImage3D(GL_TEXTURE_3D,0,GL_RGB32F,size[0],size[1],size[2],0,GL_RGB,GL_FLOAT,T);
	glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}

float* readVF(char* name, int* size){
	ifstream ifs(name,ios::binary);
	
	ifs.read((char*)size,3*sizeof(int));
	int totalsize = size[0]*size[1]*size[2];   // # of voxels
	float *T = new float[3*totalsize];  // 3 floats per voxel
	ifs.read((char*)T,3*totalsize*sizeof(float));

	return T;
}
int texture_setup ()
{
	int size[3];
	float* tex = readVF("uvw3.vf",size); // hurricane Lili
	//float* tex = readVF("square.vf",size);
	//float* tex = readVF("bf.vf",size);
	//float* tex = readVF("plume.vf",size);
	GLuint TexHandle;  // texture handle
	glGenTextures(1,&TexHandle);

	uploadTex(TexHandle, tex, size);
	return 0;
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


void setup_programs()
{
	GLuint vid = glCreateShader(GL_VERTEX_SHADER);
	GLuint fid = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint cid = glCreateShader(GL_COMPUTE_SHADER);
	GLuint gid = glCreateShader(GL_GEOMETRY_SHADER);

	string VertexSrc = ReadFromFile("vertex.glsl");
	string FragmentSrc = ReadFromFile("fragment.glsl");
	string ComputeSrc = ReadFromFile("compute.glsl");
	string GeometrySrc = ReadFromFile("geometry.glsl");

	const char *VS = VertexSrc.c_str();
	const char *FS = FragmentSrc.c_str();
	const char *CS = ComputeSrc.c_str();
	const char *GS = GeometrySrc.c_str();

	// set source of the shaders
	glShaderSource(vid,1,&VS,NULL);
	glShaderSource(fid,1,&FS,NULL);
	glShaderSource(cid,1,&CS,NULL);
	glShaderSource(gid,1,&GS,NULL);

	// compile
	glCompileShader(vid);
	glCompileShader(fid);
	glCompileShader(cid);
	glCompileShader(gid);

	// any problems?
	cout << "Vertex Shader Log: " << endl;
	PrintInfoLog(vid);
	cout << "Fragment Shader Log: " << endl;
	PrintInfoLog(fid);
	cout << "Compute Shader Log: " << endl;
	PrintInfoLog(cid);
	cout << "Geometry Shader Log: " << endl;
	PrintInfoLog(gid);

	// create program
	VFProgramHandle = glCreateProgram();
	CProgramHandle = glCreateProgram();

	// attach vertex and fragment shaders
	glAttachShader(VFProgramHandle,vid);
	glAttachShader(VFProgramHandle,fid);
	glAttachShader(VFProgramHandle,gid);
	glAttachShader(CProgramHandle,cid);

	// link
	glLinkProgram(VFProgramHandle);  
	glLinkProgram(CProgramHandle);

	// any problems?
	cout << "Vertex/Fragment Linker Log: " << endl;
	PrintInfoLog(VFProgramHandle);
	cout << "Compute Linker Log: " << endl;
	PrintInfoLog(CProgramHandle);


	ModelViewMatrixLoc = glGetUniformLocation(VFProgramHandle,"MVM");
	ProjectionMatrixLoc = glGetUniformLocation(VFProgramHandle,"PM");
	TickLoc = glGetUniformLocation(CProgramHandle,"tick");
	RevLoc = glGetUniformLocation(CProgramHandle,"rev");
	ModeLoc = glGetUniformLocation(CProgramHandle,"mode");
	AnimaLoc = glGetUniformLocation(CProgramHandle,"animate");

}

void setup_buffer(){
	GLfloat* pointarray = new GLfloat[NUM_LINES*NUM_SAMPLES*4];
	GLfloat* velocityarray = new GLfloat[NUM_LINES*NUM_SAMPLES*4];
	srand (time(NULL));
	
	/*for(int i=0; i<NUM_LINES*NUM_SAMPLES*4;i+=4 ){
		float scale = 40.0;
		float interval = scale/(pow(NUM_LINES,1/3.0)); 

		int L = i/(NUM_SAMPLES*4);
		int R = L*interval;
		glm::vec4 p = glm::vec4(L*interval,L*interval,L*interval,1);

		pointarray[i+0] = p.x;
		pointarray[i+1] = p.y;
		pointarray[i+2] = p.z;
		pointarray[i+3] = p.w;
	}*/


	for(int i=0; i<NUM_LINES; i++){
		float x = (1.0*rand()/RAND_MAX)-0.5;
		float y = (1.0*rand()/RAND_MAX)-0.5;
		float z = (1.0*rand()/RAND_MAX)-0.5;
		float scale = 40.0;
		glm::vec4 start = glm::vec4(scale*x,scale*y,scale*z,1);
		//glm::vec4 start = glm::vec4(x,scale*y,z,1);
		glm::vec4 p = start;
		for(int j=0; j<4*NUM_SAMPLES;j+=4){
			int ind = i*NUM_SAMPLES*4 + j;

			pointarray[ind+0] = p.x;
			pointarray[ind+1] = p.y;
			pointarray[ind+2] = p.z;
			pointarray[ind+3] = p.w;
		}
	}


	glGenBuffers(1,&licSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER,licSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER,NUM_LINES*NUM_SAMPLES*4*sizeof(GLfloat),pointarray,GL_STATIC_DRAW);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glGenBuffers(1,&velSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER,velSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER,NUM_LINES*NUM_SAMPLES*4*sizeof(GLfloat),velocityarray,GL_STATIC_DRAW);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);


	glGenVertexArrays(1,&licVAO);
	glBindVertexArray(licVAO);

	glBindBuffer(GL_ARRAY_BUFFER,licSSBO);
	glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,0,0);
	
	glBindBuffer(GL_ARRAY_BUFFER,velSSBO);
	glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,0,0);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindVertexArray(0);
	
}

void call_simulation(){
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, licSSBO );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, velSSBO );
	glUseProgram(CProgramHandle);
	glUniform1i(TickLoc,tick += 1);
	glUniform1f(RevLoc,rev);
	glUniform1i(ModeLoc,mode);
	glUniform1i(AnimaLoc,animate);
	glDispatchCompute( NUM_LINES/WORK_GROUP_SIZE,1, 1);
	glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
	glutPostRedisplay();
	
}

void draw ( )
{
	glUseProgram(VFProgramHandle);
	glutWarpPointer( VPD_DEFAULT/2, VPD_DEFAULT/2 );
	/* ensure we're drawing to the correct GLUT window */
	
	glutSetWindow(wid);

	glm::mat4 PMat = glm::perspective(90.0f, 1.0f, 0.1f, 100.f);
	glUniformMatrix4fv(ProjectionMatrixLoc,1,GL_FALSE,&PMat[0][0]);

	glm::mat4 MVMat;
	MVMat = R0 * glm::translate(glm::mat4(1.0),-camPos);
	glUniformMatrix4fv(ModelViewMatrixLoc,1,GL_FALSE,&MVMat[0][0]);
	glUniform1i(glGetUniformLocation(VFProgramHandle,"mode"),mode);
	// straightforward OpenGL settings
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glClearColor(0,0,0,0);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_COLOR, GL_ONE);
	//glDisable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_TEST);
	glClearColor(1.0,1.0,1.0,1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//draw all lines from one buffer
	for(int i=0; i<NUM_LINES; i++){
		glBindVertexArray(licVAO);
		glVertexPointer( 4, GL_FLOAT, 0, (void *)0 );
		glDrawArrays( GL_LINE_STRIP_ADJACENCY, i*NUM_SAMPLES, NUM_SAMPLES);
		glDisableClientState( GL_VERTEX_ARRAY );
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		
	}

	glFlush();
	glutSwapBuffers();
}

void sim_draw(){
	call_simulation();
	draw();
}

void keyboard(GLubyte key, GLint x, GLint y)
{
  switch(key) {
    case 27:
        exit(0);
	break;
	case 'a':
		camPos -= 1.25f*glm::normalize(glm::cross((camDir),glm::vec3(0,1,0)));
    break;
	case 'd':
		camPos += 1.25f*glm::normalize(glm::cross((camDir),glm::vec3(0,1,0)));
    break;    
	case 's':
		camPos -= (1.25f * glm::normalize(camDir));
    break;    
	case 'w':
		camPos += (1.25f * glm::normalize(camDir));
    break;    
	case 'r':
		rev = -rev;
    break;    

    default:  break;
  }
}

void mouse_button(GLint btn, GLint state, GLint mx, GLint my)
{
	switch( btn ) {
		case GLUT_LEFT_BUTTON:
			switch( state ) {
				case GLUT_DOWN:

				break;
				case GLUT_UP:   
		
				break;
			}
		break;
		case GLUT_RIGHT_BUTTON:
			
		break;
	}
}

GLvoid passive_motion(GLint i, GLint j)
{
	glm::vec3 axisX = glm::vec3(1,0,0);
	glm::vec3 axisY = glm::vec3(0,1,0);
	// first person camera movement
	float dX = (i - VPD_DEFAULT/2.0) / 10.0;
	float dY = (j - VPD_DEFAULT/2.0) / 10.0;
	if( dX == 0 && dY == 0 ) return;
	GLfloat angley = dY;
	GLfloat anglex = dX;
	R0 = R0 * glm::rotate(glm::mat4(1.0),anglex,axisY);
	camDir = (glm::vec4(camDir,0) * glm::rotate(glm::mat4(1.0),anglex,axisY)).xyz;
	glm::vec3 a = glm::cross(camDir,glm::vec3(0,1,0));
	R0 = R0 * glm::rotate(glm::mat4(1.0),angley,a);
	camDir = (glm::vec4(camDir,0) * glm::rotate(glm::mat4(1.0),angley,a)).xyz;
	glutPostRedisplay();
	return;
}

void button_motion(GLint i, GLint j)
{
	passive_motion(i,j);
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
		mode = ++mode % 2;
	break;
	case MENU_ANIMATE:
		animate = ++animate % 2;
	break;
	case MENU_REVERSE:
		rev = -rev;
	break;
	}
}


GLint init_glut(GLint *argc, char **argv)
{
	//good ol' init_glut
	GLint id;

	glutInit(argc,argv);

	/* size and placement hints to the window system */
	glutInitWindowSize(vpw, vph);
	glutInitWindowPosition(10,10);

	/* double buffered, RGB color mode */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	/* create a GLUT window (not drawn until glutMainLoop() is entered) */
	id = glutCreateWindow("Vector Field");    
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
  
	/* mouse event handling */
	glutMouseFunc(NULL);  
	glutMotionFunc(button_motion);         
	glutPassiveMotionFunc(passive_motion);           
	glutMouseFunc(mouse_button); 
	glutSetCursor(GLUT_CURSOR_NONE);
	/* window obscured/revealed event handler */
	glutVisibilityFunc(NULL);
	glutSpecialFunc(NULL);
	glutIdleFunc(NULL);
	glutEntryFunc(NULL);
	glutDisplayFunc(sim_draw);

	GLint menuID = glutCreateMenu(menu);
	glutAddMenuEntry("toggle synthetic/natural",MENU_SYNTH);
	glutAddMenuEntry("toggle dynamic/static",MENU_ANIMATE);
	glutAddMenuEntry("toggle flow direction",MENU_REVERSE);
	glutSetMenu(menuID);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

	return id;
}


GLint main(GLint argc, char **argv)
{

	wid = init_glut(&argc, argv);

	// initialize glew and check for OpenGL 4.3 support
	glewInit();
	if (glewIsSupported("GL_VERSION_4_3"))
		cout << "Ready for OpenGL 4.3" << endl;
	else 
	{
		cout << "OpenGL 4.3 not supported" << endl;;
		return 1;
	}
	// set up shaders, buffers, textures
	setup_programs();
	texture_setup();
	setup_buffer();
	glutMainLoop();

	system("PAUSE");
	return 0;
}


/* --------------------------------------------- */
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
static float tick = 0;
static int NUM_POINTS = 2*1024*1024;
static int WORK_GROUP_SIZE = 1024;

static float mousex, mousey;
static int mode = 0;
static float zoom = 4;

glm::mat4 R = glm::mat4();
glm::mat4 R0 = glm::mat4();

glm::vec3 camDir = glm::vec3(0,0,-1);
glm::vec3 camPos = glm::vec3(0,0,30);
glm::vec3 center = glm::vec3(0,0,0);

static const int MENU_SYNTH = 0;
static const int MENU_OCEAN = 1;
static const int MENU_IN = 2;
static const int MENU_OUT = 3;

GLint wid;               /* GLUT window id; value asigned in main() and should stay constant */
GLint vpw = VPD_DEFAULT; /* viewport dimensions; change when window is resized (resize callback) */
GLint vph = VPD_DEFAULT;

GLuint posSSBO;
GLuint velSSBO;
GLuint prevSSBO;

GLint TickLoc;
GLint MouseLoc;
GLint ModeLoc;
GLint ZoomLoc;
GLint CenterLoc;
GLint ProjectionMatrixLoc;
GLint ModelViewMatrixLoc;

GLuint VFProgramHandle;
GLuint CProgramHandle; 
GLint PointVAO;

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
	assert(c=='6'); 
	ifs >> x >> y;
	int i;
	ifs >> i;
	assert(i==255);  
	ifs.get(); // need to skip one more byte
	pixels = new rgb[x*y];
	ifs.read((char*)pixels,x*y*sizeof(rgb));

	return pixels;
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
	TickLoc = glGetUniformLocation(VFProgramHandle,"tick");
	ModeLoc = glGetUniformLocation(CProgramHandle,"mode");
	ZoomLoc = glGetUniformLocation(VFProgramHandle,"zoom");

	CenterLoc = glGetUniformLocation(CProgramHandle,"center");
}
int resolution_x, resolution_y;
rgb* pixels2D = readPPM("particle.ppm",resolution_x, resolution_y);

void uploadRGB(GLuint texture, rgb* T, int n)
{
	glActiveTexture(GL_TEXTURE0+n);
	glBindTexture(GL_TEXTURE_2D,texture);
	//glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,VPD_DEFAULT,VPD_DEFAULT,0,GL_RGB,GL_UNSIGNED_BYTE,T);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,resolution_x,resolution_y,0,GL_RGB,GL_UNSIGNED_BYTE,pixels2D);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}

int TextureSetup ()
{
	GLuint TexHandle;  // texture handle
	glGenTextures(1,&TexHandle);

	uploadRGB(TexHandle, pixels2D, 0);
	return 0;
}


void setup_buffers(){

	glGenBuffers(1,&posSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER,posSSBO);
	
	
	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	glBufferData(GL_SHADER_STORAGE_BUFFER,NUM_POINTS*4*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
	//GLfloat* pointarray = (GLfloat*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,NUM_POINTS*4*sizeof(GLfloat),bufMask);
	GLfloat* pointarray = new GLfloat[NUM_POINTS*4];
	srand (time(NULL));
	float scale = 25.0;
	float r = scale;;
	
	for(int i=0; i<4*NUM_POINTS;){
		
		float theta = (1.0*rand()/RAND_MAX);
		float phi = (1.0*rand()/RAND_MAX);
		pointarray[i++] = (r * sin(2*M_PI * theta)*cos(2*M_PI * phi));
		pointarray[i++] = (r * cos(2*M_PI * theta)*cos(2*M_PI * phi));
		pointarray[i++] = 5.0;//r * sin(2*M_PI * phi);
		pointarray[i++] = 1;
	}
	glBufferData(GL_SHADER_STORAGE_BUFFER,NUM_POINTS*4*sizeof(GLfloat),pointarray,GL_STATIC_DRAW);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glGenBuffers(1,&velSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER,velSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER,NUM_POINTS*4*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
	GLfloat* velocityarray = (GLfloat*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,NUM_POINTS*4*sizeof(GLfloat),bufMask);
	
	for(int i=0; i<4*NUM_POINTS; i+=4){

		glm::vec3 p = glm::vec3(pointarray[i+0],
								pointarray[i+1],
								pointarray[i+2]);
		
		float rad = glm::distance(center, p);
		float c = sqrt(30/rad);

		float sign = (rand()%2 - 0.5);
		glm::vec3 cr = glm::vec3(sign *(center.y - p.y),
								 -(sign *(center.x - p.x)),
								 0);
		glm::vec3 tr = c*glm::normalize(glm::cross(p,cr));

		velocityarray[i] = tr.x;
		velocityarray[i+1] = tr.y;
		velocityarray[i+2] = tr.z;
		velocityarray[i+3] = 0.0;
		//velocityarray[i] = -c*(center.y - p.y)/rad;
		//velocityarray[i+1] = c*(center.x - p.x)/rad;
		//velocityarray[i+2] = 0.0;
		//velocityarray[i+3] = 0.0;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glGenBuffers(1,&prevSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER,prevSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER,NUM_POINTS*4*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
	GLfloat* accelarray = (GLfloat*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER,0,NUM_POINTS*4*sizeof(GLfloat),bufMask);
	for(int i=0; i<4*NUM_POINTS;i++){
		accelarray[i] = pointarray[i];
	}

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	GLuint vao;
	glGenVertexArrays(1,&vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER,posSSBO);
	glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,0,0);
	glBindBuffer(GL_ARRAY_BUFFER,velSSBO);
	glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,0,0);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindVertexArray(0);

	PointVAO = vao;
}


void call_simulation(){
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, posSSBO );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, velSSBO );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2, prevSSBO );

	glUseProgram(CProgramHandle);
	glUniform3f(CenterLoc,center.x,center.y,center.z);
	glUniform1i(ModeLoc,mode);
	glDispatchCompute(NUM_POINTS/WORK_GROUP_SIZE, 1, 1);
	glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
}


void draw ( )
{
	glUseProgram(VFProgramHandle);
	glutWarpPointer( VPD_DEFAULT/2, VPD_DEFAULT/2 );
	/* ensure we're drawing to the correct GLUT window */
	glUniform1f(TickLoc,tick += 0.0001);
	glUniform1f(ZoomLoc,zoom);
	glUniform2f(MouseLoc,-mousex,mousey);
	glutSetWindow(wid);

	glm::mat4 PMat = glm::perspective(90.0f, 1.0f, 0.001f, 100.f);
	glUniformMatrix4fv(ProjectionMatrixLoc,1,GL_FALSE,&PMat[0][0]);

	glm::mat4 MVMat;
	MVMat = R0 * glm::translate(glm::mat4(1.0),-camPos);
	glUniformMatrix4fv(ModelViewMatrixLoc,1,GL_FALSE,&MVMat[0][0]);

	// straightforward OpenGL settings
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glClearColor(0,0,0, 1.0);
	//glClearColor(1.0,1.0,1.0,1.0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_COLOR, GL_ONE);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	/* flush the pipeline */

	glBindVertexArray(PointVAO);
	//glVertexPointer( 4, GL_FLOAT, 0, (void *)0 );
	glEnableClientState(GL_VERTEX_ARRAY);
	glDrawArrays( GL_POINTS, 0, NUM_POINTS );
	glDisableClientState( GL_VERTEX_ARRAY );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	glFlush();
	/* look at our handiwork */
	glutSwapBuffers();
}
void sim_draw(){
	//
	draw();
}

void keyboard(GLubyte key, GLint x, GLint y)
{
  switch(key) {
    case 27:
        exit(0);
	break;
	case 'a':
		camPos -= 1.25f*glm::cross(glm::normalize(camDir),glm::vec3(0,1,0));
    break;
	case 'd':
		camPos += 1.25f*glm::cross(glm::normalize(camDir),glm::vec3(0,1,0));
    break;    
	case 's':
		camPos -= (1.25f * glm::normalize(camDir));
    break;    
	case 'w':
		camPos += (1.25f * glm::normalize(camDir));
    break;    
	case 'r':
		center = camPos;
    break;    

    default:  break;
  }
}

void animate(){
	call_simulation();
	glutPostRedisplay();
}

void mouse_button(GLint btn, GLint state, GLint mx, GLint my)
{
	switch( btn ) {
		case GLUT_LEFT_BUTTON:
			switch( state ) {
				case GLUT_DOWN:
					center = camPos+ 15.0f*camDir;
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
	center = camPos+ 15.0f*camDir;
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
	id = glutCreateWindow("Particle Simulation");    
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
  
	/* mouse event handling */
	glutMouseFunc(NULL);           /* button press/release        */
	glutMotionFunc(button_motion);         /* mouse motion w/ button down */
	glutPassiveMotionFunc(passive_motion);           /* mouse motion with button up */
	glutMouseFunc(mouse_button); 
	glutSetCursor(GLUT_CURSOR_NONE);
	/* window obscured/revealed event handler */
	glutVisibilityFunc(NULL);
	glutSpecialFunc(NULL);
	glutIdleFunc(animate);
	glutEntryFunc(NULL);
	glutDisplayFunc(sim_draw);

	GLint menuID = glutCreateMenu(menu);
	glutAddMenuEntry("gravity",MENU_SYNTH);
	glutAddMenuEntry("spring",MENU_OCEAN);
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

	// set up shader programs

	SetUpProgram();
	TextureSetup();
	setup_buffers();
	glutMainLoop();

	system("PAUSE");
	return 0;
	}


/* --------------------------------------------- */
#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>


#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;



struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

struct Block
{
  VAO *block;
  VAO *frame;
  float x,y,z;
  short int rx,ry,rz;
  float rangle;
  float sangle;
  short int orientation;
};

struct Block player;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    //fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
//    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/


float freecamera_theta = 45;
float freecamera_omega = 45;
float tpcamera_theta = 0;
float tpcamera_theta_old = 0;
float rectangle_rotation = 0;
int sx,sy,sz=1;
short int camera=0;
short int mcam=2;
int levelno = 0;
int moves = 0;
bool paused = 0;
bool muted = 0;
float zoomfactor = 1;
int fallx = -1, fally = -1;

void keybindings(char A)
{
	if(!paused)
	{
		if(!muted) system("aplay -q door_lock.wav &");
		moves ++;
		if(A == 'U')
		{
			if(player.orientation == 2)
			{
				player.sangle += 9;
				player.y += 0.202;
				sx = -1;
				sz = 0;
				sy = 0; 
			}
			else
			{
				player.rx = -1;
				player.rz = 0;
				player.ry = 0;
				player.rangle += 9;
				player.y += 0.303;
				if(player.orientation == 0)player.z -= 0.1;
				else if(player.orientation == 1)player.z += 0.1;
			}
		}
		else if(A == 'D')
		{
			if(player.orientation == 2)
			{
				player.sangle += 9;
				player.y-=0.202;
				sx = 1;
				sz = 0;
				sy = 0; 
			}
			else
			{
				player.rx = 1;
				player.rz = 0;
				player.ry = 0;
				player.rangle += 9;
				player.y -= 0.303;
				if(player.orientation == 0) player.z -= 0.1;
				else if(player.orientation == 1) player.z += 0.1;
			}
		}
		else if(A == 'L')
		{
			if(player.orientation == 1)
			{
				player.sangle += 9;
				player.x -= 0.202;
				sy = -1;
				sx = 0;
				sz = 0;
			}
			else
			{
				player.rx = 0;
				player.rz = 0;
				player.ry = -1;
				player.x -= 0.303;
				player.rangle += 9;
				if(player.orientation == 0)player.z -= 0.1;
				else if(player.orientation == 2)player.z += 0.1;
			}
		}
		else if(A == 'R')
		{
			if(player.orientation == 1)
			{
				player.sangle += 9;
				player.x += 0.202;
				sy = 1;
				sx = 0;
				sz = 0;
			}
			else
			{
				player.rx = 0;
				player.rz = 0;
				player.ry = 1;
				player.rangle += 9;
				player.x += 0.303;
				if(player.orientation == 0)player.z -= 0.1;
				else if(player.orientation == 2)player.z += 0.1;
			}
		}
	}
}

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
  void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
  {
       // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
    	switch (key) {
    		case GLFW_KEY_UP:
        	{
        		if(camera!=1 && camera!=2) keybindings('U');
        		else
        		{
            		tpcamera_theta_old = tpcamera_theta;
            		if(fmod(fabs(tpcamera_theta),360) ==0) keybindings('U');
            		else if((fmod(tpcamera_theta,360) == 90) or (fmod(tpcamera_theta,360) == -270)) keybindings('R');
            		else if((fmod(tpcamera_theta,360) == 270) or (fmod(tpcamera_theta,360) == -90)) keybindings('L');
            		else if((fmod(tpcamera_theta,360) == 180) or (fmod(tpcamera_theta,360) == -180)) keybindings('D');
        		}
        		mcam = 2;
        		break;
        	}
        	case GLFW_KEY_DOWN:
        	{
        		if(camera!=1 && camera!=2) keybindings('D');
        		else
        		{
            		tpcamera_theta_old = tpcamera_theta;
            		if(fmod(fabs(tpcamera_theta),360)==0) keybindings('D');
            		else if((fmod(tpcamera_theta,360) == 90) || (fmod(tpcamera_theta,360) == -270)) keybindings('L');
            		else if((fmod(tpcamera_theta,360) == 270) || (fmod(tpcamera_theta,360) == -90)) keybindings('R');
            		else if((fmod(tpcamera_theta,360) == 180) || (fmod(tpcamera_theta,360) == -180)) keybindings('U');
        		}
        		tpcamera_theta -= 18;
        		mcam = 0;
        		break;
        	}
        	case GLFW_KEY_RIGHT:
        	{ 
        		if(camera!=1 && camera!=2) keybindings('R');
        		else
        		{
        	    	tpcamera_theta_old = tpcamera_theta;
        	    	if (fmod(fabs(tpcamera_theta),360) == 0)keybindings('R');
        	    	else if((fmod(tpcamera_theta,360) == 90) || (fmod(tpcamera_theta,360) == -270)) keybindings('D');
        	    	else if((fmod(tpcamera_theta,360) == 270) || (fmod(tpcamera_theta,360) == -90)) keybindings('U');
        	    	else if((fmod(tpcamera_theta,360) == 180) || (fmod(tpcamera_theta,360) == -180)) keybindings('L');
        		}
        		tpcamera_theta +=9;
        		mcam = 1;
        		break;
        	}
        	case GLFW_KEY_LEFT:
        	{
        		if(camera!=1 && camera!=2) keybindings('L');
        		else
        		{
            		tpcamera_theta_old = tpcamera_theta;
            		if (fmod(fabs(tpcamera_theta),360) == 0)keybindings('L');
            		else if((fmod(tpcamera_theta,360) == 90) || (fmod(tpcamera_theta,360) == -270)) keybindings('U');
            		else if((fmod(tpcamera_theta,360) == 270) || (fmod(tpcamera_theta,360) == -90)) keybindings('D');
            		else if((fmod(tpcamera_theta,360) == 180) || (fmod(tpcamera_theta,360) == -180)) keybindings('R');
        		}
        		mcam = -1;
        		tpcamera_theta -= 9;
        		break;
        	}
        	case GLFW_KEY_C:  
        		camera = (camera + 1)%4;
        	default:
        		break;
    	}

    }
    else if (action == GLFW_PRESS) {
    	switch (key)
    	{
        	case GLFW_KEY_A:
        		freecamera_theta += 10;
        		break;
        	case GLFW_KEY_D:
        		freecamera_theta -= 10;
        		break;
        	case GLFW_KEY_R:
        		freecamera_theta = 45;
        		zoomfactor = 1;
        		break;
        	case GLFW_KEY_ESCAPE:
        		quit(window);
        		break;
        	case GLFW_KEY_P:
        	{
        		if(!paused) cout << "GAME PAUSED, Press 'P' to resume\n" << endl;
        		else cout << "GAME RESUME, Press 'P' to pause\n" << endl;
        		paused = !paused;
        	}
        	case GLFW_KEY_M:
        	{
        		if(!muted) cout << "Audio Muted, press 'M' tu unmute.\n" << endl;
        		else cout << "Audio Unmuted, press 'M' to mute.\n" << endl;
        		muted = !muted;
        	}
        	default:
        		break;

    	}
    }
}
void scroll (GLFWwindow* window, double xoffset, double yoffset)
{
	freecamera_omega += 5*yoffset;
	freecamera_theta += 5*xoffset;
}
/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_RELEASE)
	{
    	switch (button) 
    	{
    		case GLFW_MOUSE_BUTTON_RIGHT:
    			if(zoomfactor < 1.6) zoomfactor += 0.1;
    			break;
    		case GLFW_MOUSE_BUTTON_LEFT:
    			if(zoomfactor > 0.6)zoomfactor -= 0.1;
    			break;
    	    default:
    	        break;
    	}
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 108.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
     Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    //Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

short int Area[3][12][12]={
	{
	  0,0,0,0,0,0,0,0,0,0,0,0,
	  0,1,1,1,1,0,0,0,0,0,0,0,
	  0,1,1,1,1,0,0,0,0,0,0,0,
	  0,1,1,1,1,0,0,0,0,0,0,0,
	  0,0,0,0,1,0,0,0,0,0,0,0,
	  0,0,0,0,1,0,0,0,0,0,0,0,
	  0,0,0,0,1,0,0,0,0,0,0,0,
	  0,0,0,0,1,0,0,0,0,0,0,0,
	  0,1,1,1,1,0,0,0,0,0,0,0,
	  0,1,5,1,1,0,0,0,0,0,0,0,
	  0,1,1,1,1,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
	  0,0,0,0,0,0,0,0,0,0,0,0,
	  0,1,1,3,0,0,1,1,0,0,0,0,
	  0,1,1,1,0,0,1,1,0,0,0,0,
	  0,1,1,1,2,1,1,1,0,0,0,0,
	  0,0,0,0,0,0,1,1,0,0,0,0,
	  0,0,0,0,0,0,1,1,0,0,0,0,
	  0,0,0,0,0,0,1,1,0,0,0,0,
	  0,0,0,0,0,0,1,1,0,0,0,0,
	  0,0,0,0,0,0,1,0,1,1,1,0,
	  0,0,0,0,0,0,1,2,1,5,1,0,
	  0,0,0,0,0,0,0,0,1,1,1,0,
	  0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
	  0,0,0,0,0,0,0,0,0,0,0,0,
	  0,1,1,1,0,1,1,1,0,0,0,0,
	  0,1,1,1,0,1,1,1,0,0,0,0,
	  0,1,1,1,1,3,4,4,0,0,0,0,
	  0,0,0,0,0,0,4,4,0,0,0,0,
	  0,0,0,0,0,0,4,4,0,0,0,0,
	  0,0,0,0,0,0,4,4,0,0,0,0,
	  0,0,0,0,0,0,4,4,0,0,0,0,
	  0,1,1,1,0,0,4,4,0,0,0,0,
	  0,1,5,1,2,1,4,4,0,0,0,0,
	  0,1,1,1,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0
	}
};

VAO *tile, *btile, *stile, *ftile;
float tangle = 0;
float ybridge = 2.02, zbridge = 0.8;
bool mapstart = 0, bstatus = 0;
float zswitch = -1.8;
int t=1;
int fallfactor = 0;

// Creates the rectangle object used in this sample code
void createRectangle ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -1,-1,-2, // vertex 1
    1,-1,-2, // vertex 2
    1, 1,-2, // vertex 3

    1, 1,-2, // vertex 3
    -1, 1,-2, // vertex 4
    -1,-1,-2,  // vertex 1

    -1,-1,2, // vertex 1
    1,-1,2, // vertex 2
    1, 1,2, // vertex 3

    1, 1,2, // vertex 3
    -1, 1,2, // vertex 4
    -1,-1,2,  // vertex 1

    -1,-1,-2, // vertex 1
    1,-1,-2, // vertex 2
    -1,-1,2, // vertex 1

    1,-1,2, // vertex 1
    -1,-1,2, // vertex 2
    1,-1,-2, // vertex 2

    -1,1,-2, // vertex 1
    1,1,-2, // vertex 2
    -1,1,2, // vertex 1

    1,1,2, // vertex 1
    -1,1,2, // vertex 2
    1,1,-2, // vertex 2

    1,-1,-2, // vertex 2
    1, 1,-2, // vertex 3
    1,-1,2, // vertex 2

    1,1,2, // vertex 2
    1,-1,2, // vertex 3
    1,1,-2, // vertex 2

    -1,-1,2, // vertex 1
    -1,-1,-2,  // vertex 1
    -1, 1,2, // vertex 4

    -1,1,-2, // vertex 1
    -1,1,2,  // vertex 1
    -1,-1,-2 // vertex 4 
  };

  static const GLfloat color_buffer_data1 [] = {
    0.3,0.3,0.3, // color 1
    0.3,0.3,0.3, // color 2
    0.3,0.3,0.3, // color 3

    0.3,0.3,0.3, // color 3
    0.3,0.3,0.3, // color 4
    0.3,0.3,0.3,  // color 1

    0.3,0.3,0.3, // color 1
    0.3,0.3,0.3, // color 2
    0.3,0.3,0.3, // color 3

    0.3,0.3,0.3, // color 3
    0.3,0.3,0.3, // color 4
    0.3,0.3,0.3,  // color 1

    0.3,0.3,0.3, // color 3
    0.3,0.3,0.3, // color 4
    0.3,0.3,0.3,  // color 1

    0.3,0.3,0.3, // color 3
    0.3,0.3,0.3, // color 4
    0.3,0.3,0.3,  // color 1

    0.3,0.3,0.3, // color 3
    0.3,0.3,0.3, // color 4
    0.3,0.3,0.3,  // color 1

    0.3,0.3,0.3, // color 3
    0.3,0.3,0.3, // color 4
    0.3,0.3,0.3,  // color 1

    0.3,0.3,0.3, // color 3
    0.3,0.3,0.3, // color 4
    0.3,0.3,0.3,  // color 1

    0.3,0.3,0.3, // color 3
    0.3,0.3,0.3, // color 4
    0.3,0.3,0.3, // color 1

    0.3,0.3,0.3, // color 3
    0.3,0.3,0.3, // color 4
    0.3,0.3,0.3, // color 1

    0.3,0.3,0.3, // color 3
    0.3,0.3,0.3, // color 4
    0.3,0.3,0.3 // color 1
  };

  static const GLfloat color_buffer_data2 [] = {
    0.9,0.9,0.9, // color 1
    0.9,0.9,0.9, // color 3
    0.9,0.9,0.9, // color 2

    0.9,0.9,0.9, // color 3
    0.9,0.9,0.9, // color 4
    0.9,0.9,0.9,  // color 1

    0.9,0.9,0.9, // color 1
    0.9,0.9,0.9, // color 2
    0.9,0.9,0.9, // color 3

    0.9,0.9,0.9, // color 3
    0.9,0.9,0.9, // color 4
    0.9,0.9,0.9,  // color 1

    0.9,0.9,0.9, // color 3
    0.9,0.9,0.9, // color 4
    0.9,0.9,0.9,  // color 1

    0.9,0.9,0.9, // color 3
    0.9,0.9,0.9, // color 4
    0.9,0.9,0.9,  // color 1

    0.9,0.9,0.9, // color 3 
    0.9,0.9,0.9, // color 4
    0.9,0.9,0.9,  // color 1

    0.9,0.9,0.9, // color 3
    0.9,0.9,0.9, // color 4
    0.9,0.9,0.9,  // color 1

    0.9,0.9,0.9, // color 3
    0.9,0.9,0.9, // color 4
    0.9,0.9,0.9,  // color 1

    0.9,0.9,0.9, // color 3
    0.9,0.9,0.9, // color 4
    0.9,0.9,0.9, // color 1

    0.9,0.9,0.9, // color 3
    0.9,0.9,0.9, // color 4
    0.9,0.9,0.9, // color 1

    0.9,0.9,0.9, // color 3
    0.9,0.9,0.9, // color 4
    0.9,0.9,0.9 // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  player.block = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data1, GL_FILL);

  player.frame =  create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data2, GL_LINE);
}

void createtile()
{
  static const GLfloat vertex_buffer_data [] = {
    -1,-1,-0.4, // vertex 1
    1,-1,-0.4, // vertex 2
    1, 1,-0.4, // vertex 3

    1, 1,-0.4, // vertex 3
    -1, 1,-0.4, // vertex 4
    -1,-1,-0.4,  // vertex 1

    -1,-1,0.4, // vertex 1
    1,-1,0.4, // vertex 2
    1, 1,0.4, // vertex 3

    1, 1,0.4, // vertex 3
    -1, 1,0.4, // vertex 4
    -1,-1,0.4,  // vertex 1

    -1,-1,-0.4, // vertex 1
    1,-1,-0.4, // vertex 2
    -1,-1,0.4, // vertex 1

    1,-1,0.4, // vertex 1
    -1,-1,0.4, // vertex 2
    1,-1,-0.4, // vertex 2

    -1,1,-0.4, // vertex 1
    1,1,-0.4, // vertex 2
    -1,1,0.4, // vertex 1

    1,1,0.4, // vertex 1
    -1,1,0.4, // vertex 2
    1,1,-0.4, // vertex 2

    1,-1,-0.4, // vertex 2
    1, 1,-0.4, // vertex 3
    1,-1,0.4, // vertex 2

    1,1,0.4, // vertex 2
    1,-1,0.4, // vertex 3
    1,1,-0.4, // vertex 2

    -1,-1,0.4, // vertex 1
    -1,-1,-0.4,  // vertex 1
    -1, 1,0.4, // vertex 4

    -1,1,-0.4, // vertex 1
    -1,1,0.4,  // vertex 1
    -1,-1,-0.4 // vertex 4 
  };

  static const GLfloat color_buffer_data1 [] = {
    0.75,0.75,0.75, // color 1
    0.75,0.75,0.75, // color 2
    0.75,0.75,0.75, // color 3

    0.75,0.75,0.75, // color 3
    0.75,0.75,0.75, // color 4
    0.75,0.75,0.75,  // color 1

    0.75,0.75,0.75, // color 1
    0.75,0.75,0.75, // color 2
    0.75,0.75,0.75, // color 3

    0.75,0.75,0.75, // color 3
    0.75,0.75,0.75, // color 4
    0.75,0.75,0.75,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1, // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1, // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1 // color 1
  };

    static const GLfloat color_buffer_data2 [] = {
    0.8671,0.7215,0.5294, // color 1
    0.8671,0.7215,0.5294, // color 2
    0.8671,0.7215,0.5294, // color 3

    0.8671,0.7215,0.5294, // color 3
    0.8671,0.7215,0.5294, // color 4
    0.8671,0.7215,0.5294,  // color 1

    0.8671,0.7215,0.5294, // color 1
    0.8671,0.7215,0.5294, // color 2
    0.8671,0.7215,0.5294, // color 3

    0.8671,0.7215,0.5294, // color 3
    0.8671,0.7215,0.5294, // color 4
    0.8671,0.7215,0.5294,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1, // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1, // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1 // color 1
  };

   static const GLfloat color_buffer_data3 [] = {
    0.4,0.4,0.4, // color 1
    0.4,0.4,0.4, // color 2
    0.4,0.4,0.4, // color 3

    0.4,0.4,0.4, // color 3
    0.4,0.4,0.4, // color 4
    0.4,0.4,0.4,  // color 1

    0.4,0.4,0.4, // color 1
    0.4,0.4,0.4, // color 2
    0.4,0.4,0.4, // color 3

    0.4,0.4,0.4, // color 3
    0.4,0.4,0.4, // color 4
    0.4,0.4,0.4,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1, // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1, // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1 // color 1
  };

  static const GLfloat color_buffer_data4 [] = {
    0.4156,0.46666,0.93725, // color 1
    0.4156,0.46666,0.93725, // color 2
    0.4156,0.46666,0.93725, // color 3

    0.4156,0.46666,0.93725, // color 3
    0.4156,0.46666,0.93725, // color 4
    0.4156,0.46666,0.93725,  // color 1

    0.4156,0.46666,0.93725, // color 1
    0.4156,0.46666,0.93725, // color 2
    0.4156,0.46666,0.93725, // color 3

    0.4156,0.46666,0.93725, // color 3
    0.4156,0.46666,0.93725, // color 4
    0.4156,0.46666,0.93725,	  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1,  // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1, // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1, // color 1

    0.1,0.1,0.1, // color 3
    0.1,0.1,0.1, // color 4
    0.1,0.1,0.1 // color 1
  };

  tile = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data1, GL_FILL);

  btile = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data2, GL_FILL);

  stile = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data3, GL_FILL);

  ftile = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data4, GL_FILL);
}

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
  // clear the color and depth in the frame buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
	glUseProgram (programID);

	if(mcam == 0 && tpcamera_theta_old - tpcamera_theta != 180) tpcamera_theta -= 18;
	else if(mcam == 1 && tpcamera_theta -  tpcamera_theta_old != 90) tpcamera_theta += 9;
	else if(mcam == -1 && tpcamera_theta_old - tpcamera_theta != 90) tpcamera_theta -= 9;

	if(camera == 0)
	{
    // Eye - Location of camera. Don't change unless you are sure!!
		glm::vec3 eye ( -30*cos(freecamera_theta*M_PI/180.0f)*sin(freecamera_omega*M_PI/180.0f)*zoomfactor, -30*sin(freecamera_theta*M_PI/180.0f)*sin(freecamera_omega*M_PI/180.0f)*zoomfactor, 20*cos(freecamera_omega*M_PI/180.0f));
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
		glm::vec3 target (player.x*mapstart, player.y*mapstart, player.z*mapstart);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
		glm::vec3 up (0, 0, 1);

		Matrices.view = glm::lookAt( eye, target, up );
	}

	else if (camera == 1)
	{
		glm::vec3 eye ( player.x - 6*sin(tpcamera_theta*M_PI/180.0f), player.y - 6*cos(tpcamera_theta*M_PI/180.0f), 8);
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
		glm::vec3 target (player.x + 6*sin(tpcamera_theta*M_PI/180.0f) , player.y + 6*cos(tpcamera_theta*M_PI/180.0f), player.z - 2);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
		glm::vec3 up (0, 0, 1);

		Matrices.view = glm::lookAt( eye, target, up ); 
	}
	else if(camera == 2)
	{
		glm::vec3 eye ( player.x + 2*sin(tpcamera_theta*M_PI/180.0f), player.y + 2*cos(tpcamera_theta*M_PI/180.0f),player.z + 2);
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
		glm::vec3 target (player.x + 6*sin(tpcamera_theta*M_PI/180.0f) , player.y + 6*cos(tpcamera_theta*M_PI/180.0f), player.z - 4);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
		glm::vec3 up (0, 0, 1);

		Matrices.view = glm::lookAt( eye, target, up ); 
	}
	else if(camera == 3)
	{
    	Matrices.view = glm::lookAt(glm::vec3(0,-0,18), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
	}

  	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

  	// Send our transformation to the currently bound shader, in the "MVP" uniform
  	// For each model you render, since the MVP will be different (at least the M part)
  	//  Don't change unless you are sure!!
  	glm::mat4 MVP;	// MVP = Projection * View * Model

  	// Load identity to model matrix
  	// Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  	// glPopMatrix ();

  	if(mapstart == 0)
  	{
  		player.z -= 0.5;
  		Matrices.model = glm::mat4(1.0f);

	    glm::mat4 translateRectangle = glm::translate (glm::vec3(player.x + 0.1, player.y + 0.1, player.z));        // glTranslatef
	    glm::mat4 rotateRectangle1 = glm::rotate((float)(player.rangle*M_PI/180.0f), glm::vec3(player.rx,player.ry,player.rz)); // rotate about vector (-1,1,1)
	    glm::mat4 rotateRectangle2 = glm::rotate((float)(player.sangle*M_PI/180.0f), glm::vec3(sx, sy, sz));
	    Matrices.model *= (translateRectangle * rotateRectangle2 *rotateRectangle1);
	    MVP = VP * Matrices.model;
	    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	    // draw3DObject draws the VAO given to it using current MVP matrix
	    draw3DObject(player.block);
	    draw3DObject(player.frame);

	    for(int i=0; i<12; i++)
	    {
	    	for(int j=0; j<12; j++)
	    	{
	    		Matrices.model = glm::mat4(1.0f);
	    		if(Area[levelno][i][j]==1 or Area[levelno][i][j]==3)
	    		{
	          		glm::mat4 translateRectangle = glm::translate (glm::vec3(2.02*i-10, 2.02*j-10, -502.4 + 5*t));        // glTranslatef
	          		glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	          		Matrices.model *= (translateRectangle * rotateRectangle);
	          		MVP = VP * Matrices.model;
	          		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

		          	// draw3DObject draws the VAO given to it using current MVP matrix
			    	draw3DObject(tile);
	      		}
	      		else if(Area[levelno][i][j]==4)
	      		{
	      			glm::mat4 translateRectangle = glm::translate (glm::vec3(2.02*i-10, 2.02*j-10, -502.4 + 5*t));        // glTranslatef
	          		glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	          		Matrices.model *= (translateRectangle * rotateRectangle);
	          		MVP = VP * Matrices.model;
	          		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

		          	// draw3DObject draws the VAO given to it using current MVP matrix
			    	draw3DObject(ftile);	
	      		}
	  		}
		}
		t++;
		if(t==101){mapstart = 1;}
	}
	else if(mapstart == 1)
	{
		if (bstatus == 1 and fmod(tangle,180)!=0)
		{
			zbridge -= 0.08;
			ybridge -= 0.202;
			tangle-=18;
		}
		Matrices.model = glm::mat4(1.0f);

	    glm::mat4 translateRectangle = glm::translate (glm::vec3(player.x + 0.1, player.y + 0.1, player.z));        // glTranslatef
	    glm::mat4 rotateRectangle1 = glm::rotate((float)(player.rangle*M_PI/180.0f), glm::vec3(player.rx,player.ry,player.rz)); // rotate about vector (-1,1,1)
	    glm::mat4 rotateRectangle2 = glm::rotate((float)(player.sangle*M_PI/180.0f), glm::vec3(sx, sy, sz));
	    Matrices.model *= (translateRectangle * rotateRectangle2 *rotateRectangle1);
	    MVP = VP * Matrices.model;
	    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	    // draw3DObject draws the VAO given to it using current MVP matrix
	    draw3DObject(player.block);
	    draw3DObject(player.frame);

	    if(fmod(player.sangle,10)!=0)
	    {
	    	player.sangle+=9;
	    	if(sx==-1)player.y+=0.202;
	    	else if(sx==1)player.y-=0.202;
	    	else if(sy==-1)player.x-=0.202;
	    	else if(sy==1)player.x+=0.202;
	    }
	    else
	    {
	    	player.sangle = 0;
	    	if(fmod(player.rangle,10)==0)
	    	{
	    		if(player.orientation == 0)
	    		{
	    			int tilex = round(((player.x+0.1)+10)/2.02);
	    			int tiley = round(((player.y+0.1)+10)/2.02);
	    			if(Area[levelno][tilex][tiley] == 3 and bstatus!=1){
	    				cout << "All Bridges Activated !!!\n" << endl;
	    				zswitch -= 0.39;
	    				bstatus = 1;
	    				tangle -= 18;
	    				zbridge -= 0.08;
	    				ybridge -= 0.202;
	    			}
	    			else if (Area[levelno][tilex][tiley]==0 or (Area[levelno][tilex][tiley]==2 and bstatus == 0))
	    			{
	    				player.z -= 0.5;
	    				if(player.z < -20) levelno = 4;
	    			}
	    			else if(Area[levelno][tilex][tiley] == 4)
	    			{
	    				player.z -= 0.5;
	    				fallfactor -= 1;
	    				fallx = tilex;
	    				fally = tiley;
	    				if(player.z < -20) levelno = 4;
	    			}
	    			else if (Area[levelno][tilex][tiley]==5)
	    			{
	    				player.z -=0.5;
	    				if(player.z < -10)
	    				{
	    					cout << "Level " << levelno+1;
	    					cout << " Cleared !!!" << endl;
	    					cout << "Total Moves Taken till now: "<< moves << endl;
	    					cout << "\n" <<endl;
	    					levelno ++;
	    					player.rangle = 0;
  							player.sangle = 0;
  							player.rz = 1;
  							player.z = 50;
  							player.x = -2.02*4;
  							player.y = -2.02*4;
  							mapstart = 0;
  							t=1;
  							zswitch = -1.8;
  							bstatus = 0;
  							ybridge = 2.02, zbridge = 0.8;
  							freecamera_theta = 45;
  							freecamera_omega = 45;
  							tpcamera_theta = 0;
  							tpcamera_theta_old = 0;
	    				}
	    			}
	    		}
	    		else if(player.orientation == 1)
	    		{
	    			int xc = round(((player.x+0.1)+10)/2.02);
	    			int yc1 = round((((player.y+0.1)+10)-1.01)/2.02);
	    			int yc2 = round((((player.y+0.1)+10)+1.01)/2.02);

	    			if((Area[levelno][xc][yc1] == 0 or (Area[levelno][xc][yc1]==2 and bstatus == 0)) and (Area[levelno][xc][yc2] == 0 or (Area[levelno][xc][yc2]==2 and bstatus == 0)))
	    			{
	    				if(player.z < -20) levelno = 4;
	    				player.z -= 0.5;
	    			}
	    			else if(Area[levelno][xc][yc1] == 0 or (Area[levelno][xc][yc1]==2 and bstatus == 0)) 
	    			{
	    				if(player.z < -20) levelno = 4;
	    				player.y += 0.101;
	    				player.z -= 0.5;
	    				player.rx = 1;
	    				player.rz = 0;
	    				player.ry = 0;
	    				player.rangle += 9;
	    			} 
	    			else if(Area[levelno][xc][yc2] == 0 or (Area[levelno][xc][yc2]==2 and bstatus == 0))
	    			{
	    				if(player.z < -20) levelno = 4;
	    				player.y -= 0.101;
	    				player.z -= 0.5;
	    				player.rx = -1;
	    				player.rz = 0;
	    				player.ry = 0;
	    				player.rangle += 9;
	    			}
	    		}
	    		else if(player.orientation == 2)
	    		{
	    			int xc1 = round((((player.x+0.1)+10)-1.01)/2.02);
	    			int xc2 = round((((player.x+0.1)+10)+1.01)/2.02);    
	    			int yc = round(((player.y+0.1)+10)/2.02);

    				if((Area[levelno][xc1][yc] == 0 or (Area[levelno][xc1][yc]==2 and bstatus == 0)) and (Area[levelno][xc2][yc] == 0 or (Area[levelno][xc2][yc]==2 and bstatus == 0)))
    				{
    					if(player.z < -20) levelno = 4;
    					player.z -= 0.5;
    				}
    				else if(Area[levelno][xc2][yc] == 0 or (Area[levelno][xc2][yc]==2 and bstatus == 0))
    				{
    					if(player.z < -20) levelno = 4;
    					player.rx = 0;
    					player.rz = 0;
    					player.ry = 1;
    					player.rangle += 9;
    					player.x -= 0.101;
    					player.z -= 0.5;
    				}
    				else if(Area[levelno][xc1][yc] == 0 or (Area[levelno][xc1][yc]==2 and bstatus == 0))
    				{
    					if(player.z < -20) levelno = 4;
    					player.rx = 0;
    					player.rz = 0;
    					player.ry = -1;
    					player.rangle += 9;
    					player.x += 0.101;
    					player.z -= 0.5;
    				}
    			}
    		}
    		else
    		{
    			player.rangle+=9;

	    		if(player.rx == -1)
	    		{
	    			player.y += 0.303;
	    			if(player.orientation == 0)
	    			{
	    				player.z -= 0.1;
	    				if(fmod(player.rangle,10)==0) player.orientation = 1;
	    			}
	    			else if(player.orientation == 1)
	    			{
	    				player.z += 0.1;
	    				if(fmod(player.rangle,10)==0) player.orientation = 0;
	    			}
	    		}
	    		else if(player.rx == 1)
	    		{
	    			player.y -= 0.303;
	    			if(player.orientation == 0)
	    			{
	    				player.z -= 0.1;
    					if(fmod(player.rangle,10)==0) player.orientation = 1;
    				}
    				else if(player.orientation == 1)
    				{
    					player.z += 0.1;
    					if(fmod(player.rangle,10)==0) player.orientation = 0;
    				}
    			}
	    		if(player.ry == -1)
	    		{
	    			player.x -= 0.303;
	    			if(player.orientation == 0)
	    			{
	    				player.z -= 0.1;
	    				if(fmod(player.rangle,10)==0) player.orientation = 2;
	    			}
	    			else if(player.orientation == 2)
	    			{
	    				player.z += 0.1;
	    				if(fmod(player.rangle,10)==0) player.orientation = 0;
	    			}
	    		}
	    		else if(player.ry == 1)
	    		{
	    			player.x += 0.303;
	    			if(player.orientation == 0)
	    			{
	    				player.z -= 0.1;
	    				if(fmod(player.rangle,10)==0) player.orientation = 2;
	    			}
	    			else if(player.orientation == 2)
	    			{
	    				player.z += 0.1;
	    				if(fmod(player.rangle,10)==0) player.orientation = 0;
	    			}
	    		}

	    	}
	    }
	    for(int i=0; i<12; i++)
	    {
	    	for(int j=0; j<12; j++)
	    	{
	    		Matrices.model = glm::mat4(1.0f);
	    		if(Area[levelno][i][j]==1 or Area[levelno][i][j]==3)
	    		{
	        		translateRectangle = glm::translate (glm::vec3(2.02*i-10, 2.02*j-10, -2.4));        // glTranslatef
        			glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
          			Matrices.model *= (translateRectangle * rotateRectangle);
          			MVP = VP * Matrices.model;
          			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

		          // draw3DObject draws the VAO given to it using current MVP matrix
		          draw3DObject(tile);
		      	}
		      	else if(Area[levelno][i][j]==2)
		      	{
		        	translateRectangle = glm::translate (glm::vec3(2.02*i-10, 2.02*j-10+ybridge, -2.4-zbridge));        // glTranslatef
		        	glm::mat4 rotateRectangle = glm::rotate((float)(tangle*M_PI/180.0f), glm::vec3(1,0,0)); // rotate about vector (-1,1,1)
		        	Matrices.model *= (translateRectangle * rotateRectangle);
		        	MVP = VP * Matrices.model;
        			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

		        	// draw3DObject draws the VAO given to it using current MVP matrix
          			draw3DObject(btile); 
      			}
      			else if(Area[levelno][i][j] == 4)
	      		{
	      			if(i == fallx and j == fally)
	      			{
	      				glm::mat4 translateRectangle = glm::translate (glm::vec3(2.02*i-10, 2.02*j-10, -2.4 + 0.75* fallfactor));
	      				glm::mat4 rotateRectangle = glm::rotate((float)(sin(2*fallfactor*M_PI/180.0f)), glm::vec3(1,1,0)); // rotate about vector (-1,1,1)
	          			Matrices.model *= (translateRectangle * rotateRectangle);
	          			MVP = VP * Matrices.model;
	          			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

		          		// draw3DObject draws the VAO given to it using current MVP matrix
			    		draw3DObject(ftile);
	      			}
	      			else
	      			{
	      				glm::mat4 translateRectangle = glm::translate (glm::vec3(2.02*i-10, 2.02*j-10, -2.4));
	      				glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	          			Matrices.model *= (translateRectangle * rotateRectangle);
	          			MVP = VP * Matrices.model;
	          			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

		          		// draw3DObject draws the VAO given to it using current MVP matrix
			    		draw3DObject(ftile);
			    	}	
	      		}
      			if(Area[levelno][i][j]==3)
      			{
      				Matrices.model = glm::mat4(1.0f);
        			translateRectangle = glm::translate (glm::vec3(2.02*i-10, 2.02*j-10, zswitch));        // glTranslatef
          			glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
          			glm::mat4 scaleRectangle = glm::scale (glm::vec3(0.5f, 0.5f, 0.5f));
          			Matrices.model *= (translateRectangle * rotateRectangle * scaleRectangle);
          			MVP = VP * Matrices.model;
        			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

		        	// draw3DObject draws the VAO given to it using current MVP matrix
        			draw3DObject(stile);
    			}
  			}
		}
	}
}
/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 32);

    window = glfwCreateWindow(width, height, "Tumblerz", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);
    glfwSetScrollCallback(window, scroll);       // general keyboard input

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
  // Generate the VAO, VBOs, vertices data & copy into the array buffer
    // Background color of the scene
	glClearColor (1.0f, 1.0f, 1.0f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);
  	glEnable(GL_MULTISAMPLE);

  	createRectangle ();
  	createtile();
	
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);


    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 800;
	int height = 800;
  	player.rangle = 0;
  	player.sangle = 0;
  	player.rz = 1;
  	player.z = 50;
  	player.x = -2.02*4;
  	player.y = -2.02*4;
  	moves = 0; 


    GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;

    cout << "\n\nWelcome to Tumblerz !!!\n" << endl;

    cout << "AIM: Reach the final hole without falling off the map. \nNOTE: It will only fall into the final hole if the block is in a vertical postion.\n\n";

    cout << "TYPES OF TILES:" << endl;
    cout << "1) Grey Tile - Normal tiles - Can move on them normally.\n2) Light Brown - Bridges(activated only when switch is pressed)\n3) Blue - Fragile Tiles - Tiles fall if block is oriented in vertical posotion on them.\n" << endl;

    cout << "Use the Up/Down/Left/Right keys to move the block" << endl;
    cout << "Use 'C' to change view -- Available views are: 1) Side Tower View and Free view  2) Follow Cam  3) First Person Cam  4) Top View  5) Helicopter View" << endl;
    cout << "Use W/A/S/D to move the camera in helicopter view" << endl;
    cout << "Use touchpad to control camera in free view" << endl;
    cout << "Press 'R' to reset the camera angle and position" << endl;
    cout << "Use 'P' to pause the game" << endl;
    cout << "Press 'M' to mute or unmute audio." << endl;

    /* Draw in loop */
    while (!glfwWindowShouldClose(window) and levelno<3) {

        // OpenGL Draw commands
        draw();

        if(levelno == 3)
        {
        	cout << "YOU WON THE GAME !!! " << endl;
        	quit(window);
        }
        else if(levelno == 4)
        {
        	cout << "GAME OVER ! :( YOU LOST :( \n" << endl;
        	quit(window);
        }

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            last_update_time = current_time;
        }
    }

    glfwTerminate();
//    exit(EXIT_SUCCESS);
}

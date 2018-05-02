/*
 *  CSCI 441, Computer Graphics, Fall 2017
 *
 *  Project: lab11
 *  File: main.cpp
 *
 *  Description:
 *      This file contains the basic setup to work with VAOs & VBOs using a
 *	MD5 model.
 *
 *  Author: Dr. Paone, Colorado School of Mines, 2017
 *
 *
 */

//******************************************************************************

#include <GL/glew.h>
#include <GLFW/glfw3.h>			// include GLFW framework header

// include GLM libraries and matrix functions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SOIL/SOIL.h>		// for image loading

#include <stdio.h>				// for printf functionality
#include <stdlib.h>				// for exit functionality
#include <time.h>

#include <vector>					// for vector

#include <CSCI441/objects3.hpp>
#include <CSCI441/ShaderProgram3.hpp>
#include <CSCI441/TextureUtils.hpp>
#include <CSCI441/modelLoader3.hpp>

//******************************************************************************
//
// Global Parameters

int windowWidth, windowHeight;
bool controlDown = false;
bool leftMouseDown = false;
glm::vec2 mousePosition( -9999.0f, -9999.0f );

glm::vec3 cameraAngles( 1.82f, 2.01f, 25.0f );
glm::vec3 eyePoint(   20.0f, 30.0f, 20.0f );
glm::vec3 lookAtPoint( 0.0f,  0.0f,  0.0f );
glm::vec3 upVector(    0.0f,  1.0f,  0.0f );

GLuint platformVAOd;
GLuint platformTextureHandle;
GLuint brickTexHandle;

GLuint skyboxVAOds[6];						// all of our skybox VAOs
GLuint skyboxHandles[6];                    // all of our skybox handles

CSCI441::ShaderProgram* textureShaderProgram = NULL;
GLint uniform_modelMtx_loc, uniform_viewProjetionMtx_loc, uniform_tex_loc, uniform_color_loc;
GLint attrib_vPos_loc, attrib_vTextureCoord_loc;

GLfloat groundSize = 10;

CSCI441::ShaderProgram *heightmapShaderProgram = NULL;
CSCI441::ModelLoader* plane = NULL;
GLuint heightmapTexHandle;
GLint map_v_pos, map_normal, map_mv_location, map_proj_mtx = -1, map_texCoord = -1;
GLint u_rand, u_rand2, u_time;
float r, r2 = 0;
float t;

CSCI441::ShaderProgram *genTexture = NULL;
GLuint genVAO;
GLint g_pos, g_mvp, g_res;

int framebufferWidth = 2000, framebufferHeight = 2000;
GLuint framebufferHandle;
GLuint fboTexHandle;
GLuint renderbufferHandle;

//******************************************************************************
//
// Helper Functions

// convertSphericalToCartesian() ///////////////////////////////////////////////
//
// This function updates the camera's position in cartesian coordinates based
//  on its position in spherical coordinates. Should be called every time
//  cameraAngles is updated.
//
////////////////////////////////////////////////////////////////////////////////
void convertSphericalToCartesian() {
	eyePoint.x = cameraAngles.z * sinf( cameraAngles.x ) * sinf( cameraAngles.y );
	eyePoint.y = cameraAngles.z * -cosf( cameraAngles.y );
	eyePoint.z = cameraAngles.z * -cosf( cameraAngles.x ) * sinf( cameraAngles.y );
}

bool registerOpenGLTexture(unsigned char *textureData,
                           unsigned int texWidth, unsigned int texHeight,
                           GLuint &textureHandle) {
    if(textureData == 0) {
        fprintf(stderr,"Cannot register texture; no data specified.");
        return false;
    }

    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);

    return true;
}

//******************************************************************************
//
// Event Callbacks

// error_callback() ////////////////////////////////////////////////////////////
//
//		We will register this function as GLFW's error callback.
//	When an error within GLFW occurs, GLFW will tell us by calling
//	this function.  We can then print this info to the terminal to
//	alert the user.
//
////////////////////////////////////////////////////////////////////////////////
static void error_callback(int error, const char* description) {
	fprintf(stderr, "[ERROR]: %s\n", description);
}

// key_callback() //////////////////////////////////////////////////////////////
//
//		We will register this function as GLFW's keypress callback.
//	Responds to key presses and key releases
//
////////////////////////////////////////////////////////////////////////////////
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if( (key == GLFW_KEY_ESCAPE || key == 'Q') && action == GLFW_PRESS )
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// mouse_button_callback() /////////////////////////////////////////////////////
//
//		We will register this function as GLFW's mouse button callback.
//	Responds to mouse button presses and mouse button releases.  Keeps track if
//	the control key was pressed when a left mouse click occurs to allow
//	zooming of our arcball camera.
//
////////////////////////////////////////////////////////////////////////////////
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS ) {
		leftMouseDown = true;
		controlDown = (mods & GLFW_MOD_CONTROL);
	} else {
		leftMouseDown = false;
		mousePosition.x = -9999.0f;
		mousePosition.y = -9999.0f;
		controlDown = false;
	}
}

// cursor_callback() ///////////////////////////////////////////////////////////
//
//		We will register this function as GLFW's cursor movement callback.
//	Responds to mouse movement.  When active motion is used with the left
//	mouse button an arcball camera model is followed.
//
////////////////////////////////////////////////////////////////////////////////
static void cursor_callback(GLFWwindow* window, double xpos, double ypos) {
	// make sure movement is in bounds of the window
	// glfw captures mouse movement on entire screen

	if( xpos > 0 && xpos < windowWidth ) {
		if( ypos > 0 && ypos < windowHeight ) {
			// active motion
			if( leftMouseDown ) {
				if( (mousePosition.x - -9999.0f) < 0.001f ) {
					mousePosition.x = xpos;
					mousePosition.y = ypos;
				} else {
					if( !controlDown ) {
						cameraAngles.x += (xpos - mousePosition.x)*0.005f;
						cameraAngles.y += (ypos - mousePosition.y)*0.005f;

						if( cameraAngles.y < 0 ) cameraAngles.y = 0.0f + 0.001f;
						if( cameraAngles.y >= M_PI ) cameraAngles.y = M_PI - 0.001f;
					} else {
						double totChgSq = (xpos - mousePosition.x) + (ypos - mousePosition.y);
						cameraAngles.z += totChgSq*0.01f;

						if( cameraAngles.z <= 2.0f ) cameraAngles.z = 2.0f;
						if( cameraAngles.z >= 30.0f ) cameraAngles.z = 30.0f;
					}
					convertSphericalToCartesian();

					mousePosition.x = xpos;
					mousePosition.y = ypos;
				}
			}
			// passive motion
			else {
			}
		}
	}
}

// scroll_callback() ///////////////////////////////////////////////////////////
//
//		We will register this function as GLFW's scroll wheel callback.
//	Responds to movement of the scroll where.  Allows zooming of the arcball
//	camera.
//
////////////////////////////////////////////////////////////////////////////////
static void scroll_callback(GLFWwindow* window, double xOffset, double yOffset ) {
	double totChgSq = yOffset;
	cameraAngles.z += totChgSq*0.2f;

	if( cameraAngles.z <= 4.0f ) cameraAngles.z = 4.0f;
	if( cameraAngles.z >= 50.0f ) cameraAngles.z = 50.0f;

	convertSphericalToCartesian();
}

//******************************************************************************
//
// Setup Functions

// setupGLFW() /////////////////////////////////////////////////////////////////
//
//		Used to setup everything GLFW related.  This includes the OpenGL context
//	and our window.
//
////////////////////////////////////////////////////////////////////////////////
GLFWwindow* setupGLFW() {
	// set what function to use when registering errors
	// this is the ONLY GLFW function that can be called BEFORE GLFW is initialized
	// all other GLFW calls must be performed after GLFW has been initialized
	glfwSetErrorCallback(error_callback);

	// initialize GLFW
	if (!glfwInit()) {
		fprintf( stderr, "[ERROR]: Could not initialize GLFW\n" );
		exit(EXIT_FAILURE);
	} else {
		fprintf( stdout, "[INFO]: GLFW initialized\n" );
	}

	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );						// request forward compatible OpenGL context
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );	// request OpenGL Core Profile context
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );		// request OpenGL 3.x context
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );		// request OpenGL 3.3 context

	// create a window for a given size, with a given title
	GLFWwindow *window = glfwCreateWindow(1080, 720, "Adv. Graphics Final Project", NULL, NULL);
	if( !window ) {						// if the window could not be created, NULL is returned
		fprintf( stderr, "[ERROR]: GLFW Window could not be created\n" );
		glfwTerminate();
		exit( EXIT_FAILURE );
	} else {
		fprintf( stdout, "[INFO]: GLFW Window created\n" );
	}

	glfwMakeContextCurrent(	window );	// make the created window the current window
	glfwSwapInterval( 1 );				    // update our screen after at least 1 screen refresh

	glfwSetKeyCallback( 			  window, key_callback				  );	// set our keyboard callback function
	glfwSetMouseButtonCallback( window, mouse_button_callback );	// set our mouse button callback function
	glfwSetCursorPosCallback(	  window, cursor_callback  			);	// set our cursor position callback function
	glfwSetScrollCallback(			window, scroll_callback			  );	// set our scroll wheel callback function

	return window;										// return the window that was created
}

// setupOpenGL() ///////////////////////////////////////////////////////////////
//
//      Used to setup everything OpenGL related.
//
////////////////////////////////////////////////////////////////////////////////
void setupOpenGL() {
	glEnable( GL_DEPTH_TEST );					// enable depth testing
	glDepthFunc( GL_LESS );							// use less than depth test

	glEnable(GL_BLEND);									// enable blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	// use one minus blending equation

	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// clear the frame buffer to black
}

// setupGLEW() /////////////////////////////////////////////////////////////////
//
//      Used to initialize GLEW
//
////////////////////////////////////////////////////////////////////////////////
void setupGLEW() {
	glewExperimental = GL_TRUE;
	GLenum glewResult = glewInit();

	/* check for an error */
	if( glewResult != GLEW_OK ) {
		printf( "[ERROR]: Error initalizing GLEW\n");
		/* Problem: glewInit failed, something is seriously wrong. */
  	fprintf( stderr, "[ERROR]: %s\n", glewGetErrorString(glewResult) );
		exit(EXIT_FAILURE);
	} else {
		 fprintf( stdout, "[INFO]: GLEW initialized\n" );
		 fprintf( stdout, "[INFO]: Status: Using GLEW %s\n", glewGetString(GLEW_VERSION) );
	}

	if( !glewIsSupported( "GL_VERSION_2_0" ) ) {
		printf( "[ERROR]: OpenGL not version 2.0+.  GLSL not supported\n" );
		exit(EXIT_FAILURE);
	}
}

// setupTextures() /////////////////////////////////////////////////////////////
//
//      Load and register all the tetures for our program
//
////////////////////////////////////////////////////////////////////////////////
void setupTextures() {
	platformTextureHandle = CSCI441::TextureUtils::loadAndRegisterTexture( "textures/metal.jpg" );

	// and get handles for our full skybox
  	printf( "[INFO]: registering skybox..." );
  	fflush( stdout );
  	skyboxHandles[0] = CSCI441::TextureUtils::loadAndRegisterTexture( "textures/skybox/back.png"   );   printf( "." ); fflush( stdout );
  	skyboxHandles[1] = CSCI441::TextureUtils::loadAndRegisterTexture( "textures/skybox/left.png"  );   printf( "." ); fflush( stdout );
  	skyboxHandles[2] = CSCI441::TextureUtils::loadAndRegisterTexture( "textures/skybox/front.png"  );   printf( "." ); fflush( stdout );
  	skyboxHandles[3] = CSCI441::TextureUtils::loadAndRegisterTexture( "textures/skybox/right.png"   );   printf( "." ); fflush( stdout );
  	skyboxHandles[4] = CSCI441::TextureUtils::loadAndRegisterTexture( "textures/skybox/bottom.png" );		printf( "." ); fflush( stdout );
  	skyboxHandles[5] = CSCI441::TextureUtils::loadAndRegisterTexture( "textures/skybox/top.png"    );   printf( "." ); fflush( stdout );
  	printf( "skybox textures read in and registered!\n\n" );

	heightmapTexHandle = CSCI441::TextureUtils::loadAndRegisterTexture("textures/heightmap.jpg");
}

void setupShaders() {
	textureShaderProgram = new CSCI441::ShaderProgram( "shaders/textureShader.v.glsl", "shaders/textureShader.f.glsl" );
	uniform_modelMtx_loc         = textureShaderProgram->getUniformLocation( "modelMtx" );
	uniform_viewProjetionMtx_loc = textureShaderProgram->getUniformLocation( "viewProjectionMtx" );
	uniform_tex_loc              = textureShaderProgram->getUniformLocation( "tex" );
	uniform_color_loc						 = textureShaderProgram->getUniformLocation( "color" );
	attrib_vPos_loc						   = textureShaderProgram->getAttributeLocation( "vPos" );
	attrib_vTextureCoord_loc 	   = textureShaderProgram->getAttributeLocation( "vTextureCoord" );

	// map Shader program
	heightmapShaderProgram = new CSCI441::ShaderProgram("shaders/generation.v.glsl", "shaders/generation.f.glsl");

	map_mv_location = heightmapShaderProgram->getUniformLocation("PMV");
	map_v_pos = heightmapShaderProgram->getAttributeLocation("vPos");
	map_normal = heightmapShaderProgram->getAttributeLocation("normal");
	map_texCoord = heightmapShaderProgram->getAttributeLocation("texCoord");
	u_rand = heightmapShaderProgram->getUniformLocation("r");
	u_rand2 = heightmapShaderProgram->getUniformLocation("r2");
	u_time = heightmapShaderProgram->getUniformLocation("time");
	t = float(clock());

	genTexture = new CSCI441::ShaderProgram("shaders/pass1.v.glsl", "shaders/pass1.f.glsl");
	g_pos = genTexture->getAttributeLocation("vPos");
	g_mvp = genTexture->getUniformLocation("mvp");
	g_res = genTexture->getUniformLocation("resolution");
}

// setupBuffers() //////////////////////////////////////////////////////////////
//
//      Create our VAOs & VBOs. Send vertex data to the GPU for future rendering
//
////////////////////////////////////////////////////////////////////////////////
void setupBuffers() {
	struct VertexTextured {
		float x, y, z;
		float s, t;
	};

	//////////////////////////////////////////
	//
	// PLATFORM

	VertexTextured platformVertices[4] = {
			{ -groundSize, 0.0f, -groundSize,   0.0f,  0.0f }, // 0 - BL
			{  groundSize, 0.0f, -groundSize,   1.0f,  0.0f }, // 1 - BR
			{ -groundSize, 0.0f,  groundSize,   0.0f,  1.0f }, // 2 - TL
			{  groundSize, 0.0f,  groundSize,   1.0f,  1.0f }  // 3 - TR
	};

	unsigned short platformIndices[4] = { 0, 1, 2, 3 };

	glGenVertexArrays( 1, &platformVAOd );
	glBindVertexArray( platformVAOd );

	GLuint vbods[2];
	glGenBuffers( 2, vbods );

	glBindBuffer( GL_ARRAY_BUFFER, vbods[0] );
	glBufferData( GL_ARRAY_BUFFER, sizeof( platformVertices ), platformVertices, GL_STATIC_DRAW );

	glEnableVertexAttribArray( attrib_vPos_loc );
	glVertexAttribPointer( attrib_vPos_loc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*) 0 );

	glEnableVertexAttribArray( attrib_vTextureCoord_loc );
	glVertexAttribPointer( attrib_vTextureCoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void*) (sizeof(float) * 3) );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbods[1] );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( platformIndices ), platformIndices, GL_STATIC_DRAW );

	//////////////////////////////////////////
	//
	// SKYBOX

	unsigned short groundIndices[4] = {
		0, 1, 2, 3
	};

	VertexTextured verts[6][4] = {
		{
			{-80.0f, -80.0f, -80.0f, 0.0f, 0.0f}, // 0 - BL
			{-80.0f, -80.0f, 80.0f, -1.0f, 0.0f}, // 1 - BR
			{-80.0f, 80.0f, -80.0f, 0.0f, 1.0f},  // 2 - TL
			{-80.0f, 80.0f, 80.0f, -1.0f, 1.0f}   // 3 - TR
		},
		{
			{-80.0f, -80.0f, 80.0f, 0.0f, 0.0f}, // 0 - BL
			{80.0f, -80.0f, 80.0f, -1.0f, 0.0f}, // 1 - BR
			{-80.0f, 80.0f, 80.0f, 0.0f, 1.0f},  // 2 - TL
			{80.0f, 80.0f, 80.0f, -1.0f, 1.0f}   // 3 - TR
		},
		{
			{80.0f, -80.0f, -80.0f, 0.0f, 0.0f}, // 0 - BL
			{80.0f, -80.0f, 80.0f, 1.0f, 0.0f},  // 1 - BR
			{80.0f, 80.0f, -80.0f, 0.0f, 1.0f},  // 2 - TL
			{80.0f, 80.0f, 80.0f, 1.0f, 1.0f}	// 3 - TR
		},
		{
			{-80.0f, -80.0f, -80.0f, 0.0f, 0.0f}, // 0 - BL
			{80.0f, -80.0f, -80.0f, 1.0f, 0.0f},  // 1 - BR
			{-80.0f, 80.0f, -80.0f, 0.0f, 1.0f},  // 2 - TL
			{80.0f, 80.0f, -80.0f, 1.0f, 1.0f}	// 3 - TR
		},
		{
			// Bottom Good
			{-80.0f, -80.0f, -80.0f, 0.0f, 0.0f}, // 0 - BL
			{80.0f, -80.0f, -80.0f, -1.0f, 0.0f}, // 1 - BR
			{-80.0f, -80.0f, 80.0f, 0.0f, -1.0f}, // 2 - TL
			{80.0f, -80.0f, 80.0f, -1.0f, -1.0f}  // 3 - TR
		},
		{
			// Top Good
			{-80.0f, 80.0f, -80.0f, 1.0f, -1.0f}, // 0 - BL
			{80.0f, 80.0f, -80.0f, 1.0f, 0.0f},   // 1 - BR
			{-80.0f, 80.0f, 80.0f, 0.0f, -1.0f},  // 2 - TL
			{80.0f, 80.0f, 80.0f, 0.0f, 0.0f}	 // 3 - TR
		}};

	glGenVertexArrays( 6, skyboxVAOds );

	int i = 0;
	for (GLuint id : skyboxVAOds) {

		glBindVertexArray(skyboxVAOds[i]);
		glGenBuffers(2, vbods);
		glBindBuffer(GL_ARRAY_BUFFER, vbods[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts[i]), verts[i], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbods[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(groundIndices), groundIndices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(attrib_vPos_loc);
		glVertexAttribPointer(attrib_vPos_loc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void *)0);
		glEnableVertexAttribArray(attrib_vTextureCoord_loc);
		glVertexAttribPointer(attrib_vTextureCoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void *)(sizeof(float) * 3));

		i++;
	}

	VertexTextured screenQ[4] = {
			{-1.0f, -1.0f, 0.0f, 0.0f, 0.0f}, // 0 - BL
			{1.0f, -1.0f, 0.0f, -1.0f, 0.0f}, // 1 - BR
			{-1.0f, 1.0f, 0.0f, 0.0f, 1.0f},  // 2 - TL
			{1.0f, 1.0f, 0.0f, -1.0f, 1.0f}   // 3 - TR
	};

	glGenVertexArrays(1, &genVAO);
	glBindVertexArray(genVAO);
	glGenBuffers(2, vbods);
	glBindBuffer(GL_ARRAY_BUFFER, vbods[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screenQ), screenQ, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbods[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(groundIndices), groundIndices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(attrib_vPos_loc);
	glVertexAttribPointer(attrib_vPos_loc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void *)0);
	glEnableVertexAttribArray(attrib_vTextureCoord_loc);
	glVertexAttribPointer(attrib_vTextureCoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTextured), (void *)(sizeof(float) * 3));

	// Framebuffer

	glGenFramebuffers(1, &framebufferHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferHandle);
	glGenRenderbuffers(1, &renderbufferHandle);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbufferHandle);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, framebufferWidth, framebufferHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbufferHandle);
	glGenTextures(1, &fboTexHandle);
	glBindTexture(GL_TEXTURE_2D, fboTexHandle);
	//initialize it with nothing! just tell it to hold that much space
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	/* set up filters and repeat/clamp params */
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexHandle, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_COMPLETE)
		printf("Framebuffer initialized completely!\n");
	else
		printf("Framebuffer FAILED TO INITIALIZE COMPLETELY.\n");
}

void setupPlane()
{
	plane = new CSCI441::ModelLoader();
	plane->loadModelFile("models/plane.obj");
	srand(static_cast<unsigned>(time(0)));
	r = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 5.0));
	r2 = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 5.0));
	printf("%f\n", r);
	printf("%f\n", r2);
}

//******************************************************************************
//
// Rendering / Drawing Functions - this is where the magic happens!

// renderScene() ///////////////////////////////////////////////////////////////
//
//		This method will contain all of the objects to be drawn.
//
////////////////////////////////////////////////////////////////////////////////

void renderScene(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, glm::mat4 orthoMtx)
{
	// textureShaderProgram->useProgram();

	glm::mat4 m, vp = projectionMatrix * viewMatrix;
	// glUniformMatrix4fv(uniform_modelMtx_loc, 1, GL_FALSE, &m[0][0]);
	// glUniformMatrix4fv(uniform_viewProjetionMtx_loc, 1, GL_FALSE, &vp[0][0]);
	// glUniform1ui(uniform_tex_loc, GL_TEXTURE0);

	// glm::vec3 white(1,1,1);
	// glUniform3fv( uniform_color_loc, 1, &white[0] );

	// for( unsigned int i = 0; i < 6; i++ ) {
	// 	glBindTexture( GL_TEXTURE_2D, skyboxHandles[i] );
	// 	glBindVertexArray(skyboxVAOds[i]);
	// 	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
	// }

	// glBindTexture(GL_TEXTURE_2D, platformTextureHandle);
	// glBindVertexArray(platformVAOd);
	// glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void *)0);

	heightmapShaderProgram->useProgram();

	glm::mat4 scaled = glm::scale(vp, glm::vec3(3));

	glUniformMatrix4fv(map_mv_location, 1, GL_FALSE, &scaled[0][0]);

	glBindTexture(GL_TEXTURE_2D, fboTexHandle);
	plane->draw(map_v_pos, map_normal, map_texCoord);
}

void renderTexture( glm::mat4 viewMatrix, glm::mat4 projectionMatrix ) {
	glm::mat4 m, vp = projectionMatrix * viewMatrix;

	glm::vec2 resolution = glm::vec2(windowHeight, windowWidth);

	genTexture->useProgram();
	glUniformMatrix4fv(g_mvp, 1, GL_FALSE, &vp[0][0]);
	glUniform2fv(g_res, 1, &resolution[0]);
	glBindVertexArray(genVAO);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
}

///*****************************************************************************
//
// Our main function

// main() ///////////////////////////////////////////////////////////////
//
//		Really you should know what this is by now.
//
////////////////////////////////////////////////////////////////////////////////
int main( int argc, char *argv[] ) {
	// GLFW sets up our OpenGL context so must be done first
	GLFWwindow *window = setupGLFW();					// initialize all of the GLFW specific information releated to OpenGL and our window
	setupOpenGL();										// initialize all of the OpenGL specific information
	setupGLEW();										// initialize all of the GLEW specific information
	setupShaders();										// load our shaders into memory
	setupBuffers();										// load all our VAOs and VBOs into memory
	setupTextures();									// load all textures into memory
	setupPlane();										// load plane obj

	convertSphericalToCartesian();						// set up our camera position

	CSCI441::setVertexAttributeLocations( attrib_vPos_loc, -1, attrib_vTextureCoord_loc );
	CSCI441::drawSolidSphere( 1, 16, 16 );	// strange hack I need to make spheres draw - don't have time to investigate why..it's a bug with my library
	
	//  This is our draw loop - all rendering is done here.  We use a loop to keep the window open
	//	until the user decides to close the window and quit the program.  Without a loop, the
	//	window will display once and then the program exits.
	while( !glfwWindowShouldClose(window) ) {	// check if the window was instructed to be closed
		glDrawBuffer( GL_BACK );				// work with our back frame buffer
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );	// clear the current color contents and depth buffer in the window

		// Get the size of our framebuffer.  Ideally this should be the same dimensions as our window, but
		// when using a Retina display the actual window can be larger than the requested window.  Therefore
		// query what the actual size of the window we are rendering to is.
		glfwGetFramebufferSize( window, &windowWidth, &windowHeight );

		///////////////////
		// FISRT PASS
		///////////////////

		glBindFramebuffer(GL_FRAMEBUFFER, framebufferHandle);
		glViewport(0, 0, framebufferWidth, framebufferHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the current color contents and depth buffer in the window

		glm::mat4 projectionMatrix = glm::perspective(45.0f, framebufferWidth / (float)framebufferHeight, 0.001f, 100.0f);
		glm::mat4 viewMatrix = glm::lookAt(eyePoint, lookAtPoint, upVector);

		renderTexture(viewMatrix, projectionMatrix);

		glFlush();

		///////////////////
		// SECOND PASS
		///////////////////
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, windowWidth, windowHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::mat4 orthoMtx = glm::ortho(-1.0, 1.0, -1.0, 1.0);

		renderScene(viewMatrix, projectionMatrix, orthoMtx);

		glfwSwapBuffers(window);// flush the OpenGL commands and make sure they get rendered!
		glfwPollEvents();				// check for any events and signal to redraw screen
	}

	glfwDestroyWindow( window );// clean up and close our window
	glfwTerminate();						// shut down GLFW to clean up our context

	return EXIT_SUCCESS;				// exit our program successfully!
}

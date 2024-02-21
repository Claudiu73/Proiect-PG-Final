#if defined (_APPLE_)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"

#include <iostream>
//Dimensiunea ferestrei cu scena
int glWinW = 2000;
int glWinH = 1000;

int retw, reth;
GLFWwindow* glWindow = NULL;

//Dimensiuni shadow mapping
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

//Matricile model, view, projection, nomalMatrix si lightRotation
glm::mat4 model;
GLuint modelLoc;

glm::mat4 view;
GLuint viewLoc;

glm::mat4 projection;
GLuint projectionLoc;

glm::mat3 normalMatrix;
GLuint normalMatrixLoc;

glm::mat4 lightRotation;

//Directia luminii
glm::vec3 lightDir;
GLuint lightDirLoc;

//Culoarea luminii
glm::vec3 lightColor;
GLuint lightColorLoc;

////normalize light direction
//glm::vec3 lightDirN = normalize(lightDir);
////compute view direction
//glm::vec3 viewDirN = normalize(viewPosEye - fragPosEye.xyz);
////compute half vector
//glm::vec3 halfVector = normalize(lightDirN + viewDirN);
////compute specular light
//float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
//specular = specularStrength * specCoeff * lightColor;

//Coordonate lumini punctiforme
glm::vec3 LightPos1 = glm::vec3(3.0, 1.0, 6.0), LightPos2 = glm::vec3(0.0, 1.0, 0.0);


//Sursa de lumina punctiforma nr1.
glm::vec3 pointLightPos1 = glm::vec3(4.0, 2.0, 7.0);
glm::vec3 pointLightColor1Loc = glm::vec3(0.0f, 0.0f, 1.0f);
GLuint pointLightAmbientStrength1Loc = 1.0f;
GLuint pointLightSpecularStrength1Loc = 1.0f;

//Sursa de lumina punctiforma nr 2.
glm::vec3 pointLightPos2 = glm::vec3(0.0, 1.0, 0.0);
glm::vec3 pointLightColor2Loc = glm::vec3(1.0f, 0.0f, 0.0f);
GLuint pointLightAmbientStrength2Loc = 1.0f;
GLuint pointLightSpecularStrength2Loc = 1.0f;

GLuint viewMatrixLoc;

//Coordonate pentru Camera si pozitiile ei
gps::Camera myCamera(
	glm::vec3(0.0f, 2.0f, 5.5f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.1f;//viteza camera

bool pressedKeys[1024];//pt tastatura
float angleY = 0.0f;//unghi rotatie camera
GLfloat lightAngle;//unghi rotatie lumina

//Obiectele pentru scena
gps::Model3D scenaMedievala;
gps::Model3D ground;
gps::Model3D lightCube;
gps::Model3D screenQuad;

//Shadere
gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;

//Framebuffer + textura adancime
GLuint shadowMapFBO;
GLuint depthMapTexture;

//Variabile pentru afisarea SkyBox-ului
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

//Variabila pentru ceata
int fog = 0;

//ascunderea hartii adancimii
bool showDepthMap;

bool presentation = 0;

bool firstMouse = true;
double lastX, lastY, mouseSensitivity = 0.1f;
double yaw = -90.0f, pitch = 0.0f;


//Functie pentru verificarea erorilor din OGL
GLenum glCheckError_(const char* file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)


//Callback pentru redimensionarea ferestrei
void windowResizeCallback(GLFWwindow* window, int width, int height) {
	// Seteaza viewport-ul la alte dimensiuni
	glViewport(0, 0, width, height);
	// Actualizeaza matricea projection 
	projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
	// Transmite o alta matrice de proiectie catre shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

//Callback tastatura
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	//Inchide Scena cu ESC
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	//Afiseaza depthMap
	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = showDepthMap;

	//actiunea de apasare si lasare a tastei
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void updateCameraRotation(float xOffset, float yOffset) {
	xOffset *= mouseSensitivity;
	yOffset *= mouseSensitivity;

	yaw += xOffset;
	pitch += yOffset;

	// Normalizarea unghiurilor yaw și pitch
	if (yaw > 360.0f) yaw -= 360.0f;
	else if (yaw < 0.0f) yaw += 360.0f;

	if (pitch > 89.0f) pitch = 89.0f;
	else if (pitch < -89.0f) pitch = -89.0f;

	myCamera.rotate(pitch, yaw);
}

void printCameraPositionAndFront(const gps:: Camera& camera) {
	// Presupunând că clasa Camera are membrii publici sau metode getter pentru poziția și direcția camerei
	glm::vec3 pos = camera.cameraPosition; 
	glm::vec3 front = camera.cameraFrontDirection; 

	std::cout << "Camera Position - X: " << pos.x << ", Y: " << pos.y << ", Z: " << pos.z << std::endl;
	std::cout << "Camera Front Direction - X: " << front.x << ", Y: " << front.y << ", Z: " << front.z << std::endl;
}


void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (presentation) return;

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xOffset = xpos - lastX;
	float yOffset = lastY - ypos; // Inversăm axa Y pentru a se potrivi cu coordonatele ferestrei
	lastX = xpos;
	lastY = ypos;

	updateCameraRotation(xOffset, yOffset);

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		printCameraPositionAndFront(myCamera);
	}
}




//Cum se misca scena?
void processMovement()
{
	//Rotatie la dreapta
	if (pressedKeys[GLFW_KEY_E]) {
		angleY -= 1.0f;
	}
	//Rotatie la stanga
	if (pressedKeys[GLFW_KEY_Q]) {
		angleY += 1.0f;
	}
	//Modifica unghiul de lumina
	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle -= 1.0f;
	}
	if (pressedKeys[GLFW_KEY_J]) {
		lightAngle += 1.0f;
	}
	//Se duce inainte
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}
	//Marsarier
	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}
	//Stanga
	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}
	//Dreapta
	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}
	//Tasta pentru activarea cetei
	if (pressedKeys[GLFW_KEY_N]) {
		fog = 1;
		myCustomShader.useShaderProgram();
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "fog"), fog);
	}
	//Dezactiveaza ceata
	if (pressedKeys[GLFW_KEY_B]) {
		fog = 0;
		myCustomShader.useShaderProgram();
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "fog"), fog);
	}
	//solid
	if (pressedKeys[GLFW_KEY_I]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	//wireframe
	if (pressedKeys[GLFW_KEY_O]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}
	//normal
	if (pressedKeys[GLFW_KEY_P]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
}
//Initializeaza fereastra openGL
bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glWindow = glfwCreateWindow(glWinW, glWinH, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}
	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwMakeContextCurrent(glWindow);
	glfwSwapInterval(1);

#if not defined (_APPLE_)
	glewExperimental = GL_TRUE;
	glewInit();
#endif

	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);
	glfwGetFramebufferSize(glWindow, &retw, &reth);
	return true;
}
//Initializare stare OpenGL
void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retw, reth);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); 
	glFrontFace(GL_CCW); 
	glEnable(GL_FRAMEBUFFER_SRGB);
}

//Initializare Obiecte Scena
void initObjects() {
	
	//ground.LoadModel("objects/scena.obj");
	//lightCube.LoadModel("objects/Project_PG.obj");
	scenaMedievala.LoadModel("objects/scena.obj");
	//screenQuad.LoadModel("objects/scena.obj");

}

//Initializare Shader-e
void initShaders() 
{
	//Modelul principal de Shader
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	
	//Pentru Screen
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();

	//Pentru Lumina
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	lightShader.useShaderProgram();
	
	//Pentru mapa de shadow
	depthMapShader.loadShader("shaders/depthMapShader.vert", "shaders/depthMapShader.frag");
	depthMapShader.useShaderProgram();

	//Pentru SkyBox
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
}

void initUniforms() {
	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//Matricea model

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	//Matricea view

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	//Matricea normala

	projection = glm::perspective(glm::radians(45.0f), (float)retw / (float)reth, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
	//Matricea proiectie

	// Setări pentru iluminarea direcțională (de exemplu, soare).
	// Uniforme pentru lumină direcțională (e.g., soarele).
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	// Updateaza uniformele pentru Point Light 1
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightColor1"), 1, glm::value_ptr(pointLightColor1Loc));
	glm::vec3 pointLightPosEye1 = glm::vec3(view * model * glm::vec4(LightPos1, 1.0f));
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightPosEye1"), 1, glm::value_ptr(pointLightPosEye1));
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightAmbientStrength1"), pointLightAmbientStrength1Loc);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightSpecularStrength1"), pointLightSpecularStrength1Loc);

	// Updateaza uniformele pentru Point Light 2
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightColor2"), 1, glm::value_ptr(pointLightColor2Loc));
	glm::vec3 pointLightPosEye2 = glm::vec3(view * model * glm::vec4(LightPos2, 1.0f));
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightPosEye2"), 1, glm::value_ptr(pointLightPosEye2));
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightAmbientStrength2"), pointLightAmbientStrength2Loc);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightSpecularStrength2"), pointLightSpecularStrength2Loc);
}

//Initializarea Framebuffer-ului
void initFBO() {
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glGenFramebuffers(1, &shadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	//Atasare textura framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void generateShadowMap() {
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

	// Aici redăm fiecare obiect interesat de umbre
	// Pentru nanosuit
	model = glm::mat4(1.0f); // Ajustează aceasta la transformările necesare pentru obiect
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	scenaMedievala.Draw(lightShader);

	// Repetă pentru alte obiecte, dacă este necesar
	// De exemplu, pentru ground
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.5f)); // Ajustează aceasta conform nevoilor tale
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	ground.Draw(lightShader);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, retw, reth);
}



glm::mat4 computeLightSpaceTrMatrix() {
	float near_plane = 1.0f, far_plane = 7.5f;
	glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
	return lightSpaceTrMatrix;
}

//Desenarea Obiectelor
void drawObjects(bool depthPass, gps::Shader shader) {

	shader.useShaderProgram();

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	scenaMedievala.Draw(shader);

	model = glm::scale(model, glm::vec3(0.5f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	

	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	ground.Draw(shader);
}

float delta = 0;
float movementSpeed = 2; // unitati per secunda
//Miscarea scenei
void updateDelta(double elapsedSeconds) 
{
	delta = delta + movementSpeed * elapsedSeconds;
}
double lastTimeStamp = glfwGetTime();

//Functia de randare
void renderScene() {

	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	// Updateaza pozitiile luminilor punctiforme (transformate in spatiul de vizualizare).
	
	glm::vec3 pointLightPosE2 = glm::vec3(view * model * glm::vec4(pointLightColor2Loc, 1.0));
	glm::vec3 pointLightPosE1 = glm::vec3(view * model * glm::vec4(pointLightColor1Loc, 1.0));
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightPosE1"), 1, glm::value_ptr(pointLightPosE1));
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLightPosE2"), 1, glm::value_ptr(pointLightPosE2));
	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "lightSpaceMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	// Render the scene from the light's point of view
	drawObjects(true, lightShader);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (!showDepthMap) {
		// final scene rendering pass (with shadows)

		glViewport(0, 0, retw, reth);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		myCustomShader.useShaderProgram();

		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));



		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

		drawObjects(false, myCustomShader);
		mySkyBox.Draw(skyboxShader, view, projection);

		//draw a white cube around the light

		lightShader.useShaderProgram();

		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		model = lightRotation;
		model = glm::translate(model, 1.0f * lightDir);
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		lightCube.Draw(lightShader);
	}
	else {

		//Randare harta adancime
		glViewport(0, 0, retw, reth);

		glClear(GL_COLOR_BUFFER_BIT);

		screenQuadShader.useShaderProgram();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		double currentTimeStamp = glfwGetTime();
		updateDelta(currentTimeStamp - lastTimeStamp);
		lastTimeStamp = currentTimeStamp;
		model = glm::translate(model, glm::vec3(delta, 0, 0));

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
		
	}
}

//Initializarea pentru skyBox
void initSkyBox() {
	std::vector<const GLchar*> faces;
	faces.push_back("skybox/right.tga");
	faces.push_back("skybox/left.tga");
	faces.push_back("skybox/top.tga");
	faces.push_back("skybox/bottom.tga");
	faces.push_back("skybox/back.tga");
	faces.push_back("skybox/front.tga");
	mySkyBox.Load(faces);
}

//Inchidere aplicatie OpenGL
void cleanup() {
	glDeleteTextures(1, &depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);	
	glfwTerminate();
}


int main(int argc, const char* argv[]) {


	if (!initOpenGLWindow()) {//Daca nu e initializata deschiderea fereastrei
		glfwTerminate();	//Gata
		return 1;
	}

	initOpenGLState();	//Initializarea Starilor
	initShaders();		//Initializarea Shader-elor
	initObjects();		//Initializarea Obiectelor
	initFBO();			//Initializarea Framebuffer
	initUniforms();		//Initializarea Uniformelor
	initSkyBox();		//Initializarea SkyBox

	glCheckError();		//Afiseaza erorile

	while (!glfwWindowShouldClose(glWindow)) {//Bucla de procesare a activitatilor scenei
		processMovement();//Procesul de miscare
		renderScene();	  //Randarea Scenei
		glfwPollEvents();	
		glfwSwapBuffers(glWindow);
	}

	cleanup();	//Inchiderea aplicatiei

	return 0;	//THE END.
}
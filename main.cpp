/**
This application renders a textured mesh that was loaded with Assimp.
*/
#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <memory>
#include <glad/glad.h>

#include "Mesh3D.h"
#include "Object3D.h"
#include "Animator.h"
#include "RotationAnimation.h"
#include "ShaderProgram.h"

#include "Skeletal.h"
#include <algorithm>
#include <Skeletal.h>
#include <SkeletalAnimator.h>

#define PI glm::pi<float>()

/**
 * @brief Defines a collection of objects that should be rendered with a specific shader program.
 */
struct Scene {
	ShaderProgram defaultShader;
	std::vector<Object3D> objects;
	std::vector<Animator> animators;
};

/**
 * @brief Constructs a shader program that renders textured meshes in the Phong reflection model.
 * The shaders used here are incomplete; see their source codes.
 * @return
 */
ShaderProgram phongLighting() {
	ShaderProgram program;
	try {
		program.load("shaders/light_perspective.vert", "shaders/lighting.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return program;
}

/**
 * @brief Constructs a shader program that renders textured meshes without lighting.
 */
ShaderProgram textureMapping() {
	ShaderProgram program;
	try {
		program.load("shaders/texture_perspective.vert", "shaders/texturing.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return program;
}

ShaderProgram sameColor() {
	ShaderProgram program;
	try {
		program.load("shaders/texture_perspective.vert", "shaders/same_color.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return program;
}

ShaderProgram skeletalShader() {
	ShaderProgram program;
	try {
		program.load("shaders/skeletal.vert", "shaders/lighting.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return program;
}

ShaderProgram shadowMapShader() {
	ShaderProgram program;
	try {
		program.load("shaders/shadow_map.vert", "shaders/shadow_map.frag", "shaders/shadow_map.gs");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return program;
}

/**
 * @brief Loads an image from the given path into an OpenGL texture.
 */
Texture loadTexture(const std::filesystem::path& path, const std::string& samplerName = "baseTexture") {
	sf::Image i;
	i.loadFromFile(path.string());
	return Texture::loadImage(i, samplerName);
}

// Shadow
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
GLuint depthMapFBO;
unsigned int depthCubemap;
ShaderProgram setUpShadow() {

	glGenFramebuffers(1, &depthMapFBO);
	// Create depth texture
	glGenTextures(1, &depthCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
	for (unsigned int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return shadowMapShader();
}

// set up for 1 light source, multiple light sources use multiple parameters like these
void setUpLight(ShaderProgram& program) {
	program.activate();
	program.setUniform("ambientColor", glm::vec3(1, 1, 1));
	program.setUniform("directionalColor", glm::vec3(1, 1, 1));

	// parameter for object, should be different for each object
	program.setUniform("material", glm::vec4(0.5, 0.5, 1, 32));

	//program.setUniform("hasDirectionalLight", true);
	//program.setUniform("directionalLight", glm::vec3(0, 0, -1));

	program.setUniform("light_constant", 1.0f);
	// distance inf
	program.setUniform("light_linear", 0.0f);
	program.setUniform("light_quadratic", 0.0f);
	// distance 100
	//program.setUniform("light_linear", 0.045f);
	//program.setUniform("light_quadratic", 0.0075f);
	// distance 50
	//program.setUniform("light_linear", 0.09f);
	//program.setUniform("light_quadratic", 0.0032f);
}

void renderSkeletal(sf::RenderWindow& window, ShaderProgram& program, Object3D& obj, std::vector<glm::mat4>& transforms) {
	program.activate();
	program.setUniform("skeletal", true);
	for (int i = 0; i < transforms.size(); ++i)
		program.setUniform("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
	obj.render(window, program);
	program.setUniform("skeletal", false);
}

Scene lightScene() {
	Texture tmp_texture;
	auto mesh = Mesh3D::cube(tmp_texture);
	auto light_cube = Object3D(std::vector<Mesh3D>{mesh});
	light_cube.move(glm::vec3(0, 9, 0));
	light_cube.grow(glm::vec3(0.1, 0.1, 0.1));
	std::vector<Object3D> objects;
	objects.push_back(std::move(light_cube));

	return Scene{
		sameColor(),
			std::move(objects),
	};
}

// line segment - circle intersect
struct Circle {
	glm::vec2 center;
	float radius;
};

struct Wall {
	glm::vec2 start;
	glm::vec2 end;
	glm::vec2 normal;  // Assuming normal is a unit vector

	Object3D wall_object;

	Wall(std::vector<Texture>& textures, glm::vec3 pos, glm::vec3 rot, float width, float height) {
		wall_object = Object3D(std::vector<Mesh3D>{Mesh3D::square(textures)});

		wall_object.grow(glm::vec3(width, height, 1));
		wall_object.move(pos);
		wall_object.rotate(rot);

		start = glm::vec2(-0.5f, 0.0f);
		end = glm::vec2(0.5f, 0.0f);

		float c = rot.y;
		glm::mat2 rotationMatrix = glm::mat2(
			glm::cos(c), -glm::sin(c),
			glm::sin(c), glm::cos(c)
		);
		start = rotationMatrix * start;
		end = rotationMatrix * end;

		normal = glm::vec2(0, 1);
		normal = glm::rotate(normal, -c);

		start *= width;
		end *= width;

		start += glm::vec2(pos.x, pos.z);
		end += glm::vec2(pos.x, pos.z);
	}
};

float pointLineSignedDistance(const glm::vec2& point, const glm::vec2& start, const glm::vec2& end, const glm::vec2& normal) {
	glm::vec2 pointVec = point - start;
	float signedDist = glm::dot(pointVec, normal);
	return signedDist;
}

void checkCollision(Circle& circle, const Wall& wall, glm::vec2* target = nullptr) {
	float distance = pointLineSignedDistance(circle.center, wall.start, wall.end, wall.normal);
	if (!target) {
		if (distance <= circle.radius) {
			float overlap = circle.radius - distance;
			if (overlap > 0) {
				circle.center += wall.normal * overlap;
			}
		}
	}
	else {
		auto a = glm::normalize(*target - circle.center);
		auto b = wall.normal;
		auto distant_target_wall = pointLineSignedDistance(*target, wall.start, wall.end, wall.normal);
		if (glm::dot(a, b) > 0 && distance <= circle.radius && distant_target_wall >= 0) {

			float overlap = circle.radius - distance;
			if (overlap > 0) {
				circle.center += wall.normal * overlap;
			}
		}
	}
}

bool isCollideBallWall(Circle& circle, const Wall& wall) {
	auto x = circle.center.x,
		y = circle.center.y;
	if ((x < std::min(wall.start.x, wall.end.x) || x > std::max(wall.start.x, wall.end.x))
		&& (y < std::min(wall.start.y, wall.end.y) || y > std::max(wall.start.y, wall.end.y))) {
		return false;
	}

	float distance = pointLineSignedDistance(circle.center, wall.start, wall.end, wall.normal);

	if (distance <= circle.radius) {
		return true;
	}
	return false;
}


// Object3D is same as Object3D, except Object3D has bones array for skeletal animation.
int main() {
	// Initialize the window and OpenGL.
	sf::ContextSettings Settings;
	Settings.depthBits = 24; // Request a 24 bits depth buffer
	Settings.stencilBits = 8;  // Request a 8 bits stencil buffer
	Settings.antialiasingLevel = 2;  // Request 2 levels of antialiasing
	sf::RenderWindow window(sf::VideoMode{ 1200, 800 }, "SFML Demo", sf::Style::Resize | sf::Style::Close, Settings);
	gladLoadGL();
	glEnable(GL_DEPTH_TEST);

	// shadow set up 
	auto shadow_shader = setUpShadow();

	// main shader set up

	ShaderProgram skeletal_shader = skeletalShader();

	auto perspective = glm::perspective(glm::radians(45.0), static_cast<double>(window.getSize().x) / window.getSize().y, 0.1, 100.0);
	skeletal_shader.activate();
	skeletal_shader.setUniform("projection", perspective);

	setUpLight(skeletal_shader);

	// coach clapping 
	Skeletal coach_model("models/coach/Clapping.dae", true);

	SkeletalAnimation coach_clap("models/coach/Clapping.dae", &coach_model);
	SkeletalAnimator coach_animator(&coach_clap);

	auto& coach = coach_model.getRoot();
	coach.grow(glm::vec3(1.2, 1.2, 1.2));
	coach.move(glm::vec3(3, 0, -5));

	// goalkeeper 
	Skeletal goalkeeper_model("models/goalkeeper/goalkeeper.dae", true);

	SkeletalAnimation goalkeeper_stand("models/goalkeeper/goalkeeper.dae", &goalkeeper_model);
	SkeletalAnimator goalkeeper_animator(&goalkeeper_stand);

	auto& goalkeeper = goalkeeper_model.getRoot();
	goalkeeper.grow(glm::vec3(1.2, 1.2, 1.2));
	goalkeeper.move(glm::vec3(0, 0, -6));

	// kid 
	Skeletal kid_model("models/kid/kid.dae", true);

	SkeletalAnimation kid_wave("models/kid/kid.dae", &kid_model);
	SkeletalAnimator kid_animator(&kid_wave);

	SkeletalAnimation idle_animation("models/kid/idle.dae", &kid_model);
	SkeletalAnimator idle_animator(&idle_animation);

	auto& kid = kid_model.getRoot();
	//kid.move(glm::vec3(0, 0, 0));
	float_t kid_scale = 1.0;
	kid.grow(glm::vec3(kid_scale, kid_scale, kid_scale));
	kid.setMass(10);

	float_t kid_height = 2;
	float_t kid_velocity = 4;
	glm::vec3 kid_forward = glm::vec3(0, 0, 1);

	glm::vec3 desired_direction;
	Animator rotate_kid;
	rotate_kid.addAnimation(
		[&kid, &kid_forward, &desired_direction]() {
			auto a = kid_forward;
			a.y = 0;
			a = glm::normalize(a);
			auto b = desired_direction;
			b.y = 0;
			b = glm::normalize(b);

			auto tmp = glm::dot(a, b);
			tmp = std::max(tmp, -1.0f);
			tmp = std::min(tmp, 1.0f);
			auto angle = acos(tmp);

			//std::cout <<a<<b << "\n";
			angle *= glm::cross(a, b).y >= 0 ? 1 : -1;
			kid_forward = desired_direction;
			//std::cout << "---------------  " << glm::degrees(angle) << "\n";
			return std::make_unique<RotationAnimation>(kid, 0.15, glm::vec3(0, angle, 0));
		}
	);

	// wall
	std::vector<Texture> textures = {
		loadTexture("models/frame/wall.jpg", "baseTexture"),
		//loadTexture("models/brick_wall/brickwall_normal.jpg", "normalMap"),
	};

	std::vector<Texture> groundTexture = {
		loadTexture("models/frame/ground2.jpg", "baseTexture"),
	};
	auto mesh = Mesh3D::square(groundTexture);
	auto ground = Object3D(std::vector<Mesh3D>{mesh});
	ground.rotate(glm::vec3(-PI / 2, 0, 0));
	ground.grow(glm::vec3(30, 30, 30));

	std::vector<Wall> walls = {
		Wall(textures, glm::vec3(-10, 5, 0), glm::vec3(0, PI / 2, 0), 30.0f, 15.0f),
		Wall(textures, glm::vec3(10, 5, 0), glm::vec3(0, -PI / 2, 0), 30.0f, 15.0f),
		Wall(textures, glm::vec3(0, 5, -10), glm::vec3(0, 0, 0), 30.0f, 15.0f),
		Wall(textures, glm::vec3(0, 5, 10), glm::vec3(0, PI, 0), 30.0f, 15.0f),
	};

	auto ceiling = Object3D(std::vector<Mesh3D>{Mesh3D::square(textures)});
	ceiling.rotate(glm::vec3(PI / 2, 0, 0));
	ceiling.grow(glm::vec3(30, 30, 30));
	ceiling.move(glm::vec3(0, 10, 0));

	// ---------------------------------------------------------------------------------------------------------------

	auto ball = Skeletal("models/basketball/Basketball.obj", true).getRoot();
	float_t ball_radius = 0.01;
	float_t mass = 1;	
	ball.setMass(mass);
	ball.grow(glm::vec3(0.2, 0.2, 0.2));
	ball.move(glm::vec3(0, 2, -3));

	auto goal = Skeletal("models/goal/gawang.obj", true).getRoot();
	float_t goal_radius = 0.3;
	float_t mass2 = 1;
	goal.setMass(mass2);
	goal.grow(glm::vec3(1, 1, 1));
	goal.move(glm::vec3(0, 0, -8));

	// light source
	auto light_scene = lightScene();
	auto light_cube = light_scene.objects[0];
	ShaderProgram& light_shader = light_scene.defaultShader;
	light_shader.activate();
	light_shader.setUniform("projection", perspective);
	light_shader.setUniform("color", glm::vec4(1, 1, 1, 1));

	// camera
	float radius = 10.0f;
	float azimuth = 0;   // Horizontal angle
	float elevation = 0; // Vertical angle
	auto target = kid.getPosition();
	target.y += kid_height;
	glm::vec3 camera_pos(
		target.x + radius * cos(elevation) * sin(azimuth),
		target.y + radius * sin(elevation),
		target.z + radius * cos(elevation) * cos(azimuth)
	);
	auto up_vector = glm::vec3(0, 1, 0);
	auto camera = glm::lookAt(camera_pos, target, up_vector);

	// ----------------------------------------------------------------------------------------------------------------------
	// Run
	bool running = true;
	sf::Clock c;

	sf::Vector2i last_mouse_position = sf::Vector2i(window.getSize().x / 2, window.getSize().y / 2);
	bool moving = false,
		move_left = false,
		move_right = false,
		move_forward = false,
		move_backward = false,
		jumping = false;
	auto last_gravity_time = c.getElapsedTime();

	bool dung = false;

	auto last = c.getElapsedTime();
	while (running) {
		sf::Event ev;
		while (window.pollEvent(ev)) {
			if (ev.type == sf::Event::Closed) {
				running = false;
			}
			else if (ev.type == sf::Event::KeyPressed)
			{
				moving = true;
				if (ev.key.code == sf::Keyboard::W) {
					move_forward = true;
				}
				if (ev.key.code == sf::Keyboard::S) {
					move_backward = true;
				}
				if (ev.key.code == sf::Keyboard::A) {
					move_left = true;
				}
				if (ev.key.code == sf::Keyboard::D) {
					move_right = true;
				}
				if (ev.key.code == sf::Keyboard::Space) {
					jumping = true;
				}
				if (ev.key.code == 'Q' - 'A') {
					ball.addForce(glm::vec3(0, 200, 0));
				}
			}
			else if (ev.type == sf::Event::KeyReleased) {
				if (ev.key.code == sf::Keyboard::W) {
					move_forward = false;
				}
				if (ev.key.code == sf::Keyboard::S) {
					move_backward = false;
				}
				if (ev.key.code == sf::Keyboard::A) {
					move_left = false;
				}
				if (ev.key.code == sf::Keyboard::D) {
					move_right = false;
				}
				if (ev.key.code == sf::Keyboard::Space) {
					jumping = false;
				}
			}
		}

		auto now = c.getElapsedTime();
		auto diff = now - last;
		auto diffSeconds = diff.asSeconds();
		last = now;

		if (ball.getPosition().y - ground.getPosition().y <= ball_radius) {
			auto v = glm::normalize(ball.getVelocity());
			auto n = glm::normalize(glm::vec3(0, 1, 0));
			auto r = v - 2.0f * n * glm::dot(n, v);
			r = 0.7f * glm::length(ball.getVelocity()) * r;
			ball.setVelocity(r);

			auto temp = ball.getPosition();
			temp.y = ball_radius;
			ball.setPosition(temp);

		}

		// control camera

		sf::Vector2i mouse_position = sf::Mouse::getPosition();

		if (window.getSize().x - mouse_position.x <= 1 || mouse_position.x <= 1) {
			sf::Mouse::setPosition(sf::Vector2i(window.getSize().x / 2, mouse_position.y));
			last_mouse_position = sf::Mouse::getPosition();
		}
		else {
			float_t delta_x = 1.0f * (mouse_position.y - last_mouse_position.y) / (window.getSize().y) * 3.14 / 2.0;
			float_t delta_y = -1.0f * (mouse_position.x - last_mouse_position.x) / (window.getSize().x) * 3.14 / 2.0;

			azimuth += delta_y;
			elevation += delta_x;
			elevation = std::max(-glm::half_pi<float>() + 0.01f, std::min(glm::half_pi<float>() - 0.01f, elevation));


			target = kid.getPosition();
			target.y += kid_height;
			camera_pos = glm::vec3(
				target.x + radius * cos(elevation) * sin(azimuth),
				std::max(0.2f, target.y + radius * sin(elevation)),
				target.z + radius * cos(elevation) * cos(azimuth)
			);

			// intersect wall - cam
			Circle cam_circle = { {camera_pos.x, camera_pos.z}, 0.2f };

			for (auto& wall : walls) {
				auto tmp = glm::vec2(target.x, target.z);
				checkCollision(cam_circle, wall, &tmp);
			}
			camera_pos.x = cam_circle.center.x;
			camera_pos.z = cam_circle.center.y;
			// ----------------------------------------------------------------------------------------------------

			camera = glm::lookAt(camera_pos, target, up_vector);

			last_mouse_position = mouse_position;
		}

		skeletal_shader.activate();
		skeletal_shader.setUniform("view", camera);
		skeletal_shader.setUniform("viewPos", camera_pos);

		// control character
		if ((!move_left && !move_right && !move_forward && !move_backward) || jumping) {
			moving = false;
		}
		else {
			moving = true;
		}
		std::vector <glm::mat4> kid_transforms;
		if (moving && kid.getPosition().y == 0) {
			kid_animator.UpdateAnimation(diff.asSeconds());
			kid_transforms = kid_animator.GetFinalBoneMatrices();
		}
		else {
			idle_animator.UpdateAnimation(diff.asSeconds());
			kid_transforms = idle_animator.GetFinalBoneMatrices();
		}

		glm::vec3 forward_cam = target - camera_pos;
		forward_cam.y = 0;
		forward_cam = glm::normalize(forward_cam);
		glm::vec3 right_cam = glm::cross(forward_cam, up_vector);
		right_cam.y = 0;
		right_cam = glm::normalize(right_cam);

		desired_direction = glm::vec3(0);
		if (move_forward) {
			desired_direction += forward_cam;
		}
		else if (move_backward) {
			desired_direction -= forward_cam;
		}

		if (move_right) {
			desired_direction += right_cam;
		}
		else if (move_left) {
			desired_direction -= right_cam;
		}

		auto kid_velocity_y = kid.getVelocity().y;
		kid.setVelocity(glm::vec3(0, kid_velocity_y, 0) + desired_direction * kid_velocity);

		if (desired_direction.x != 0 || desired_direction.z != 0) {
			if (rotate_kid.finish()) {
				rotate_kid.start();
			}
		}

		rotate_kid.tick(diffSeconds);

		if ((now - last_gravity_time).asMilliseconds() > 1.0f) {
			kid.addForce(glm::vec3(0, -9.8, 0) * kid.getMass());
			ball.addForce(glm::vec3(0, -9.8, 0)* mass);

			if (jumping && kid.getPosition().y == 0) {

				kid.addForce(glm::vec3(0, 15000, 0));
				jumping = false;
			}
			//std::cout << kid.getPosition() << "\n";
			kid.tick((now - last_gravity_time).asSeconds());
			ball.tick((now - last_gravity_time).asSeconds());
			last_gravity_time = now;
		}

		auto kid_pos = kid.getPosition();
		if (kid_pos.y <= 0.0005) {
			kid_pos.y = 0;
			kid.setPosition(kid_pos);
			auto kid_vel = kid.getVelocity();
			kid_vel.y = 0.0;
			kid.setVelocity(kid_vel);
		}

		// character collides wall 
		Circle kid_circle = { {kid_pos.x, kid_pos.z}, 0.5f };
		for (auto& wall : walls) {
			checkCollision(kid_circle, wall);
		}
		kid_pos.x = kid_circle.center.x;
		kid_pos.z = kid_circle.center.y;
		kid.setPosition(kid_pos);

		// ball collides wall
		auto ball_pos = ball.getPosition();
		Circle ball_circle = { {ball_pos.x, ball_pos.z}, 0.5f };

		if (glm::length(ball_circle.center - kid_circle.center) < ball_circle.radius + kid_circle.radius) {
			if (!dung) {
				auto a = desired_direction;
				a.y = 0.3;
				a = glm::normalize(a);
				ball.addForce(a * 200.0f);
				dung = true;
			}
		}
		else {
			dung = false;
		}
		
		//goalkeeper and ball
		auto goalkeeper_pos = goalkeeper.getPosition();
		Circle goalkeeper_circle = { {goalkeeper_pos.x, goalkeeper_pos.z}, 0.5f };
		if (glm::length(ball_circle.center - goalkeeper_circle.center) < ball_circle.radius + goalkeeper_circle.radius) {
			if (!dung) {
				auto a = -ball.getVelocity();
				a.y = 0.3;
				a = glm::normalize(a);
				ball.addForce(a * 200.0f);
				dung = true;
			}
		}
		else {
			dung = false;
		}

		for (auto& wall : walls) {
			if (isCollideBallWall(ball_circle, wall)) {
				auto normal = glm::vec3(wall.normal.x, 0, wall.normal.y);
			
				auto v = glm::normalize(ball.getVelocity());
				auto n = glm::normalize(normal);
				auto r = v - 2.0f * n * glm::dot(n, v);
				r = 0.7f * glm::length(ball.getVelocity()) * r;
				ball.setVelocity(r);
			}
			checkCollision(ball_circle, wall);
		}
		ball_pos.x = ball_circle.center.x;
		ball_pos.z = ball_circle.center.y;
		ball.setPosition(ball_pos);

		//std::cout << ball.getPosition()<<"\n";
		//std::cout << kid.getPosition() << std::endl;
		// skeletal animator

		//std::cout << ground.getPosition() << "\n";

		coach_animator.UpdateAnimation(diffSeconds);
		auto coach_transforms = coach_animator.GetFinalBoneMatrices();

		goalkeeper_animator.UpdateAnimation(diffSeconds);
		auto goalkeeper_transforms = goalkeeper_animator.GetFinalBoneMatrices();

		// render to create depth map (shadow map)
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float near_plane = 0.1f;
		float far_plane = 100.0f;
		//glm::mat4 shadowProj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
		std::vector<glm::mat4> shadowTransforms;
		auto lightPos = light_cube.getPosition();
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		shadow_shader.activate();
		for (unsigned int i = 0; i < 6; ++i)
			shadow_shader.setUniform("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
		shadow_shader.setUniform("far_plane", far_plane);
		shadow_shader.setUniform("lightPos", lightPos);

		renderSkeletal(window, shadow_shader, kid, kid_transforms);
		renderSkeletal(window, shadow_shader, coach, coach_transforms);
		renderSkeletal(window, shadow_shader, goalkeeper, goalkeeper_transforms);

		ground.render(window, shadow_shader);
		ceiling.render(window, shadow_shader);
		ball.render(window, shadow_shader);
		goal.render(window, shadow_shader);

		for (auto& wall : walls) {
			wall.wall_object.render(window, shadow_shader);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// main objects render 
		glViewport(0, 0, window.getSize().x, window.getSize().y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		skeletal_shader.activate();
		skeletal_shader.setUniform("lightPos", light_cube.getPosition());
		skeletal_shader.setUniform("far_plane", far_plane);

		// there are 3 textures for base texture(diffuse map), normal map, specular map, so use GL_TEXTURE0 + 4 to avoid those 3
		// but in this code, we can set GL_TEXTURE0 + 0, still working (maybe b/c set uniform right after binding)
		glActiveTexture(GL_TEXTURE0 + 4);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
		skeletal_shader.setUniform("depthMap", 4);

		renderSkeletal(window, skeletal_shader, kid, kid_transforms);
		renderSkeletal(window, skeletal_shader, coach, coach_transforms);
		renderSkeletal(window, skeletal_shader, goalkeeper, goalkeeper_transforms);

		ground.render(window, skeletal_shader);
		ceiling.render(window, skeletal_shader);
		ball.render(window, skeletal_shader);
		goal.render(window, skeletal_shader);

		for (auto& wall : walls) {
			wall.wall_object.render(window, skeletal_shader);
		}

		// light cube render
		light_shader.activate();
		light_shader.setUniform("view", camera);
		for (auto& o : light_scene.objects) {
			o.render(window, light_shader);
		}

		glDepthFunc(GL_LESS);

		window.display();
	}

	return 0;
}



#pragma once

#include "gl_errors.hpp"
#include "load_save_png.hpp"

#include <SDL.h>
#include <glm/glm.hpp>

#include <memory>

struct Mode : std::enable_shared_from_this< Mode > {
	virtual ~Mode() { }

	//handle_event is called when new mouse or keyboard events are received:
	// (note that this might be many times per frame or never)
	//The function should return 'true' if it handled the event.
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) { return false; }

	//update is called at the start of a new frame, after events are handled:
	// 'elapsed' is time in seconds since the last call to 'update'
	virtual void update(float elapsed) { }

	//draw is called after update:
	virtual void draw(glm::uvec2 const &drawable_size) = 0;

	// putting this here to be lazy and not annoying:
	static GLuint load_texture(std::string const &filename) {
		glm::uvec2 size;
		std::vector< glm::u8vec4 > data;
		load_png(filename, &size, &data, LowerLeftOrigin);

		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		GL_ERRORS();

		return tex;
	};

	//Mode::current is the Mode to which events are dispatched.
	// use 'set_current' to change the current Mode (e.g., to switch to a menu)
	static std::shared_ptr< Mode > current;
	static void set_current(std::shared_ptr< Mode > const &);
};


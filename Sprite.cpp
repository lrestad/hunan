#include "Sprite.hpp"

Sprite::Sprite(std::string path) {
    // Load the png
    load_png(path, &size, &data, LowerLeftOrigin);

    // From DrawLines.cpp
    { //set up vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.
	}

    { //vertex array mapping buffer for color_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program->Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Sprite::Vertex), //stride
			(GLbyte *)0 + offsetof(Sprite::Vertex, Position) //offset
		);
		glEnableVertexAttribArray(color_texture_program->Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program->Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Sprite::Vertex), //stride
			(GLbyte *)0 + offsetof(Sprite::Vertex, Color) //offset
		);
		glEnableVertexAttribArray(color_texture_program->Color_vec4);

        glVertexAttribPointer(
			color_texture_program->TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Sprite::Vertex), //stride
			(GLbyte *)0 + offsetof(Sprite::Vertex, TexCoord) //offset
		);
		glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);
	}

    GL_ERRORS(); //PARANOIA: make sure nothing strange happened during setup

    // Followed https://learnopengl.com/Getting-started/Textures for help with setting up textures
    {//assign the png as the texture
        
        glGenTextures(1, &png_texture);
        glBindTexture(GL_TEXTURE_2D, png_texture);

        // Fill with png texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

        // Setting wrapping & filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Generate a mipmap
        glGenerateMipmap(GL_TEXTURE_2D);

        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GL_ERRORS(); //PARANOIA part 2: make sure nothing strange happened during setup
}

Sprite::~Sprite() {
    glDeleteBuffers(1, &vertex_buffer);
    glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
    glDeleteTextures(1, &png_texture);

    vertex_buffer = 0;
    vertex_buffer_for_color_texture_program = 0;
    png_texture = 0;
}

void Sprite::set_drawable_size(const glm::uvec2 &new_drawable_size) {
    this->drawable_size = new_drawable_size;
}

void Sprite::draw(glm::vec2 center_pos, float scale, glm::u8vec4 hue) {
    std::vector< Vertex > vertices;

    // References https://learnopengl.com/Getting-started/Hello-Triangle
    // Function to help with grabbing the vertices needed to create a rectangle
    auto create_rect = [&vertices](glm::vec2 const &center, glm::vec2 const &dim, glm::u8vec4 const &color) {
        // First triangle
        vertices.emplace_back(glm::vec3(center.x-dim.x/2, center.y-dim.y/2, 0.0f), color, glm::vec2(0.0f, 0.0f)); // Bottom left
        vertices.emplace_back(glm::vec3(center.x+dim.x/2, center.y-dim.y/2, 0.0f), color, glm::vec2(1.0f, 0.0f)); // Bottom right
        vertices.emplace_back(glm::vec3(center.x+dim.x/2, center.y+dim.y/2, 0.0f), color, glm::vec2(1.0f, 1.0f)); // Top right

        // Second triangle
        vertices.emplace_back(glm::vec3(center.x-dim.x/2, center.y-dim.y/2, 0.0f), color, glm::vec2(0.0f, 0.0f)); // Bottom left
        vertices.emplace_back(glm::vec3(center.x+dim.x/2, center.y+dim.y/2, 0.0f), color, glm::vec2(1.0f, 1.0f)); // Top right
        vertices.emplace_back(glm::vec3(center.x-dim.x/2, center.y+dim.y/2, 0.0f), color, glm::vec2(0.0f, 1.0f)); // Top left
    };

    create_rect(center_pos, glm::vec2(size.x * scale, size.y * scale), hue);

    // OpenGL state
	// ------------
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST); 

    // Push vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(color_texture_program->program);
    glm::mat4 projection = glm::ortho(0.0f, (float)drawable_size.x, 0.0f, (float)drawable_size.y);
    glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(vertex_buffer_for_color_texture_program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, png_texture);
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(0);
    glUseProgram(0);

    GL_ERRORS();
}


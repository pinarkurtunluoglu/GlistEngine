/*
 * gTexture.cpp
 *
 *  Created on: May 10, 2020
 *      Author: noyan
 */

#include "gTexture.h"
#include <iostream>
#if defined(WIN32) || defined(LINUX)
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#if defined(APPLE)
#include <OpenGL/gl.h>
#include <GL/glew.h>
#include <OpenGL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#endif
#include "gPlane.h"
//#ifndef STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#endif
#include "stb/stb_image.h"

const int gTexture::TEXTURETYPE_DIFFUSE = 0;
const int gTexture::TEXTURETYPE_SPECULAR = 1;
const int gTexture::TEXTURETYPE_NORMAL = 2;
const int gTexture::TEXTURETYPE_HEIGHT = 3;


gTexture::gTexture() {
	id = GL_NONE;
	format = GL_RGBA;
	texturetype[0] = "texture_diffuse";
	texturetype[1] = "texture_specular";
	texturetype[2] = "texture_normal";
	texturetype[3] = "texture_height";
	type = TEXTURETYPE_DIFFUSE;
	path = "";
	width = 0;
	height = 0;
	bsubpartdrawn = false;
	ismutable = false;
	isfbo = false;
	setupRenderData();
}

gTexture::gTexture(int w, int h, int format, bool isFbo) {
	id = GL_NONE;
	this->format = format;
	texturetype[0] = "texture_diffuse";
	texturetype[1] = "texture_specular";
	texturetype[2] = "texture_normal";
	texturetype[3] = "texture_height";
	type = TEXTURETYPE_DIFFUSE;
	path = "";
	width = w;
	height = h;
	bsubpartdrawn = false;
	ismutable = false;
	isfbo = isFbo;
    glGenTextures(1, &id);
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); // TODO: BEFORE SHADOWMAP GL_REPEAT
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // TODO: BEFORE SHADOWMAP GL_REPEAT
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	setupRenderData();
}

gTexture::~gTexture() {
	if (ismutable) delete data;
}

unsigned int gTexture::load(std::string fullPath) {
	fullpath = fullPath;
	directory = getDirName(fullpath);
	path = getFileName(fullpath);

    glGenTextures(1, &id);

    data = stbi_load(fullpath.c_str(), &width, &height, &componentnum, 0);
    setData(data, false);

	setupRenderData();
    return id;
}

unsigned int gTexture::loadTexture(std::string texturePath) {
	return load(gGetTexturesDir() + texturePath);
}

void gTexture::setData(unsigned char* textureData, bool isMutable) {
	ismutable = isMutable;
	data = textureData;
    if (data) {
        if (componentnum == 1)
            format = GL_RED;
        else if (componentnum == 3)
            format = GL_RGB;
        else if (componentnum == 4)
            format = GL_RGBA;

        bind();
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexImage2D(GL_TEXTURE_2D, 0, 0, getWidth(), getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (!ismutable) stbi_image_free(data);
        unbind();
    } else {
        std::cout << "Texture failed to load at path: " << fullpath << std::endl;
        stbi_image_free(data);
    }
}

unsigned char* gTexture::getData() {
	return data;
}

bool gTexture::isMutable() {
	return ismutable;
}

void gTexture::bind() {
	glBindTexture(GL_TEXTURE_2D, id);
}

void gTexture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned int gTexture::getId() {
	return id;
}

unsigned int gTexture::getFormat() {
	return format;
}

void gTexture::setType(int textureType) {
	type = textureType;
}

int gTexture::getType() {
	return type;
}

std::string gTexture::getTypeName() {
	return texturetype[type];
}

std::string gTexture::getTypeName(int textureType) {
	return texturetype[textureType];
}

std::string gTexture::getFilename() {
	return path;
}

std::string gTexture::getDir() {
	return directory;
}

std::string gTexture::getFullPath() {
	return fullpath;
}

int gTexture::getWidth() {
	return width;
}

int gTexture::getHeight() {
	return height;
}

int gTexture::getComponentNum() {
	return componentnum;
}

void gTexture::draw(int x, int y) {
	draw(x, y, width, height);
}

void gTexture::draw(int x, int y, int w, int h) {
	beginDraw();
	imagematrix = glm::translate(imagematrix, glm::vec3(x, y, 0.0f));  // first translate (transformations are: scale happens first, then rotation, and then final translation happens; reversed order)
	imagematrix = glm::scale(imagematrix, glm::vec3(w, h, 1.0f));
	endDraw();
}

void gTexture::draw(int x, int y, int w, int h, float rotate) {
	draw(glm::vec2(x, y), glm::vec2(w, h), rotate);
}

void gTexture::draw(glm::vec2 position, glm::vec2 size, float rotate) {
	beginDraw();
	imagematrix = glm::translate(imagematrix, glm::vec3(position, 0.0f));  // first translate (transformations are: scale happens first, then rotation, and then final translation happens; reversed order)

	imagematrix = glm::translate(imagematrix, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f));
	imagematrix = glm::rotate(imagematrix, glm::radians(rotate), glm::vec3(0.0f, 0.0f, 1.0f));
	imagematrix = glm::translate(imagematrix, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f));

	imagematrix = glm::scale(imagematrix, glm::vec3(size.x, size.y, 1.0f));
	endDraw();
}

void gTexture::drawSub(int x, int y, int sx, int sy, int sw, int sh) {
	drawSub(x, y, sw, sh, sx, sy, sw, sh);
}

void gTexture::drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh) {
	setupRenderData(sx, sy, sw, sh);
	bsubpartdrawn = true;
	draw(x, y, w, h);
}

void gTexture::drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh, float rotate) {
	drawSub(glm::vec2(x, y), glm::vec2(w, h), glm::vec2(sx, sy), glm::vec2(sw, sh), rotate);
}

void gTexture::drawSub(const gRect& src, const gRect& dst, float rotate) {
	drawSub(dst.left(), dst.top(), dst.getWidth(), dst.getHeight(), src.left(), src.top(), src.getWidth(), src.getHeight(), rotate);
}

void gTexture::drawSub(glm::vec2 pos, glm::vec2 size, glm::vec2 subpos, glm::vec2 subsize, float rotate) {
	setupRenderData(subpos.x, subpos.y, subsize.x, subsize.y);
	bsubpartdrawn = true;
	draw(pos, size, rotate);
}

void gTexture::beginDraw() {
	renderer->getImageShader()->use();
	imagematrix = glm::mat4(1.0f);
	renderer->setProjectionMatrix2d(glm::ortho(0.0f, (float)renderer->getWidth(), (float)renderer->getHeight(), 0.0f, -1.0f, 1.0f));
}

void gTexture::endDraw() {
	renderer->getImageShader()->setMat4("projection", renderer->getProjectionMatrix2d());
	renderer->getImageShader()->setMat4("model", imagematrix);
	renderer->getImageShader()->setVec3("spriteColor", glm::vec3(renderer->getColor()->r, renderer->getColor()->g, renderer->getColor()->b));
	renderer->getImageShader()->setInt("image", 0);

	glActiveTexture(GL_TEXTURE0);
    bind();
    if (format == GL_RGBA) {
        glEnable(GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    if (format == GL_RGBA) glDisable(GL_BLEND);
    unbind();
    if(bsubpartdrawn) {	setupRenderData(); }
}

void gTexture::setupRenderData() {
	setupRenderData(0, 0, width, height);
	bsubpartdrawn = false;
}

void gTexture::setupRenderData(int sx, int sy, int sw, int sh) {
	glDeleteBuffers(1, &quadVBO);
	glDeleteVertexArrays(1, &quadVAO);
    float vertices[] = {
        // pos      // tex
        0.0f, 1.0f, (float)sx / width, (float)(sy + sh) / height,
        1.0f, 0.0f, (float)(sx + sw) / width, (float)sy / height,
        0.0f, 0.0f, (float)sx / width, (float)sy / height,

        0.0f, 1.0f, (float)sx / width, (float)(sy + sh) / height,
        1.0f, 1.0f, (float)(sx + sw) / width, (float)(sy + sh) / height,
        1.0f, 0.0f, (float)(sx + sw) / width, (float)sy / height
    };
    float vertices2[] = {
        // pos      // tex
        0.0f, 1.0f, (float)sx / width, (float)(sy) / height,
        1.0f, 0.0f, (float)(sx + sw) / width, (float)(sy + sh) / height,
        0.0f, 0.0f, (float)sx / width, (float)(sy + sh) / height,

        0.0f, 1.0f, (float)sx / width, (float)(sy) / height,
        1.0f, 1.0f, (float)(sx + sw) / width, (float)(sy) / height,
        1.0f, 0.0f, (float)(sx + sw) / width, (float)(sy + sh) / height
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    if (isfbo) glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
    else glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(quadVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

std::string gTexture::getDirName(const std::string& fname) {
     size_t pos = fname.find_last_of("\\/");
     return (std::string::npos == pos)
         ? ""
         : fname.substr(0, pos);
}

std::string gTexture::getFileName(const std::string& fname) {
     size_t pos = fname.find_last_of("\\/");
     return (std::string::npos == pos)
         ? ""
         : fname.substr(pos + 1, fname.size());
}


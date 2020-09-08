//
// Created by jared on 9/2/20.
//

#ifndef TINYRENDER_SSAO_H
#define TINYRENDER_SSAO_H

/*
	This file is part of TinyRender, an educative rendering system.
	Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
	Derek Nowrouzezahrai, McGill University.
*/
#pragma once

#include "core/renderpass.h"
#include "tiny_obj_loader.h"

TR_NAMESPACE_BEGIN

/**
 * SSAO (Screen Space Ambient Occlusion) renderpass.
 */
    struct SSAOPass : RenderPass {
        GLuint gbuffer;
        GLuint texturePosition;
        GLuint textureNormal;
        GLuint textureDepth;
        GLuint geometryShader;

        GLuint quadVBO;
        GLuint quadVAO;
        GLuint shaderSSAO;

        GLuint uvAttrib{ 1 };

        explicit SSAOPass(const Scene& scene) : RenderPass(scene) { }

        virtual bool init(const Config& config) override {
            RenderPass::init(config);

            // GOAL: three passes deferred rendering:
            //     1. draw to G-buffer
            //     2. regular shading
            //     3. screen quad post process
            //
            // 0. glDrawBuffers(2, attachments); connects the outputs of fragment shader to corresponding textures.
            //
            // 1. build VBO and VAO for every shape
            //        VBO----(store data with pointer and offset)
            //        gen-bind VAO, gen-bind VBO, buffer data
            //
            //        glGenVertexArrays(1, &obj.vao);
            //        glBindVertexArray(obj.vao);
            //        glGenBuffers(1, &obj.vbo);
            //        glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);
            //        glBufferData(GL_ARRAY_BUFFER,
            //                 sizeof(GLfloat) * obj.nVerts * N_ATTR_PER_VERT,
            //                 (GLvoid*) (&obj.vertices[0]),
            //                 GL_STATIC_DRAW);
            //
            //
            // 2. Define vertices attributes
            //    if the layout index in shader is same as the first argument in glVertexAttribPointer
            //    then the related buffer data will be passed into shader variables with corresponding location automatically
            //
            //        VAO----set the input layout to vertex shader
            //        bind VAO, bind VBO, glEnableVertexAttribArray, glVertexAttribPointer
            //
            //        glBindVertexArray(objects[objectIdx].vao);
            //        glBindBuffer(GL_ARRAY_BUFFER, objects[objectIdx].vbo);
            //        glEnableVertexAttribArray(posAttrib);
            //        glEnableVertexAttribArray(normalAttrib);
            //        glVertexAttribPointer(posAttrib,
            //                          3,
            //                          GL_FLOAT,
            //                          GL_FALSE,
            //                          sizeof(GLfloat) * N_ATTR_PER_VERT,
            //                          (GLvoid*) (0 * sizeof(GLfloat)));
            //        glVertexAttribPointer(normalAttrib,
            //                          3,
            //                          GL_FLOAT,
            //                          GL_FALSE,
            //                          sizeof(GLfloat) * N_ATTR_PER_VERT,
            //                          (GLvoid*) (3 * sizeof(GLfloat)));
            //
            //        glBindVertexArray(0);
            //        glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Create vertex buffers
            const auto& shapes = scene.worldData.shapes;
            objects.resize(shapes.size());
            for (size_t i = 0; i < shapes.size(); i++) {
                const tinyobj::shape_t& s = shapes[i];
                GLObject& obj = objects[i];
                buildVBO(i);
                buildVAO(i);
            }

            // 3. Create shader to build GBuffer
            //     3.1 geometry.vs----Unprojected M,V transformed Position and Normal
            //     3.2 geometry.fs----store geometry info into screen texture pixels
            //     compileShader, compileProgram
            //     3.3 gen texture:
            //         gen-active-bind-TexImage2D-min max warps/t-unbind
            //     3.4 gen Gbuffer
            //         glGenFramebuffers-bind-glFramebufferTexture2D
            //     3.4 assign target to draw buffer to texture
            //         glDrawBuffers( to attachments )
            //         unbind
            GLuint vs = compileShader("geometry.vs", GL_VERTEX_SHADER);
            GLuint fs = compileShader("geometry.fs", GL_FRAGMENT_SHADER);
            geometryShader = compileProgram(vs, fs);
            glDeleteShader(vs);
            glDeleteShader(fs);

            // Create position texture (GBuffer)
            glGenTextures(1, &texturePosition);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturePosition);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, config.width, config.height, 0, GL_RGB, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            // Create normal texture (GBuffer)
            glGenTextures(1, &textureNormal);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textureNormal);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, config.width, config.height, 0, GL_RGB, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            // Create depth texture (GBuffer)
            glGenTextures(1, &textureDepth);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, textureDepth);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, config.width, config.height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            // Create the GBuffer
            glGenFramebuffers(1, &gbuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texturePosition, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textureNormal, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureDepth, 0);
            unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, attachments);
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) return false;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // 4. Create quad VBO and VAO
            //    gen-bind VAO, gen-bind-bufferdata VBO
            //    enable shader input
            //    assign vertex data to shader

            // Create quad VBO
            glGenVertexArrays(1, &quadVAO);
            glBindVertexArray(quadVAO);
            GLfloat quadVertices[] = {
                    //x, y     u, v
                    -1, 1, 0, 1,
                    -1, -1, 0, 0,
                    1, -1, 1, 0,
                    -1, 1, 0, 1,
                    1, -1, 1, 0,
                    1, 1, 1, 1
            };
            glGenBuffers(1, &quadVBO);
            glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
            glBufferData(GL_ARRAY_BUFFER, 4 * 6 * sizeof(GLfloat), (GLvoid*)(&quadVertices[0]), GL_STATIC_DRAW);
            glEnableVertexAttribArray(posAttrib);
            glEnableVertexAttribArray(uvAttrib);
            glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (GLvoid*)(0 * sizeof(GLfloat)));
            glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (GLvoid*)(2 * sizeof(GLfloat)));


            // 5. create Postprocess (SSAO) shader
            //    compileShader-compileProgram-glDeleteShader


            // Create SSAO shader
            {
                GLuint vs = compileShader("quad.vs", GL_VERTEX_SHADER);
                GLuint fs = compileShader("ssao.fs", GL_FRAGMENT_SHADER);
                shaderSSAO = compileProgram(vs, fs);
                glDeleteShader(vs);
                glDeleteShader(fs);
            }

            return true;
        }

        virtual void cleanUp() override {
            // Delete GBuffer
            glDeleteTextures(1, &texturePosition);
            glDeleteTextures(1, &textureNormal);
            glDeleteTextures(1, &textureDepth);
            glDeleteFramebuffers(1, &gbuffer);
            glDeleteProgram(geometryShader);

            // Delete SSAO shader
            glDeleteBuffers(1, &quadVBO);
            glDeleteVertexArrays(1, &quadVAO);
            glDeleteProgram(shaderSSAO);

            // Delete vertex buffers
            for (size_t i = 0; i < objects.size(); i++) {
                GLObject obj = objects[i];
                glDeleteBuffers(1, &obj.vbo);
                glDeleteVertexArrays(1, &obj.vao);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            RenderPass::cleanUp();
        }

        virtual void render() override {
            // I. Geometry pass (GBuffer)
            // =======================================================================================

            /**
             * 1) Bind the GBuffer.
             */
            // TODO: Implement this

            glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            // Update camera
            glm::mat4 model, view, projection;
            camera.Update();
            camera.GetMatricies(projection, view, model);

            /**
             * 1) Use the shader for the geometry pass.
             * 2) Pass the necessary uniforms.
             * 3) Bind vertex array of current object.
             * 4) Draw its triangles.
             * 5) Unbind the vertex array.
             */
            // TODO: Implement this

            //1.
            glUseProgram(geometryShader);

            //2. Pass uniforms
            GLuint modelMatUniform = GLuint(glGetUniformLocation(geometryShader, "model"));
            GLuint viewMatUniform = GLuint(glGetUniformLocation(geometryShader, "view"));
            GLuint projectionMatUniform = GLuint(glGetUniformLocation(geometryShader, "projection"));

            glUniformMatrix4fv(modelMatUniform, 1, GL_FALSE, &(modelMat[0][0]));
            glUniformMatrix4fv(viewMatUniform, 1, GL_FALSE, &(view[0][0]));
            glUniformMatrix4fv(projectionMatUniform, 1, GL_FALSE, &(projection[0][0]));


            for (size_t i = 0; i < objects.size(); i++) {
                GLObject obj = objects[i];
                //3.
                glBindVertexArray(obj.vao);
                //4.
                glDrawArrays(GL_TRIANGLES, 0, obj.nVerts);
                //5.
                glBindVertexArray(0);

            }

            // II. SSAO pass
            // =======================================================================================
            /**
             * 1) Bind the screen buffer (postprocess_fboScreen).
             */
            // TODO: Implement this
            glBindFramebuffer(GL_FRAMEBUFFER, postprocess_fboScreen);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);

            /**
             * 1) Use the shader for the SSAO pass.
             * 2) Pass the necessary uniforms.
             * 3) Bind the textures for position and normal from the GBuffer.
             * 4) Bind vertex array of the quad representing the screen texture.
             * 5) Draw the quad.
             * 6) Unbind the vertex array.
             * 7) Unbind the textures.
             */
            // TODO: Implement this

            //1.
            glUseProgram(shaderSSAO);
            //2.
            GLuint projectionMatUniformSSAO = GLuint(glGetUniformLocation(shaderSSAO, "projection"));

            glUniformMatrix4fv(projectionMatUniformSSAO, 1, GL_FALSE, &(projection[0][0]));

            //3.
            GLuint texturePos = GLuint(glGetUniformLocation(shaderSSAO, "texturePosition"));
            GLuint textureNorm = GLuint(glGetUniformLocation(shaderSSAO, "textureNormal"));

            glUniform1i(texturePos, 0);
            glUniform1i(textureNorm, 1);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturePosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textureNormal);

            //4.
            glBindVertexArray(quadVAO);

            //5.
            glDrawArrays(GL_TRIANGLES, 0, 6);
            //6.
            glBindVertexArray(0);
            //7.
            glBindTexture(GL_TEXTURE_2D, 0);

            RenderPass::render();
        }

    };

TR_NAMESPACE_END
#endif //TINYRENDER_SSAO_H

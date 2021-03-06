/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#include <core/core.h>
#include <core/accel.h>
#include <core/renderer.h>
#include <GL/glew.h>

#ifdef __APPLE__
#include "SDL.h"
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#include <GL/gl.h>
#include "SDL.h"
#else
#include <GL/gl.h>
#include "SDL2/SDL.h"
#endif
#endif

#include <bsdfs/diffuse.h>

#include <integrators/normal.h>
#include <renderpasses/normal.h>

#include <integrators/simple.h>
#include <renderpasses/simple.h>
#include <bsdfs/phong.h>

#include <integrators/ao.h>
#include <integrators/ro.h>

#include <integrators/direct.h>
#include <integrators/polygonal.h>
#include <renderpasses/polygonal.h>

#include <integrators/path.h>
#include <renderpasses/gi.h>
#include <bsdfs/mixture.h>

#include <renderpasses/ssao.h>


TR_NAMESPACE_BEGIN

Renderer::Renderer(const Config& config) : scene(config) { }

bool Renderer::init(const bool isRealTime, bool nogui) {
    realTime = isRealTime;
    this->nogui = nogui;
    realTimeCameraFree = false;

    if (!scene.load(isRealTime)) return false;

    if (realTime) {
        if (scene.config.renderpass == ENormalRenderPass) {
            renderpass = std::unique_ptr<NormalPass>(new NormalPass(scene));
        }
        else if (scene.config.renderpass == EDirectRenderPass) {
            renderpass = std::unique_ptr<SimplePass>(new SimplePass(scene));
        }
			// Commented out to prevent compile errors now that the code referenced to this has been removed. 
			// TODO: Remove this entirely? 
        /*else if (scene.config.renderpass == ESSAORenderPass) {
            renderpass = std::unique_ptr<SSAOPass>(new SSAOPass(scene));
        }*/

		else if (scene.config.renderpass == EPolygonalRenderPass) {
			renderpass = std::unique_ptr<PolygonalPass>(new PolygonalPass(scene));
		}

        else if (scene.config.renderpass == EGIRenderPass) {
            renderpass = std::unique_ptr<GIPass>(new GIPass(scene));
        }

        else if (scene.config.renderpass == ESSAORenderPass) {
            renderpass = std::unique_ptr<SSAOPass>(new SSAOPass(scene));
        }
        else {
            throw std::runtime_error("Invalid renderpass type");
        }

        bool succ = renderpass.get()->initOpenGL(scene.config.width, scene.config.height);
        if (!succ) return false;

        return renderpass->init(scene.config);
    } else {
        if (scene.config.integrator == ENormalIntegrator) {
            integrator = std::unique_ptr<NormalIntegrator>(new NormalIntegrator(scene));
        }
        else if (scene.config.integrator == EAOIntegrator) {
            integrator = std::unique_ptr<AOIntegrator>(new AOIntegrator(scene));
        } else if (scene.config.integrator == EROIntegrator) {
            integrator = std::unique_ptr<ROIntegrator>(new ROIntegrator(scene));
        }
        else if (scene.config.integrator == ESimpleIntegrator) {
            integrator = std::unique_ptr<SimpleIntegrator>(new SimpleIntegrator(scene));
        }
        else if (scene.config.integrator == EDirectIntegrator) {
            integrator = std::unique_ptr<DirectIntegrator>(new DirectIntegrator(scene));
        }
        else if (scene.config.integrator == EPolygonalIntegrator) {
            integrator = std::unique_ptr<PolygonalIntegrator>(new PolygonalIntegrator(scene));
        }
        else if (scene.config.integrator == EPathTracerIntegrator) {
            integrator = std::unique_ptr<PathTracerIntegrator>(new PathTracerIntegrator(scene));
        }
        else {
            throw std::runtime_error("Invalid integrator type");
        }

        return integrator->init();
    }
}

void Renderer::render() {
    if (realTime) {

		if (nogui) return renderpass->render(); // Simply render a single image if --nogui is specified

        /**
         * 1) Detect and handle the quit event.
         * 2) Call the render function using renderpass->render().
         * 3) Output the rendered image into the GUI window using SDL_GL_SwapWindow(renderpass->window).
         */
        // TODO(A1): Implement this
        // Model matrix: model -> world
        // View : world -> camera
        // Projection : camera -> CVV (cube)
        // divide w : CVV -> NDC      (-1,1)
        // viewport : NDC -> screen

        // OGL realtime pipeline:
        // 1. initOpenGL
        // 2. initWindow
        // 3. Create shader (vs,fs)
        // 4. Create uniforms (mvp normal)
        // 5. Create vertex buffers (VAO VBO) FOR each object

        // A Vertex Buffer Object is an OpenGL data buffer (using GPU's memory) that is typically
        // used to store vertex attributes, such as the position, normal vector and color of each vertex
        // (of each triangle and of each object) in your scene.
        // The vertex positions are typically prescribed in object space. A Vertex Array Object allows
        // us to define the memory layout of the attributes in the VBO.
        // One VBO is assigned to each VAO.
        // You can query the ID assigned to the VBO and VAO of each object using their GLObject variables.

        // VAO: bunch of DATA for a pointer per object
        // VBO: layout for indexing VAO

        // render stage :

        // 6. enable shader created and compiled before
        // 7. Update camera
        // 8. Pass uniforms
        // 9. Draw
        //     1) Bind vertex array of current object.
        //     2) Draw its triangles.
        //     3) Bind vertex array to 0.

        // 10. clean up : delete VAO and VBO for each object

        // gl_Position : default NDC coordinates, only vertices are between [-1,1] can be draw.

        while(true)
        {
            SDL_Event event;
            SDL_PollEvent( &event );
            if ( event.type == SDL_QUIT )
            {
                break;
            }
            else
                renderpass->updateCamera( event );
            renderpass->render();
            SDL_GL_SwapWindow( renderpass->window );
        }
    } else {
        /**
         * 1) Calculate the camera perspective, the camera-to-world transformation matrix and the aspect ratio.
         * 2) Clear integral RGB buffer.
         * 3) Loop over all pixels on the image plane.
				- For the outermost loop, you should use `ThreadPool::ParallelFor(...)` for faster execution *in release mode only*.
				  You can use the `#ifdef NDEBUG` macro to detect when the code is running in release mode.
         * 4) Generate `spp` number of rays randomly through each pixel.
         * 5) Splat their contribution onto the image plane.
		 * HINT: All configuration options and camera properties can be found in object `scene.config`.
         */
        // TODO(A1): Implement this

        // camera parameters
        // coordinate origin point position first, than perform basis transformation(camera uvw basis at origin point)
        // lookat view matrix: basis transformation matrix * translation matrix ( * camera space position), world to camera

        // Do not require persipective transformation in ray tracing.
        // sphere to plane is not a conformal mapping, which causes stretching

        // at, up is directions! not points
        glm::vec3 at = scene.config.camera.at;
        glm::vec3 eye = scene.config.camera.o;
        glm::vec3 up = scene.config.camera.up;
        // x point right, y point up, z point out
        // negative direction -> inverse translation
        // orthogonal transpose -> inverse basis transformation
        // very important! translation is meaningless to vector, which implicit homogeneous w coordinate is zero
        // it also means we only require inverse basis transformation to transform a vector from camera to world!!!
        // so using transpose of lookup matrix instead of inverse can save computations.

        // !!!!! OPENGL uses ( eye - center ) as camera direction vector!!!
        // so objects only can be seen when the z coordinates in view space is negative !!!!!
        glm::mat4 viewMatrix = glm::lookAt( eye, at, up );
        // clear RGB buffer
        integrator->rgb->clear();

        // width of view plane
        float fov = scene.config.camera.fov;
        float distance = 1.0;
        float height = tanf( fov / 360.0 * M_PI ) * distance * 2;
        float width = scene.config.width * 1.0 / scene.config.height * height;
        // donot know why using aspectRatio * fovScale as width

        //sampler
        int divideNum = 4;
        int sqrtDivideNum = 2;
        Sampler* sampler;

#ifdef NDEBUG // Running in release mode - Use threads, start, end, *func
        ThreadPool::ParallelFor(0, scene.config.height, [&] (int y)
        {
#else   // Running in debug mode - Don't use threads
        ThreadPool::SequentialFor(0, scene.config.height, [&](int y)
        {
#endif
            // thread safe random
            sampler = new Sampler( 47567 + y );
            // Your code here
            // for each pixel, y is paralleled
            for (size_t x = 0; x < scene.config.width; x++)
            {
                glm::fvec3 color(0, 0, 0);
                // compute pixel center pos, start from left top
                float xCenterPos = 0 +  width * (x - scene.config.width / 2.0 + 0.5) / scene.config.width;
                float yCenterPos = 0 +  height * (scene.config.height - y - scene.config.height / 2.0 + 0.5) / scene.config.height;

                for (int i = 0; i < scene.config.spp - 1; i++)
                {
                    float xoffset = (i % sqrtDivideNum - sqrtDivideNum / 2.0 + 0.5) / sqrtDivideNum * width / scene.config.width;
                    float yoffset = (i / sqrtDivideNum % sqrtDivideNum - sqrtDivideNum / 2.0 + 0.5) / sqrtDivideNum * height / scene.config.height;
                    glm::fvec2 r2 = sampler->next2D();
                    float xjitter = (r2[0] - 0.5) / sqrtDivideNum * width / scene.config.width;
                    float yjitter = (r2[1] - 0.5) / sqrtDivideNum * height / scene.config.height;

                    float xSamplePos = xCenterPos + xoffset + xjitter;
                    float ySamplePos = yCenterPos + yoffset + yjitter;

                    glm::fvec3 rayDirection = normalize( glm::fvec3( transpose(viewMatrix) * glm::fvec4( xSamplePos, ySamplePos, -distance, 0 ) ) );
                    Ray ray = Ray( scene.config.camera.o, rayDirection );
                    color += integrator->render( ray, *sampler ) / scene.config.spp;
                }
                glm::fvec3 rayDirection = normalize( glm::fvec3( transpose(viewMatrix) * glm::fvec4( xCenterPos, yCenterPos, -distance, 0 ) ) );
                Ray ray = Ray( scene.config.camera.o, rayDirection );
                color += integrator->render( ray, *sampler ) / scene.config.spp;
                integrator->rgb->data[ y * scene.config.width + x ] = color;
            }
        });
    }
}

/**
 * Post-rendering step.
 */
void Renderer::cleanUp() {
    if (realTime) {
        renderpass->cleanUp();
    } else {
        integrator->cleanUp();
    }
}

BSDF::BSDF(const WorldData& d, const Config& c, const size_t matID) : worldData(d), config(c) {
    emission = glm::make_vec3(worldData.materials[matID].emission);
}

Scene::Scene(const Config& config) : config(config) { }

bool Scene::load(bool isRealTime) {
    fs::path file(config.objFile);
    bool ret = false;
    std::string err;

    if (!file.is_absolute())
        file = (config.tomlFile.parent_path() / file).make_preferred();

    tinyobj::attrib_t* attrib_ = &worldData.attrib;
    std::vector<tinyobj::shape_t>* shapes_ = &worldData.shapes;
    std::vector<tinyobj::material_t>* materials_ = &worldData.materials;
    std::string* err_ = &err;
    const string filename_ = file.string();
    const string mtl_basedir_ = file.make_preferred().parent_path().string();
    ret = tinyobj::LoadObj(attrib_, shapes_, materials_, err_, filename_.c_str(), mtl_basedir_.c_str(), true);

    if (!err.empty()) { std::cout << "Error: " << err.c_str() << std::endl; }
    if (!ret) {
        std::cout << "Failed to load scene " << config.objFile << " " << std::endl;
        return false;
    }

    // Build list of BSDFs
    bsdfs = std::vector<std::unique_ptr<BSDF>>(worldData.materials.size());
    for (size_t i = 0; i < worldData.materials.size(); i++) {
        if (worldData.materials[i].illum == 7)
            bsdfs[i] = std::unique_ptr<BSDF>(new DiffuseBSDF(worldData, config, i));
        if (worldData.materials[i].illum != 5 && worldData.materials[i].illum != 7 && worldData.materials[i].illum != 8)
            bsdfs[i] = std::unique_ptr<BSDF>(new PhongBSDF(worldData, config, i));
        if (worldData.materials[i].illum == 8)
            bsdfs[i] = std::unique_ptr<BSDF>(new MixtureBSDF(worldData, config, i));
    }

    // Build list of emitters (and print what has been loaded)
    std::string nbShapes = worldData.shapes.size() > 1 ? " shapes" : " shape";
    std::cout << "Found " << worldData.shapes.size() << nbShapes << std::endl;
    worldData.shapesCenter.resize(worldData.shapes.size());
    worldData.shapesAABOX.resize(worldData.shapes.size());

    for (size_t i = 0; i < worldData.shapes.size(); i++) {
        const tinyobj::shape_t& shape = worldData.shapes[i];
        const BSDF* bsdf = bsdfs[shape.mesh.material_ids[0]].get();
        std::cout << "Mesh " << i << ": " << shape.name << " ["
                  << shape.mesh.indices.size() / 3 << " primitives | ";

        if (bsdf->isEmissive()) {
            Distribution1D faceAreaDistribution;
            float shapeArea = getShapeArea(i, faceAreaDistribution);
            emitters.emplace_back(Emitter{i, shapeArea, bsdf->emission, faceAreaDistribution});
            std::cout << "Emitter]" << std::endl;
        } else {
            std::cout << bsdf->toString() << "]" << std::endl;
        }

        // Build world AABB and shape centers
        worldData.shapesCenter[i] = v3f(0.0);
        for (auto idx: shape.mesh.indices) {
            v3f p = {worldData.attrib.vertices[3 * idx.vertex_index + 0],
                     worldData.attrib.vertices[3 * idx.vertex_index + 1],
                     worldData.attrib.vertices[3 * idx.vertex_index + 2]};
            worldData.shapesCenter[i] += p;
            worldData.shapesAABOX[i].expandBy(p);
            aabb.expandBy(p);
        }
        worldData.shapesCenter[i] /= float(shape.mesh.indices.size());
    }

    // Build BVH
    bvh = std::unique_ptr<TinyRender::AcceleratorBVH>(new TinyRender::AcceleratorBVH(this->worldData));

    const clock_t beginBVH = clock();
    bvh->build();
    std::cout << "BVH built in " << float(clock() - beginBVH) / CLOCKS_PER_SEC << "s" << std::endl;

    return true;
}

float Scene::getShapeArea(const size_t shapeID, Distribution1D& faceAreaDistribution) {
    const tinyobj::shape_t& s = worldData.shapes[shapeID];

    for (size_t i = 0; i < s.mesh.indices.size(); i += 3) {
        const int i0 = s.mesh.indices[i + 0].vertex_index;
        const int i1 = s.mesh.indices[i + 1].vertex_index;
        const int i2 = s.mesh.indices[i + 2].vertex_index;
        const v3f v0{worldData.attrib.vertices[3 * i0 + 0], worldData.attrib.vertices[3 * i0 + 1],
                     worldData.attrib.vertices[3 * i0 + 2]};
        const v3f v1{worldData.attrib.vertices[3 * i1 + 0], worldData.attrib.vertices[3 * i1 + 1],
                     worldData.attrib.vertices[3 * i1 + 2]};
        const v3f v2{worldData.attrib.vertices[3 * i2 + 0], worldData.attrib.vertices[3 * i2 + 1],
                     worldData.attrib.vertices[3 * i2 + 2]};

        const v3f e1{v1 - v0};
        const v3f e2{v2 - v0};
        const v3f e3{glm::cross(e1, e2)};
        faceAreaDistribution.add(0.5f * std::sqrt(e3.x * e3.x + e3.y * e3.y + e3.z * e3.z));
    }
    const float area = faceAreaDistribution.cdf.back();
    faceAreaDistribution.normalize();
    return area;
}

v3f Scene::getFirstLightPosition() const {
    return worldData.shapesCenter[emitters[0].shapeID];
}

v3f Scene::getFirstLightIntensity() const {
    return emitters[0].getRadiance(); // point lights are defined by intensity not radiance
}

float Scene::getShapeRadius(const size_t shapeID) const {
    assert(shapeID < worldData.shapes.size());
    v3f emitterCenter = worldData.shapesCenter[shapeID];
    return worldData.shapesAABOX[shapeID].max.x - emitterCenter.x;
}

v3f Scene::getShapeCenter(const size_t shapeID) const {
    assert(shapeID < worldData.shapes.size());
    return worldData.shapesCenter[shapeID];
}

size_t Scene::getFirstLight() const {
    if (emitters.size() <= 0) return -1;
    return emitters[0].shapeID;
}

v3f Scene::getObjectVertexPosition(size_t objectIdx, size_t vertexIdx) const {
    const tinyobj::attrib_t& sa = worldData.attrib;
    const tinyobj::shape_t& s = worldData.shapes[objectIdx];

    int idx = s.mesh.indices[vertexIdx].vertex_index;
    float x = sa.vertices[3 * idx + 0];
    float y = sa.vertices[3 * idx + 1];
    float z = sa.vertices[3 * idx + 2];
    return v3f(x,y,z);
}

v3f Scene::getObjectVertexNormal(size_t objectIdx, size_t vertexIdx) const {
    const tinyobj::attrib_t& sa = worldData.attrib;
    const tinyobj::shape_t& s = worldData.shapes[objectIdx];

    int idx_n = s.mesh.indices[vertexIdx].normal_index;
    float nx = sa.normals[3 * idx_n + 0];
    float ny = sa.normals[3 * idx_n + 1];
    float nz = sa.normals[3 * idx_n + 2];
    return glm::normalize(v3f(nx,ny,nz));
}

size_t Scene::getObjectNbVertices(size_t objectIdx) const {
    return worldData.shapes[objectIdx].mesh.indices.size();
}

int Scene::getPrimitiveID(size_t vertexIdx) const {
    return vertexIdx / 3;
}

int Scene::getMaterialID(size_t objectIdx, int primID) const {
    return worldData.shapes[objectIdx].mesh.material_ids[primID];
}

TR_NAMESPACE_END

/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#pragma once

TR_NAMESPACE_BEGIN

/**
 * Simple direct illumination integrator.
 */
struct SimpleIntegrator : Integrator {
    explicit SimpleIntegrator(const Scene& scene) : Integrator(scene) { }

    v3f render(const Ray& ray, Sampler& sampler) const override {
        v3f Li(0.f);

        // TODO(A2): Implement this
        // 1. compute light dir: wi
        // 2. integrate Li
        // 3. shadow ray

        v3f lightPos = scene.getFirstLightPosition();
        v3f lightIntens = scene.getFirstLightIntensity();

        SurfaceInteraction hitInfo;
        if ( scene.bvh->intersect( ray, hitInfo ) )
        {
            Ray shadowRay = TinyRender::Ray(hitInfo.p, normalize((lightPos - hitInfo.p)));
            shadowRay.max_t = glm::distance(hitInfo.p, lightPos);

            if ( !scene.bvh->intersect( shadowRay, hitInfo ) )
            {
                // distance falloff
                v3f distance = lightPos - hitInfo.p;
                hitInfo.wi = normalize( hitInfo.frameNs.toLocal( distance ) );
                Li = ( lightIntens / glm::length2( distance ) ) * ( getBSDF( hitInfo )->eval( hitInfo ) );
            }
        }
        else
            Li = v3f(0.f, 0.f, 0.f);
        return Li;
    }
};

TR_NAMESPACE_END
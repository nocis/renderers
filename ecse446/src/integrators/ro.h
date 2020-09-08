/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#pragma once
#include <random>
#include "bsdfs/phong.h"

TR_NAMESPACE_BEGIN

/**
 * Reflection occlusion integrator
 */
struct ROIntegrator : Integrator {

    float m_exponent;

    explicit ROIntegrator(const Scene& scene) : Integrator(scene) {
        m_exponent = scene.config.integratorSettings.ro.exponent;
    }

    inline v3f reflect(const v3f& d) const {
        return v3f(-d.x, -d.y, d.z);
    }


    v3f render(const Ray& ray, Sampler& sampler) const override {
        v3f Li(0.f);

        // sampling the phong lobe
        // a cone area of lights which contributes to i.wo direction( view direction )
        // TODO(A3): Implement this
        float L = 0.f;
        SurfaceInteraction info;

        if (scene.bvh->intersect(ray, info))
        {
            v3f wi = Warp::squareToPhongLobe(sampler.next2D(), m_exponent);
            wi = normalize(wi);
            v3f wr = normalize(info.frameNs.toWorld(reflect(info.wo)));
            Frame lobe(wr);

            float distance = scene.aabb.getBSphere().radius / 2;
            Ray shadow_ray = Ray(info.p, normalize(lobe.toWorld(wi)), Epsilon, distance * 1.5);

            SurfaceInteraction shadowInfo;
            if (!scene.bvh->intersect(shadow_ray, shadowInfo))
            {
                v3f wiLocal = info.frameNs.toLocal(lobe.toWorld(wi));
                float cosTheta = wiLocal.z;
                float cosAlpha = fmax(0.0, wi.z);
                float BRDF = (m_exponent + 2.0) * INV_TWOPI * fmax(0.0f, pow(cosAlpha, m_exponent));
                float lightIntense = cosTheta;

                if ( info.wo.z > 0 && wiLocal.z > 0 )
                {
                    float pdf =  Warp::squareToPhongLobePdf(wi, m_exponent);
                    L = BRDF * lightIntense / pdf;
                }
            }
        }
        Li = v3f(L);
        return Li;
    }
};

TR_NAMESPACE_END
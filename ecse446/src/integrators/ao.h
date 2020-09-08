/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#pragma once

TR_NAMESPACE_BEGIN

/**
 * Ambient occlusion integrator
 */
struct AOIntegrator : Integrator {

	// Use this in your switch statement to select the sampling type 
	ESamplingType m_samplingStrategy;

    explicit AOIntegrator(const Scene& scene) : Integrator(scene) { 
		m_samplingStrategy = scene.config.integratorSettings.ao.sampling_type;
	}

    v3f render(const Ray& ray, Sampler& sampler) const override {
        v3f Li(0.f);
		
		/*
		Use the m_sampling_type variable to set wi and the corresponding pdf 
		appropriately for sphere, hemisphere, or cosine sampling.

		You can use a switch statement or an if/else block.

		The m_sampling_type variable is an enum. The different values of the enum 
		can be accessed through:
		ESamplingType::ESpherical
		ESamplingType::EHemispherical
		ESamplingType::ECosineHemispherical
		*/

		// estimators for two specialized direct illumination effects,
		// ambient occlusion (AO)
		// reflection occlusion (RO)


		// 1. find intersection
		// 2. random select a direction( by uniform sampling or importance sampling )
		// 3. cast shadow ray and test the visibility
		// 4. ( BRDF(1 / PI) * projected income light intensity(1 * cosTheta) ) / PDF( MC integration )

        // TODO(A3): Implement this
        float L = 0.f;
        SurfaceInteraction info;

        if (scene.bvh->intersect(ray, info))
        {
            p2f randSample = sampler.next2D();
            v3f wi(0.f);

            if (m_samplingStrategy == ESamplingType::ESpherical)
            {
                wi = Warp::squareToUniformSphere(randSample);
            }
            else if (m_samplingStrategy == ESamplingType::EHemispherical)
            {
                wi = Warp::squareToUniformHemisphere(randSample);
            }
            else if (m_samplingStrategy == ESamplingType::ECosineHemispherical)
            {
                wi = Warp::squareToCosineHemisphere(randSample);
            }
            wi = normalize(wi);

            float distance = scene.aabb.getBSphere().radius / 2;
            Ray shadow_ray = Ray(info.p, normalize(info.frameNs.toWorld(wi)), Epsilon, distance);

            SurfaceInteraction shadowInfo;
            if (!scene.bvh->intersect(shadow_ray, shadowInfo))
            {
                float BRDF = INV_PI;
                float cosTheta = wi.z;
                float lightIntense = cosTheta;

                if ( wi.z > 0 )
                {

                    float pdf = 0.f;
                    if (m_samplingStrategy == ESamplingType::ESpherical)
                    {
                        pdf = Warp::squareToUniformSpherePdf();
                    }
                    else if (m_samplingStrategy == ESamplingType::EHemispherical)
                    {
                        pdf = Warp::squareToUniformHemispherePdf(v3f(0));
                    }
                    else if (m_samplingStrategy == ESamplingType::ECosineHemispherical)
                    {
                        pdf = Warp::squareToCosineHemispherePdf(wi);
                    }

                    L = BRDF * lightIntense / pdf;
                }
            }
        }
        Li = v3f(L);

        return Li;
    }
};

TR_NAMESPACE_END
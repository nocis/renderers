/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#pragma once

TR_NAMESPACE_BEGIN

/**
 * Direct illumination integrator with MIS
 */
struct DirectIntegrator : Integrator {
    explicit DirectIntegrator(const Scene& scene) : Integrator(scene) {
        m_emitterSamples = scene.config.integratorSettings.di.emitterSamples;
        m_bsdfSamples = scene.config.integratorSettings.di.bsdfSamples;
        m_samplingStrategy = scene.config.integratorSettings.di.samplingStrategy;
    }

    static inline float balanceHeuristic(float nf, float fPdf, float ng, float gPdf) {
        float f = nf * fPdf, g = ng * gPdf;
        return f / (f + g);
    }

    void sampleSphereByCosineHemisphere(const p2f& sample,
                                        const v3f& n,
                                        const p3f& pShading,
                                        const v3f& emitterCenter,
                                        float emitterRadius,
                                        v3f& wiW,
                                        float& pdf) const {
        // TODO(A3): Implement this
        wiW = Warp::squareToCosineHemisphere(sample);
        pdf = Warp::squareToCosineHemispherePdf(wiW);

        Frame frameNs(n);
        wiW = glm::normalize(frameNs.toWorld(wiW));
    }

    void sampleSphereByArea(const p2f& sample,
                            const p3f& ShadingPos,
                            const v3f& emitterCenter,
                            float emitterRadius,
                            v3f& pos,
                            v3f& ne,
                            v3f& wiW,
                            float& pdf) const {
        // TODO(A3): Implement this
        v3f emNormal = normalize(ShadingPos - emitterCenter);
        Frame Yfm(emNormal);
        // uniform area sample
        v3f sampleY = Warp::squareToUniformHemisphere(sample);
        pdf = Warp::squareToUniformHemispherePdf(sampleY) / pow(emitterRadius, 2);
        pos = normalize( Yfm.toWorld( sampleY ) ) * emitterRadius + emitterCenter;
        wiW = normalize( pos - ShadingPos );
        ne = normalize( pos - emitterCenter );
    }

    void sampleSphereBySolidAngle(const p2f& sample,
                                  const p3f& pShading,
                                  const v3f& emitterCenter,
                                  float emitterRadius,
                                  v3f& wiW,
                                  float& pdf) const {
        // TODO(A3): Implement this
        float dist = distance(emitterCenter, pShading);
        float cosThetaMax = sqrt(pow(dist, 2) - pow(emitterRadius, 2)) / dist;

        pdf = Warp::squareToUniformConePdf(cosThetaMax);
        v3f wi = Warp::squareToUniformCone(sample, cosThetaMax);

        v3f emNormal = normalize( emitterCenter - pShading );
        Frame Yfm(emNormal);
        wiW = normalize(Yfm.toWorld(wi));
    }

    v3f renderArea(const Ray& ray, Sampler& sampler) const {
        v3f Lr(0.f);

        SurfaceInteraction info;
        // TODO(A3): Implement this
        if ( scene.bvh->intersect(ray, info) )
        {
            if (getEmission( info )!=v3f(0))
                return getEmission(info);

            //cout<<m_emitterSamples<<endl;
            for ( size_t i = 0; i < m_emitterSamples; i++ )
            {
                float samplePdf;
                v3f rayDir;

                float emitterPdf;
                size_t id = selectEmitter(sampler.next(), emitterPdf);
                const Emitter& em = getEmitterByID(id);
                const v3f emitterCenter = scene.getShapeCenter(em.shapeID);
                float emitterRadius = scene.getShapeRadius(em.shapeID);

                v3f Ypos, Ynormal, wiW;

                sampleSphereByArea(sampler.next2D(), info.p, emitterCenter, emitterRadius,
                        Ypos, Ynormal, wiW, samplePdf);

                info.wi = normalize(info.frameNs.toLocal(wiW));
                // No more cosTheta!!!!
                // BSDF is already multiplied by cosTheta

                SurfaceInteraction shadowInfo;
                Ray shadowRay(info.p, normalize(wiW), Epsilon);
                //cout<< rayDir.x<< rayDir.y<<rayDir.z<<endl;

                // pdf of solid angle is different from pdf of area(surface point)
                // pdf of solid angle:         1 / 4PI
                // pdf of area(surface point): 1 / (4PI * r^2)

                // convert {pdf of area(surface point)} of sphere light to {pdf of solid angle} of BSDF sphere
                // PA(X) / cosTheta = Pw(wi) / d^2


                // uniform sample the whole sphere ensures each solid angle sample maps to two surface points
                // hemisphere cannot ensures this uniformity.

                if ( scene.bvh->intersect(shadowRay, shadowInfo) && shadowInfo.shapeID==em.shapeID )
                {
                    //cout<<shadowInfo.shapeID<<" "<<em.shapeID<<endl;
                    v3f lightIntense = getEmission( shadowInfo );
                    // need to convert to solid angle pdf of BSDF sphere
                    // because the render eq integrates the wi!!!!!

                    // pdf of sphere light area sample = p1
                    // pdf of corresponding BSDF sphere area sample = P1 / cosTheta
                    // pdf of corresponding BSDF solid angle sample = P1 / cosTheta * d^2
                    samplePdf = samplePdf / abs(dot(Ynormal, -wiW)) * glm::length2(Ypos-info.p);
                    if (dot(Ynormal, -wiW) <= Epsilon)
                    {
                        //cout<<2222<<endl;
                        lightIntense = v3f(0.);
                    }

                    v3f bsdf = getBSDF(info)->eval(info);
                    Lr += bsdf * lightIntense / samplePdf / emitterPdf;
                }
            }
        }

        return Lr / m_emitterSamples;
    }

    v3f renderCosineHemisphere(const Ray& ray, Sampler& sampler) const {
        v3f Lr(0.f);

        SurfaceInteraction info;
        // TODO(A3): Implement this
        if ( scene.bvh->intersect(ray, info) )
        {
            if (getEmission( info )!=v3f(0))
                return getEmission(info);

            //cout<<m_emitterSamples<<endl;
            for ( size_t i = 0; i < m_emitterSamples; i++ )
            {
                float samplePdf, emitterRadius = 0.f;
                v3f pShading, emitterCenter;
                v3f rayDir;
                sampleSphereByCosineHemisphere(sampler.next2D(), info.frameNs.n, pShading,
                        emitterCenter, emitterRadius, rayDir, samplePdf);

                info.wi = normalize(info.frameNs.toLocal(rayDir));
                // No more cosTheta!!!!
                // BSDF is already multiplied by cosTheta

                SurfaceInteraction emitterInfo;
                Ray lightRay(info.p, normalize(rayDir), Epsilon);
                //cout<< rayDir.x<< rayDir.y<<rayDir.z<<endl;

                if ( scene.bvh->intersect(lightRay, emitterInfo) && getEmission( emitterInfo )!=v3f(0) )
                {
                    v3f lightIntense = getEmission( emitterInfo );
                    v3f bsdf = getBSDF(info)->eval(info);
                    Lr += bsdf * lightIntense / samplePdf;
                }
            }
        }

        return Lr / m_emitterSamples;
    }

    v3f renderBSDF(const Ray& ray, Sampler& sampler) const {
        v3f Lr(0.f);

        SurfaceInteraction info;
        // TODO(A3): Implement this
        if ( scene.bvh->intersect(ray, info) )
        {
            if (getEmission( info )!=v3f(0))
                return getEmission(info);

            //cout<<m_bsdfSamples<<endl;
            for ( size_t i = 0; i < m_bsdfSamples; i++ )
            {
                float pdf;
                v3f bsdf = getBSDF(info)->sample(info, sampler, &pdf);
                float cosTheta = info.wi.z;
                // No more cosTheta!!!!
                // BSDF is already multiplied by cosTheta

                Ray lightRay(info.p, normalize(info.frameNs.toWorld(info.wi)), Epsilon);

                SurfaceInteraction emitterInfo;
                //cout<< rayDir.x<< rayDir.y<<rayDir.z<<endl;

                if ( scene.bvh->intersect(lightRay, emitterInfo) && getEmission( emitterInfo )!=v3f(0) )
                {
                    v3f lightIntense = getEmission( emitterInfo );
                    Lr += bsdf * lightIntense / pdf;
                }
            }
        }

        return Lr / m_bsdfSamples;
    }

    v3f renderSolidAngle(const Ray& ray, Sampler& sampler) const {
        v3f Lr(0.f);

        SurfaceInteraction info;
        // TODO(A3): Implement this
        if ( scene.bvh->intersect(ray, info) )
        {
            if (getEmission( info )!=v3f(0))
                return getEmission(info);

            for ( size_t i = 0; i < m_emitterSamples; i++ )
            {
                float samplePdf;
                v3f rayDir;

                float emitterPdf;
                size_t id = selectEmitter(sampler.next(), emitterPdf);
                const Emitter& em = getEmitterByID(id);
                const v3f emitterCenter = scene.getShapeCenter(em.shapeID);
                float emitterRadius = scene.getShapeRadius(em.shapeID);

                v3f wiW;

                sampleSphereBySolidAngle(sampler.next2D(), info.p, emitterCenter, emitterRadius, wiW, samplePdf);

                info.wi = normalize(info.frameNs.toLocal(wiW));

                SurfaceInteraction shadowInfo;
                Ray shadowRay(info.p, normalize(wiW), Epsilon);
                if ( scene.bvh->intersect(shadowRay, shadowInfo) && shadowInfo.shapeID==em.shapeID )
                {
                    v3f lightIntense = getEmission( shadowInfo );
                    v3f bsdf = getBSDF(info)->eval(info);
                    Lr += bsdf * lightIntense / samplePdf / emitterPdf;
                }
            }
        }
        return Lr / m_emitterSamples;
    }

    v3f renderMIS(const Ray& ray, Sampler& sampler) const {

        v3f Lr(0.f);
        v3f LM(0.f), LE(0.f);

        float wm, we;

        SurfaceInteraction info;
        // TODO(A4): Implement this

        if ( scene.bvh->intersect(ray, info) )
        {
            if (getEmission( info )!=v3f(0))
                return getEmission(info);

            for ( size_t i = 0; i < m_emitterSamples; i++ )
            {
                float samplePdf;
                v3f rayDir;

                float emitterPdf;
                size_t id = selectEmitter(sampler.next(), emitterPdf);
                const Emitter& em = getEmitterByID(id);
                const v3f emitterCenter = scene.getShapeCenter(em.shapeID);
                float emitterRadius = scene.getShapeRadius(em.shapeID);

                v3f wiW;

                sampleSphereBySolidAngle(sampler.next2D(), info.p, emitterCenter, emitterRadius, wiW, samplePdf);

                info.wi = normalize(info.frameNs.toLocal(wiW));

                SurfaceInteraction shadowInfo;
                Ray shadowRay(info.p, normalize(wiW), Epsilon);
                if ( scene.bvh->intersect(shadowRay, shadowInfo) && shadowInfo.shapeID==em.shapeID )
                {
                    v3f lightIntense = getEmission( shadowInfo );
                    v3f bsdf = getBSDF(info)->eval(info);
                    we = balanceHeuristic(m_emitterSamples, samplePdf * emitterPdf, m_bsdfSamples, getBSDF(info)->pdf(info));
                    LE += bsdf * lightIntense / samplePdf / emitterPdf * we;
                }
            }

            for ( size_t i = 0; i < m_bsdfSamples; i++ )
            {
                float pdf;
                v3f bsdf = getBSDF(info)->sample(info, sampler, &pdf);
                float cosTheta = info.wi.z;
                Ray lightRay(info.p, normalize(info.frameNs.toWorld(info.wi)), Epsilon);

                SurfaceInteraction emitterInfo;

                if ( scene.bvh->intersect(lightRay, emitterInfo) && getEmission( emitterInfo )!=v3f(0) )
                {
                    float samplePdf;
                    const Emitter& em = getEmitterByID(int(getEmitterIDByShapeID(emitterInfo.shapeID)));
                    float emitterRadius = scene.getShapeRadius(em.shapeID);
                    v3f emitterCenter = scene.getShapeCenter(em.shapeID);
                    float dist = distance(emitterCenter, info.p);
                    float cosThetaMax = sqrt(pow(dist, 2) - pow(emitterRadius, 2)) / dist;
                    samplePdf = Warp::squareToUniformConePdf(cosThetaMax);

                    wm = balanceHeuristic(m_bsdfSamples, pdf, m_emitterSamples, samplePdf * 1.f / scene.emitters.size());
                    v3f lightIntense = getEmission( emitterInfo );
                    LM += bsdf * lightIntense / pdf * wm;
                }
            }
        }
        if (m_emitterSamples)
            Lr += LE / m_emitterSamples;
        
        if (m_bsdfSamples)
            Lr += LM / m_bsdfSamples;
        return Lr;
    }

    v3f render(const Ray& ray, Sampler& sampler) const override {
        if (m_samplingStrategy == ESamplingStrategy::EMIS)
            return this->renderMIS(ray, sampler);
        else if (m_samplingStrategy == ESamplingStrategy::EArea)
            return this->renderArea(ray, sampler);
        else if (m_samplingStrategy == ESamplingStrategy::ESolidAngle)
            return this->renderSolidAngle(ray, sampler);
        else if (m_samplingStrategy == ESamplingStrategy::ECosineHemisphere)
            return this->renderCosineHemisphere(ray, sampler);
        else
            return this->renderBSDF(ray, sampler);
    }

    size_t m_emitterSamples;     // Number of emitter samples
    size_t m_bsdfSamples;        // Number of BSDF samples
    ESamplingStrategy m_samplingStrategy;   // Sampling strategy to use
};

TR_NAMESPACE_END
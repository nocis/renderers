/*
    This file is part of TinyRender, an educative rendering system.

    Designed for ECSE 446/546 Realistic/Advanced Image Synthesis.
    Derek Nowrouzezahrai, McGill University.
*/

#pragma once

#include "core/core.h"

TR_NAMESPACE_BEGIN

/**
 * Modified Phong reflectance model
 */
struct PhongBSDF : BSDF {

    std::unique_ptr<Texture < v3f>> specularReflectance;
    std::unique_ptr<Texture < v3f>> diffuseReflectance;
    std::unique_ptr<Texture < float>> exponent;
    float specularSamplingWeight;
    float scale;

    PhongBSDF(const WorldData& scene, const Config& config, const size_t& matID) : BSDF(scene, config, matID) {
        const tinyobj::material_t& mat = scene.materials[matID];

        if (mat.specular_texname.empty())
            specularReflectance = std::unique_ptr<Texture<v3f>>(new ConstantTexture3f(glm::make_vec3(mat.specular)));
        else
            specularReflectance = std::unique_ptr<Texture<v3f>>(new BitmapTexture3f(config, mat.specular_texname));

        if (mat.diffuse_texname.empty())
            diffuseReflectance = std::unique_ptr<Texture<v3f>>(new ConstantTexture3f(glm::make_vec3(mat.diffuse)));
        else
            diffuseReflectance = std::unique_ptr<Texture<v3f>>(new BitmapTexture3f(config, mat.diffuse_texname));

        exponent = std::unique_ptr<Texture<float>>(new ConstantTexture1f(mat.shininess));

        //get scale value to ensure energy conservation
        v3f maxValue = specularReflectance->getMax() + diffuseReflectance->getMax();
        float actualMax = max(max(maxValue.x, maxValue.y), maxValue.z);
        scale = actualMax > 1.0f ? 0.99f * (1.0f / actualMax) : 1.0f;

        float dAvg = getLuminance(diffuseReflectance->getAverage() * scale);
        float sAvg = getLuminance(specularReflectance->getAverage() * scale);
        specularSamplingWeight = sAvg / (dAvg + sAvg);

        components.push_back(EGlossyReflection);
        components.push_back(EDiffuseReflection);

        combinedType = 0;
        for (unsigned int component : components)
            combinedType |= component;
    }

    inline float getExponent(const SurfaceInteraction& i) const override {
        return exponent->eval(worldData, i);
    }

    inline v3f reflect(const v3f& d) const {
        return v3f(-d.x, -d.y, d.z);
    }

    v3f eval(const SurfaceInteraction& i) const override {
        v3f val(0.f);
        // 1. reflectivity/albedo map == color map
        //    BRDF: function about albedo!!!!!!
        //
        //    shade = Radiance of out light
        //          = projected irradiance ( total input light energy : light color * cosTheta) * BRDF( evaluated for special in and out light path )
        //
        // 2. BRDF normalization: FOR ENERGY CONSERVATION!!
        //    BRDF                                  : irradiance reflection rate on single direction!
        //    directional-hemispherical reflectance : irradiance reflection rate on hemisphere( total )!
        //
        // 3. directional-hemispherical reflectance : hemispherical INTEGRATION of BRDF on normal direction projection
        //    directional-hemispherical reflectance should LESS EQUAL to material color( ENERGY CONSERVATION )!
        //
        //    divide BRDF by a factor( e.g, PI )!!!!!!
        //
        // (Supplement) the PI in specular normalization factor usually cancelled by point light!!
        //              Because people use the average irradiance of a spherical field for light source as light irradiance(color)!!!!
        //              which means light color is already divided by PI;
        //
        //              " The version of this equation typically used in real-time rendering applications
        //              lacks the 1/PI term. This is because real-time applications typically
        //              factor this term into the light’s irradiance (effectively using ELk/PI as the
        //              light property indicating color and brightness, instead of ELk ). "
        //
        // 4. Phong shade = (normalized diffusecolor + normalized specular) normalizefactor * BRDF * lightcolor * cosTheta( only partial input contributes to irradiance )!!!!!)
        //
        // 5. Normalized Phong BRDF FORMULA (same as UNITY)
        //    diffuse + specular
        //    BRDF = fr(x,wi,wo) = fr,d(x,wi,wo) + fr,s(x,wi,wr)
        //         = ρd/pi + ρs(n+2)/2pi * max(0, pow(cos(r,v), n))
        //
        // http://www.farbrausch.de/~fg/stuff/phong.pdf : for why not n+1?

        // 6. reflection dir : r = 2|l|(n·l)n - l
        // TODO(A2): Implement this


        // wo : view vector
        // reflect(wi)  : light reflectance
        float exp = exponent->eval(worldData, i);
        v3f diffuseColor = diffuseReflectance->eval(worldData, i) * scale;
        v3f specularColor = specularReflectance->eval(worldData, i) * scale;
        // energy conservation : scale, max energy(specularMax + diffuseMax) <= 1.0

        if (i.wo.z > 0 && i.wi.z > 0) //front-facing test
        {
            float cosAlpha = glm::dot(reflect(i.wi), i.wo );
            float cosTheta = i.wi.z;
            cosAlpha = cosAlpha>0? pow(cosAlpha, exp) : 0;
            val = /*diffuseColor * INV_PI + */specularColor * ( exp + 2.f ) * INV_TWOPI * cosAlpha;
            val *= cosTheta; // foreshortening factor
        }

        return val;
    }

    float pdf(const SurfaceInteraction& i) const override {
        float pdf = 0.f;

        // TODO(A3): Implement this
        float exp = exponent->eval(worldData,i);
        v3f wr = normalize(i.frameNs.toWorld(reflect(i.wo)));
        Frame lobe(wr);
        v3f dir = lobe.toLocal(i.frameNs.toWorld(i.wi));

        pdf = Warp::squareToPhongLobePdf(dir, exp);
        return pdf;
    }

    v3f sample(SurfaceInteraction& i, Sampler& sampler, float* _pdf) const override {
        v3f val(0.f);

        // TODO(A3): Implement this

        float exp = exponent->eval(worldData,i);
        v3f wr = normalize(i.frameNs.toWorld(reflect(i.wo)));
        Frame lobe(wr);

        // weighted example:
        //     spec / 60 + diff / 40 = ( spec / (60 / 100) + diff / (40 / 100) ) / 100

        if (sampler.next() <= specularSamplingWeight)
        {
            v3f dir = lobe.toWorld(Warp::squareToPhongLobe(sampler.next2D(), exp));
            i.wi = normalize(i.frameNs.toLocal(dir));
            *_pdf = pdf(i);
            val = eval(i) / specularSamplingWeight;
        }
        else
        {
            i.wi = normalize(Warp::squareToCosineHemisphere(sampler.next2D()));
            *_pdf = Warp::squareToCosineHemispherePdf(i.wi);
            if (i.wo.z > 0 && i.wi.z > 0)
            {
                float cosTheta = i.wi.z;
                val = diffuseReflectance->eval(worldData, i) * scale * INV_PI * cosTheta;
                val /= (1.0-specularSamplingWeight);
            }
        }
        return val;
    }

    std::string toString() const override { return "Phong"; }
};

TR_NAMESPACE_END
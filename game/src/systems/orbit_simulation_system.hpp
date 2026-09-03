#pragma once

#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <webgpu/webgpu_cpp.h>

#include <resources/resource_shader.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class OrbitalElementsComponent;

class OrbitSimulationSystem : public System
{
public:
    OrbitSimulationSystem();
    ~OrbitSimulationSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    static glm::dvec3 CalculateCartesianPosition(const OrbitalElementsComponent& orbitalElements);
    static void OrientPlanets(entt::registry& registry, double gmst);
    static double SolveKeplerEquation(double meanAnomaly, double eccentricity, int maxIterations = 10, double tolerance = 1e-10);

private:
    void CreateComputePipeline();
    void CreateStorageBuffers(size_t numOrbitalElements);
    
    bool m_UseSGP4 = true;
    bool m_Initialized = false;

    ResourceShaderSharedPtr m_pShader;
    wgpu::ComputePipeline m_ComputePipeline;
    wgpu::Buffer m_OrbitalElementsBuffer;
    wgpu::Buffer m_PropagatedPositionsBuffer;
    wgpu::Buffer m_PropagatedPositionsReadbackBuffer;
    wgpu::BindGroup m_BindGroup;
    size_t m_NumOrbitalElements = 0;
};

} // namespace WingsOfSteel

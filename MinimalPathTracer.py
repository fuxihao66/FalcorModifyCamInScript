
from falcor import *
import time

SceneFilename = 'D:/DeepIllumination/fireplace_room/asian_dragon.fscene'
def render_graph_PathTracerGraph():
    g = RenderGraph("PathTracerGraph")
    loadRenderPassLibrary("AccumulatePass.dll")
    loadRenderPassLibrary("GBuffer.dll")
    loadRenderPassLibrary("ToneMapper.dll")
    loadRenderPassLibrary("MegakernelPathTracer.dll")
    # AccumulatePass = createPass("AccumulatePass", {'enableAccumulation': True})
    # g.addPass(AccumulatePass, "AccumulatePass")
    # AlbedoAccumulatePass = createPass("AccumulatePass", {'enabled': True})
    # g.addPass(AlbedoAccumulatePass, "AlbedoAccumulatePass")
    ToneMappingPass = createPass("ToneMapper", {'autoExposure': False, 'exposureValue': 0.0})
    g.addPass(ToneMappingPass, "ToneMappingPass")
    GBufferRT = createPass("GBufferRT", {'forceCullMode': False, 'cull': CullMode.CullBack, 'samplePattern': SamplePattern.Stratified, 'sampleCount': 16})
    g.addPass(GBufferRT, "GBufferRT")
    MegakernelPathTracer = createPass("MegakernelPathTracer", {'mSharedParams': PathTracerParams(useVBuffer=0)})
    g.addPass(MegakernelPathTracer, "MegakernelPathTracer")

    # OptixDenoiser = createPass("OptixDenoiser")
    # g.addPass(OptixDenoiser, "OptixDenoiser")

    g.addEdge("GBufferRT.vbuffer", "MegakernelPathTracer.vbuffer")      # Required by ray footprint.
    g.addEdge("GBufferRT.posW", "MegakernelPathTracer.posW")
    g.addEdge("GBufferRT.normW", "MegakernelPathTracer.normalW")
    g.addEdge("GBufferRT.tangentW", "MegakernelPathTracer.tangentW")
    g.addEdge("GBufferRT.faceNormalW", "MegakernelPathTracer.faceNormalW")
    g.addEdge("GBufferRT.viewW", "MegakernelPathTracer.viewW")
    g.addEdge("GBufferRT.diffuseOpacity", "MegakernelPathTracer.mtlDiffOpacity")
    g.addEdge("GBufferRT.specRough", "MegakernelPathTracer.mtlSpecRough")
    g.addEdge("GBufferRT.emissive", "MegakernelPathTracer.mtlEmissive")
    g.addEdge("GBufferRT.matlExtra", "MegakernelPathTracer.mtlParams")
    # g.addEdge("MegakernelPathTracer.color", "AccumulatePass.input")
    # g.addEdge("AccumulatePass.output", "ToneMappingPass.src")

    # g.addEdge("MegakernelPathTracer.albedo", "AlbedoAccumulatePass.input")
    # g.addEdge("ToneMappingPass.dst", "OptixDenoiser.color")
    # g.addEdge("AlbedoAccumulatePass.output", "OptixDenoiser.albedo")
    # g.addEdge("GBufferRT.normW", "OptixDenoiser.normal")

    g.markOutput("MegakernelPathTracer.color")
    g.markOutput("MegakernelPathTracer.albedo")
    g.markOutput("GBufferRT.normW")
    return g

PathTracerGraph = render_graph_PathTracerGraph()
m.loadScene(SceneFilename,SceneBuilderFlags.Default)

try: m.addGraph(PathTracerGraph)
except NameError: None


fc.outputDir = "D:/TempOutput"
for idx in range(3):
    fc.baseFilename = f"res.{idx+1:04d}"

    renderFrame()
    fc.capture()
    time.sleep(1.5)
    m.useNextCamera("-0.0000615 0.993 -0.548824 0.80556 0.803517 -1.1 -0.005632 1 0.003827 65.0 0.10 10000.0 1.9")
    # m.scene.camera.mData.posW.x = 1.0

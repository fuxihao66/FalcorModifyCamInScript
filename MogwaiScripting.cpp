/***************************************************************************
 # Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/
#include "stdafx.h"

namespace Mogwai
{
    namespace
    {
        const std::string kRunScript = "script";
        const std::string kLoadScene = "loadScene";
        const std::string kUseNextCamera = "useNextCamera";
        const std::string kSaveConfig = "saveConfig";
        const std::string kAddGraph = "addGraph";
        const std::string kRemoveGraph = "removeGraph";
        const std::string kGetGraph = "getGraph";
        const std::string kUI = "ui";
        const std::string kResizeSwapChain = "resizeSwapChain";
        const std::string kActiveGraph = "activeGraph";
        const std::string kScene = "scene";

        const std::string kRendererVar = "m";
        const std::string kTimeVar = "t";

        template<typename T>
        std::string prepareHelpMessage(const T& g)
        {
            std::string s = Renderer::getVersionString() + "\nGlobal utility objects:\n";
            static const size_t kMaxSpace = 8;
            for (auto n : g)
            {
                s += "\t'" + n.first + "'";
                s += (n.first.size() >= kMaxSpace) ? " " : std::string(kMaxSpace - n.first.size(), ' ');
                s += n.second;
                s += "\n";
            }

            s += "\nGlobal functions\n";
            s += "\trenderFrame()      Render a frame. If the clock is not paused, it will advance by one tick. You can use it inside for loops, for example to loop over a specific time-range\n";
            s += "\texit()             Exit Mogwai\n";
            return s;
        }

        std::string windowConfig()
        {
            std::string s;
            SampleConfig c = gpFramework->getConfig();
            s += "# Window Configuration\n";
            s += Scripting::makeMemberFunc(kRendererVar, kResizeSwapChain, c.windowDesc.width, c.windowDesc.height);
            s += Scripting::makeSetProperty(kRendererVar, kUI, c.showUI);
            return s;
        }
    }

    void Renderer::saveConfig(const std::string& filename) const
    {
        std::string s;

        if (!mGraphs.empty())
        {
            s += "# Graphs\n";
            for (const auto& g : mGraphs)
            {
                s += RenderGraphExporter::getIR(g.pGraph);
                s += kRendererVar + "." + kAddGraph + "(" + RenderGraphIR::getFuncName(g.pGraph->getName()) + "())\n";
            }
            s += "\n";
        }

        if (mpScene)
        {
            s += "# Scene\n";
            s += Scripting::makeMemberFunc(kRendererVar, kLoadScene, Scripting::getFilenameString(mpScene->getFilename()));
            const std::string sceneVar = kRendererVar + "." + kScene;
            s += mpScene->getScript(sceneVar);
            s += "\n";
        }

        s += windowConfig() + "\n";

        s += "# Time Settings\n";
        s += gpFramework->getGlobalClock().getScript(kTimeVar) + "\n";

        for (auto& pe : mpExtensions)
        {
            auto eStr = pe->getScript();
            if (eStr.size()) s += eStr + "\n";
        }

        std::ofstream(filename) << s;
    }

    void Renderer::registerScriptBindings(pybind11::module& m)
    {
        pybind11::class_<Renderer> renderer(m, "Renderer");
        renderer.def(kRunScript.c_str(), &Renderer::loadScript, "filename"_a = std::string());
        renderer.def(kLoadScene.c_str(), &Renderer::loadScene, "filename"_a = std::string(), "buildFlags"_a = SceneBuilder::Flags::Default);


        renderer.def(kUseNextCamera.c_str(), &Renderer::useNextCamera, "filename"_a = std::string());

        

        renderer.def(kSaveConfig.c_str(), &Renderer::saveConfig, "filename"_a);
        renderer.def(kAddGraph.c_str(), &Renderer::addGraph, "graph"_a);
        renderer.def(kRemoveGraph.c_str(), pybind11::overload_cast<const std::string&>(&Renderer::removeGraph), "name"_a);
        renderer.def(kRemoveGraph.c_str(), pybind11::overload_cast<const RenderGraph::SharedPtr&>(&Renderer::removeGraph), "graph"_a);
        renderer.def(kGetGraph.c_str(), &Renderer::getGraph, "name"_a);
        renderer.def("graph", &Renderer::getGraph); // PYTHONDEPRECATED
        auto envMap = [](Renderer* pRenderer, const std::string& filename) { if (pRenderer->getScene()) pRenderer->getScene()->loadEnvMap(filename); };
        renderer.def("envMap", envMap, "filename"_a); // PYTHONDEPRECATED

        // PYTHONDEPRECATED Use the global function defined in the script bindings in Sample.cpp when resizing from a Python script.
        auto resize = [](Renderer* pRenderer, uint32_t width, uint32_t height) {gpFramework->resizeSwapChain(width, height); };
        renderer.def(kResizeSwapChain.c_str(), resize);

        renderer.def_property_readonly(kScene.c_str(), &Renderer::getScene);
        renderer.def_property_readonly(kActiveGraph.c_str(), &Renderer::getActiveGraph);

        auto getUI = [](Renderer* pRenderer) { return gpFramework->isUiEnabled(); };
        auto setUI = [](Renderer* pRenderer, bool show) { gpFramework->toggleUI(show); };
        renderer.def_property(kUI.c_str(), getUI, setUI);

        Extension::Bindings b(m, renderer);
        b.addGlobalObject(kRendererVar, this, "The engine");
        b.addGlobalObject(kTimeVar, &gpFramework->getGlobalClock(), "Time Utilities");
        for (auto& pe : mpExtensions) pe->scriptBindings(b);
        mGlobalHelpMessage = prepareHelpMessage(b.mGlobalObjects);

        // Replace the `help` function
        auto globalHelp = [this]() { pybind11::print(mGlobalHelpMessage);};
        m.def("help", globalHelp);

        auto objectHelp = [](pybind11::object o)
        {
            auto b = pybind11::module::import("builtins");
            auto h = b.attr("help");
            h(o);
        };
        m.def("help", objectHelp, "object"_a);
    }
}

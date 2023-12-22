/*
Copyright(c) 2016-2023 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES ==============================
#include "pch.h"
#include "Shader_Reloader.h"
#include "Renderer.h"
#include "../RHI/RHI_Shader.h"
#include "ThreadPool.h"
#include <fstream>

using namespace std;
using namespace Spartan;

namespace Spartan
{
    // TODO built two version, 1 on thread other on update
    void RunShaderReloadThread()
    {
        ThreadPool::AddTask
        (
            []() -> void
            {

                const array<shared_ptr<RHI_Shader>, Spartan::Renderer::number_shaders>& shaders = Spartan::Renderer::GetShaders();
                std::unordered_map<shared_ptr<Spartan::RHI_Shader>, long long> map{};
                if (!shaders.empty())
                {
                    for (auto& shader : shaders)
                    {
                        if (!shader)
                        {
                            continue;
                        }
                        auto b = filesystem::path(shader->GetFilePath());
                        auto a = std::filesystem::last_write_time(b);
                        std::chrono::system_clock::rep c = a.time_since_epoch().count();
                        map[shader] = c;
                    }
                }
                // just run per frame instead
                // race condition with shader editor!!
                while (true)
                {
                    //std::this_thread::sleep_for(std::chrono::milliseconds(16));
                    for (auto& shader : shaders)
                    {
                        if (!shader)
                        {
                            continue;
                        }
                        auto b = filesystem::path(shader->GetFilePath());
                        if (!filesystem::exists(b))
                            continue;
                        auto a = std::filesystem::last_write_time(b);
                        auto c = a.time_since_epoch();
                        auto d = c.count();
                        auto oldTime = map[shader];
                        if (d > oldTime)
                        {
                            shader->Compile(shader->GetShaderStage(), shader->GetFilePath(), false);
                            map[shader] = d;
                        }
                    }
                }
            }
        );
    }
}

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

// Engine macros
#include "Definitions.h"

//= STD =====================
#include <string>
#include <algorithm>
#include <type_traits>
#include <memory>
#include <fstream>
#include <sstream>
#include <limits>
#include <cassert>
#include <cstdint>
#include <atomic>
#include <map>
#include <unordered_map>
#include <cstdio>
#include <filesystem>
#include <regex>
#include <locale>
#include <codecvt>
#include <array>
#include <deque>
#include <vector>
#include <iostream>
#include <cstdarg>
#include <thread>
#include <condition_variable>
#include <set>
#include <variant>
#include <cstring>
#include <unordered_set>
//===========================

//= RUNTIME ====================
// Core
#include "Engine.h"
#include "Event.h"
#include "Settings.h"
#include "Timer.h"
#include "FileSystem.h"
#include "Stopwatch.h"

// Logging
#include "../Logging/Log.h"

// Math
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Math/Ray.h"
#include "../Math/RayHit.h"
#include "../Math/Rectangle.h"
#include "../Math/BoundingBox.h"
#include "../Math/Sphere.h"
#include "../Math/Matrix.h"
#include "../Math/Frustum.h"
#include "../Math/Plane.h"
#include "../Math/MathHelper.h"
//==============================

#if !defined(_MSC_VER)
    #define FFX_GCC
#endif

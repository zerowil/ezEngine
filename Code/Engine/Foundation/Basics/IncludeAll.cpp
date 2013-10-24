#include <Foundation/PCH.h>

// This file includes everything from Foundation to ensure that all template code is looked at when building the Foundation library.
// For non-template code this is not really necessary, but to be sure, just include everything.

#include <Foundation/Algorithm/Comparer.h>
#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/Algorithm/Sorting.h>

#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Basics/Types.h>
#include <Foundation/Basics/TypeTraits.h>
#include <Foundation/Basics/Types/Bitflags.h>
#include <Foundation/Basics/Types/Delegate.h>
#include <Foundation/Basics/Types/Id.h>
#include <Foundation/Basics/Types/RefCounted.h>
#include <Foundation/Basics/Types/Variant.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Communication/MessageQueue.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/Configuration/Plugin.h>

#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Containers/List.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Containers/StaticRingBuffer.h>

#include <Foundation/IO/IBinaryStream.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Math/FixedPoint.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Memory/EndianHelper.h>
#include <Foundation/Memory/IAllocator.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Memory/Policies/AlignedAllocation.h>
#include <Foundation/Memory/Policies/AlignedHeapAllocation.h>
#include <Foundation/Memory/Policies/HeapAllocation.h>
#include <Foundation/Memory/Policies/ProxyAllocation.h>
#include <Foundation/Memory/Policies/Tracking.h>

#include <Foundation/Profiling/Profiling.h>

#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/SharedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Strings/StringIterator.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/UnicodeUtils.h>

#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/AtomicUtils.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadLocalPointer.h>
#include <Foundation/Threading/ThreadLocalStorage.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/Time/Clock.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Time/Time.h>

#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/Utilities/StackTracer.h>
#include <Foundation/Utilities/Stats.h>







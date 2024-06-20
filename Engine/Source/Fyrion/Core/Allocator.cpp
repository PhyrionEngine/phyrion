
#include "Allocator.hpp"
#include <mimalloc.h>
#include "mimalloc/types.h"

#include <cpptrace/cpptrace.hpp>
#include <unordered_map>

namespace Fyrion
{
    namespace
    {
        HeapAllocator defaultAllocator{};
        bool captureTrace = false;
        std::unordered_map<usize, cpptrace::raw_trace> traces{};
        std::mutex traceMutex{};
    }

    VoidPtr HeapAllocator::MemAlloc(usize bytes, usize alignment)
    {
        VoidPtr ptr = mi_malloc_aligned(bytes, alignment);

        if(captureTrace)
        {
            usize ptrAddress = reinterpret_cast<usize>(ptr);
            auto trace = cpptrace::generate_raw_trace();
            traceMutex.lock();
            traces.insert(std::make_pair(ptrAddress, std::move(trace)));
            traceMutex.unlock();
        }
        return ptr;
    }

    void HeapAllocator::MemFree(VoidPtr ptr)
    {
        if(captureTrace)
        {

        }

        mi_free(ptr);
    }

    void MemoryGlobals::SetOptions(AllocatorOptions options)
    {
       if (options & AllocatorOptions_ShowStats)
       {
           mi_option_set(mi_option_show_stats, true);
       }

        if (options & AllocatorOptions_Verbose)
        {
            mi_option_set(mi_option_verbose, true);
        }

        if (options & AllocatorOptions_ShowErrors)
        {
            mi_option_set(mi_option_show_errors, true);
        }

        if (options & AllocatorOptions_CaptureTrace)
        {
            captureTrace = true;
        }
    }

    Allocator& MemoryGlobals::GetDefaultAllocator()
    {
        return defaultAllocator;
    }

    HeapStats MemoryGlobals::GetHeapStats()
    {
        mi_heap_t* heap = mi_heap_get_default();
        return HeapStats{
            .totalAllocated = heap->tld->stats.normal.allocated + heap->tld->stats.large.allocated  + heap->tld->stats.huge.allocated,
            .totalFreed = heap->tld->stats.normal.freed + heap->tld->stats.large.freed  + heap->tld->stats.huge.freed,
        };
    }
}
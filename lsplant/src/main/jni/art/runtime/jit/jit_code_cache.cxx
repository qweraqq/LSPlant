module;

#include "logging.hpp"
#include "utils/hook_helper.hpp"

export module jit_code_cache;

import art_method;
import common;
import thread;

namespace lsplant::art::jit {
export class JitCodeCache {
    CREATE_MEM_FUNC_SYMBOL_ENTRY(void, MoveObsoleteMethod, JitCodeCache *thiz,
                                 ArtMethod *old_method, ArtMethod *new_method) {
        if (MoveObsoleteMethodSym) [[likely]] {
            MoveObsoleteMethodSym(thiz, old_method, new_method);
        } else {
            // fallback to set data
            new_method->SetData(old_method->GetData());
            old_method->SetData(nullptr);
        }
    }

    CREATE_MEM_HOOK_STUB_ENTRY("_ZN3art3jit12JitCodeCache19GarbageCollectCacheEPNS_6ThreadE", void,
                               GarbageCollectCache, (JitCodeCache * thiz, Thread *self), {
                                   auto movements = GetJitMovements();
                                   LOGD("Before jit cache gc, moving %zu hooked methods",
                                        movements.size());
                                   for (auto [target, backup] : movements) {
                                       MoveObsoleteMethod(thiz, target, backup);
                                   }
                                   backup(thiz, self);
                               });

public:
    static bool Init(const HookHandler &handler) {
        auto sdk_int = GetAndroidApiLevel();
        if (sdk_int >= __ANDROID_API_O__) [[likely]] {
            if (!RETRIEVE_MEM_FUNC_SYMBOL(
                    MoveObsoleteMethod,
                    "_ZN3art3jit12JitCodeCache18MoveObsoleteMethodEPNS_9ArtMethodES3_"))
                [[unlikely]] {
                return false;
            }
        }

        // TODO: 处理垃圾回收以后HOOK可能失效？低版本安卓？
        // if (sdk_int >= __ANDROID_API_N__) [[likely]] {
        //     if (!HookSyms(handler, GarbageCollectCache)) [[unlikely]] {
        //         return false;
        //     }
        // }
        return true;
    }
};
}  // namespace lsplant::art::jit

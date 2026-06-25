#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach/mach.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <typeinfo>

// 声明 core 层记录抛出状态的 C 接口
extern "C" void zwMobileGuardRecordThrowState(void* thrown_exception, std::type_info* tinfo);

typedef void (*CxaThrowType)(void*, std::type_info*, void (*)(void*));
typedef void (*CxaRethrowType)();

struct OriginalCxaThrow {
    const void* imageBase;
    CxaThrowType function;
};

static pthread_mutex_t g_cxaHookMutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_cxaThrowRebindingInstalled = false;
static OriginalCxaThrow* g_originalCxaThrows = nullptr;
static size_t g_originalCxaThrowCount = 0;
static size_t g_originalCxaThrowCapacity = 0;

static CxaThrowType lookupOriginalCxaThrow() {
    return (CxaThrowType)dlsym(RTLD_NEXT, "__cxa_throw");
}

static CxaRethrowType lookupOriginalCxaRethrow() {
    return (CxaRethrowType)dlsym(RTLD_NEXT, "__cxa_rethrow");
}

static void rememberOriginalCxaThrow(const void* imageBase, CxaThrowType function) {
    if (imageBase == nullptr || function == nullptr) {
        return;
    }

    for (size_t i = 0; i < g_originalCxaThrowCount; ++i) {
        if (g_originalCxaThrows[i].imageBase == imageBase) {
            g_originalCxaThrows[i].function = function;
            return;
        }
    }

    if (g_originalCxaThrowCount == g_originalCxaThrowCapacity) {
        size_t newCapacity = g_originalCxaThrowCapacity == 0 ? 32 : g_originalCxaThrowCapacity * 2;
        OriginalCxaThrow* newItems = (OriginalCxaThrow*)realloc(
            g_originalCxaThrows, newCapacity * sizeof(OriginalCxaThrow));
        if (newItems == nullptr) {
            return;
        }
        g_originalCxaThrows = newItems;
        g_originalCxaThrowCapacity = newCapacity;
    }

    g_originalCxaThrows[g_originalCxaThrowCount++] = { imageBase, function };
}

static CxaThrowType originalCxaThrowForImage(const void* imageBase) {
    for (size_t i = 0; i < g_originalCxaThrowCount; ++i) {
        if (g_originalCxaThrows[i].imageBase == imageBase) {
            return g_originalCxaThrows[i].function;
        }
    }
    return lookupOriginalCxaThrow();
}

static void zwMobileGuardCxaThrowDecorator(void* thrown_exception,
                                           std::type_info* tinfo,
                                           void (*dest)(void*)) {
    zwMobileGuardRecordThrowState(thrown_exception, tinfo);

    void* frames[2] = { nullptr, nullptr };
    int frameCount = backtrace(frames, 2);
    CxaThrowType original = nullptr;
    if (frameCount == 2) {
        Dl_info callerInfo;
        if (dladdr(frames[1], &callerInfo) && callerInfo.dli_fbase != nullptr) {
            original = originalCxaThrowForImage(callerInfo.dli_fbase);
        }
    }
    if (original == nullptr) {
        original = lookupOriginalCxaThrow();
    }
    original(thrown_exception, tinfo, dest);
    __builtin_unreachable();
}

static bool makeWritable(void* address, size_t size, int* oldProtection) {
    if (address == nullptr || size == 0) {
        return false;
    }

    vm_address_t region = (vm_address_t)address;
    vm_size_t regionSize = 0;
    mach_port_t objectName = MACH_PORT_NULL;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t infoCount = VM_REGION_BASIC_INFO_COUNT_64;
    kern_return_t kr = vm_region_64(mach_task_self(), &region, &regionSize, VM_REGION_BASIC_INFO_64,
                                    (vm_region_info_t)&info, &infoCount, &objectName);
    if (kr == KERN_SUCCESS) {
        *oldProtection = info.protection;
    } else {
        *oldProtection = PROT_READ;
    }

    long pageSize = sysconf(_SC_PAGESIZE);
    uintptr_t start = (uintptr_t)address & ~((uintptr_t)pageSize - 1);
    uintptr_t end = ((uintptr_t)address + size + (uintptr_t)pageSize - 1) & ~((uintptr_t)pageSize - 1);
    return mprotect((void*)start, end - start, PROT_READ | PROT_WRITE) == 0;
}

static void restoreProtection(void* address, size_t size, int oldProtection) {
    long pageSize = sysconf(_SC_PAGESIZE);
    uintptr_t start = (uintptr_t)address & ~((uintptr_t)pageSize - 1);
    uintptr_t end = ((uintptr_t)address + size + (uintptr_t)pageSize - 1) & ~((uintptr_t)pageSize - 1);

    int protection = 0;
    if (oldProtection & VM_PROT_READ) {
        protection |= PROT_READ;
    }
    if (oldProtection & VM_PROT_WRITE) {
        protection |= PROT_WRITE;
    }
    if (oldProtection & VM_PROT_EXECUTE) {
        protection |= PROT_EXEC;
    }
    if (protection == 0) {
        protection = PROT_READ;
    }
    mprotect((void*)start, end - start, protection);
}

static const struct segment_command_64* findSegment64(const struct mach_header_64* header, const char* name) {
    if (header == nullptr) {
        return nullptr;
    }
    const uint8_t* cursor = (const uint8_t*)header + sizeof(struct mach_header_64);
    for (uint32_t i = 0; i < header->ncmds; ++i) {
        const struct load_command* command = (const struct load_command*)cursor;
        if (command->cmd == LC_SEGMENT_64) {
            const struct segment_command_64* segment = (const struct segment_command_64*)cursor;
            if (strncmp(segment->segname, name, sizeof(segment->segname)) == 0) {
                return segment;
            }
        }
        cursor += command->cmdsize;
    }
    return nullptr;
}

static const struct load_command* findCommand(const struct mach_header_64* header, uint32_t commandType) {
    if (header == nullptr) {
        return nullptr;
    }
    const uint8_t* cursor = (const uint8_t*)header + sizeof(struct mach_header_64);
    for (uint32_t i = 0; i < header->ncmds; ++i) {
        const struct load_command* command = (const struct load_command*)cursor;
        if (command->cmd == commandType) {
            return command;
        }
        cursor += command->cmdsize;
    }
    return nullptr;
}

static void rebindSection(const struct section_64* section,
                          intptr_t slide,
                          const void* imageBase,
                          struct nlist_64* symbolTable,
                          char* stringTable,
                          uint32_t* indirectSymbolTable) {
    if (section == nullptr || symbolTable == nullptr || stringTable == nullptr || indirectSymbolTable == nullptr) {
        return;
    }

    uint32_t* indirectSymbolIndexes = indirectSymbolTable + section->reserved1;
    void** bindings = (void**)((uintptr_t)slide + section->addr);
    int oldProtection = PROT_READ;
    bool changedProtection = makeWritable(bindings, section->size, &oldProtection);
    if (!changedProtection) {
        return;
    }

    for (uint32_t i = 0; i < section->size / sizeof(void*); ++i) {
        uint32_t symbolIndex = indirectSymbolIndexes[i];
        if (symbolIndex == INDIRECT_SYMBOL_ABS ||
            symbolIndex == INDIRECT_SYMBOL_LOCAL ||
            symbolIndex == (INDIRECT_SYMBOL_LOCAL | INDIRECT_SYMBOL_ABS)) {
            continue;
        }

        const char* symbolName = stringTable + symbolTable[symbolIndex].n_un.n_strx;
        if (symbolName[0] == '_' && strcmp(symbolName + 1, "__cxa_throw") == 0) {
            if (bindings[i] == (void*)zwMobileGuardCxaThrowDecorator) {
                continue;
            }
            rememberOriginalCxaThrow(imageBase, (CxaThrowType)bindings[i]);
            bindings[i] = (void*)zwMobileGuardCxaThrowDecorator;
        }
    }

    restoreProtection(bindings, section->size, oldProtection);
}

static void rebindSegmentSections(const struct segment_command_64* segment,
                                  intptr_t slide,
                                  const void* imageBase,
                                  struct nlist_64* symbolTable,
                                  char* stringTable,
                                  uint32_t* indirectSymbolTable) {
    if (segment == nullptr) {
        return;
    }

    const struct section_64* section = (const struct section_64*)((const uint8_t*)segment + sizeof(*segment));
    for (uint32_t i = 0; i < segment->nsects; ++i, ++section) {
        uint32_t sectionType = section->flags & SECTION_TYPE;
        if (sectionType == S_LAZY_SYMBOL_POINTERS || sectionType == S_NON_LAZY_SYMBOL_POINTERS) {
            rebindSection(section, slide, imageBase, symbolTable, stringTable, indirectSymbolTable);
        }
    }
}

static void rebindImage(const struct mach_header* rawHeader, intptr_t slide) {
    if (rawHeader == nullptr || rawHeader->magic != MH_MAGIC_64) {
        return;
    }

    const struct mach_header_64* header = (const struct mach_header_64*)rawHeader;
    const struct symtab_command* symtabCommand = (const struct symtab_command*)findCommand(header, LC_SYMTAB);
    const struct dysymtab_command* dysymtabCommand = (const struct dysymtab_command*)findCommand(header, LC_DYSYMTAB);
    const struct segment_command_64* linkeditSegment = findSegment64(header, SEG_LINKEDIT);
    if (symtabCommand == nullptr || dysymtabCommand == nullptr || linkeditSegment == nullptr) {
        return;
    }

    uintptr_t linkeditBase = (uintptr_t)slide + linkeditSegment->vmaddr - linkeditSegment->fileoff;
    struct nlist_64* symbolTable = (struct nlist_64*)(linkeditBase + symtabCommand->symoff);
    char* stringTable = (char*)(linkeditBase + symtabCommand->stroff);
    uint32_t* indirectSymbolTable = (uint32_t*)(linkeditBase + dysymtabCommand->indirectsymoff);

    rebindSegmentSections(findSegment64(header, SEG_DATA), slide, header, symbolTable, stringTable, indirectSymbolTable);
    rebindSegmentSections(findSegment64(header, "__DATA_CONST"), slide, header, symbolTable, stringTable, indirectSymbolTable);
}

#pragma mark - 动态链接HOOK,拦截并替换所有加载的动态库
extern "C" void zwMobileGuardInstallCxaThrowHook(void) {
    pthread_mutex_lock(&g_cxaHookMutex);
    if (!g_cxaThrowRebindingInstalled) {
        g_cxaThrowRebindingInstalled = true;
        // 注册 dyld 镜像加载回调函数
        _dyld_register_func_for_add_image(rebindImage);
    }
    pthread_mutex_unlock(&g_cxaHookMutex);
}

#pragma mark - 静态链接时自动拦截 __cxa_throw
// 用 weak 避免和其他库符号冲突编译报错
extern "C" __attribute__((weak, visibility("default")))
void __cxa_throw(void *thrown_exception, std::type_info *tinfo, void (*dest)(void *)) {
    // 此时栈还没有展开，在此处获取调用栈
    zwMobileGuardRecordThrowState(thrown_exception, tinfo);

    // 获取系统的原始 __cxa_throw
    static CxaThrowType originalCxaThrow = lookupOriginalCxaThrow();
    
    // 转发原始异常
    originalCxaThrow(thrown_exception, tinfo, dest);
    // 因为 __cxa_throw 是 noreturn，标记为不可达
    __builtin_unreachable();
}

// 异常重抛，预留扩展，该函数和 __cxa_throw关系紧密，一起重写，避免在安卓等静态链接时符号拉取冲突报错
extern "C" __attribute__((weak, visibility("default")))
void __cxa_rethrow() {
    static CxaRethrowType original_cxa_rethrow = lookupOriginalCxaRethrow();
    
    // 转发重抛
    original_cxa_rethrow();
    __builtin_unreachable();
}

#include "ZWMobileGuardReportJSON.h"

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach/machine.h>
#include <uuid/uuid.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#ifdef __APPLE__
static uintptr_t firstCmdAfterHeader(const struct mach_header *const header) {
    switch (header->magic) {
        case MH_MAGIC:
        case MH_CIGAM:
            return (uintptr_t)(header + 1);
        case MH_MAGIC_64:
        case MH_CIGAM_64:
            return (uintptr_t)(((struct mach_header_64 *)header) + 1);
        default:
            return 0;
    }
}

// 参照 kscrash 获取动态库信息
static void parseMachHeader(const struct mach_header *header, intptr_t slide, ZWRawBinaryImage *image) {
    if (header == nullptr || image == nullptr) {
        return;
    }

    uintptr_t cmdPtr = firstCmdAfterHeader(header);
    if (cmdPtr == 0) {
        return;
    }

    uint64_t imageSize = 0;

    for (uint32_t i = 0; i < header->ncmds; ++i) {
        struct load_command *command = (struct load_command *)cmdPtr;
        // 设置uuid
        if (command->cmd == LC_UUID) {
            struct uuid_command *uuidCommand = (struct uuid_command *)cmdPtr;
            uuid_unparse_upper(uuidCommand->uuid, image->uuid);
        } else if (command->cmd == LC_SEGMENT_64) {
            struct segment_command_64 *segment = (struct segment_command_64 *)cmdPtr;
            // 参照 KSCrash 的做法：仅以 __TEXT 段大小作为二进制文件的 size
            if (strcmp(segment->segname, SEG_TEXT) == 0) {
                imageSize = segment->vmsize;
            }
        } else if (command->cmd == LC_SEGMENT) {
            struct segment_command *segment = (struct segment_command *)cmdPtr;
            if (strcmp(segment->segname, SEG_TEXT) == 0) {
                imageSize = segment->vmsize;
            }
        }
        if (command->cmdsize == 0) {
            break;
        }
        cmdPtr += command->cmdsize;
    }

    // 参照 KSCrash 的做法：加载地址直接用 header 头部指针
    image->base = (uintptr_t)header;
    image->size = imageSize;

    // 动态库架构设置
    switch (header->cputype) {
        case CPU_TYPE_ARM64:
            snprintf(image->arch, sizeof(image->arch), "%s", "arm64");
            break;
        case CPU_TYPE_X86_64:
            snprintf(image->arch, sizeof(image->arch), "%s", "x86_64");
            break;
        default:
            snprintf(image->arch, sizeof(image->arch), "%s", "unknown");
            break;
    }
}

// 后两段路径
static std::string lastTwoPathComponents(const char *path) {
    if (path == nullptr) {
        return "";
    }

    std::string fullPath(path);
    const size_t lastSlash = fullPath.find_last_of('/');
    // 没有 / 使用全路径
    if (lastSlash == std::string::npos) {
        return fullPath;
    }
    // 只有一个 / 在起始位置，返回去掉 / 的字符
    if (lastSlash == 0) {
        return fullPath.substr(1);
    }
    // 从开始检索到最后一个/前，找第二个斜杠
    const size_t previousSlash = fullPath.find_last_of('/', lastSlash - 1);
    // 只有一个斜杠，使用全路径
    if (previousSlash == std::string::npos) {
        return fullPath;
    }

    return fullPath.substr(previousSlash + 1);
}
#endif

void zwMobileGuardRefreshBinaryImages(void) {
#ifdef __APPLE__

    if (g_binaryImages) {
        free(g_binaryImages);
    }
    // 获取准确的动态库总量
    uint32_t imageCount = _dyld_image_count();
    g_binaryImages = (ZWRawBinaryImage *)calloc(imageCount, sizeof(ZWRawBinaryImage));
    g_binaryImageCount = imageCount;

    for (uint32_t i = 0; i < imageCount; ++i) {
        ZWRawBinaryImage *image = &g_binaryImages[i];
        // 设置基地址、大小、uuid等信息
        parseMachHeader(_dyld_get_image_header(i), _dyld_get_image_vmaddr_slide(i), image);
        const char *path = _dyld_get_image_name(i);
        // 截取后两段为路径
        const std::string shortPath = lastTwoPathComponents(path);
        snprintf(image->path, sizeof(image->path), "%s", shortPath.c_str());
        // 从后向前查找/,路径中最后一个是名字
        const char *last = strrchr(path, '/');
        snprintf(image->name, sizeof(image->name), "%s", last ? last + 1 : path);
    }
#else
    g_binaryImageCount = 0;
#endif
}

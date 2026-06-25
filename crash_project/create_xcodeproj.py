import os

def generate_uuid(name):
    # 根据名字生成确定性的 24 字符十六进制 UUID
    import hashlib
    h = hashlib.sha1(name.encode('utf-8')).hexdigest()
    return h[:24].upper()

def create_project():
    proj_dir = "/Users/kwb/Downloads/开题报告/未命名文件夹/crash_project/demo/ios/ZWCADGuardDemo/ZWCADGuardDemo.xcodeproj"
    os.makedirs(proj_dir, exist_ok=True)
    
    # 所有的 UUID 生成
    UUID_PROJ = generate_uuid("PROJECT")
    UUID_CONTAINER = generate_uuid("CONTAINER")
    
    # Group UUIDs
    UUID_GRP_MAIN = generate_uuid("GRP_MAIN")
    UUID_GRP_SDK = generate_uuid("GRP_SDK")
    UUID_GRP_DEMO = generate_uuid("GRP_DEMO")
    UUID_GRP_PRODUCTS = generate_uuid("GRP_PRODUCTS")
    
    # SDK Files UUIDs
    UUID_FILE_DWG_H = generate_uuid("FILE_DWG_H")
    UUID_FILE_DWG_CPP = generate_uuid("FILE_DWG_CPP")
    UUID_FILE_BACK_H = generate_uuid("FILE_BACK_H")
    UUID_FILE_BACK_CPP = generate_uuid("FILE_BACK_CPP")
    UUID_FILE_GUARD_H = generate_uuid("FILE_GUARD_H")
    UUID_FILE_GUARD_MM = generate_uuid("FILE_GUARD_MM")
    UUID_FILE_HOOK_CPP = generate_uuid("FILE_HOOK_CPP")
    
    # Demo Files UUIDs
    UUID_FILE_MAIN = generate_uuid("FILE_MAIN")
    UUID_FILE_APP_H = generate_uuid("FILE_APP_H")
    UUID_FILE_APP_M = generate_uuid("FILE_APP_M")
    UUID_FILE_SCENE_H = generate_uuid("FILE_SCENE_H")
    UUID_FILE_SCENE_M = generate_uuid("FILE_SCENE_M")
    UUID_FILE_VIEW_H = generate_uuid("FILE_VIEW_H")
    UUID_FILE_VIEW_M = generate_uuid("FILE_VIEW_M")
    UUID_FILE_PLIST = generate_uuid("FILE_PLIST")
    
    # Products UUIDs
    UUID_PROD_LIB = generate_uuid("PROD_LIB")
    UUID_PROD_APP = generate_uuid("PROD_APP")
    
    # Build Configuration List UUIDs
    UUID_CONFIG_LIST_PROJ = generate_uuid("CONFIG_LIST_PROJ")
    UUID_CONFIG_LIST_LIB = generate_uuid("CONFIG_LIST_LIB")
    UUID_CONFIG_LIST_APP = generate_uuid("CONFIG_LIST_APP")
    
    # Build Configurations
    UUID_CONFIG_PROJ_DEBUG = generate_uuid("CONFIG_PROJ_DEBUG")
    UUID_CONFIG_PROJ_RELEASE = generate_uuid("CONFIG_PROJ_RELEASE")
    UUID_CONFIG_LIB_DEBUG = generate_uuid("CONFIG_LIB_DEBUG")
    UUID_CONFIG_LIB_RELEASE = generate_uuid("CONFIG_LIB_RELEASE")
    UUID_CONFIG_APP_DEBUG = generate_uuid("CONFIG_APP_DEBUG")
    UUID_CONFIG_APP_RELEASE = generate_uuid("CONFIG_APP_RELEASE")
    
    # Targets UUIDs
    UUID_TARGET_LIB = generate_uuid("TARGET_LIB")
    UUID_TARGET_APP = generate_uuid("TARGET_APP")
    
    # Build Phases UUIDs (Library)
    UUID_PHASE_LIB_SOURCES = generate_uuid("PHASE_LIB_SOURCES")
    UUID_PHASE_LIB_HEADERS = generate_uuid("PHASE_LIB_HEADERS")
    
    # Build Phases UUIDs (App)
    UUID_PHASE_APP_SOURCES = generate_uuid("PHASE_APP_SOURCES")
    UUID_PHASE_APP_FRAMEWORKS = generate_uuid("PHASE_APP_FRAMEWORKS")
    UUID_PHASE_APP_RESOURCES = generate_uuid("PHASE_APP_RESOURCES")
    
    # Build File References UUIDs
    UUID_BUILD_FILE_DWG_CPP = generate_uuid("BUILD_FILE_DWG_CPP")
    UUID_BUILD_FILE_BACK_CPP = generate_uuid("BUILD_FILE_BACK_CPP")
    UUID_BUILD_FILE_GUARD_MM = generate_uuid("BUILD_FILE_GUARD_MM")
    UUID_BUILD_FILE_HOOK_CPP = generate_uuid("BUILD_FILE_HOOK_CPP")
    
    UUID_BUILD_FILE_MAIN = generate_uuid("BUILD_FILE_MAIN")
    UUID_BUILD_FILE_APP_M = generate_uuid("BUILD_FILE_APP_M")
    UUID_BUILD_FILE_SCENE_M = generate_uuid("BUILD_FILE_SCENE_M")
    UUID_BUILD_FILE_VIEW_M = generate_uuid("BUILD_FILE_VIEW_M")
    UUID_BUILD_FILE_LIB = generate_uuid("BUILD_FILE_LIB")
    
    # Target Dependency & Container Item Proxy
    UUID_TARGET_DEP = generate_uuid("TARGET_DEP")
    UUID_CONTAINER_PROXY = generate_uuid("CONTAINER_PROXY")

    content = f"""// !$*UTF8*$!
{{
	archiveVersion = 1;
	classes = {{
	}};
	objectVersion = 50;
	objects = {{

/* Begin PBXBuildFile section */
		{UUID_BUILD_FILE_DWG_CPP} /* dwg_guard.cpp in Sources */ = {{isa = PBXBuildFile; fileRef = {UUID_FILE_DWG_CPP} /* dwg_guard.cpp */; }};
		{UUID_BUILD_FILE_BACK_CPP} /* dwg_guard_backtrace.cpp in Sources */ = {{isa = PBXBuildFile; fileRef = {UUID_FILE_BACK_CPP} /* dwg_guard_backtrace.cpp */; }};
		{UUID_BUILD_FILE_GUARD_MM} /* ZWCADGuard.mm in Sources */ = {{isa = PBXBuildFile; fileRef = {UUID_FILE_GUARD_MM} /* ZWCADGuard.mm */; }};
		{UUID_BUILD_FILE_HOOK_CPP} /* cxa_throw_hook.cpp in Sources */ = {{isa = PBXBuildFile; fileRef = {UUID_FILE_HOOK_CPP} /* cxa_throw_hook.cpp */; }};
		{UUID_BUILD_FILE_MAIN} /* main.m in Sources */ = {{isa = PBXBuildFile; fileRef = {UUID_FILE_MAIN} /* main.m */; }};
		{UUID_BUILD_FILE_APP_M} /* AppDelegate.m in Sources */ = {{isa = PBXBuildFile; fileRef = {UUID_FILE_APP_M} /* AppDelegate.m */; }};
		{UUID_BUILD_FILE_SCENE_M} /* SceneDelegate.m in Sources */ = {{isa = PBXBuildFile; fileRef = {UUID_FILE_SCENE_M} /* SceneDelegate.m */; }};
		{UUID_BUILD_FILE_VIEW_M} /* ViewController.m in Sources */ = {{isa = PBXBuildFile; fileRef = {UUID_FILE_VIEW_M} /* ViewController.m */; }};
		{UUID_BUILD_FILE_LIB} /* libZWCADGuard.a in Frameworks */ = {{isa = PBXBuildFile; fileRef = {UUID_PROD_LIB} /* libZWCADGuard.a */; }};
/* End PBXBuildFile section */

/* Begin PBXContainerItemProxy section */
		{UUID_CONTAINER_PROXY} /* PBXContainerItemProxy */ = {{
			isa = PBXContainerItemProxy;
			containerPortal = {UUID_CONTAINER} /* Project object */;
			proxyType = 1;
			remoteGlobalIDString = {UUID_TARGET_LIB};
			remoteInfo = ZWCADGuard;
		}};
/* End PBXContainerItemProxy section */

/* Begin PBXFileReference section */
		{UUID_FILE_DWG_H} /* dwg_guard.h */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.c.h; name = dwg_guard.h; path = ../../../sdk/core/dwg_guard.h; sourceTree = "<group>"; }};
		{UUID_FILE_DWG_CPP} /* dwg_guard.cpp */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.cpp.cpp; name = dwg_guard.cpp; path = ../../../sdk/core/dwg_guard.cpp; sourceTree = "<group>"; }};
		{UUID_FILE_BACK_H} /* dwg_guard_backtrace.h */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.c.h; name = dwg_guard_backtrace.h; path = ../../../sdk/core/dwg_guard_backtrace.h; sourceTree = "<group>"; }};
		{UUID_FILE_BACK_CPP} /* dwg_guard_backtrace.cpp */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.cpp.cpp; name = dwg_guard_backtrace.cpp; path = ../../../sdk/core/dwg_guard_backtrace.cpp; sourceTree = "<group>"; }};
		{UUID_FILE_GUARD_H} /* ZWCADGuard.h */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.c.h; name = ZWCADGuard.h; path = ../../../sdk/ios/ZWCADGuard.h; sourceTree = "<group>"; }};
		{UUID_FILE_GUARD_MM} /* ZWCADGuard.mm */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.cpp.objcpp; name = ZWCADGuard.mm; path = ../../../sdk/ios/ZWCADGuard.mm; sourceTree = "<group>"; }};
		{UUID_FILE_HOOK_CPP} /* cxa_throw_hook.cpp */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.cpp.cpp; name = cxa_throw_hook.cpp; path = ../../../sdk/ios/cxa_throw_hook.cpp; sourceTree = "<group>"; }};
		
		{UUID_FILE_MAIN} /* main.m */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.c.objc; path = main.m; sourceTree = "<group>"; }};
		{UUID_FILE_APP_H} /* AppDelegate.h */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.c.h; path = AppDelegate.h; sourceTree = "<group>"; }};
		{UUID_FILE_APP_M} /* AppDelegate.m */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.c.objc; path = AppDelegate.m; sourceTree = "<group>"; }};
		{UUID_FILE_SCENE_H} /* SceneDelegate.h */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.c.h; path = SceneDelegate.h; sourceTree = "<group>"; }};
		{UUID_FILE_SCENE_M} /* SceneDelegate.m */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.c.objc; path = SceneDelegate.m; sourceTree = "<group>"; }};
		{UUID_FILE_VIEW_H} /* ViewController.h */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.c.h; path = ViewController.h; sourceTree = "<group>"; }};
		{UUID_FILE_VIEW_M} /* ViewController.m */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.cpp.objcpp; path = ViewController.m; sourceTree = "<group>"; }};
		{UUID_FILE_PLIST} /* Info.plist */ = {{isa = PBXFileReference; lastKnownFileType = text.plist.xml; path = Info.plist; sourceTree = "<group>"; }};
		
		{UUID_PROD_LIB} /* libZWCADGuard.a */ = {{isa = PBXFileReference; explicitFileType = archive.ar; includeInIndex = 0; path = libZWCADGuard.a; sourceTree = BUILT_PRODUCTS_DIR; }};
		{UUID_PROD_APP} /* ZWCADGuardDemo.app */ = {{isa = PBXFileReference; explicitFileType = wrapper.application; includeInIndex = 0; path = ZWCADGuardDemo.app; sourceTree = BUILT_PRODUCTS_DIR; }};
/* End PBXFileReference section */

/* Begin PBXFrameworksBuildPhase section */
		{UUID_PHASE_APP_FRAMEWORKS} /* Frameworks */ = {{
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
				{UUID_BUILD_FILE_LIB} /* libZWCADGuard.a in Frameworks */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		}};
/* End PBXFrameworksBuildPhase section */

/* Begin PBXGroup section */
		{UUID_GRP_MAIN} = {{
			isa = PBXGroup;
			children = (
				{UUID_GRP_SDK} /* SDK */,
				{UUID_GRP_DEMO} /* ZWCADGuardDemo */,
				{UUID_GRP_PRODUCTS} /* Products */,
			);
			sourceTree = "<group>";
		}};
		{UUID_GRP_SDK} = {{
			isa = PBXGroup;
			name = SDK;
			children = (
				{UUID_FILE_DWG_H} /* dwg_guard.h */,
				{UUID_FILE_DWG_CPP} /* dwg_guard.cpp */,
				{UUID_FILE_BACK_H} /* dwg_guard_backtrace.h */,
				{UUID_FILE_BACK_CPP} /* dwg_guard_backtrace.cpp */,
				{UUID_FILE_GUARD_H} /* ZWCADGuard.h */,
				{UUID_FILE_GUARD_MM} /* ZWCADGuard.mm */,
				{UUID_FILE_HOOK_CPP} /* cxa_throw_hook.cpp */,
			);
			sourceTree = "<group>";
		}};
		{UUID_GRP_DEMO} = {{
			isa = PBXGroup;
			name = ZWCADGuardDemo;
			path = ZWCADGuardDemo;
			children = (
				{UUID_FILE_MAIN} /* main.m */,
				{UUID_FILE_APP_H} /* AppDelegate.h */,
				{UUID_FILE_APP_M} /* AppDelegate.m */,
				{UUID_FILE_SCENE_H} /* SceneDelegate.h */,
				{UUID_FILE_SCENE_M} /* SceneDelegate.m */,
				{UUID_FILE_VIEW_H} /* ViewController.h */,
				{UUID_FILE_VIEW_M} /* ViewController.m */,
				{UUID_FILE_PLIST} /* Info.plist */,
			);
			sourceTree = "<group>";
		}};
		{UUID_GRP_PRODUCTS} = {{
			isa = PBXGroup;
			name = Products;
			children = (
				{UUID_PROD_LIB} /* libZWCADGuard.a */,
				{UUID_PROD_APP} /* ZWCADGuardDemo.app */,
			);
			sourceTree = "<group>";
		}};
/* End PBXGroup section */

/* Begin PBXHeadersBuildPhase section */
		{UUID_PHASE_LIB_HEADERS} /* Headers */ = {{
			isa = PBXHeadersBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 0;
		}};
/* End PBXHeadersBuildPhase section */

/* Begin PBXNativeTarget section */
		{UUID_TARGET_LIB} /* ZWCADGuard */ = {{
			isa = PBXNativeTarget;
			buildConfigurationList = {UUID_CONFIG_LIST_LIB} /* Build configuration list for PBXNativeTarget "ZWCADGuard" */;
			buildPhases = (
				{UUID_PHASE_LIB_HEADERS} /* Headers */,
				{UUID_PHASE_LIB_SOURCES} /* Sources */,
			);
			buildRules = (
			);
			dependencies = (
			);
			name = ZWCADGuard;
			productName = ZWCADGuard;
			productReference = {UUID_PROD_LIB} /* libZWCADGuard.a */;
			productType = "com.apple.product-type.library.static";
		}};
		{UUID_TARGET_APP} /* ZWCADGuardDemo */ = {{
			isa = PBXNativeTarget;
			buildConfigurationList = {UUID_CONFIG_LIST_APP} /* Build configuration list for PBXNativeTarget "ZWCADGuardDemo" */;
			buildPhases = (
				{UUID_PHASE_APP_SOURCES} /* Sources */,
				{UUID_PHASE_APP_FRAMEWORKS} /* Frameworks */,
				{UUID_PHASE_APP_RESOURCES} /* Resources */,
			);
			buildRules = (
			);
			dependencies = (
				{UUID_TARGET_DEP} /* PBXTargetDependency */,
			);
			name = ZWCADGuardDemo;
			productName = ZWCADGuardDemo;
			productReference = {UUID_PROD_APP} /* ZWCADGuardDemo.app */;
			productType = "com.apple.product-type.application";
		}};
/* End PBXNativeTarget section */

/* Begin PBXProject section */
		{UUID_CONTAINER} /* Project object */ = {{
			isa = PBXProject;
			attributes = {{
				LastUpgradeCheck = 1100;
				TargetAttributes = {{
					{UUID_TARGET_LIB} = {{
						CreatedOnToolsVersion = 11.0;
						ProvisioningStyle = Automatic;
					}};
					{UUID_TARGET_APP} = {{
						CreatedOnToolsVersion = 11.0;
						ProvisioningStyle = Automatic;
					}};
				}};
			}};
			buildConfigurationList = {UUID_CONFIG_LIST_PROJ} /* Build configuration list for PBXProject "ZWCADGuardDemo" */;
			compatibilityVersion = "Xcode 9.3";
			developmentRegion = en;
			hasScannedForEncodings = 0;
			knownRegions = (
				en,
				Base,
			);
			mainGroup = {UUID_GRP_MAIN};
			productRefGroup = {UUID_GRP_PRODUCTS};
			projectDirPath = "";
			projectRoot = "";
			targets = (
				{UUID_TARGET_LIB} /* ZWCADGuard */,
				{UUID_TARGET_APP} /* ZWCADGuardDemo */,
			);
		}};
/* End PBXProject section */

/* Begin PBXResourcesBuildPhase section */
		{UUID_PHASE_APP_RESOURCES} /* Resources */ = {{
			isa = PBXResourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 0;
		}};
/* End PBXResourcesBuildPhase section */

/* Begin PBXSourcesBuildPhase section */
		{UUID_PHASE_LIB_SOURCES} /* Sources */ = {{
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
				{UUID_BUILD_FILE_DWG_CPP} /* dwg_guard.cpp in Sources */,
				{UUID_BUILD_FILE_BACK_CPP} /* dwg_guard_backtrace.cpp in Sources */,
				{UUID_BUILD_FILE_GUARD_MM} /* ZWCADGuard.mm in Sources */,
				{UUID_BUILD_FILE_HOOK_CPP} /* cxa_throw_hook.cpp in Sources */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		}};
		{UUID_PHASE_APP_SOURCES} /* Sources */ = {{
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
				{UUID_BUILD_FILE_MAIN} /* main.m in Sources */,
				{UUID_BUILD_FILE_APP_M} /* AppDelegate.m in Sources */,
				{UUID_BUILD_FILE_SCENE_M} /* SceneDelegate.m in Sources */,
				{UUID_BUILD_FILE_VIEW_M} /* ViewController.m in Sources */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		}};
/* End PBXSourcesBuildPhase section */

/* Begin PBXTargetDependency section */
		{UUID_TARGET_DEP} /* PBXTargetDependency */ = {{
			isa = PBXTargetDependency;
			target = {UUID_TARGET_LIB} /* ZWCADGuard */;
			targetProxy = {UUID_CONTAINER_PROXY} /* PBXContainerItemProxy */;
		}};
/* End PBXTargetDependency section */

/* Begin XCBuildConfiguration section */
		{UUID_CONFIG_PROJ_DEBUG} /* Debug */ = {{
			isa = XCBuildConfiguration;
			buildSettings = {{
				ALWAYS_SEARCH_USER_PATHS = NO;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "gnu++17";
				CLANG_CXX_LIBRARY = "libc++";
				CLANG_ENABLE_MODULES = YES;
				CLANG_ENABLE_OBJC_ARC = YES;
				CLANG_ENABLE_OBJC_WEAK = YES;
				CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
				CLANG_WARN_BOOL_CONVERSION = YES;
				CLANG_WARN_COMMA = YES;
				CLANG_WARN_CONSTANT_CONVERSION = YES;
				CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
				CLANG_WARN_DIRECT_OBJC_REPRODUCER = YES_ERROR;
				CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
				CLANG_WARN_EMPTY_BODY = YES;
				CLANG_WARN_ENUM_CONVERSION = YES;
				CLANG_WARN_INFINITE_RECURSION = YES;
				CLANG_WARN_INT_CONVERSION = YES;
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
				CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
				CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
				CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
				CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
				CLANG_WARN_STRICT_PROTOTYPES = YES;
				CLANG_WARN_SUSPICIOUS_MOVE = YES;
				CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
				CLANG_WARN_UNREACHABLE_CODE = YES;
				COPY_PHASE_STRIP = NO;
				DEBUG_INFORMATION_FORMAT = dwarf;
				ENABLE_STRICT_OBJC_MSGSEND = YES;
				ENABLE_TESTABILITY = YES;
				GCC_C_LANGUAGE_STANDARD = gnu11;
				GCC_DYNAMIC_NO_PIC = NO;
				GCC_NO_COMMON_BLOCKS = YES;
				GCC_OPTIMIZATION_LEVEL = 0;
				GCC_PREPROCESSOR_DEFINITIONS = (
					"DEBUG=1",
					"$(inherited)",
				);
				GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
				GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
				GCC_WARN_UNDECLARED_SELECTOR = YES;
				GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
				GCC_WARN_UNUSED_FUNCTION = YES;
				GCC_WARN_UNUSED_VARIABLE = YES;
				IPHONEOS_DEPLOYMENT_TARGET = 13.0;
				MTL_ENABLE_DEBUG_INFO = INCLUDE_SOURCE;
				MTL_FAST_MATH = YES;
				ONLY_ACTIVE_ARCH = YES;
				SDKROOT = iphoneos;
			}};
			name = Debug;
		}};
		{UUID_CONFIG_PROJ_RELEASE} /* Release */ = {{
			isa = XCBuildConfiguration;
			buildSettings = {{
				ALWAYS_SEARCH_USER_PATHS = NO;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "gnu++17";
				CLANG_CXX_LIBRARY = "libc++";
				CLANG_ENABLE_MODULES = YES;
				CLANG_ENABLE_OBJC_ARC = YES;
				CLANG_ENABLE_OBJC_WEAK = YES;
				CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
				CLANG_WARN_BOOL_CONVERSION = YES;
				CLANG_WARN_COMMA = YES;
				CLANG_WARN_CONSTANT_CONVERSION = YES;
				CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
				CLANG_WARN_DIRECT_OBJC_REPRODUCER = YES_ERROR;
				CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
				CLANG_WARN_EMPTY_BODY = YES;
				CLANG_WARN_ENUM_CONVERSION = YES;
				CLANG_WARN_INFINITE_RECURSION = YES;
				CLANG_WARN_INT_CONVERSION = YES;
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
				CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
				CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
				CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
				CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
				CLANG_WARN_STRICT_PROTOTYPES = YES;
				CLANG_WARN_SUSPICIOUS_MOVE = YES;
				CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
				CLANG_WARN_UNREACHABLE_CODE = YES;
				COPY_PHASE_STRIP = NO;
				DEBUG_INFORMATION_FORMAT = "dwarf-with-dsym";
				ENABLE_NS_ASSERTIONS = NO;
				ENABLE_STRICT_OBJC_MSGSEND = YES;
				GCC_C_LANGUAGE_STANDARD = gnu11;
				GCC_NO_COMMON_BLOCKS = YES;
				GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
				GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
				GCC_WARN_UNDECLARED_SELECTOR = YES;
				GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
				GCC_WARN_UNUSED_FUNCTION = YES;
				GCC_WARN_UNUSED_VARIABLE = YES;
				IPHONEOS_DEPLOYMENT_TARGET = 13.0;
				MTL_ENABLE_DEBUG_INFO = NO;
				MTL_FAST_MATH = YES;
				SDKROOT = iphoneos;
				VALIDATE_PRODUCT = YES;
			}};
			name = Release;
		}};
		{UUID_CONFIG_LIB_DEBUG} /* Debug */ = {{
			isa = XCBuildConfiguration;
			buildSettings = {{
				HEADER_SEARCH_PATHS = (
					"$(inherited)",
					"../../../sdk/core",
				);
				PRODUCT_NAME = ZWCADGuard;
				SKIP_INSTALL = YES;
				TARGETED_DEVICE_FAMILY = "1,2";
			}};
			name = Debug;
		}};
		{UUID_CONFIG_LIB_RELEASE} /* Release */ = {{
			isa = XCBuildConfiguration;
			buildSettings = {{
				HEADER_SEARCH_PATHS = (
					"$(inherited)",
					"../../../sdk/core",
				);
				PRODUCT_NAME = ZWCADGuard;
				SKIP_INSTALL = YES;
				TARGETED_DEVICE_FAMILY = "1,2";
			}};
			name = Release;
		}};
		{UUID_CONFIG_APP_DEBUG} /* Debug */ = {{
			isa = XCBuildConfiguration;
			buildSettings = {{
				ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
				CODE_SIGN_IDENTITY = "iPhone Developer";
				CODE_SIGN_STYLE = Automatic;
				HEADER_SEARCH_PATHS = (
					"$(inherited)",
					"../../../sdk/core",
					"../../../sdk/ios",
				);
				INFOPLIST_FILE = ZWCADGuardDemo/Info.plist;
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
				);
				PRODUCT_BUNDLE_IDENTIFIER = com.zwsoft.zwcadguard.ZWCADGuardDemo;
				PRODUCT_NAME = "$(TARGET_NAME)";
				TARGETED_DEVICE_FAMILY = "1,2";
			}};
			name = Debug;
		}};
		{UUID_CONFIG_APP_RELEASE} /* Release */ = {{
			isa = XCBuildConfiguration;
			buildSettings = {{
				ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
				CODE_SIGN_IDENTITY = "iPhone Distribution";
				CODE_SIGN_STYLE = Automatic;
				HEADER_SEARCH_PATHS = (
					"$(inherited)",
					"../../../sdk/core",
					"../../../sdk/ios",
				);
				INFOPLIST_FILE = ZWCADGuardDemo/Info.plist;
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
				);
				PRODUCT_BUNDLE_IDENTIFIER = com.zwsoft.zwcadguard.ZWCADGuardDemo;
				PRODUCT_NAME = "$(TARGET_NAME)";
				TARGETED_DEVICE_FAMILY = "1,2";
			}};
			name = Release;
		}};
/* End XCBuildConfiguration section */

/* Begin XCConfigurationList section */
		{UUID_CONFIG_LIST_PROJ} /* Build configuration list for PBXProject "ZWCADGuardDemo" */ = {{
			isa = XCConfigurationList;
			buildConfigurations = (
				{UUID_CONFIG_PROJ_DEBUG} /* Debug */,
				{UUID_CONFIG_PROJ_RELEASE} /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		}};
		{UUID_CONFIG_LIST_LIB} /* Build configuration list for PBXNativeTarget "ZWCADGuard" */ = {{
			isa = XCConfigurationList;
			buildConfigurations = (
				{UUID_CONFIG_LIB_DEBUG} /* Debug */,
				{UUID_CONFIG_LIB_RELEASE} /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		}};
		{UUID_CONFIG_LIST_APP} /* Build configuration list for PBXNativeTarget "ZWCADGuardDemo" */ = {{
			isa = XCConfigurationList;
			buildConfigurations = (
				{UUID_CONFIG_APP_DEBUG} /* Debug */,
				{UUID_CONFIG_APP_RELEASE} /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		}};
/* End XCConfigurationList section */
	}};
	rootObject = {UUID_CONTAINER} /* Project object */;
}}
"""
    with open(os.path.join(proj_dir, "project.pbxproj"), "w", encoding="utf-8") as f:
        f.write(content)
    print("Successfully generated project.pbxproj without storyboard at:", proj_dir)

if __name__ == '__main__':
    create_project()

#import <Foundation/Foundation.h>

USE_UTIL_NS


void getMacSystemVersion(long *pMajorVer, long *pMinorVer, long *pPatchVer) {
    if ([[NSProcessInfo processInfo] respondsToSelector:@selector(operatingSystemVersion)]) {
        NSOperatingSystemVersion sysVer = [[NSProcessInfo processInfo] operatingSystemVersion];
        if (pMajorVer) {
            *pMajorVer = sysVer.majorVersion;
        }

        if (pMinorVer) {
            *pMinorVer = sysVer.minorVersion;
        }

        if (pPatchVer) {
            *pPatchVer = sysVer.patchVersion;
        }
    }
    else {
        SInt32 systemVersion,systemVersionMajor,systemVersionMinor;
        Gestalt(gestaltSystemVersion, &systemVersion);
        Gestalt(gestaltSystemVersionMajor, &systemVersionMajor);
        Gestalt(gestaltSystemVersionMinor, &systemVersionMinor);
        if (pMajorVer) {
            *pMajorVer = systemVersionMajor;
        }

        if (pMinorVer) {
            *pMinorVer = systemVersionMinor;
        }
    }
}


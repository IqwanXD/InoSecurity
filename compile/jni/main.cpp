#include <jni.h>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <algorithm>

// --- KONFIGURASI APLIKASI ANDA (OTOMATIS DIINJECT OLEH SCRIPT) ---
const std::string EXPECTED_PACKAGE  = "INJECT_PACKAGE_HERE";
const std::string EXPECTED_APP_NAME = "INJECT_APP_NAME_HERE"; 
// INPUT SHA: Sudah otomatis dikonversi ke HURUF KAPITAL tanpa titik dua (:) oleh script
const std::string EXPECTED_SHA256   = "INJECT_SHA256_HERE";     

// --- HELPER: Deteksi Frida & Xposed di Memori ---
bool isHookDetected() {
    std::ifstream maps("/proc/self/maps");
    std::string line;
    if (maps.is_open()) {
        while (std::getline(maps, line)) {
            if (line.find("frida") != std::string::npos || line.find("xposed") != std::string::npos) {
                maps.close();
                return true;
            }
        }
        maps.close();
    }
    return false;
}

// --- HELPER: Validasi Installer Terpercaya ---
bool isTrustedInstaller(std::string installer) {
    std::transform(installer.begin(), installer.end(), installer.begin(), ::tolower);
    std::vector<std::string> trusted = {
        "google", "packageinstaller", "permissioncontroller", "samsung", 
        "miui", "xiaomi", "oppo", "realme", "vivo", "huawei", 
        "apkmirror", "split", "android.shell", "android.adb", "com.android.vending"
    };
    for (const auto& t : trusted) {
        if (installer.find(t) != std::string::npos) return true;
    }
    return false;
}

// AUTOMATION FIX: Menggunakan penanda JNI package agar fungsi ikut berganti nama secara otomatis
extern "C"
JNIEXPORT jobject JNICALL
Java_INJECT_JNI_PKG_HERE_InoSecuritySystem_nativeCheck(JNIEnv *env, jclass clazz, jobject activity_obj) {
    
    jclass arraylist_cls = env->FindClass("java/util/ArrayList");
    jmethodID arraylist_init = env->GetMethodID(arraylist_cls, "<init>", "()V");
    jobject reasons_list = env->NewObject(arraylist_cls, arraylist_init);
    jmethodID arraylist_add = env->GetMethodID(arraylist_cls, "add", "(Ljava/lang/Object;)Z");

    // 1. Cek Memory Hooking (Frida/Xposed)
    if (isHookDetected()) {
        jstring msg = env->NewStringUTF("Runtime hook detected");
        env->CallBooleanMethod(reasons_list, arraylist_add, msg);
        return reasons_list;
    }

    // Ambil Context & PackageManager
    jclass activity_cls = env->GetObjectClass(activity_obj);
    jmethodID get_pkg_name = env->GetMethodID(activity_cls, "getPackageName", "()Ljava/lang/String;");
    jstring current_pkg_jstr = (jstring)env->CallObjectMethod(activity_obj, get_pkg_name);
    
    const char *pkg_chars = env->GetStringUTFChars(current_pkg_jstr, nullptr);
    std::string current_package(pkg_chars);
    env->ReleaseStringUTFChars(current_pkg_jstr, pkg_chars);

    jmethodID get_pm = env->GetMethodID(activity_cls, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jobject pm_obj = env->CallObjectMethod(activity_obj, get_pm);
    jclass pm_cls = env->GetObjectClass(pm_obj);

    // 2. Validasi Package Name
    if (current_package != EXPECTED_PACKAGE) {
        jstring msg = env->NewStringUTF("Package modified");
        env->CallBooleanMethod(reasons_list, arraylist_add, msg);
    }

    // Ambil info versi SDK Android
    jclass build_version_cls = env->FindClass("android/os/Build$VERSION");
    jfieldID sdk_int_id = env->GetStaticFieldID(build_version_cls, "SDK_INT", "I");
    jint sdk_int = env->GetStaticIntField(build_version_cls, sdk_int_id);
    jint flags = (sdk_int >= 28) ? 134217728 : 64; // GET_SIGNING_CERTIFICATES : GET_SIGNATURES

    // Ambil PackageInfo dengan penanganan Java Exception
    jmethodID get_pkg_info = env->GetMethodID(pm_cls, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
    jobject pi_obj = env->CallObjectMethod(pm_obj, get_pkg_info, current_pkg_jstr, flags);

    if (env->ExceptionCheck()) {
        env->ExceptionClear(); 
        jstring msg = env->NewStringUTF("Security error: Package info unavailable");
        env->CallBooleanMethod(reasons_list, arraylist_add, msg);
        return reasons_list;
    }

    jclass pi_cls = env->GetObjectClass(pi_obj);

    // 3. Validasi App Name (Anti-Rename)
    jfieldID app_info_id = env->GetFieldID(pi_cls, "applicationInfo", "Landroid/content/pm/ApplicationInfo;");
    jobject ai_obj = env->CallObjectMethod(pm_obj, env->GetMethodID(pm_cls, "getApplicationLabel", "(Landroid/content/pm/ApplicationInfo;)Ljava/lang/CharSequence;"), env->GetObjectField(pi_obj, app_info_id));
    
    if (ai_obj != nullptr) {
        jclass charseq_cls = env->GetObjectClass(ai_obj);
        jstring app_name_jstr = (jstring)env->CallObjectMethod(ai_obj, env->GetMethodID(charseq_cls, "toString", "()Ljava/lang/String;"));
        
        const char *name_chars = env->GetStringUTFChars(app_name_jstr, nullptr);
        if (std::string(name_chars) != EXPECTED_APP_NAME) {
            jstring msg = env->NewStringUTF("App name modified");
            env->CallBooleanMethod(reasons_list, arraylist_add, msg);
        }
        env->ReleaseStringUTFChars(app_name_jstr, name_chars);
    }

    // 4. Validasi Debuggable Flag
    jfieldID flags_id = env->GetFieldID(env->FindClass("android/content/pm/ApplicationInfo"), "flags", "I");
    jint app_flags = env->GetIntField(env->GetObjectField(pi_obj, app_info_id), flags_id);
    if ((app_flags & 2) != 0) { // FLAG_DEBUGGABLE
        jstring msg = env->NewStringUTF("Debuggable detected");
        env->CallBooleanMethod(reasons_list, arraylist_add, msg);
    }

    // 5. Validasi Installer Package Name
    std::string installer_str = "";
    if (sdk_int >= 30) {
        jmethodID get_source_info = env->GetMethodID(pm_cls, "getInstallSourceInfo", "(Ljava/lang/String;)Landroid/content/pm/InstallSourceInfo;");
        jobject source_info_obj = env->CallObjectMethod(pm_obj, get_source_info, current_pkg_jstr);
        if (source_info_obj != nullptr) {
            jstring inst_jstr = (jstring)env->CallObjectMethod(source_info_obj, env->GetMethodID(env->GetObjectClass(source_info_obj), "getInstallingPackageName", "()Ljava/lang/String;"));
            if (inst_jstr != nullptr) {
                const char *inst_chars = env->GetStringUTFChars(inst_jstr, nullptr);
                installer_str = std::string(inst_chars);
                env->ReleaseStringUTFChars(inst_jstr, inst_chars);
            }
        }
    } else {
        jmethodID get_inst_pkg = env->GetMethodID(pm_cls, "getInstallerPackageName", "(Ljava/lang/String;)Ljava/lang/String;");
        jstring inst_jstr = (jstring)env->CallObjectMethod(pm_obj, get_inst_pkg, current_pkg_jstr);
        if (inst_jstr != nullptr) {
            const char *inst_chars = env->GetStringUTFChars(inst_jstr, nullptr);
            installer_str = std::string(inst_chars);
            env->ReleaseStringUTFChars(inst_jstr, inst_chars);
        }
    }

    if (!installer_str.empty() && !isTrustedInstaller(installer_str)) {
        jstring msg = env->NewStringUTF(("Installer not trusted: " + installer_str).c_str());
        env->CallBooleanMethod(reasons_list, arraylist_add, msg);
    }

    // 6. Validasi Signature SHA-256
    jobjectArray signatures_array = nullptr;
    if (sdk_int >= 28) {
        jfieldID signing_info_id = env->GetFieldID(pi_cls, "signingInfo", "Landroid/content/pm/SigningInfo;");
        jobject signing_info_obj = env->GetObjectField(pi_obj, signing_info_id);
        if (signing_info_obj != nullptr) {
            jclass si_cls = env->GetObjectClass(signing_info_obj);
            jmethodID has_mult = env->GetMethodID(si_cls, "hasMultipleSigners", "()Z");
            if (env->CallBooleanMethod(signing_info_obj, has_mult)) {
                signatures_array = (jobjectArray)env->CallObjectMethod(signing_info_obj, env->GetMethodID(si_cls, "getApkContentsSigners", "()[Landroid/content/pm/Signature;"));
            } else {
                signatures_array = (jobjectArray)env->CallObjectMethod(signing_info_obj, env->GetMethodID(si_cls, "getSigningCertificateHistory", "()[Landroid/content/pm/Signature;"));
            }
        }
    } else {
        jfieldID signatures_id = env->GetFieldID(pi_cls, "signatures", "[Landroid/content/pm/Signature;");
        signatures_array = (jobjectArray)env->GetObjectField(pi_obj, signatures_id);
    }

    if (signatures_array != nullptr && env->GetArrayLength(signatures_array) > 0) {
        jobject sig_obj = env->GetObjectArrayElement(signatures_array, 0);
        jclass sig_cls = env->GetObjectClass(sig_obj);
        jbyteArray cert_bytes = (jbyteArray)env->CallObjectMethod(sig_obj, env->GetMethodID(sig_cls, "toByteArray", "()[B"));

        jclass md_cls = env->FindClass("java/security/MessageDigest");
        jobject md_obj = env->CallStaticObjectMethod(md_cls, env->GetStaticMethodID(md_cls, "getInstance", "(Ljava/lang/String;)Ljava/security/MessageDigest;"), env->NewStringUTF("SHA-256"));
        jbyteArray sha_bytes = (jbyteArray)env->CallObjectMethod(md_obj, env->GetMethodID(md_cls, "digest", "([B)[B"), cert_bytes);

        jsize len = env->GetArrayLength(sha_bytes);
        jbyte* bytes = env->GetByteArrayElements(sha_bytes, nullptr);
        
        std::string current_sha = "";
        char hex_buf[3];
        for (int i = 0; i < len; ++i) {
            sprintf(hex_buf, "%02X", (unsigned char)bytes[i]);
            current_sha += hex_buf;
        }
        env->ReleaseByteArrayElements(sha_bytes, bytes, JNI_ABORT);

        if (current_sha != EXPECTED_SHA256) {
            jstring msg = env->NewStringUTF("Signature/SHA modified");
            env->CallBooleanMethod(reasons_list, arraylist_add, msg);
        }
    }

    return reasons_list;
}

# InoSecurity Native Shield (Anti-Tamper & Anti-Hook)

[![NDK Version](https://img.shields.io/badge/NDK-r29-green.svg)](https://developer.android.com/ndk)
[![Platform](https://img.shields.io/badge/Platform-Android-blue.svg)](https://developer.android.com)
[![Environment](https://img.shields.io/badge/Environment-Termux%20%2F%20Linux-orange.svg)](https://termux.dev)

Sebuah sistem keamanan tingkat *native* (C++) untuk aplikasi Android yang dirancang khusus agar dapat dikompilasi langsung di dalam **Termux** menggunakan **Android NDK r29**. Pustaka (`.so`) yang dihasilkan berfungsi untuk memproteksi aplikasi dari berbagai teknik manipulasi (*tampering*), klon, modifikasi tanda tangan (*signature*), serta deteksi *runtime hooking* secara dinamis.

---

## 🛡️ Fitur Keamanan (Security Checks)

Sistem proteksi ini berjalan di layer *native* (JNI) untuk mempersulit analisis statis maupun dinamis (*reverse engineering*):

1. **Anti-Runtime Hooking:** Memindai `/proc/self/maps` secara *real-time* untuk mendeteksi keberadaan *framework* injeksi memori populer seperti **Frida** dan **Xposed**.
2. **Validasi Nama Paket (Anti-Clone):** Memastikan nama paket (*Package Name*) aplikasi berjalan sesuai dengan manifes asli yang diharapkan.
3. **Validasi Nama Aplikasi (Anti-Rename):** Melindungi aplikasi agar tidak diubah namanya (*label*) saat didistribusikan ulang oleh pihak ketiga.
4. **Deteksi Debuggable Flag:** Memeriksa apakah aplikasi berjalan dalam mode debug (`FLAG_DEBUGGABLE`), mencegah penempelan debugger (*anti-debugging*).
5. **Validasi Sumber Penginstal (Trusted Installer):** Memeriksa integritas penginstal untuk memastikan aplikasi diunduh dari sumber terpercaya (seperti Google Play Store, Package Installer bawaan OS, ADB, dll.).
6. **Integritas Tanda Tangan (Anti-Signature Tamper):** Menghitung nilai SHA-256 dari sertifikat tanda tangan APK asli secara langsung di memori dan mencocokkannya dengan nilai yang sah.

---

## 📁 Struktur Proyek

Pastikan struktur direktori Anda di Termux diatur sebagai berikut di dalam folder kerja kompilasi Anda (misalnya `/storage/emulated/0/compile`):

```text
compile/
├── compiler.sh        # Script otomatisasi injeksi & kompilasi
└── jni/
    ├── Android.mk     # Konfigurasi modul dan flag compiler
    ├── Application.mk # Konfigurasi target arsitektur (ABI) & STL
    └── main.cpp       # Source code utama proteksi C++ (JNI)

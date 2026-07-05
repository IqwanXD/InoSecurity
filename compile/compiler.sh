#!/bin/sh

# Hentikan eksekusi jika terjadi error
set -e

NDK=~/android-ndk-r29/ndk-build
PROJECT=/storage/emulated/0/compile
TMP_OUT=~/compile_build
CPP_FILE="$PROJECT/jni/main.cpp" # Sesuaikan dengan lokasi main.cpp Anda

# --- 1. MEMINTA INPUT USER ---
echo "========================================="
echo "       INOSECURITY NATIVE COMPILER       "
echo "========================================="
printf "Masukkan Nama Package (ex: ru.ino.senseix): "
read -r INPUT_PKG
printf "Masukkan Nama Aplikasi (ex: SenseiX): "
read -r INPUT_NAME
printf "Masukkan SHA256 (Huruf kapital/kecil tanpa titik dua): "
read -r INPUT_SHA

# Validasi input tidak boleh kosong
if [ -z "$INPUT_PKG" ] || [ -z "$INPUT_NAME" ] || [ -z "$INPUT_SHA" ]; then
    echo "[-] Error: Semua input harus diisi!"
    exit 1
fi

# Normalisasi SHA ke huruf kapital murni
INPUT_SHA=$(echo "$INPUT_SHA" | tr '[:lower:]' '[:upper:]' | sed 's/://g')

# FIX AUTOMATION: Mengubah "ru.ino.senseix" menjadi "ru_ino_senseix" untuk fungsi JNI C++
INPUT_JNI_PKG=$(echo "$INPUT_PKG" | sed 's/\./_/g')

# --- 2. BACKUP & INJEKSI DATA KE C++ ---
echo "[+] Menyiapkan source code..."
if [ ! -f "$CPP_FILE" ]; then
    echo "[-] Error: File $CPP_FILE tidak ditemukan!"
    exit 1
fi

# Backup file main.cpp asli agar konfigurasi placeholder tidak hilang selamanya
cp "$CPP_FILE" "${CPP_FILE}.bak"

# Proses injeksi menggunakan sed
sed -i "s/INJECT_PACKAGE_HERE/$INPUT_PKG/g" "$CPP_FILE"
sed -i "s/INJECT_APP_NAME_HERE/$INPUT_NAME/g" "$CPP_FILE"
sed -i "s/INJECT_SHA256_HERE/$INPUT_SHA/g" "$CPP_FILE"
sed -i "s/INJECT_JNI_PKG_HERE/$INPUT_JNI_PKG/g" "$CPP_FILE"

# --- 3. PROSES COMPILATION (NDK-BUILD) ---
echo "[+] Memulai kompilasi dengan NDK..."

$NDK \
  NDK_PROJECT_PATH=$PROJECT \
  NDK_OUT=$TMP_OUT/obj \
  NDK_LIBS_OUT=$TMP_OUT/libs \
  clean

$NDK \
  NDK_PROJECT_PATH=$PROJECT \
  NDK_OUT=$TMP_OUT/obj \
  NDK_LIBS_OUT=$TMP_OUT/libs

# --- 4. PEMINDAHAN OUTPUT & CLEANUP ---
echo "[+] Memindahkan file .so ke folder output..."
mkdir -p "$PROJECT/libs/arm64-v8a"
mkdir -p "$PROJECT/libs/armeabi-v7a"

cp -r $TMP_OUT/libs/* "$PROJECT/libs/"

# Bersihkan sisa build temporer
rm -rf "$TMP_OUT"

# Kembalikan file main.cpp asli dari backup agar siap digunakan untuk compile berikutnya
mv "${CPP_FILE}.bak" "$CPP_FILE"

echo "========================================="
echo "[+] SELESAI! Hasil kompilasi ada di: $PROJECT/libs"
echo "========================================="

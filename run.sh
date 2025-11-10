#!/bin/bash

# Detect operating system
OS=$(uname -s)
AUDIO_CONFIG=""
DEBUG_FLAGS=""

if [ $# -gt 0 ] && [ "$1" = "gdb" ]; then
    DEBUG_FLAGS="-s -S"
    echo "Modo depuración GDB: habilitando flags -s -S"
    shift
fi

# Detect if running under WSL
IS_WSL=0
if grep -q "Microsoft" /proc/version 2>/dev/null || grep -q "WSL" /proc/version 2>/dev/null; then
    IS_WSL=1
    echo "Detectado Windows WSL"
fi

case $OS in
    "Darwin")
        # macOS: prefer CoreAudio and disable HVF explicitly
        echo "Aceleración por hardware deshabilitada (ejecución forzada sin HVF)"
        AUDIO_CONFIG="-audiodev coreaudio,id=speaker -machine pcspk-audiodev=speaker"
        echo "Configurando audio para macOS - usando CoreAudio"
        ;;
    "Linux")
        if [ $IS_WSL -eq 1 ]; then
            # WSL: try several fallback audio backends
            echo "Configurando audio para WSL - probando opciones compatibles"

            # Option 1: SDL (most compatible with WSL)
            AUDIO_CONFIG="-audiodev sdl,id=speaker -machine pcspk-audiodev=speaker"

            # Alternative if a PulseAudio server is configured on Windows
            # AUDIO_CONFIG="-audiodev pa,id=speaker -machine pcspk-audiodev=speaker"
        else
            # Linux host: enable KVM when possible
            KVM_FLAGS=""
            if [ -e /dev/kvm ] && [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
                KVM_FLAGS="-enable-kvm"
                echo "KVM disponible - habilitando aceleración por hardware"
            else
                echo "KVM no disponible - ejecutando sin aceleración"
            fi
            
            if command -v pulseaudio >/dev/null 2>&1; then
                AUDIO_CONFIG="-audiodev pa,id=speaker -machine pcspk-audiodev=speaker $KVM_FLAGS"
                echo "Configurando audio para Linux con PulseAudio"
            else
                AUDIO_CONFIG="-audiodev alsa,id=speaker,out.try-poll=on -machine pcspk-audiodev=speaker $KVM_FLAGS"
                echo "Configurando audio para Linux - usando ALSA"
            fi
        fi
        ;;
    CYGWIN*|MINGW*|MSYS*)
        # Windows (Cygwin/MinGW/MSYS): prefer WHPX or HAXM when available
        echo "Aceleración por hardware deshabilitada (sin WHPX/HAXM)"
        AUDIO_CONFIG="-audiodev dsound,id=speaker -machine pcspk-audiodev=speaker"
        echo "Configurando audio para Windows - usando DirectSound"
        ;;
    *)
        # Unknown host OS
        AUDIO_CONFIG="-audiodev sdl,id=speaker -machine pcspk-audiodev=speaker"
        echo "Sistema operativo no reconocido ($OS) - usando SDL"
        ;;
esac

# Launch QEMU with the computed settings
echo "Ejecutando: qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512 $AUDIO_CONFIG $DEBUG_FLAGS $*"
qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512 $AUDIO_CONFIG $DEBUG_FLAGS "$@"

# Fallback: disable audio on WSL if the primary attempt failed
if [ $? -ne 0 ] && [ $IS_WSL -eq 1 ]; then
    echo "Error con la configuración de audio. Probando alternativa sin audio específico..."
    qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512 $DEBUG_FLAGS "$@"
fi

#!/bin/bash

# Detectar el sistema operativo
OS=$(uname -s)
AUDIO_CONFIG=""

# Comprobar si estamos en WSL
IS_WSL=0
if grep -q "Microsoft" /proc/version 2>/dev/null || grep -q "WSL" /proc/version 2>/dev/null; then
    IS_WSL=1
    echo "Detectado Windows WSL"
fi

case $OS in
    "Darwin")
        # macOS - usar CoreAudio
        AUDIO_CONFIG="-audiodev coreaudio,id=speaker -machine pcspk-audiodev=speaker"
        echo "Configurando audio para macOS - usando CoreAudio"
        ;;
    "Linux")
        if [ $IS_WSL -eq 1 ]; then
            # WSL - Intentar múltiples opciones
            echo "Configurando audio para WSL - probando opciones compatibles"

            # Opción 1: SDL (más compatible con WSL)
            AUDIO_CONFIG="-audiodev sdl,id=speaker -machine pcspk-audiodev=speaker"

            # Alternativa si has configurado el servidor Pulseaudio en Windows
            # AUDIO_CONFIG="-audiodev pa,id=speaker -machine pcspk-audiodev=speaker"
        else
            # Linux nativo
            if command -v pulseaudio >/dev/null 2>&1; then
                AUDIO_CONFIG="-audiodev pa,id=speaker -machine pcspk-audiodev=speaker"
                echo "Configurando audio para Linux con PulseAudio"
            else
                AUDIO_CONFIG="-audiodev alsa,id=speaker,out.try-poll=on -machine pcspk-audiodev=speaker"
                echo "Configurando audio para Linux - usando ALSA"
            fi
        fi
        ;;
    CYGWIN*|MINGW*|MSYS*)
        # Windows nativo con Cygwin/MinGW/MSYS
        AUDIO_CONFIG="-audiodev dsound,id=speaker -machine pcspk-audiodev=speaker"
        echo "Configurando audio para Windows - usando DirectSound"
        ;;
    *)
        # Sistema desconocido
        AUDIO_CONFIG="-audiodev sdl,id=speaker -machine pcspk-audiodev=speaker"
        echo "Sistema operativo no reconocido ($OS) - usando SDL"
        ;;
esac

# Ejecutar QEMU con la configuración adecuada
echo "Ejecutando: qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512 $AUDIO_CONFIG"
qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512 $AUDIO_CONFIG

# Si lo anterior falla, probar estas alternativas:
if [ $? -ne 0 ] && [ $IS_WSL -eq 1 ]; then
    echo "Error con la configuración de audio. Probando alternativa sin audio específico..."
    qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512
fi
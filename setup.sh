#!/bin/bash

echo "========================================"
echo "  GRUPO 4 - Setup completo"
echo "========================================"
echo ""
echo "Actualizando repositorios..."
sudo apt-get update -qq

echo "Instalando dependencias..."
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
    metasploit-framework \
    swaks \
    icoutils \
    zip \
    wget \
    curl

echo ""
echo "Creando carpeta del proyecto..."
mkdir -p /home/kali/proyecto-grupo4/capturas

echo ""
echo "Copiando scripts al directorio del proyecto..."
cp generar_payload.sh /home/kali/proyecto-grupo4/
cp listener.rc /home/kali/proyecto-grupo4/
chmod +x /home/kali/proyecto-grupo4/generar_payload.sh

echo ""
echo "========================================"
echo "  Setup completado"
echo "========================================"
echo ""
echo "Todo listo. Pasos para comenzar:"
echo ""
echo "  1. Generar el payload:"
echo "     bash /home/kali/proyecto-grupo4/generar_payload.sh"
echo ""
echo "  2. Iniciar el listener:"
echo "     msfconsole -r /home/kali/proyecto-grupo4/listener.rc"
echo ""
echo "  3. Enviar el archivo a la víctima y esperar la sesión."
echo ""

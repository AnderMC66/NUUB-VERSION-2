# Guia de Deteccion Antivirus - Como Funcionan los Detectores

**Para:** Clase de Ciberseguridad
**Proyecto de referencia:** NUUB RAT v2.0 (herramienta educacional de testing autorizado)

---

## Tabla de Contenidos

1. [Visión General: Las 5 Capas de Detección](#1-visión-general)
2. [Capa 1: Firma Estática (Static Signatures)](#2-firma-estática)
3. [Capa 2: Análisis Heurístico (Heuristic Analysis)](#3-análisis-heurístico)
4. [Capa 3: Análisis de Comportamiento (Behavioral Analysis)](#4-análisis-de-comportamiento)
5. [Capa 4: Machine Learning / IA](#5-machine-learning)
6. [Capa 5: Sandbox / Análisis Dinámico](#6-sandbox)
7. [Técnicas de Evasión en el Código (y por qué se detectan)](#7-técnicas-de-evasión)
8. [MITRE ATT&CK Mapping](#8-mitre-attck)
9. [Cómo Protegerse (Defensa)](#9-defensa)

---

## 1. Visión General: Las 5 Capas de Detección

Los antivirus modernos no usan UN solo método. Usan **5 capas** superpuestas:

```
┌─────────────────────────────────────────────┐
│  Capa 5: Sandbox / Análisis Dinámico        │  ← Ejecuta en entorno controlado
├─────────────────────────────────────────────┤
│  Capa 4: Machine Learning / IA              │  ← Modelos entrenados con malware
├─────────────────────────────────────────────┤
│  Capa 3: Análisis de Comportamiento         │  ← Monitorea acciones en runtime
├─────────────────────────────────────────────┤
│  Capa 2: Análisis Heurístico                │  ← Patrones sospechosos en código
├─────────────────────────────────────────────┤
│  Capa 1: Firma Estática                     │  ← Hashes y strings conocidos
└─────────────────────────────────────────────┘
```

**Regla clave:** Si tu herramienta tiene ALGUNA combinación de estas capas que "huele a malware", será detectada. No importa cuántas técnicas de evasión agregues.

---

## 2. Capa 1: Firma Estática (Static Signatures)

### ¿Qué es?
Comparación directa del binario contra una base de datos de hashes y patrones conocidos.

### ¿Cómo funciona?
```
HASH DEL ARCHIVO (SHA256/MD5)
    ↓
Base de datos de VirusTotal / Fabricantes
    ↓
¿Match? → MALWARE DETECTADO
```

### ¿Qué detecta?

| Tipo de firma | Ejemplo |
|---------------|---------|
| **Hash** | SHA256: `a1b2c3...` = Trojan.Generic |
| **Strings** | `"EtwEventWrite"`, `"IsDebuggerPresent"`, `"NtUnmapViewOfSection"` |
| **Patrones de bytes** | `48 33 C0 C3` (xor rax,rax; ret = patch ETW) |
| **Imports sospechosos** | `VirtualAllocEx` + `WriteProcessMemory` + `ResumeThread` |

### Ejemplo real de NUUB RAT
```cpp
// EtwPatch.hpp - Este patrón de bytes es una FIRMA CONOCIDA
etw_write[0] = 0x48; // REX.W prefix
etw_write[1] = 0x33; // XOR
etw_write[2] = 0xC0; // RAX, RAX
etw_write[3] = 0xC3; // RET
```
**Los AVs conocen este patrón.** Es el "etw patch" más común del mundo.

### Nivel de detección: ~70% de los AVs
Una vez que VirusTotal tiene el hash, TODOS los AVs lo detectan en minutos.

---

## 3. Capa 2: Análisis Heurístico (Heuristic Analysis)

### ¿Qué es?
El AV analiza el binario **sin ejecutarlo** buscando patrones sospechosos.

### ¿Cómo funciona?
```python
# Pseudo-código de análisis heurístico
score = 0

# API calls sospechosas
if imports("VirtualProtect") and imports("PAGE_EXECUTE_READWRITE"):
    score += 30  # RWX memory = muy sospechoso

if imports("CreateProcess") and imports("CREATE_SUSPENDED"):
    score += 40  # Process hollowing

if imports("NtUnmapViewOfSection"):
    score += 50  # Definitivamente process hollowing

# Strings ofuscados
if has_xor_decode_loop():
    score += 20  # Ofuscación

# Anti-análisis
if imports("IsDebuggerPresent"):
    score += 15  # Anti-debug

if score > THRESHOLD:
   判定 MALWARE (Heuristic.Trojan.Generic)
```

### ¿Qué detecta en NUUB RAT?

| Técnica en el código | Score heurístico | Por qué |
|---------------------|-----------------|---------|
| `PAGE_EXECUTE_READWRITE` en `VirtualAlloc` | +30 | RWX memory es antinatural |
| `CreateProcess` + `CREATE_SUSPENDED` | +40 | Process hollowing pattern |
| `NtUnmapViewOfSection` | +50 | API solo usada por malware |
| `VirtualProtect` para parchear código | +35 | Modificación de memoria ejecutable |
| `GetModuleHandle("ntdll")` + `GetProcAddress` | +20 | Dynamic API resolution |
| Strings: `"ollydbg.exe"`, `"x64dbg.exe"` | +15 | Lista de herramientas de análisis |
| `__cpuid()` check bit 31 | +10 | Anti-VM detection |
| Registry keys: `"VMware"`, `"VirtualBox"` | +10 | Anti-VM detection |

### Nivel de detección: ~50% de los AVs

---

## 4. Capa 3: Análisis de Comportamiento (Behavioral Analysis)

### ¿Qué es?
El AV monitorea qué hace el programa **mientras se ejecuta**.

### ¿Cómo funciona?
```
┌──────────────────┐
│  nuub.exe arranca │
└────────┬─────────┘
         │
    ┌────▼────┐
    │ APIHook │ ← El AV intercepta cada llamada al API
    └────┬────┘
         │
    ┌────▼──────────────────────┐
    │ ¿Llamó a VirtualProtect   │
    │  para cambiar permisos    │ → SOSPECHOSO
    │  de memoria a RWX?        │
    └────┬──────────────────────┘
         │
    ┌────▼──────────────────────┐
    │ ¿Creó proceso suspendido  │
    │  y escribió código en él? │ → MALWARE
    └───────────────────────────┘
```

### ¿Qué detecta?

| Comportamiento | Detección |
|----------------|-----------|
| Abre `ntdll.dll` y escribe en su memoria | ETW Patching → **MALWARE** |
| Crea `explorer.exe` suspendido, lo modifica, lo reanuda | Process Hollowing → **MALWARE** |
| Carga DLL legítima y sobreescribe su `.text` | Module Stomping → **MALWARE** |
| `VirtualAlloc` + `memcpy` + `CreateThread` con RWX | Shellcode execution → **MALWARE** |
| Lee `/proc/cpuinfo` o usa `CPUID` bit 31 | Anti-VM → **SOSPECHOSO** |
| Abre procesos y busca "ollydbg", "ida", "x64dbg" | Anti-Debug → **SOSPECHOSO** |

### Nivel de detección: ~80% de los AVs
El comportamiento es el factor #1 de detección moderno.

---

## 5. Capa 4: Machine Learning / IA

### ¿Qué es?
Modelos de ML entrenados con millones de muestras de malware y software legítimo.

### ¿Qué analiza?

**Features estáticas:**
- Proporción de código vs datos
- Entropía de las secciones PE
- Número y tipo de imports
- Presencia de secciones cifradas
- Tamaño del archivo
- Compilador Used

**Features de comportamiento:**
- Secuencia de llamadas al API
- Patrones de acceso a memoria
- Patrones de red
- Modificación del registry

### Ejemplo de detección ML
```
INPUT: nuub.exe
    │
    ├─ Entropía alta en sección .text → +0.3
    ├─ Imports: VirtualAlloc, VirtualProtect, CreateProcess → +0.4
    ├─ Patrón: RWX memory + process creation → +0.5
    ├─ Strings cifrados detectados → +0.2
    │
    ├─ Modelo ML: Probabilidad de malware = 94.7%
    │
    └─ OUTPUT: Trojan.Win64.Malgent!ML
```

### Nivel de detección: ~60% (mejora constantemente)

---

## 6. Capa 5: Sandbox / Análisis Dinámico

### ¿Qué es?
El AV ejecuta el binario en una **máquina virtual aislada** y observa todo.

### ¿Qué monitorea?

| Categoría | Qué buscan |
|-----------|-----------|
| **Archivos** | ¿Crea archivos? ¿Dónde? ¿Los cifra? |
| **Registry** | ¿Modifica auto-start? ¿Deshabilita seguridad? |
| **Red** | ¿Se conecta a servidores C2? ¿Exfiltra datos? |
| **Procesos** | ¿Inyecta código en otros procesos? |
| **Memoria** | ¿Usa RWX memory? ¿Parchea DLLs del sistema? |
| **Anti-sandbox** | ¿Detecta que está en sandbox? (ironía: lo detectan a ti) |

### Detección de sandbox en NUUB RAT
```cpp
// AntiVM detecta VMware, VirtualBox, Sandboxie
// Esto ES sospechoso porque los malwares hacen EXACTAMENTE esto
const char* vm_files[] = {
    "C:\\Windows\\System32\\vmGuestLib.dll",  // VMware
    "C:\\Windows\\System32\\VBoxHook.dll",    // VirtualBox
    "C:\\Windows\\System32\\SbieDll.dll",     // Sandboxie ← ¡BINGO!
};
```

**Paradoja:** Tu herramienta detecta sandboxes para evadirlos, pero el AV interpreta esa detección como evidencia de malicia.

### Nivel de detección: ~90% si el sandbox es bueno

---

## 7. Técnicas de Evasión en el Código (y por qué se detectan)

### Mapeo completo: NUUB RAT → Detección

| # | Técnica | Archivo | API/Sistema que usa | ¿Por qué se detecta? | MITRE ATT&CK |
|---|---------|---------|---------------------|----------------------|--------------|
| 1 | **Anti-Debug** | `AntiAnalysis.hpp` | `IsDebuggerPresent()`, PEB access, DR registers | Patrón universal de malware. Los AVs monitorean estas APIs. | T1622 |
| 2 | **Anti-VM** | `AntiAnalysis.hpp` | Registry keys, `CPUID` bit 31, DLL checks | Lista de VMs = lista de sandboxes. Muy sospechoso. | T1497 |
| 3 | **ETW Patch** | `EtwPatch.hpp` | `VirtualProtect` → `EtwEventWrite` = `xor rax,rax; ret` | Patrón de bytes ES UNA FIRMA. Todos los AVs lo conocen. | T1562.001 |
| 4 | **Process Hollowing** | `ProcessHollowing.hpp` | `CreateProcess(SUSPENDED)` → `NtUnmapViewOfSection` → `WriteProcessMemory` → `ResumeThread` | Secuencia de APIs = MALWARE. 100% detectado. | T1055.012 |
| 5 | **Module Stomping** | `ModuleStomping.hpp` | `LoadLibrary` → `VirtualProtect(RWX)` → `memcpy` over `.text` | Sobreescribir código legítimo = MALWARE | T1055.001 |
| 6 | **Fileless Execution** | `FilelessExec.hpp` | `VirtualAlloc(RWX)` → `memcpy` → `CreateThread` | Shellcode execution pattern. Clásico. | T1055 |
| 7 | **API Hashing** | `ApiHash.hpp` | `GetProcAddress` con hash de nombre | Ofuscación de imports = sospechoso | T1027.007 |
| 8 | **String Encryption** | `StringTable.hpp` | XOR/encode strings | Strings ocultos = algo que esconder | T1027.013 |
| 9 | **Domain Fronting** | `DomainFronting.hpp` | HTTPS con SNI diferente al host | Técnica de C2 para evadir firewalls | T1090.004 |
| 10 | **Direct Syscalls** | `DirectSyscall.hpp` | Syscall directo sin ntdll | Evade hooks de EDR = SOSPECHOSO | T1106 |

### Por qué NUUB RAT es IMPOSIBLE de ocultar

El problema no es una técnica individual. Es la **COMBINACIÓN**:

```
Anti-Debug + Anti-VM + ETW Patch + Process Hollowing + Module Stomping
= PERFIL DE MALWARE COMPLETO

Cualquier AV moderno detecta esto porque:
1. El binario TIENE las firmas de estas técnicas
2. El comportamiento es 100% malicioso
3. Los modelos ML lo clasifican como malware
4. Los sandboxes ven todo el flujo
```

---

## 8. MITRE ATT&CK Mapping

NUUB RAT mapea a estas técnicas del framework MITRE ATT&CK:

| Táctica | Técnica | ID | Descripción |
|---------|---------|-----|-------------|
| **Defense Evasion** | Obfuscated Files | T1027 | Ofuscación de strings y código |
| **Defense Evasion** | Process Injection: Process Hollowing | T1055.012 | Inyección en procesos legítimos |
| **Defense Evasion** | Process Injection: Module Stomping | T1055.001 | Sobreescribir módulos legítimos |
| **Defense Evasion** | Impair Defenses: Disable ETW | T1562.001 | Deshabilitar telemetría de Windows |
| **Defense Evasion** | Virtualization/Sandbox Evasion | T1497 | Detección de VMs y sandboxes |
| **Defense Evasion** | Debugging Evasion | T1622 | Detección de debuggers |
| **Defense Evasion** | Dynamic API Resolution | T1027.007 | API hashing |
| **Defense Evasion** | Indicator Removal from Tools | T1027.005 | Strings cifrados |
| **Defense Evasion** | Fileless Storage | T1027.011 | Ejecución sin archivo en disco |
| **Defense Evasion** | Direct Volume Access | - | Acceso directo a disco |
| **Persistence** | Boot or Logon Autostart | T1547.001 | Auto-start en Windows |
| **Collection** | Screen Capture | T1113 | Screenshots |
| **Collection** | Keylogging | T1056.001 | Registro de teclas |
| **Collection** | Audio Capture | T1123 | Grabación de audio |
| **Exfiltration** | Exfiltration Over C2 | T1041 | Datos por canal C2 |
| **C2** | Application Layer: Web Protocols | T1071 | Telegram como canal C2 |

---

## 9. Defensa (¿Cómo se protege un sistema?)

### Para los estudiantes de ciberseguridad:

| Capa de defensa | Herramienta | Qué detecta |
|-----------------|-------------|-------------|
| **Antivirus** | Windows Defender, Kaspersky, ESET | Firmas, heurística, ML |
| **EDR** | CrowdStrike, SentinelOne, Carbon Black | Comportamiento en tiempo real |
| **Firewall** | Windows Firewall, pfSense | Conexiones de red sospechosas |
| **SIEM** | Splunk, Elastic, Microsoft Sentinel | Correlación de eventos |
| **AppLocker** | Windows Enterprise | Solo permite apps aprobadas |
| **Sysmon** | Microsoft Sysmon | Logging detallado del sistema |
| **Honeytokens** | Canary tokens, honeyfiles | Detección de movimiento lateral |

### Señales de alerta en un sistema comprometido

```bash
# Sysmon event ID 1: Process Creation con command line sospechosa
EventID=1 AND Image=*explorer.exe* AND CommandLine=*powershell*

# ETW: Memory protection change a RWX
EventID=5 AND Protection=PAGE_EXECUTE_READWRITE

# Registry: Auto-start nuevo
EventID=13 AND TargetObject=*CurrentVersion\Run*

# Network: Conexión a Telegram API (si no es uso legítimo)
DestinationPort=443 AND DestinationIp=149.154.*
```

---

## Resumen para la Clase

### ¿Por qué NUUB RAT fue detectado?

1. **Firma estática**: VirusTotal ya tiene el hash (o hashes parciales de técnicas conocidas)
2. **Heurística**: Las APIs importadas (`NtUnmapViewOfSection`, `VirtualProtect RWX`) son rojo
3. **Comportamiento**: Process hollowing + ETW patching = malware
4. **ML**: El modelo clasifica el binario como malicioso
5. **Sandbox**: Si lo ejecutas en un sandbox, ve TODO el flujo de evasión

### Lección clave

> **No puedes "esconder" malware de un AV moderno.**
> Las 5 capas de detección se superponen.
> Si tienes 3+ técnicas de evasión, el AV YA TE DETECTÓ.
> La única forma de "evadir" es entender CADA capa y diseñar para evadir TODAS simultáneamente — lo cual es extremadamente difícil y los AVs se actualizan constantemente.

### Para tu presentación

Usa este documento como guía visual. Los diagramas y tablas son ideales para slides. Cada sección es un slide independiente.

---

**Referencias:**
- MITRE ATT&CK Framework: https://attack.mitre.org/
- VirusTotal: https://www.virustotal.com/
- Microsoft Defender Antivirus: https://docs.microsoft.com/en-us/microsoft-365/security/defender-endpoint/
- Sysmon: https://docs.microsoft.com/en-us/sysinternals/downloads/sysmon

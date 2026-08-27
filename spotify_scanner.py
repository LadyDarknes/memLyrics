import ctypes
from ctypes import wintypes
import sys
import time
import struct

sys.stdout.reconfigure(encoding='utf-8', line_buffering=True)

PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010
MEM_COMMIT = 0x1000
PAGE_READWRITE = 0x04
PAGE_WRITECOPY = 0x08
PAGE_EXECUTE_READWRITE = 0x40

TH32CS_SNAPPROCESS = 0x00000002

class PROCESSENTRY32(ctypes.Structure):
    _fields_ = [
        ("dwSize", wintypes.DWORD),
        ("cntUsage", wintypes.DWORD),
        ("th32ProcessID", wintypes.DWORD),
        ("th32DefaultHeapID", ctypes.c_void_p),
        ("th32ModuleID", wintypes.DWORD),
        ("cntThreads", wintypes.DWORD),
        ("th32ParentProcessID", wintypes.DWORD),
        ("pcPriClassBase", wintypes.LONG),
        ("dwFlags", wintypes.DWORD),
        ("szExeFile", ctypes.c_char * 260),
    ]

class MEMORY_BASIC_INFORMATION(ctypes.Structure):
    _fields_ = [
        ("BaseAddress", ctypes.c_void_p),
        ("AllocationBase", ctypes.c_void_p),
        ("AllocationProtect", wintypes.DWORD),
        ("PartitionId", wintypes.WORD),
        ("RegionSize", ctypes.c_size_t),
        ("State", wintypes.DWORD),
        ("Protect", wintypes.DWORD),
        ("Type", wintypes.DWORD),
    ]

kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)

def get_spotify_pids():
    hSnapshot = kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    if hSnapshot == -1:
        return []
    
    pe = PROCESSENTRY32()
    pe.dwSize = ctypes.sizeof(PROCESSENTRY32)
    pids = []
    
    if kernel32.Process32First(hSnapshot, ctypes.byref(pe)):
        while True:
            exe_name = pe.szExeFile.decode('utf-8', errors='ignore').lower()
            if exe_name == "spotify.exe":
                pids.append(pe.th32ProcessID)
            if not kernel32.Process32Next(hSnapshot, ctypes.byref(pe)):
                break
                
    kernel32.CloseHandle(hSnapshot)
    return pids

def scan_strings_in_pid(pid, query):
    hProc = kernel32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid)
    if not hProc:
        return []

    q_u8 = query.encode('utf-8')
    q_u16 = query.encode('utf-16le')

    addr = 0
    mbi = MEMORY_BASIC_INFORMATION()
    found = []

    while kernel32.VirtualQueryEx(hProc, ctypes.c_void_p(addr), ctypes.byref(mbi), ctypes.sizeof(mbi)):
        if mbi.State == MEM_COMMIT and (mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE)):
            size = mbi.RegionSize
            if 0 < size <= 8 * 1024 * 1024:
                buf = ctypes.create_string_buffer(size)
                bytesRead = ctypes.c_size_t()
                if kernel32.ReadProcessMemory(hProc, ctypes.c_void_p(addr), buf, size, ctypes.byref(bytesRead)):
                    raw = buf.raw[:bytesRead.value]
                    if q_u8 in raw:
                        idx = raw.find(q_u8)
                        found.append((addr + idx, "UTF-8"))
                    if q_u16 in raw:
                        idx = raw.find(q_u16)
                        found.append((addr + idx, "UTF-16"))
        addr += mbi.RegionSize
        if addr >= 0x7FFFFFFFFFFF:
            break

    kernel32.CloseHandle(hProc)
    return found

def scan_live_clock(pid):
    hProc = kernel32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid)
    if not hProc:
        return []

    print(f"[*] PID {pid} bellek haritasi taranıyor...", flush=True)
    snap = {}
    addr = 0
    mbi = MEMORY_BASIC_INFORMATION()

    while kernel32.VirtualQueryEx(hProc, ctypes.c_void_p(addr), ctypes.byref(mbi), ctypes.sizeof(mbi)):
        if mbi.State == MEM_COMMIT and (mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY)):
            size = mbi.RegionSize
            if 0 < size <= 4 * 1024 * 1024:
                buf = ctypes.create_string_buffer(size)
                bytesRead = ctypes.c_size_t()
                if kernel32.ReadProcessMemory(hProc, ctypes.c_void_p(addr), buf, size, ctypes.byref(bytesRead)):
                    raw = buf.raw[:bytesRead.value]
                    for off in range(0, len(raw) - 4, 4):
                        val = struct.unpack_from('<I', raw, off)[0]
                        if 2000 <= val <= 360000:
                            snap[addr + off] = val
        addr += mbi.RegionSize
        if addr >= 0x7FFFFFFFFFFF:
            break

    if not snap:
        kernel32.CloseHandle(hProc)
        return []

    print(f"[+] {len(snap)} adet olasi aday bulundu. 2 saniye bekleniyor...", flush=True)
    time.sleep(2.0)

    matches = []
    for t_addr, old_val in snap.items():
        buf = ctypes.create_string_buffer(4)
        bytesRead = ctypes.c_size_t()
        if kernel32.ReadProcessMemory(hProc, ctypes.c_void_p(t_addr), buf, 4, ctypes.byref(bytesRead)):
            new_val = struct.unpack('<I', buf.raw)[0]
            diff = new_val - old_val
            if 1700 <= diff <= 2300:
                matches.append((t_addr, old_val, new_val, diff))

    kernel32.CloseHandle(hProc)
    return matches

if __name__ == "__main__":
    print("========================================", flush=True)
    print("      Spotify Fast Memory Scanner       ", flush=True)
    print("========================================", flush=True)

    pids = get_spotify_pids()
    print(f"[+] Bulunan Spotify PID'leri: {pids}\n", flush=True)

    query = sys.argv[1] if len(sys.argv) > 1 else "Madrigal"
    print(f"[*] '{query}' metni icin araniyor...", flush=True)
    for p in pids:
        res = scan_strings_in_pid(p, query)
        if res:
            print(f"  [!] PID {p} -> {len(res)} adres bulundu: 0x{res[0][0]:X} ({res[0][1]})", flush=True)

    print("\n[*] Canli oynatma saati araniyor...", flush=True)
    for p in pids:
        clocks = scan_live_clock(p)
        if clocks:
            print(f"\n[+] PID {p} ICINDE CANLI SAAT BULUNDU! ({len(clocks)} adres)", flush=True)
            for c in clocks[:5]:
                print(f"   -> 0x{c[0]:X} : {c[1]} ms -> {c[2]} ms (+{c[3]} ms)", flush=True)
            break
